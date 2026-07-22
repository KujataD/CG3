#include "Player.h"

#include <scene/MovementUtil.h>

using namespace KujakuEngine;

namespace {

// 自身または子孫からAnimatorComponentを探す(モデルは子のPawnModelに分離されている)。
AnimatorComponent* FindAnimatorRecursive(GameObject* object) {
	if (!object) {
		return nullptr;
	}
	if (AnimatorComponent* animator = object->GetComponent<AnimatorComponent>()) {
		return animator;
	}
	for (GameObject* child : object->GetChildren()) {
		if (AnimatorComponent* animator = FindAnimatorRecursive(child)) {
			return animator;
		}
	}
	return nullptr;
}

} // namespace

void Player::Initialize() {
}

void Player::OnPlayStart() {
	animator_ = FindAnimatorRecursive(owner_);
	modelObject_ = animator_ ? animator_->GetOwner() : nullptr;
}

void Player::Update() {
	float deltaTime = Time::GetDeltaTime();

	// アニメーションのルートモーション(攻撃の踏み込み等)をコライダー側の最上位へ反映する。
	TransferModelRootMotion();

	// 硬直中は入力移動を受け付けず、ノックバックで滑るだけ。
	if (stunTimer_ > 0.0f) {
		UpdateKnockback(deltaTime);
		return;
	}

	Move();
}

void Player::TransferModelRootMotion() {
	// モデルが自分自身(旧構成: Animatorが最上位に付いている)の場合は何もしない。
	if (!owner_ || !modelObject_ || modelObject_ == owner_) {
		return;
	}

	WorldTransform& modelTransform = modelObject_->GetTransform();
	// 水平(x/z)のみ移す。yはPlayerWalkのボビング等の見た目専用アニメーションのため残す。
	Vector3 localOffset = {modelTransform.translation_.x, 0.0f, modelTransform.translation_.z};
	if (localOffset.x * localOffset.x + localOffset.z * localOffset.z <= 1.0e-10f) {
		return;
	}

	// モデルのローカルオフセットを最上位のスケール・回転でワールドの移動量へ変換して移動する。
	// (モデルのワールド位置は変えず、親子間で移動量を付け替えるだけ)
	WorldTransform& rootTransform = owner_->GetTransform();
	Vector3 scaledOffset = {
	    localOffset.x * rootTransform.scale_.x,
	    0.0f,
	    localOffset.z * rootTransform.scale_.z,
	};
	rootTransform.translation_ += rootTransform.GetRotationQuaternion().RotateVector(scaledOffset);
	modelTransform.translation_.x = 0.0f;
	modelTransform.translation_.z = 0.0f;
}

void Player::ApplyKnockback(const Vector3& velocity, float stunDuration) {
	knockbackVelocity_ = velocity;
	if (stunDuration > stunTimer_) {
		stunTimer_ = stunDuration;
	}

	// のけぞりアニメーション。localRotationチャンネルなので今向いている方向を基準に後ろへ傾く。
	if (animator_) {
		animator_->PlayByName("PlayerRecoil");
	}
}

void Player::UpdateKnockback(float deltaTime) {
	stunTimer_ -= deltaTime;
	if (stunTimer_ <= 0.0f) {
		stunTimer_ = 0.0f;
		knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
		return;
	}

	if (!owner_) {
		return;
	}

	// 初速から指数減衰しながら滑る(フレームレート非依存)。
	owner_->GetTransform().translation_ += knockbackVelocity_ * deltaTime;
	float damping = std::exp(-knockbackDamping_ * deltaTime);
	knockbackVelocity_ = knockbackVelocity_ * damping;
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
