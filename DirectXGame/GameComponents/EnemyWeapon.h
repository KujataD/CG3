#pragma once

#include <KujakuEngine.h>
#include <unordered_map>

class EnemyWeapon : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "EnemyWeapon"; }

	void Initialize() override;
	void Update() override;
	void OnTriggerStay(KujakuEngine::ColliderComponent* other) override;

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		// attack_はbool型アニメーションチャンネル。キーフレームで攻撃判定をON/OFFする(ため中はOFF)。
		KUJAKU_REGISTER_BOOL(attack_);
		KUJAKU_REGISTER_FLOAT(damageValue_, 1.0f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(knockback_, 0.1f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(hitInterval_, 0.05f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(stunDuration_, 0.05f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_BOOL(attack_, false);
	KUJAKU_FIELD_FLOAT(damageValue_, 10);
	// ノックバックの初速[unit/s]。
	KUJAKU_FIELD_FLOAT(knockback_, 5);
	// 同一対象への再ヒット間隔[s]。1回のアニメーション中でも、この間隔ごとに当たるたびダメージが入る。
	KUJAKU_FIELD_FLOAT(hitInterval_, 0.5f);
	// ヒット時にプレイヤーが動けなくなる時間[s]。
	KUJAKU_FIELD_FLOAT(stunDuration_, 0.6f);

	// 対象ごとの再ヒットまでの残り時間。OnTriggerStayは毎フレーム呼ばれるため、
	// これが無いと接触中に毎フレームダメージが入ってしまう。攻撃振り開始時にクリアする。
	std::unordered_map<KujakuEngine::GameObject*, float> hitCooldowns_;
	bool prevAttack_ = false;

	void ApplyDamageToPlayer(KujakuEngine::GameObject* target);
};
