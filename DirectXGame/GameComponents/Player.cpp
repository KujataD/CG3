#include "Player.h"
#include "PlayerHealth.h"

#include <scene/MovementUtil.h>

using namespace KujakuEngine;

namespace {

// フレーム数指定(60fps基準)を秒へ変換する係数。
constexpr float kFrameSeconds = 1.0f / 60.0f;

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
	health_ = GetComponent<PlayerHealth>();
	dodgeTimer_ = 0.0f;
	dodgeCooldownTimer_ = 0.0f;
	invincibleTimer_ = 0.0f;
}

void Player::Update() {
	float deltaTime = Time::GetDeltaTime();

	// アニメーションのルートモーション(攻撃の踏み込み等)をコライダー側の最上位へ反映する。
	TransferModelRootMotion();

	// 無敵時間は状態に関わらず進める(回避が中断されても無敵だけ残らないように)。
	TickInvincibility(deltaTime);

	// 回避クールダウンは常に進める(クールダウン中も移動や攻撃は可能。再回避だけ不可)。
	if (dodgeCooldownTimer_ > 0.0f) {
		dodgeCooldownTimer_ -= deltaTime;
		if (dodgeCooldownTimer_ < 0.0f) {
			dodgeCooldownTimer_ = 0.0f;
		}
	}

	// 回避中: 前転しながら向いている方向へ前進。移動含め他の行動はできない。
	if (dodgeTimer_ > 0.0f) {
		UpdateDodge(deltaTime);
		return;
	}

	// 硬直中は入力移動を受け付けず、ノックバックで滑るだけ。
	if (stunTimer_ > 0.0f) {
		UpdateKnockback(deltaTime);
		return;
	}

	// 回避入力(Aボタン / Space)。クールダウン中は受け付けない。
	if (dodgeCooldownTimer_ <= 0.0f && (Input::GetControllerButtonTrigger(XINPUT_GAMEPAD_A) || Input::GetKeyTrigger(DIK_SPACE))) {
		StartDodge();
		return;
	}

	Move();
}

void Player::StartDodge() {
	dodgeTimer_ = static_cast<float>(dodgeDurationFrames_) * kFrameSeconds;
	invincibleTimer_ = static_cast<float>(invincibleFrames_) * kFrameSeconds;

	if (health_ && invincibleTimer_ > 0.0f) {
		health_->SetInvincible(true);
	}

	// 前転アニメーション(モデルのみ回転。コライダーは直立のまま)。
	if (animator_) {
		animator_->PlayByName("PlayerRoll");
	}
}

void Player::UpdateDodge(float deltaTime) {
	dodgeTimer_ -= deltaTime;

	// 向いている方向(ルートの前方)へ前進する。
	if (owner_) {
		WorldTransform& transform = owner_->GetTransform();
		Vector3 forward = transform.GetRotationQuaternion().RotateVector({0.0f, 0.0f, 1.0f});
		forward.y = 0.0f;
		float length = Length(forward);
		if (length > 0.0001f) {
			transform.translation_ += forward * (dodgeSpeed_ * deltaTime / length);
		}
	}

	if (dodgeTimer_ <= 0.0f) {
		dodgeTimer_ = 0.0f;
		dodgeCooldownTimer_ = static_cast<float>(dodgeCooldownFrames_) * kFrameSeconds;
	}
}

void Player::TickInvincibility(float deltaTime) {
	if (invincibleTimer_ <= 0.0f) {
		return;
	}
	invincibleTimer_ -= deltaTime;
	if (invincibleTimer_ <= 0.0f) {
		invincibleTimer_ = 0.0f;
		if (health_) {
			health_->SetInvincible(false);
		}
	}
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
	// 回避中(無敵切れ後)に被弾したら回避を中断してクールダウンへ移行する。
	if (dodgeTimer_ > 0.0f) {
		dodgeTimer_ = 0.0f;
		dodgeCooldownTimer_ = static_cast<float>(dodgeCooldownFrames_) * kFrameSeconds;
	}

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
