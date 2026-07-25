#include "RotatorComponent.h"
#include "../scene/GameObject.h"
#include <cmath>

namespace KujakuEngine {

void RotatorComponent::Update() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().rotation_.y += speed_;

	// Y軸上下(sin波): 基準位置は保存せず、前回適用したオフセットとの差分だけを足す。[
	// これにより他のコードがtranslation_を動かしても基準が追従し、位置がドリフトしない。]]'./
	bobPhase_ += bobSpeed_;
	float bobOffset = std::sin(bobPhase_) * bobDistance_;
	owner->GetTransform().translation_.y += bobOffset - appliedBobOffset_;
	appliedBobOffset_ = bobOffset;
}

} // namespace KujakuEngine
