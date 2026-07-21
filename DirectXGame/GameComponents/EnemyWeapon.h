#pragma once

#include <KujakuEngine.h>
#include <unordered_set>

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
	}

	KUJAKU_FIELD_BOOL(attack_, false);
	KUJAKU_FIELD_FLOAT(damageValue_, 10);
	KUJAKU_FIELD_FLOAT(knockback_, 5);

	// 1回の攻撃振り(attackがOFF→ONの間)でダメージ済みの敵。振り開始時にクリアする。
	// OnTriggerStayは毎フレーム呼ばれるため、これが無いと1振りで多段ヒットしてしまう。
	std::unordered_set<KujakuEngine::GameObject*> hitThisSwing_;
	bool prevAttack_ = false;

	void ApplyDamageToEnemy(KujakuEngine::GameObject* enemy);
};
