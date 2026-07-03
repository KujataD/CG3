#include "RotatorComponent.h"
#include "../scene/GameObject.h"

namespace KujakuEngine {

void RotatorComponent::Update() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().rotation_.y += speed_;
}

} // namespace KujakuEngine
