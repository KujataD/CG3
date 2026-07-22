#include "HammerEnemyComponent.h"

#include "../KujakuEngine/components/AnimatorComponent.h"
#include "EnemyHealth.h"
#include <cmath>
#include <numbers>

namespace {

constexpr const char* kBTSetFolder = "Resources/bt_set/HammerEnemyBT";

// 攻撃アニメーションのクリップ名(Animations/*.anim.jsonのnameと一致させる)。
constexpr const char* kSpinClipName = "HammerSpin";
constexpr const char* kSlamClipName = "HammerSlam";
constexpr const char* kRecoilClipName = "HammerRecoil";

// ハンマーの回転中心となる子孫オブジェクト名。
constexpr const char* kHammerPivotName = "HammerPivot";

// 角度を[-π, π]へ折り返す(KujakuEngine::WrapAngleと衝突しないローカル名)。
float WrapYawAngle(float angle) {
	constexpr float pi = std::numbers::pi_v<float>;
	angle = std::fmod(angle + pi, 2.0f * pi);
	if (angle < 0.0f) {
		angle += 2.0f * pi;
	}
	return angle - pi;
}

// 自身または子孫から名前でGameObjectを探す(モデルは子のHammerEnemyModelに分離されている)。
KujakuEngine::GameObject* FindDescendantByName(KujakuEngine::GameObject* object, const char* name) {
	if (!object) {
		return nullptr;
	}
	for (KujakuEngine::GameObject* child : object->GetChildren()) {
		if (!child) {
			continue;
		}
		if (child->GetName() == name) {
			return child;
		}
		if (KujakuEngine::GameObject* found = FindDescendantByName(child, name)) {
			return found;
		}
	}
	return nullptr;
}

// 自身または子孫からAnimatorComponentを探す。
KujakuEngine::AnimatorComponent* FindAnimatorRecursive(KujakuEngine::GameObject* object) {
	if (!object) {
		return nullptr;
	}
	if (KujakuEngine::AnimatorComponent* animator = object->GetComponent<KujakuEngine::AnimatorComponent>()) {
		return animator;
	}
	for (KujakuEngine::GameObject* child : object->GetChildren()) {
		if (KujakuEngine::AnimatorComponent* animator = FindAnimatorRecursive(child)) {
			return animator;
		}
	}
	return nullptr;
}

} // namespace

using namespace KujakuEngine;

void HammerEnemyComponent::Initialize() {
	// BTのアクション登録。FunctionCatalogへ同時登録するとBahamutAIEditorのAction一覧に反映される。
	BahamutAI::FunctionCatalog catalog;

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("MoveToPlayer").Category("Movement").Description("プレイヤーへ歩いて近づく").Float("speed", {2.0f}, "歩行速度(m/s)").Float("stopDistance", {4.0f}, "この距離まで近づいたらSuccess"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return MoveToPlayer(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("FacePlayer").Category("Movement").Description("プレイヤーの方向へ旋回する").Float("turnSpeed", {6.0f}, "旋回速度(rad/s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return FacePlayer(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("IsPlayerInRange").Category("Sense").Description("プレイヤーが範囲内ならSuccess").Float("range", {6.0f}, "判定距離"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return IsPlayerInRange(params);
	    });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("SpinAttack").Category("Attack").Description("パターン1: ハンマーを取り出して振りかぶり、自身を中心に2秒間回転切り"),
	    [this](BahamutAI::AIContext& context) { return SpinAttack(context); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("SlamAttack").Category("Attack").Description("パターン2: 振りかぶり→溜め→プレイヤーへ歩きながら上段からたたきつけ"),
	    [this](BahamutAI::AIContext& context) { return SlamAttack(context); });

	catalog.SaveToBTSetFolder(kBTSetFolder);
}

void HammerEnemyComponent::OnPlayStart() {
	LoadBTSet();

	// 実プレイインスタンスとして初めて監視オブザーバーを生成する(EnemyComponentの"Main"と衝突しない別チャンネル)。
	if (!btObserver_) {
		btObserver_ = std::make_unique<BahamutAI::UdpTreeObserver>("HammerEnemy");
	}

	// ハンマーは攻撃時だけ取り出す。開始時は収納。
	spinActive_ = false;
	slamActive_ = false;
	slamTimer_ = 0.0f;
	recoilTimer_ = 0.0f;
	SetHammerVisible(false);

	// 被弾(HPの変化)でのけぞりを起こす。
	if (owner_) {
		if (EnemyHealth* health = owner_->GetComponent<EnemyHealth>()) {
			health->SetOnHealthChanged([this](float) { OnDamaged(); });
		}
	}
}

void HammerEnemyComponent::RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) {
	registry.Add("LoadBTSet", [this]() { LoadBTSet(); });
}

void HammerEnemyComponent::Update() {
	if (!owner_ || !btRuntime_.IsLoaded()) {
		return;
	}

	// のけぞりの硬直時間を進める。
	if (recoilTimer_ > 0.0f) {
		recoilTimer_ -= Time::GetDeltaTime();
		if (recoilTimer_ < 0.0f) {
			recoilTimer_ = 0.0f;
		}
	}

	BahamutAI::AIContext context{localBlackboard_};
	context.deltaTime = Time::GetDeltaTime();
	context.SetOwner(*owner_);
	context.observer = btObserver_.get();

	btRuntime_.Tick(context);
}

void HammerEnemyComponent::LoadBTSet() {
	if (!btRuntime_.LoadFromBTSetFolder(kBTSetFolder, btFactory_)) {
		const BahamutAI::BehaviorTreeLoadResult& result = btRuntime_.GetLastLoadResult();
		KujakuEngine::Logger::Log(std::string("[HammerEnemyComponent] BT load failed: ") + result.GetErrorMessage());
	}
}

// ---------------------------------------------------------------------------
// BT Actions
// ---------------------------------------------------------------------------

BahamutAI::BTStatus HammerEnemyComponent::MoveToPlayer(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	if (IsRecoiling()) {
		return BahamutAI::BTStatus::Failure;
	}
	GameObject* player = FindPlayer();
	if (!owner_ || !player) {
		return BahamutAI::BTStatus::Failure;
	}

	float speed = params.GetFloat("speed", walkSpeed_);
	float stopDistance = params.GetFloat("stopDistance", 4.0f);

	Vector3 toPlayer = player->GetTransform().translation_ - owner_->GetTransform().translation_;
	toPlayer.y = 0.0f;
	float distance = Length(toPlayer);
	if (distance <= stopDistance) {
		return BahamutAI::BTStatus::Success;
	}

	RotateTowardsPlayer(turnSpeed_, context.deltaTime);
	MoveTowardsPlayer(speed, context.deltaTime);
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus HammerEnemyComponent::FacePlayer(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	if (IsRecoiling()) {
		return BahamutAI::BTStatus::Failure;
	}
	GameObject* player = FindPlayer();
	if (!owner_ || !player) {
		return BahamutAI::BTStatus::Failure;
	}

	float turnSpeed = params.GetFloat("turnSpeed", turnSpeed_);
	if (RotateTowardsPlayer(turnSpeed, context.deltaTime)) {
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus HammerEnemyComponent::IsPlayerInRange(const BahamutAI::NodeParams& params) {
	GameObject* player = FindPlayer();
	if (!owner_ || !player) {
		return BahamutAI::BTStatus::Failure;
	}

	float range = params.GetFloat("range", 6.0f);
	Vector3 toPlayer = player->GetTransform().translation_ - owner_->GetTransform().translation_;
	toPlayer.y = 0.0f;
	return (Length(toPlayer) <= range) ? BahamutAI::BTStatus::Success : BahamutAI::BTStatus::Failure;
}

BahamutAI::BTStatus HammerEnemyComponent::SpinAttack(BahamutAI::AIContext& context) {
	(void)context;

	if (IsRecoiling()) {
		return BahamutAI::BTStatus::Failure;
	}
	AnimatorComponent* animator = GetAnimator();
	if (!owner_ || !animator) {
		return BahamutAI::BTStatus::Failure;
	}

	if (!spinActive_) {
		// 予備動作開始: ハンマーを取り出してクリップ再生。
		SetHammerVisible(true);
		if (!animator->PlayByName(kSpinClipName)) {
			FinishAttack();
			return BahamutAI::BTStatus::Failure;
		}
		spinActive_ = true;
		return BahamutAI::BTStatus::Running;
	}

	// クリップ(振りかぶり→2秒回転→戻し)が終わるまでRunning。
	if (animator->IsPlaying()) {
		return BahamutAI::BTStatus::Running;
	}

	FinishAttack();
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus HammerEnemyComponent::SlamAttack(BahamutAI::AIContext& context) {
	if (IsRecoiling()) {
		return BahamutAI::BTStatus::Failure;
	}
	AnimatorComponent* animator = GetAnimator();
	if (!owner_ || !animator) {
		return BahamutAI::BTStatus::Failure;
	}

	if (!slamActive_) {
		// 予備動作開始: ハンマーを取り出して振りかぶりクリップ再生。
		SetHammerVisible(true);
		if (!animator->PlayByName(kSlamClipName)) {
			FinishAttack();
			return BahamutAI::BTStatus::Failure;
		}
		slamActive_ = true;
		slamTimer_ = 0.0f;
		return BahamutAI::BTStatus::Running;
	}

	slamTimer_ += context.deltaTime;

	if (slamTimer_ < slamWalkStart_) {
		// 振りかぶり〜溜め: その場でプレイヤーへ向き続ける。
		RotateTowardsPlayer(turnSpeed_, context.deltaTime);
	} else if (slamTimer_ <= slamWalkEnd_) {
		// たたきつけ: プレイヤーの方向へ歩きながら振り下ろす。
		RotateTowardsPlayer(turnSpeed_, context.deltaTime);
		MoveTowardsPlayer(slamWalkSpeed_, context.deltaTime);
	}

	if (animator->IsPlaying()) {
		return BahamutAI::BTStatus::Running;
	}

	FinishAttack();
	return BahamutAI::BTStatus::Success;
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

KujakuEngine::GameObject* HammerEnemyComponent::FindPlayer() {
	if (!owner_ || !owner_->GetScene()) {
		return nullptr;
	}
	return owner_->GetScene()->FindGameObjectByName(playerName_);
}

KujakuEngine::GameObject* HammerEnemyComponent::FindHammerPivot() {
	// モデル分離により HammerEnemyModel/HammerPivot の階層になったため子孫まで探す。
	return FindDescendantByName(owner_, kHammerPivotName);
}

KujakuEngine::AnimatorComponent* HammerEnemyComponent::GetAnimator() {
	// Animatorはモデル側(子のHammerEnemyModel)にある。
	return FindAnimatorRecursive(owner_);
}

void HammerEnemyComponent::OnDamaged() {
	if (!recoilEnabled_) {
		return;
	}
	// 状態ごとののけぞり可否。既定では回転攻撃中はのけぞらない(スーパーアーマー)。
	if (spinActive_ && !recoilDuringSpin_) {
		return;
	}
	if (slamActive_ && !recoilDuringSlam_) {
		return;
	}

	AnimatorComponent* animator = GetAnimator();
	if (!animator) {
		return;
	}

	// 攻撃中なら中断してハンマーを収納し、のけぞりを再生する。
	FinishAttack();
	if (animator->PlayByName(kRecoilClipName)) {
		recoilTimer_ = recoilDuration_;
	}
}

void HammerEnemyComponent::SetHammerVisible(bool visible) {
	GameObject* hammer = FindHammerPivot();
	if (hammer) {
		// 非アクティブ化で描画もトリガー判定も同時に消える(CollectSceneCollidersはActiveInHierarchyのみ収集)。
		hammer->SetActive(visible);
	}
}

bool HammerEnemyComponent::RotateTowardsPlayer(float turnSpeed, float deltaTime) {
	GameObject* player = FindPlayer();
	if (!owner_ || !player) {
		return false;
	}

	Vector3 toPlayer = player->GetTransform().translation_ - owner_->GetTransform().translation_;
	toPlayer.y = 0.0f;
	if (Length(toPlayer) <= 0.0001f) {
		return true;
	}

	// +Z前方の左手系Yawはatan2(x, z)。
	float targetYaw = std::atan2(toPlayer.x, toPlayer.z);
	float currentYaw = owner_->GetTransform().rotation_.y;
	float difference = WrapYawAngle(targetYaw - currentYaw);

	float maxStep = turnSpeed * deltaTime;
	if (std::fabs(difference) <= maxStep) {
		owner_->GetTransform().rotation_.y = targetYaw;
		return true;
	}

	owner_->GetTransform().rotation_.y = WrapYawAngle(currentYaw + (difference > 0.0f ? maxStep : -maxStep));
	return false;
}

void HammerEnemyComponent::MoveTowardsPlayer(float speed, float deltaTime) {
	GameObject* player = FindPlayer();
	if (!owner_ || !player) {
		return;
	}

	Vector3 toPlayer = player->GetTransform().translation_ - owner_->GetTransform().translation_;
	toPlayer.y = 0.0f;
	float distance = Length(toPlayer);
	if (distance <= 0.0001f) {
		return;
	}

	owner_->GetTransform().translation_ += toPlayer * (speed * deltaTime / distance);
}

void HammerEnemyComponent::FinishAttack() {
	spinActive_ = false;
	slamActive_ = false;
	slamTimer_ = 0.0f;
	SetHammerVisible(false);
}
