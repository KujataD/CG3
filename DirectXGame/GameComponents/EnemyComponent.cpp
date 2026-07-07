#include "EnemyComponent.h"

void EnemyComponent::Update() {
	KujakuEngine::GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().rotation_.y += speed_;
}
