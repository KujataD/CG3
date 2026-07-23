#include "CharacterMotor.h"
#include "PlayerHealth.h"

#include <scene/MovementUtil.h>

using namespace KujakuEngine;

namespace {

// フレーム数指定(60fps基準)を秒へ変換する係数。
constexpr float kFrameSeconds = 1.0f / 60.0f;

// 自身または子孫からAnimatorComponentを探す(モデルは子に分離されている構成のため)。
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

void CharacterMotor::OnPlayStart() {
	animator_ = FindAnimatorRecursive(owner_);
	modelObject_ = animator_ ? animator_->GetOwner() : nullptr;
	health_ = GetComponent<PlayerHealth>();
	stunTimer_ = 0.0f;
	knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
	dodgeTimer_ = 0.0f;
	dodgeCooldownTimer_ = 0.0f;
	invincibleTimer_ = 0.0f;
}

void CharacterMotor::Update() {
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

	// 回避中: 前転しながら向いている方向へ前進。移動指示は受け付けない。
	if (dodgeTimer_ > 0.0f) {
		UpdateDodge(deltaTime);
		return;
	}

	// 硬直中は移動指示を受け付けず、ノックバックで滑るだけ。
	if (stunTimer_ > 0.0f) {
		UpdateKnockback(deltaTime);
	}
}

void CharacterMotor::MoveCameraRelative(const Vector3& input) {
	if (!owner_ || IsActionLocked()) {
		return;
	}

	// カメラ基準の方向へ移動し、移動方向へTurn Speedでスムーズに旋回する(エンジン標準関数)。
	MovementUtil::MoveAndFace(*owner_, input, speed_, turnSpeed_, Time::GetDeltaTime());
}

void CharacterMotor::MoveWorld(const Vector3& direction, float speedScale) {
	if (!owner_ || IsActionLocked()) {
		return;
	}

	Vector3 horizontal = {direction.x, 0.0f, direction.z};
	float length = Length(horizontal);
	if (length <= 0.0001f) {
		return;
	}
	horizontal = horizontal / length;

	float deltaTime = Time::GetDeltaTime();
	WorldTransform& transform = owner_->GetTransform();
	MovementUtil::FaceDirection(transform, horizontal, turnSpeed_, deltaTime);
	transform.translation_ += horizontal * (speed_ * speedScale * deltaTime);
}

void CharacterMotor::FaceWorld(const Vector3& direction) {
	if (!owner_ || IsActionLocked()) {
		return;
	}

	Vector3 horizontal = {direction.x, 0.0f, direction.z};
	if (Length(horizontal) <= 0.0001f) {
		return;
	}
	MovementUtil::FaceDirection(owner_->GetTransform(), horizontal, turnSpeed_, Time::GetDeltaTime());
}

bool CharacterMotor::TryDodge() {
	if (IsActionLocked() || IsDodgeCooldown()) {
		return false;
	}

	dodgeTimer_ = static_cast<float>(dodgeDurationFrames_) * kFrameSeconds;
	invincibleTimer_ = static_cast<float>(invincibleFrames_) * kFrameSeconds;

	if (health_ && invincibleTimer_ > 0.0f) {
		health_->SetInvincible(true);
	}

	// 前転アニメーション(モデルのみ回転。コライダーは直立のまま)。
	if (animator_ && !dodgeClipName_.empty()) {
		animator_->PlayByName(dodgeClipName_.c_str());
	}
	return true;
}

void CharacterMotor::UpdateDodge(float deltaTime) {
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

void CharacterMotor::TickInvincibility(float deltaTime) {
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

void CharacterMotor::TransferModelRootMotion() {
	// モデルが自分自身(旧構成: Animatorが最上位に付いている)の場合は何もしない。
	if (!owner_ || !modelObject_ || modelObject_ == owner_) {
		return;
	}

	WorldTransform& modelTransform = modelObject_->GetTransform();
	// 水平(x/z)のみ移す。yはボビング等の見た目専用アニメーションのため残す。
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

void CharacterMotor::ApplyKnockback(const Vector3& velocity, float stunDuration) {
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
	if (animator_ && !recoilClipName_.empty()) {
		animator_->PlayByName(recoilClipName_.c_str());
	}
}

void CharacterMotor::UpdateKnockback(float deltaTime) {
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
