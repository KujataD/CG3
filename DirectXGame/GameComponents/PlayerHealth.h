#pragma once

#include <KujakuEngine.h>
#include <functional>

class PlayerHealth : public KujakuEngine::Component {
public:
	// EnemyHealthが"HealthComponent"名で登録済みのため、衝突しない固有名にする。
	const char* GetTypeName() const override { return "PlayerHealth"; }

	void Initialize() override;

	void TakeDamage(float damage);
	float GetHealthPercent() const;
	bool IsAlive() const;

	/// <summary>無敵(回避中など)。trueの間TakeDamageを無視する。</summary>
	void SetInvincible(bool invincible) { invincible_ = invincible; }
	bool IsInvincible() const { return invincible_; }

	void SetOnHealthChanged(std::function<void(float)> cb);
	void SetOnDeath(std::function<void()> cb);

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(maxHealth_, 1.0f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(health_, 1.0f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_FLOAT(maxHealth_, 100);
	KUJAKU_FIELD_FLOAT(health_, 100);

	std::function<void(float)> onHealthChanged_;
	std::function<void()> onDeath_;

	// 無敵中か(シリアライズしないランタイム状態)。
	bool invincible_ = false;
};
