#include "Player.h"

using namespace KujakuEngine;

void Player::Initialize() {

}

void Player::Update() {
	Vector3 velocity{};
	if (Input::GetKey(DIK_W)) {
		velocity.z += speed_;
	}
	if (Input::GetKey(DIK_S)) {
		velocity.z -= speed_;
	}
	if (Input::GetKey(DIK_D)) {
		velocity.x += speed_;
	}
	if (Input::GetKey(DIK_A)) {
		velocity.x -= speed_;
	}
	owner_->GetTransform().translation_ += velocity;
}
