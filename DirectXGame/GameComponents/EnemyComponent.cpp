#include "EnemyComponent.h"

void EnemyComponent::Initialize() {
	// BTのアクション登録
	KujakuEngine::GameObject* owner = GetOwner();

	btFactory_.RegisterAction("MoveForward", [this](BahamutAI::AIContext& context) { return MoveForward(context); });
	btFactory_.RegisterAction("MoveBackward", [this](BahamutAI::AIContext& context) { return MoveBackward(context); });
	btFactory_.RegisterAction("RotateY", [this](BahamutAI::AIContext& context) { return RotateY(context); });
	btFactory_.RegisterAction("RotateX", [this](BahamutAI::AIContext& context) { return RotateX(context); });
}

void EnemyComponent::Update() {
	KujakuEngine::GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}
}

BahamutAI::BTStatus EnemyComponent::MoveForward(BahamutAI::AIContext& context) {
	owner_->GetTransform().translation_.z += speed_;
	if (owner_->GetTransform().translation_.z < 5.0f) {
		return BahamutAI::BTStatus::Running;
	}
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::MoveBackward(BahamutAI::AIContext& context) {
	owner_->GetTransform().translation_.z -= speed_;
	if (owner_->GetTransform().translation_.z > 5.0f) {
		return BahamutAI::BTStatus::Running;
	}
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus EnemyComponent::RotateY(BahamutAI::AIContext& context) {}

BahamutAI::BTStatus EnemyComponent::RotateX(BahamutAI::AIContext& context) {}
