#pragma once

#include <KujakuEngine.h>
#include <functional>

class EnemyHealth : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "HealthComponent"; }

	void Initialize() override;

	void TakeDamage(float damage);
	float GetHealthPercent() const;
	bool IsAlive() const;

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
};
