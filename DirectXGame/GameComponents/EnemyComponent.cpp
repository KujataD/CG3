#include "EnemyComponent.h"

namespace {
constexpr const char* kBTSetFolder = "Resources/bt_set/EnemyBT";
}

void EnemyComponent::Initialize() {
	// 作業ディレクトリはDirectXGame/なので "Resources/..." で参照する。

	// BTのアクション登録。
	// FunctionCatalogにも同時登録することで、BahamutAIEditorのAction一覧に名前が反映される。
	BahamutAI::FunctionCatalog catalog;

	BahamutAI::RegisterAction(
		btFactory_, catalog,
		BahamutAI::ActionDef("MoveForward").Category("Movement").Description("前進する"),
		[this](BahamutAI::AIContext& context) { return MoveForward(context); });

	BahamutAI::RegisterAction(
		btFactory_, catalog,
		BahamutAI::ActionDef("MoveBackward").Category("Movement").Description("後退する"),
		[this](BahamutAI::AIContext& context) { return MoveBackward(context); });

	BahamutAI::RegisterAction(
		btFactory_, catalog,
		BahamutAI::ActionDef("RotateY").Category("Movement").Description("Y軸方向に回転する"),
		[this](BahamutAI::AIContext& context) { return RotateY(context); });

	BahamutAI::RegisterAction(
		btFactory_, catalog,
		BahamutAI::ActionDef("RotateX").Category("Movement").Description("X軸方向に回転する"),
		[this](BahamutAI::AIContext& context) { return RotateX(context); });

	catalog.SaveToBTSetFolder(kBTSetFolder);

}

void EnemyComponent::OnPlayStart() {
	// ツリー(木構造)をBTSetフォルダからロードする。
	if (!btRuntime_.LoadFromBTSetFolder(kBTSetFolder, btFactory_)) {
		const BahamutAI::BehaviorTreeLoadResult& result = btRuntime_.GetLastLoadResult();
		KujakuEngine::Logger::Log(std::string("[EnemyComponent] BT load failed: ") + result.GetErrorMessage());
	}
}

void EnemyComponent::Update() {
	if (!owner_ || !btRuntime_.IsLoaded()) {
		return;
	}

	// 毎フレーム、AIContextを組み立ててTickする。
	BahamutAI::AIContext context{ localBlackboard_ };
	context.deltaTime = Time::GetDeltaTime();
	context.SetOwner(*owner_); // Action内で context.GetOwner<GameObject>() したい場合に使える

	btRuntime_.Tick(context);
}

BahamutAI::BTStatus EnemyComponent::MoveForward(BahamutAI::AIContext& context) {
	if (owner_->GetTransform().translation_.z >= 5.0f) {
		return BahamutAI::BTStatus::Failure;
	}
	owner_->GetTransform().translation_.z += speed_;
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::MoveBackward(BahamutAI::AIContext& context) {
	owner_->GetTransform().translation_.z -= speed_;
	if (owner_->GetTransform().translation_.z > 5.0f) {
		return BahamutAI::BTStatus::Running;
	}
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
