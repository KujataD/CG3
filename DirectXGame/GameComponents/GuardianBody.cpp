#include "GuardianBody.h"

#include "GuardianGait.h"
#include "IGuardianLegRig.h"
#include "GuardianRigMath.h"

#include <math/Noise.h>

#include <algorithm>
#include <cmath>
#include <numbers>

using KujataEngine::GameObject;
using KujataEngine::Vector3;
using KujataEngine::WorldTransform;

namespace {

constexpr float kDegreeToRadian = std::numbers::pi_v<float> / 180.0f;

/// <summary>rate[1/s]の指数追従。rateが0以下なら即座に目標値へ飛ばす。</summary>
float Approach(float current, float target, float rate, float deltaTime) {
	if (rate <= 0.0f) {
		return target;
	}
	float t = std::clamp(rate * deltaTime, 0.0f, 1.0f);
	return current + (target - current) * t;
}

} // namespace

void GuardianBody::OnPlayStart() {
	initialized_ = false;
	bobPhase_ = 0.0f;
	noiseTime_ = 0.0f;
	currentSpin_ = 0.0f;
}

void GuardianBody::Update() {
	IGuardianLegRig* rig = GetComponent<IGuardianLegRig>();
	GameObject* owner = GetOwner();
	if (!rig || !owner) {
		return;
	}

	GameObject* bodyObject = rig->GetBodyObject();
	// ボディが解決できていない、またはルート自身が返っている場合は動かさない
	// (ルートを動かすと歩行の基準位置ごとずれてしまうため)。
	if (!bodyObject || bodyObject == owner) {
		return;
 	}

	float deltaTime = Time::GetDeltaTime();
	if (deltaTime <= 0.0f) {
		return;
	}

	const GuardianGait* gait = GetComponent<GuardianGait>();

	// --- 接地している足から、あるべき高さと傾きを求める ---
	float averageHeight = 0.0f;
	float pitchSlope = 0.0f;
	float rollSlope = 0.0f;
	bool hasFeet = GatherPlantedFeet(*rig, gait, averageHeight, pitchSlope, rollSlope);

	GuardianRigMath::WorldPose rootPose = GuardianRigMath::ComputeWorldPose(owner);

	// 全脚が浮いていれば足を参照しようがないので、ルートからの定位置へ戻る(ジャンプ中の姿勢)。
	float targetHeight = rideHeight_;
	if (hasFeet) {
		// ボディはルートの直下にぶら下がっている前提で、ワールド高さをローカルYへ落とす。
		float followHeight = (averageHeight + rideHeight_) - rootPose.position.y;

		// ここでクランプしないと、ルートのYがそのまま打ち消されて胴体のワールド高さが
		// 「足の高さ + rideHeight」に固定される。するとルートを上げても接合部が上がらず、
		// 脚が伸びきらないので GuardianGait の離脱判定が永久に発火しない(ジャンプにならない)。
		// 足への追従はサスペンションのストローク内に限り、それを超える分はルートに引かれる。
		targetHeight = std::clamp(followHeight, rideHeight_ - maxSuspensionTravel_, rideHeight_ + maxSuspensionTravel_);
	}

	float targetPitch = 0.0f;
	float targetRoll = 0.0f;
	if (hasFeet && alignToFeet_) {
		float maxTilt = maxTiltDeg_ * kDegreeToRadian;
		targetPitch = std::clamp(std::atan(pitchSlope) * tiltStrength_, -maxTilt, maxTilt);
		targetRoll = std::clamp(std::atan(rollSlope) * tiltStrength_, -maxTilt, maxTilt);
	}

	if (!initialized_) {
		currentHeight_ = targetHeight;
		currentPitch_ = targetPitch;
		currentRoll_ = targetRoll;
		initialized_ = true;
	} else {
		currentHeight_ = Approach(currentHeight_, targetHeight, heightSmoothing_, deltaTime);
		currentPitch_ = Approach(currentPitch_, targetPitch, tiltSmoothing_, deltaTime);
		currentRoll_ = Approach(currentRoll_, targetRoll, tiltSmoothing_, deltaTime);
	}

	// --- 歩行に合わせた上下の揺れ。速く歩くほど大きく揺れる ---
	float speed = gait ? gait->GetPlanarSpeed() : 0.0f;
	float speedFactor = std::clamp(speed / (std::max)(bobSpeedReference_, 1.0e-3f), 0.0f, 1.0f);
	bobPhase_ += deltaTime * bobFrequency_ * std::numbers::pi_v<float> * 2.0f * speedFactor;
	if (bobPhase_ > std::numbers::pi_v<float> * 2.0f) {
		bobPhase_ -= std::numbers::pi_v<float> * 2.0f;
	}
	float bob = std::sin(bobPhase_) * bobAmplitude_ * speedFactor;

	// --- 立ち止まっている間の揺れ。歩行のBobとちょうど入れ替わりで効く ---
	// **正弦波ではなくノイズにするのが要点。** 周期が読めないので機械的にならず、
	// 巨体が自重で微かに軋んでいるように見える。
	noiseTime_ += deltaTime;
	float idleSway = 0.0f;
	if (idleSwayAmplitude_ > 0.0f) {
		// 脚側(GuardianGait)とはレーンも周波数も別にしてある。噛み合うと共振して不自然に揺れる。
		idleSway = KujataEngine::PerlinNoise(noiseTime_ * idleSwayFrequency_ + 1.3f, 41.7f) * idleSwayAmplitude_ *
		           (1.0f - speedFactor);
	}

	// --- ボディだけを回す。脚は接地したままなので球体が独立して回って見える ---
	currentSpin_ += spinSpeedDeg_ * kDegreeToRadian * deltaTime;
	if (currentSpin_ > std::numbers::pi_v<float> * 2.0f) {
		currentSpin_ -= std::numbers::pi_v<float> * 2.0f;
	} else if (currentSpin_ < -std::numbers::pi_v<float> * 2.0f) {
		currentSpin_ += std::numbers::pi_v<float> * 2.0f;
	}

	WorldTransform& transform = bodyObject->GetTransform();
	// heightOffset_ は平滑化の外側で足す。溜めの沈み込みなど、即座に効いてほしい用途のため。
	transform.translation_.y = currentHeight_ + bob + idleSway + heightOffset_;
	// pitchOffset_ は平滑化の外側で足す。仰け反りは即座に効いてほしいため。
	transform.rotation_ = {currentPitch_ + pitchOffset_, currentSpin_, currentRoll_};
}

bool GuardianBody::GatherPlantedFeet(
    IGuardianLegRig& rig, const GuardianGait* gait, float& outAverageHeight, float& outPitchSlope, float& outRollSlope) const {

	// 0=前左, 1=前右, 2=後右, 3=後左。
	float heights[kGuardianLegCount] = {0.0f, 0.0f, 0.0f, 0.0f};
	bool planted[kGuardianLegCount] = {false, false, false, false};

	float total = 0.0f;
	int count = 0;

	const int legCount = rig.GetLegCount();
	for (int index = 0; index < legCount; ++index) {
		// 踏み出し中や攻撃で持ち上げ中の脚は、ボディを持ち上げる根拠にならないので除外する。
		if (rig.IsCurveDriven(index)) {
			continue;
		}
		if (gait && !gait->IsPlanted(index)) {
			continue;
		}

		heights[index] = rig.GetProceduralTarget(index).y;
		planted[index] = true;
		total += heights[index];
		++count;
	}

	if (count == 0) {
		return false;
	}
	outAverageHeight = total / static_cast<float>(count);

	// --- 傾きは「足の定位置」で重み付けして求める ---
	// **固定の添字で前後左右を組まない。** 0=前左/1=前右/2=後右/3=後左 という並びを前提にすると
	// 脚を3本に減らした瞬間に前後の対応が壊れる。定位置のx(左右)とz(前後)を重みに使えば、
	// 何本でも・どんな配置でも同じ式で扱える。4脚の既定配置では従来と同じ値になる。
	float pitchNumerator = 0.0f;
	float pitchWeight = 0.0f;
	float rollNumerator = 0.0f;
	float rollWeight = 0.0f;

	for (int index = 0; index < legCount; ++index) {
		if (!planted[index]) {
			continue;
		}
		Vector3 home = rig.GetHomeLocal(index);
		// 前が高いほど頭を上げたいので、zの符号を反転して掛ける。
		pitchNumerator += heights[index] * -home.z;
		pitchWeight += std::fabs(home.z);
		rollNumerator += heights[index] * home.x;
		rollWeight += std::fabs(home.x);
	}

	outPitchSlope = 0.0f;
	outRollSlope = 0.0f;
	// 重みの半分で割ると、4脚の既定配置で従来の「後ろ平均 - 前平均」と一致する。
	if (pitchWeight > 1.0e-4f) {
		outPitchSlope = pitchNumerator / (pitchWeight * 0.5f);
	}
	if (rollWeight > 1.0e-4f) {
		outRollSlope = rollNumerator / (rollWeight * 0.5f);
	}

	return true;
}
