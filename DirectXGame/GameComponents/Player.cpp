#include "Player.h"

#include <scene/MovementUtil.h>

using namespace KujakuEngine;

void Player::Initialize() {
}

void Player::OnPlayStart() {
	animator_ = GetComponent<AnimatorComponent>();
}

void Player::Update() {
	Move();
}

void Player::Move() {
	// ローカル入力(x=横, z=前後)。左スティック + WASD併用。
	Vector2 stick = Input::GetLeftStick();
	Vector3 input = { stick.x, 0.0f, stick.y };
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

	// カメラ基準の方向へ移動し、移動方向へTurn Speedでスムーズに旋回する(エンジン標準関数)。
	MovementUtil::MoveAndFace(*owner_, input, speed_, turnSpeed_, Time::GetDeltaTime());
}
