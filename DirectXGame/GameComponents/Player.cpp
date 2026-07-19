#include "Player.h"

using namespace KujakuEngine;

void Player::Initialize() {

}

void Player::Update() {

	Vector2 velocity = Input::GetLeftStick() * speed_;

	owner_->GetTransform().translation_.x += velocity.x;
	owner_->GetTransform().translation_.z += velocity.y;
}
