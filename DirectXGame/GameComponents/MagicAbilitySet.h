#pragma once
#include "IAbilitySet.h"
#include <components/AnimatorComponent.h>
#include <vector>

class CharacterMotor;

/// <summary>
/// 魔法攻撃(Bishop用)のAbilitySet。
/// スロット0=前方へ魔法弾を発射する。弾はPrefab(Projectile Prefab)からランタイム生成し、
/// 非アクティブになったものをプールとして再利用する(Prefab読み込みは初弾のみ)。
/// 見た目・コライダー・発光はPrefabとそのMaterialアセット側で調整する。
/// 速度・ダメージなどの「技の性能」はこちらのフィールドで持ち、Fire()で弾へ注入する。
/// </summary>
class MagicAbilitySet : public IAbilitySet {
public:
	const char* GetTypeName() const override { return "MagicAbilitySet"; }
	bool AllowMultiple() const override { return false; }

	void OnPlayStart() override;
	void Update() override;

	bool TryUse(int slot) override;
	bool IsBusy() const override;

private:
	/// <summary>プールから休眠中の弾を返す。無ければPrefabから新規生成する。</summary>
	KujakuEngine::GameObject* AcquireProjectile();

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_STRING_NAMED(projectilePrefabPath_, "Projectile Prefab");
		KUJAKU_REGISTER_FLOAT_NAMED(cooldownSeconds_, "Cooldown Seconds", 0.05f, 0.0f, 10.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(projectileSpeed_, "Projectile Speed", 0.1f, 0.1f, 100.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(projectileLifetime_, "Projectile Lifetime", 0.1f, 0.1f, 30.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(projectileDamage_, "Projectile Damage", 1.0f, 0.0f, 1000.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(muzzleForward_, "Muzzle Forward", 0.05f, 0.0f, 10.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(muzzleHeight_, "Muzzle Height", 0.05f, -10.0f, 10.0f);
		KUJAKU_REGISTER_STRING_NAMED(castClipName_, "Cast Clip");
	}

	// 弾のPrefab(プロジェクトルート相対)。見た目・スケール・コライダー・発光Materialはここで定義する。
	KUJAKU_FIELD_STRING(projectilePrefabPath_, "Prefabs/MagicBolt.prefab.json");

	// 発射間隔[s]。
	KUJAKU_FIELD_FLOAT(cooldownSeconds_, 1.0f);
	// 弾速[unit/s]。
	KUJAKU_FIELD_FLOAT(projectileSpeed_, 15.0f);
	// 弾の寿命[s]。
	KUJAKU_FIELD_FLOAT(projectileLifetime_, 3.0f);
	// 弾のダメージ。
	KUJAKU_FIELD_FLOAT(projectileDamage_, 10.0f);
	// 発射位置: 自分の前方オフセット。
	KUJAKU_FIELD_FLOAT(muzzleForward_, 1.2f);
	// 発射位置: 上方向オフセット。
	KUJAKU_FIELD_FLOAT(muzzleHeight_, 0.5f);
	// 発射時に再生する詠唱クリップ名(空なら再生しない。Bishop用クリップができたら設定する)。
	KUJAKU_FIELD_STRING(castClipName_, "");

	// 発射クールダウンの残り[s]。
	float cooldownTimer_ = 0.0f;

	// 生成済みの弾プール(Playインスタンス内のみ。停止で消える)。
	std::vector<KujakuEngine::GameObject*> projectilePool_;

	// モデル(子)のAnimator(詠唱モーション用。無くてもよい)。
	KujakuEngine::AnimatorComponent* animator_ = nullptr;
	// 同じGameObjectのCharacterMotor(行動不能チェック用)。
	CharacterMotor* motor_ = nullptr;
};
