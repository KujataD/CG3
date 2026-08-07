#pragma once

#include <KujataEngine.h>
#include <functional>

class EnemyHealth : public KujataEngine::Component {
public:
	const char* GetTypeName() const override { return "HealthComponent"; }

	void Initialize() override;

	void TakeDamage(float damage);
	float GetHealthPercent() const;
	bool IsAlive() const;

	void SetOnHealthChanged(std::function<void(float)> cb);
	void SetOnDeath(std::function<void()> cb);

private:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		KUJATA_REGISTER_FLOAT(maxHealth_, 1.0f, 0.0f, 0.0f);
		KUJATA_REGISTER_FLOAT(health_, 1.0f, 0.0f, 0.0f);
	}

	KUJATA_FIELD_FLOAT(maxHealth_, 100);
	KUJATA_FIELD_FLOAT(health_, 100);

	std::function<void(float)> onHealthChanged_;
	std::function<void()> onDeath_;
};
