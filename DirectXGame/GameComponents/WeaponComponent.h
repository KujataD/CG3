#pragma once

#include <KujataEngine.h>
#include <unordered_set>

class WeaponComponent : public KujataEngine::Component {
public:
	const char* GetTypeName() const override { return "WeaponComponent"; }

	void Initialize() override;
	void Update() override;
	void OnTriggerStay(KujataEngine::ColliderComponent* other) override;

private:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		// attack_はbool型アニメーションチャンネル。キーフレームで攻撃判定をON/OFFする(ため中はOFF)。
		KUJATA_REGISTER_BOOL(attack_);
		KUJATA_REGISTER_FLOAT(damageValue_, 1.0f, 0.0f, 0.0f);
		KUJATA_REGISTER_FLOAT(knockback_, 0.1f, 0.0f, 0.0f);
	}

	KUJATA_FIELD_BOOL(attack_, false);
	KUJATA_FIELD_FLOAT(damageValue_, 10);
	KUJATA_FIELD_FLOAT(knockback_, 5);

	// 1回の攻撃振り(attackがOFF→ONの間)でダメージ済みの敵。振り開始時にクリアする。
	// OnTriggerStayは毎フレーム呼ばれるため、これが無いと1振りで多段ヒットしてしまう。
	std::unordered_set<KujataEngine::GameObject*> hitThisSwing_;
	bool prevAttack_ = false;

	void ApplyDamageToEnemy(KujataEngine::GameObject* enemy);
};
