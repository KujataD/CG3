#include "GuardianGait.h"

#include "IGuardianLegRig.h"
#include "GuardianRigMath.h"

#include <components/RigidbodyComponent.h>

#include <algorithm>
#include <cmath>
#include <numbers>

using KujataEngine::ColliderComponent;
using KujataEngine::Component;
using KujataEngine::GameObject;
using KujataEngine::Vector3;

namespace {

/// <summary>objがrootの子孫(またはroot自身)か。自分の脚を地面と誤認しないための判定。</summary>
bool IsSelfOrDescendantOf(const GameObject* object, const GameObject* root) {
	for (const GameObject* current = object; current; current = current->GetParent()) {
		if (current == root) {
			return true;
		}
	}
	return false;
}

/// <summary>
/// 足を置いてよい「動かない床」か。
/// エンジンの動的判定(Scene.cpp の IsMovable)と同じ規約で、
/// 「Rigidbodyを持ち、かつIs Staticでない」ものだけを動くコライダーとみなす。
/// Rigidbody無し = 純粋な静的壁、Is Static = キネマティックで、どちらも足場として扱う。
/// </summary>
bool IsStaticGround(GameObject& gameObject) {
	// GameObject::GetComponent は非constのみなので、参照も非constで受ける。
	KujataEngine::RigidbodyComponent* rigidbody = gameObject.GetComponent<KujataEngine::RigidbodyComponent>();
	return !rigidbody || rigidbody->IsStatic();
}

/// <summary>
/// コライダーの実形状に対して真下レイを撃つ。
///
/// AABBで代用すると、傾いた箱では「一番高い角の高さ」に、球やカプセルでは「頂点の高さ」に
/// 足が浮いてしまう。形状ごとに分岐するのは OrbitCameraComponent::ComputeOccludedDistance と同じ方針。
/// 箱は回転が無ければAABBで済ませる(そちらのほうが軽い)。
/// </summary>
bool RaycastDownCollider(ColliderComponent& collider, const Vector3& origin, float maxDistance, float& outDistance) {
	switch (collider.GetShapeType()) {
	case KujataEngine::ColliderShapeType::Box: {
		auto* box = dynamic_cast<KujataEngine::BoxColliderComponent*>(&collider);
		if (box && box->UsesWorldOBB()) {
			return GuardianRigMath::RaycastDownObb(origin, maxDistance, box->GetWorldOBB(), outDistance);
		}
		return GuardianRigMath::RaycastDownAabb(origin, maxDistance, collider.GetWorldAABB(), outDistance);
	}
	case KujataEngine::ColliderShapeType::Capsule: {
		auto* capsule = dynamic_cast<KujataEngine::CapsuleColliderComponent*>(&collider);
		if (capsule) {
			return GuardianRigMath::RaycastDownCapsule(origin, maxDistance, capsule->GetWorldCapsule(), outDistance);
		}
		return GuardianRigMath::RaycastDownSphere(origin, maxDistance, collider.GetWorldSphere(), outDistance);
	}
	case KujataEngine::ColliderShapeType::Sphere:
	default:
		return GuardianRigMath::RaycastDownSphere(origin, maxDistance, collider.GetWorldSphere(), outDistance);
	}
}

/// <summary>両端がなめらかに繋がる補間。踏み出しの加速/減速に使う。</summary>
float SmoothStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

float HorizontalDistance(const Vector3& a, const Vector3& b) {
	float dx = a.x - b.x;
	float dz = a.z - b.z;
	return std::sqrt(dx * dx + dz * dz);
}

} // namespace

IGuardianLegRig* GuardianGait::GetRig() { return GetComponent<IGuardianLegRig>(); }

void GuardianGait::OnPlayStart() {
	hasPreviousPosition_ = false;
	rootVelocity_ = {0.0f, 0.0f, 0.0f};
	planarSpeed_ = 0.0f;
	ResetToHome();
}

void GuardianGait::ResetToHome() {
	IGuardianLegRig* rig = GetRig();
	if (!rig) {
		return;
	}

	for (int index = 0; index < kGuardianLegCount; ++index) {
		Vector3 home = rig->GetHomeWorld(index);
		home.y = SampleGroundHeight(home) + footClearance_;

		legStates_[index] = LegState{};
		legStates_[index].planted = home;
		rig->SetProceduralTarget(index, home);
	}
}

void GuardianGait::Update() {
	IGuardianLegRig* rig = GetRig();
	GameObject* owner = GetOwner();
	if (!rig || !owner) {
		return;
	}

	float deltaTime = Time::GetDeltaTime();
	if (deltaTime <= 0.0f) {
		return;
	}

	// --- ルートの移動速度を測る。歩幅の先読みに使う ---
	Vector3 rootPosition = GuardianRigMath::ComputeWorldPose(owner).position;
	if (hasPreviousPosition_) {
		Vector3 frameDelta = rootPosition - previousRootPosition_;
		Vector3 instantVelocity = frameDelta / deltaTime;
		// 1フレームの揺れで歩幅が暴れないよう指数平滑する。
		rootVelocity_ = GuardianRigMath::Lerp3(rootVelocity_, instantVelocity, std::clamp(deltaTime * 10.0f, 0.0f, 1.0f));
	} else {
		hasPreviousPosition_ = true;
	}
	previousRootPosition_ = rootPosition;
	planarSpeed_ = std::sqrt(rootVelocity_.x * rootVelocity_.x + rootVelocity_.z * rootVelocity_.z);

	for (int index = 0; index < kGuardianLegCount; ++index) {
		LegState& state = legStates_[index];

		// --- 曲線レイヤーが主導権を持っている間は歩行を止める ---
		if (rig->IsCurveDriven(index)) {
			state.stepping = false;
			// 解放された瞬間にズレ判定を待たずに踏み直させる。
			state.needsReplant = true;
			// 曲線が運んだ先を「今いる場所」として引き継ぐ。ブレンドが戻るときに足が飛ばない。
			state.planted = rig->GetFootWorld(index);
			rig->SetProceduralTarget(index, state.planted);
			continue;
		}

		Vector3 home = rig->GetHomeWorld(index);
		home.y = SampleGroundHeight(home) + footClearance_;

		// --- 接地と離脱の切り替え ---
		// 脚には最大長があるので、胴体が浮けばいずれ足は地面から引き剥がされる。
		// 離脱と着地でしきい値を変えてヒステリシスを作り、境界でのばたつきを防ぐ。
		Vector3 hip = rig->GetHipWorld(index);
		float maxReach = (std::max)(rig->GetMaxReach(index), 1.0e-4f);

		if (state.airborne) {
			if (KujataEngine::Length(home - hip) <= maxReach * landReachRatio_) {
				// 地面が届く範囲に戻ったので、その場に着地する。
				state.airborne = false;
				state.stepping = false;
				state.planted = home;
				state.needsReplant = true;
			}
		} else {
			// 踏み出し中は着地予定点が、そうでなければ今の接地点が引き剥がされるかを見る。
			Vector3 anchor = state.stepping ? state.stepTo : state.planted;
			bool outOfReach = KujataEngine::Length(anchor - hip) > maxReach * detachReachRatio_;

			// 届かなくなった理由を切り分ける。地面がまだ届くなら「胴体が浮いた」のではなく
			// 「高速移動で足が置き去りになった」だけなので、浮かせずに下の緊急踏み直しへ回す。
			// ここを分けないと、速く動いた瞬間に4本とも宙に浮いて歩行が崩壊する。
			bool groundOutOfReach = KujataEngine::Length(home - hip) > maxReach * landReachRatio_;
			if (outOfReach && groundOutOfReach) {
				state.airborne = true;
				state.stepping = false;
				// 今いる位置から垂れ始める(足が瞬間移動しないように)。
				state.airPosition = state.planted;
			}
		}

		if (state.airborne) {
			// 接合部からぶら下げる。歩容の判定は一切通さない(浮いている脚は踏み出さない)。
			Vector3 dangle = ComputeDanglePosition(hip, rootPosition, maxReach);
			state.airPosition =
			    GuardianRigMath::Lerp3(state.airPosition, dangle, std::clamp(dangleSmoothing_ * deltaTime, 0.0f, 1.0f));
			state.planted = state.airPosition;
			rig->SetProceduralTarget(index, state.airPosition);
			continue;
		}

		// --- 踏み出しの開始判定 ---
		if (!state.stepping) {
			bool drifted = HorizontalDistance(state.planted, home) > stepThreshold_;
			// 脚が伸びきる寸前まで置き去りにされている。歩容の順番を待っていると崩壊するので割り込む。
			bool urgent = KujataEngine::Length(state.planted - hip) > maxReach * urgentReachRatio_;

			if ((drifted || state.needsReplant || urgent) && (urgent || CanStartStep(index))) {
				state.stepping = true;
				state.timer = 0.0f;
				state.stepFrom = state.planted;
				state.duration = ComputeStepDuration();

				// 着地する瞬間に定位置が来ている場所へ踏み込む。1歩の時間を使って先読みするので、
				// 速度が変わっても足が後ろに着地しない。
				Vector3 landing = home + rootVelocity_ * (state.duration * stepLeadFactor_);
				landing.y = SampleGroundHeight(landing) + footClearance_;
				state.stepTo = landing;
				state.needsReplant = false;
			}
		}

		// --- 足先の位置を決める ---
		Vector3 footPosition;
		if (state.stepping) {
			state.timer += deltaTime;
			float t = std::clamp(state.timer / (std::max)(state.duration, 1.0e-3f), 0.0f, 1.0f);

			footPosition = GuardianRigMath::Lerp3(state.stepFrom, state.stepTo, SmoothStep(t));
			// half-sineの弧で持ち上げる。両端がちょうど0になるので着地/離地が滑らか。
			footPosition.y += std::sin(std::numbers::pi_v<float> * t) * stepHeight_;

			if (t >= 1.0f) {
				state.stepping = false;
				state.planted = state.stepTo;
				footPosition = state.stepTo;
			}
		} else {
			footPosition = state.planted;
		}

		rig->SetProceduralTarget(index, footPosition);
	}
}

float GuardianGait::ComputeStepDuration() const {
	float maxDuration = (std::max)(stepDuration_, 1.0e-3f);
	float minDuration = std::clamp(minStepDuration_, 1.0e-3f, maxDuration);

	if (planarSpeed_ <= 1.0e-3f) {
		return maxDuration;
	}

	// 歩幅を速度で割れば「その歩幅を進むのにかかる時間」。速いほど1歩が短くなる。
	return std::clamp(strideLength_ / planarSpeed_, minDuration, maxDuration);
}

bool GuardianGait::CanStartStep(int index) const {
	if (!alternateGait_) {
		return true;
	}

	// 0=前左, 1=前右, 2=後右, 3=後左。対角ペアは {0,2} と {1,3} で、添字の偶奇で分けられる。
	int group = index % 2;
	for (int other = 0; other < kGuardianLegCount; ++other) {
		if (other == index) {
			continue;
		}
		if (legStates_[other].stepping && (other % 2) != group) {
			return false;
		}
	}
	return true;
}

bool GuardianGait::IsPlanted(int index) const {
	if (index < 0 || index >= kGuardianLegCount) {
		return false;
	}
	// 浮いている脚は胴体を支えないので、接地扱いにしない。
	return !legStates_[index].stepping && !legStates_[index].airborne;
}

bool GuardianGait::IsAirborne(int index) const {
	if (index < 0 || index >= kGuardianLegCount) {
		return false;
	}
	return legStates_[index].airborne;
}

bool GuardianGait::HasGroundContact() const {
	for (const LegState& state : legStates_) {
		if (!state.airborne) {
			return true;
		}
	}
	return false;
}

KujataEngine::Vector3 GuardianGait::ComputeDanglePosition(
    const KujataEngine::Vector3& hipPosition, const KujataEngine::Vector3& rootPosition, float maxReach) const {

	// ルート中心から接合部へ向かう水平方向。ここへ開くと4本が均等に広がる。
	Vector3 outward = hipPosition - rootPosition;
	outward.y = 0.0f;
	outward = GuardianRigMath::SafeNormalize(outward, {0.0f, 0.0f, 1.0f});

	Vector3 direction = GuardianRigMath::SafeNormalize(Vector3{0.0f, -1.0f, 0.0f} + outward * dangleSpread_, {0.0f, -1.0f, 0.0f});
	return hipPosition + direction * (maxReach * dangleReachRatio_);
}

float GuardianGait::GetStepActivity() const {
	int stepping = 0;
	for (const LegState& state : legStates_) {
		if (state.stepping) {
			++stepping;
		}
	}
	return static_cast<float>(stepping) / static_cast<float>(kGuardianLegCount);
}

float GuardianGait::SampleGroundHeight(const KujataEngine::Vector3& position) const {
	if (!raycastGround_) {
		return groundY_;
	}

	const GameObject* owner = GetOwner();
	if (!owner) {
		return groundY_;
	}
	KujataEngine::Scene* scene = owner->GetScene();
	if (!scene) {
		return groundY_;
	}

	Vector3 origin = {position.x, position.y + rayUp_, position.z};
	float maxDistance = rayUp_ + rayDown_;
	float nearestDistance = maxDistance;
	bool hit = false;

	for (const std::unique_ptr<GameObject>& gameObject : scene->GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}
		// 自分の胴体や脚を地面と勘違いしないよう、ガーディアンの階層は丸ごと除外する。
		if (IsSelfOrDescendantOf(gameObject.get(), owner)) {
			continue;
		}
		// 動くコライダー(プレイヤー・敵・飛び道具など)には足を置かない。
		// 乗り上げてしまうと、相手が動いた瞬間に足が宙に取り残されて歩容が破綻する。
		if (staticGroundOnly_ && !IsStaticGround(*gameObject)) {
			continue;
		}

		uint32_t layer = (std::min)(gameObject->GetLayer(), 31u);
		if ((groundLayerMask_ & (1u << layer)) == 0) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			ColliderComponent* collider = dynamic_cast<ColliderComponent*>(component.get());
			if (!collider || !collider->IsEnabled() || collider->IsTrigger()) {
				continue;
			}

			float distance = 0.0f;
			if (RaycastDownCollider(*collider, origin, maxDistance, distance)) {
				hit = true;
				nearestDistance = (std::min)(nearestDistance, distance);
			}
		}
	}

	if (!hit) {
		return groundY_;
	}
	return origin.y - nearestDistance;
}
