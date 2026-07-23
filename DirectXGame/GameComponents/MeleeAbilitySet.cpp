#include "MeleeAbilitySet.h"
#include "CharacterMotor.h"

using namespace KujakuEngine;

void MeleeAbilitySet::OnPlayStart() {
	motor_ = GetComponent<CharacterMotor>();
	animator_ = GetComponentInChildren<AnimatorComponent>();
}

bool MeleeAbilitySet::TryUse(int slot) {
	if (slot != 0 || !animator_ || attackClipName_.empty()) {
		return false;
	}

	// 行動不能中(被弾のけぞり・回避中)や、前の振りがまだ再生中なら出せない。
	if (motor_ && motor_->IsActionLocked()) {
		return false;
	}
	if (animator_->IsPlaying()) {
		return false;
	}

	return animator_->PlayByName(attackClipName_.c_str());
}

bool MeleeAbilitySet::IsBusy() const {
	return animator_ && animator_->IsPlaying();
}
