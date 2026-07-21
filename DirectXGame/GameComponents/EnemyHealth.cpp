#include "EnemyHealth.h"
#include <algorithm>

void EnemyHealth::Initialize() {
	health_ = maxHealth_;
}

void EnemyHealth::TakeDamage(float damage) {
	if (!IsAlive()) {
		return;
	}

	health_ = std::max(0.0f, health_ - damage);

	if (onHealthChanged_) {
		onHealthChanged_(health_);
	}

	if (health_ <= 0 && onDeath_) {
		onDeath_();
	}
}

float EnemyHealth::GetHealthPercent() const {
	if (maxHealth_ <= 0) {
		return 0.0f;
	}
	return health_ / maxHealth_;
}

bool EnemyHealth::IsAlive() const {
	return health_ > 0;
}

void EnemyHealth::SetOnHealthChanged(std::function<void(float)> cb) {
	onHealthChanged_ = cb;
}

void EnemyHealth::SetOnDeath(std::function<void()> cb) {
	onDeath_ = cb;
}
