#include "GuardianBody.h"

#include "GuardianGait.h"
#include "IGuardianLegRig.h"
#include "GuardianRigMath.h"

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

	// --- ボディだけを回す。脚は接地したままなので球体が独立して回って見える ---
	currentSpin_ += spinSpeedDeg_ * kDegreeToRadian * deltaTime;
	if (currentSpin_ > std::numbers::pi_v<float> * 2.0f) {
		currentSpin_ -= std::numbers::pi_v<float> * 2.0f;
	} else if (currentSpin_ < -std::numbers::pi_v<float> * 2.0f) {
		currentSpin_ += std::numbers::pi_v<float> * 2.0f;
	}

	WorldTransform& transform = bodyObject->GetTransform();
	// heightOffset_ は平滑化の外側で足す。溜めの沈み込みなど、即座に効いてほしい用途のため。
	transform.translation_.y = currentHeight_ + bob + heightOffset_;
	transform.rotation_ = {currentPitch_, currentSpin_, currentRoll_};
}

bool GuardianBody::GatherPlantedFeet(
    IGuardianLegRig& rig, const GuardianGait* gait, float& outAverageHeight, float& outPitchSlope, float& outRollSlope) const {

	// 0=前左, 1=前右, 2=後右, 3=後左。
	float heights[kGuardianLegCount] = {0.0f, 0.0f, 0.0f, 0.0f};
	bool planted[kGuardianLegCount] = {false, false, false, false};

	float total = 0.0f;
	int count = 0;

	for (int index = 0; index < kGuardianLegCount; ++index) {
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

	// 前後・左右で「接地している側だけ」の平均を取り、その差を傾きの元にする。
	// 片側が全部浮いている場合は傾けない(0を返す)。
	auto averageOf = [&](int a, int b, float& outValue) {
		float sum = 0.0f;
		int used = 0;
		if (planted[a]) {
			sum += heights[a];
			++used;
		}
		if (planted[b]) {
			sum += heights[b];
			++used;
		}
		if (used == 0) {
			return false;
		}
		outValue = sum / static_cast<float>(used);
		return true;
	};

	outPitchSlope = 0.0f;
	outRollSlope = 0.0f;

	float front = 0.0f;
	float back = 0.0f;
	if (averageOf(0, 1, front) && averageOf(2, 3, back)) {
		// 前が高いほど頭を上げる。実距離で割らず素の高低差を使い、強さはtiltStrength_で調整する。
		outPitchSlope = back - front;
	}

	float left = 0.0f;
	float right = 0.0f;
	if (averageOf(0, 3, left) && averageOf(1, 2, right)) {
		outRollSlope = right - left;
	}

	return true;
}
