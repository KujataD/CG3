#include "PlayerAnimator.h"

using namespace KujakuEngine;

void PlayerAnimator::OnPlayStart() {
	wasPressed_ = false;
}

AnimatorComponent* PlayerAnimator::FindAnimator() const {
	GameObject* owner = GetOwner();
	if (!owner) {
		return nullptr;
	}
	for (const std::unique_ptr<Component>& component : owner->GetComponents()) {
		if (AnimatorComponent* animator = dynamic_cast<AnimatorComponent*>(component.get())) {
			return animator;
		}
	}
	return nullptr;
}

void PlayerAnimator::Update() {
	AnimatorComponent* animator = FindAnimator();
	if (!animator) {
		return;
	}

	// R2(右トリガー)は0〜1のアナログ値なので、しきい値でボタン化してエッジ検出する。
	float rightTrigger = Input::GetRightTrigger();
	bool isPressed = rightTrigger >= triggerThreshold_;

	if (isPressed && !wasPressed_) {
		// 押した瞬間に再生。Clip Name指定があればそのクリップへ切り替える。
		if (clipName_.empty()) {
			animator->Play();
		} else {
			if (!animator->PlayByName(clipName_)) {
				// 名前が見つからない場合は選択中クリップを再生する。
				animator->Play();
			}
		}
	}

	if (stopOnRelease_ && !isPressed && wasPressed_) {
		animator->Stop();
	}

	wasPressed_ = isPressed;
}
