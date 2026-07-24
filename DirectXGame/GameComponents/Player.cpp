#include "Player.h"
#include "CharacterMotor.h"
#include "IAbilitySet.h"

using namespace KujakuEngine;

void Player::OnPlayStart() {
	motor_ = GetComponent<CharacterMotor>();
	abilitySet_ = GetComponent<IAbilitySet>();
	wasAttackPressed_ = false;
}

void Player::Update() {
	if (!motor_) {
		return;
	}

	// 行動不能かどうか
	if (motor_->IsActionLocked()) {
		wasAttackPressed_ = false;
		return;
	}

	// 攻撃入力(R2)
	bool isAttackPressed = Input::GetRightTrigger() >= triggerThreshold_;
	if (!wasAttackPressed_ && abilitySet_) {	
		if (isAttackPressed || Input::GetKeyTrigger(DIK_K)) {
			abilitySet_->TryUse(0);
		}
	}
	wasAttackPressed_ = isAttackPressed;

	// 回避入力(Aボタン / Space)
	if (Input::GetControllerButtonTrigger(XINPUT_GAMEPAD_A) || Input::GetKeyTrigger(DIK_SPACE)) {
		if (motor_->TryDodge()) {
			return;
		}
	}

	// ローカル入力 (Lスティック / WASD)
	Vector2 stick = Input::GetLeftStick();
	Vector3 input = {stick.x, 0.0f, stick.y};
	if (Input::GetKey(DIK_W)) {
		input.z += 1.0f;
	}
	if (Input::GetKey(DIK_S)) {
		input.z -= 1.0f;
	}
	if (Input::GetKey(DIK_D)) {
		input.x += 1.0f;
	}
	if (Input::GetKey(DIK_A)) {
		input.x -= 1.0f;
	}

	motor_->MoveCameraRelative(input);
}
