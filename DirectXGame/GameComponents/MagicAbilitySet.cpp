#include "MagicAbilitySet.h"
#include "CharacterMotor.h"
#include "MagicProjectile.h"

#include <Editor/PrefabAsset.h>
#include <scene/MovementUtil.h>

using namespace KujakuEngine;

void MagicAbilitySet::OnPlayStart() {
	motor_ = GetComponent<CharacterMotor>();
	animator_ = GetComponentInChildren<AnimatorComponent>();
	cooldownTimer_ = 0.0f;
	// プールはPlayインスタンスごとに作り直す(前回Playの弾ポインタは無効)。
	projectilePool_.clear();
}

void MagicAbilitySet::Update() {
	if (cooldownTimer_ > 0.0f) {
		cooldownTimer_ -= Time::GetDeltaTime();
		if (cooldownTimer_ < 0.0f) {
			cooldownTimer_ = 0.0f;
		}
	}
}

bool MagicAbilitySet::TryUse(int slot) {
	if (slot != 0 || !owner_) {
		return false;
	}
	if (cooldownTimer_ > 0.0f) {
		return false;
	}
	if (motor_ && motor_->IsActionLocked()) {
		return false;
	}

	GameObject* projectileObject = AcquireProjectile();
	if (!projectileObject) {
		return false;
	}

	// 自分の前方へ、少し浮かせた位置から撃つ。
	Vector3 forward = MovementUtil::GetForward(*owner_);
	forward.y = 0.0f;
	float length = Length(forward);
	if (length > 0.0001f) {
		forward = forward / length;
	} else {
		forward = {0.0f, 0.0f, 1.0f};
	}

	// 位置だけ発射時に決める(スケール等の見た目はPrefab側の定義を尊重する)。
	WorldTransform& transform = projectileObject->GetTransform();
	transform.translation_ = owner_->GetTransform().translation_ + forward * muzzleForward_ + Vector3{0.0f, muzzleHeight_, 0.0f};
	projectileObject->SetActive(true);

	if (MagicProjectile* projectile = projectileObject->GetComponent<MagicProjectile>()) {
		projectile->Fire(forward, projectileSpeed_, projectileLifetime_, projectileDamage_);
	}

	// 詠唱モーション(クリップができたらInspectorで指定する)。
	if (animator_ && !castClipName_.empty() && !animator_->IsPlaying()) {
		animator_->PlayByName(castClipName_.c_str());
	}

	cooldownTimer_ = cooldownSeconds_;
	return true;
}

bool MagicAbilitySet::IsBusy() const {
	return cooldownTimer_ > 0.0f;
}

GameObject* MagicAbilitySet::AcquireProjectile() {
	// 休眠中(非アクティブ)の弾を再利用する。
	for (GameObject* pooled : projectilePool_) {
		if (pooled && !pooled->IsActive()) {
			return pooled;
		}
	}

	Scene* scene = owner_ ? owner_->GetScene() : nullptr;
	if (!scene) {
		return nullptr;
	}

	// Prefabから新規生成(Playインスタンス内のオブジェクトなので、Play停止で消える)。
	// ランタイム弾にエディタのPrefab関連付けは不要なのでlinkInstance=false。
	PrefabAsset::InstantiateResult result = PrefabAsset::Instantiate(*scene, projectilePrefabPath_, false);
	if (!result.succeeded || !result.rootObject) {
		Logger::Log("[MagicAbilitySet] projectile prefab load failed (" + projectilePrefabPath_ + "): " + result.message);
		return nullptr;
	}

	projectilePool_.push_back(result.rootObject);
	return result.rootObject;
}
