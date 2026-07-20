#include "EnemyComponent.h"

namespace {
constexpr const char* kBTSetFolder = "Resources/bt_set/EnemyBT";
}

using namespace KujakuEngine;

void EnemyComponent::Initialize() {
	// 作業ディレクトリはDirectXGame/なので "Resources/..." で参照する。

	// BTのアクション登録。
	// FunctionCatalogにも同時登録することで、BahamutAIEditorのAction一覧に名前が反映される。
	BahamutAI::FunctionCatalog catalog;

	BahamutAI::RegisterAction(
	    btFactory_, catalog, BahamutAI::ActionDef("MoveToTarget").Category("Movement").Description("参照先のTargetへ近づく"), [this](BahamutAI::AIContext& context) { return MoveToTarget(context); });

	BahamutAI::RegisterOwnerAction<EnemyComponent>(
	    btFactory_, catalog, BahamutAI::ActionDef("Move").Category("Movement").Description("前進する").Vec3("direction", {0.0f, 0.0f, 0.0f}, "移動方向").Float("speed", {0.01f}), [](EnemyComponent& owner, BahamutAI::AIContext& context, const BahamutAI::NodeParams& params)
		{ 
			BahamutAI::Vec3 direction = params.GetVec3("direction");
			float speed = params.GetFloat("speed");

			owner.owner_->GetTransform().translation_ += Vector3{direction.x, direction.y, direction.z} * speed;

			return BahamutAI::BTStatus::Success;
		});

	BahamutAI::RegisterAction(
	    btFactory_, catalog, BahamutAI::ActionDef("MoveBackward").Category("Movement").Description("後退する"), [this](BahamutAI::AIContext& context) { return MoveBackward(context); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog, BahamutAI::ActionDef("RotateY").Category("Movement").Description("Y軸方向に回転する"), [this](BahamutAI::AIContext& context) { return RotateY(context); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog, BahamutAI::ActionDef("RotateX").Category("Movement").Description("X軸方向に回転する"), [this](BahamutAI::AIContext& context) { return RotateX(context); });

	catalog.SaveToBTSetFolder(kBTSetFolder);
}

void EnemyComponent::OnPlayStart() {
	LoadBTSet();

	// 実プレイインスタンスとして初めて監視オブザーバーを生成する(ここで"Main"のprimaryを取得)。
	// 一度作ったらPlay/Stopを跨いで使い回す(再生成すると再度primaryを取り損ねるため)。
	if (!btObserver_) {
		btObserver_ = std::make_unique<BahamutAI::UdpTreeObserver>("Main");
	}
}

void EnemyComponent::RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) {
	// Inspectorで Button.onClick のメソッド候補に "LoadBTSet" が並ぶ。
	registry.Add("LoadBTSet", [this]() { LoadBTSet(); });
}

void EnemyComponent::Update() {
	if (!owner_ || !btRuntime_.IsLoaded()) {
		return;
	}
	localBlackboard_.SetValue("AAA", 10.0f);
	// 毎フレーム、AIContextを組み立ててTickする。
	BahamutAI::AIContext context{localBlackboard_};
	context.deltaTime = Time::GetDeltaTime();
	context.SetOwner(*owner_); // Action内で context.GetOwner<GameObject>() したい場合に使える
	context.observer = btObserver_.get();

	btRuntime_.Tick(context);
}

void EnemyComponent::LoadBTSet() {
	if (!btRuntime_.LoadFromBTSetFolder(kBTSetFolder, btFactory_)) {
		const BahamutAI::BehaviorTreeLoadResult& result = btRuntime_.GetLastLoadResult();
		KujakuEngine::Logger::Log(std::string("[EnemyComponent] BT load failed: ") + result.GetErrorMessage());
	}
}

BahamutAI::BTStatus EnemyComponent::MoveForward(BahamutAI::AIContext& context) {
	owner_->GetTransform().translation_.z += speed_;
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::MoveBackward(BahamutAI::AIContext& context) {
	owner_->GetTransform().translation_.z -= speed_;
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::RotateY(BahamutAI::AIContext& context) {
	owner_->GetTransform().rotation_.y += speed_;
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::RotateX(BahamutAI::AIContext& context) {
	owner_->GetTransform().rotation_.x += speed_;
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::MoveToTarget(BahamutAI::AIContext& context) {
	(void)context;

	// target_ はInspectorでドラッグ&ドロップ代入され、ロード時にPlayer*へ解決済み。
	// 未設定/対象消滅時はFailureを返す。
	if (!owner_ || !target_) {
		return BahamutAI::BTStatus::Failure;
	}

	// 型付き参照なのでPlayerのpublicへ直接アクセスできる(例: target_->GetOwner())。
	WorldTransform& self = owner_->GetTransform();
	const WorldTransform& targetTransform = target_->GetOwner()->GetTransform();

	Vector3 toTarget = targetTransform.translation_ - self.translation_;
	float distance = Length(toTarget);

	if (distance <= speed_ || distance <= 0.0001f) {
		return BahamutAI::BTStatus::Success;
	}

	self.translation_ += toTarget * (speed_ / distance);
	return BahamutAI::BTStatus::Running;
}
