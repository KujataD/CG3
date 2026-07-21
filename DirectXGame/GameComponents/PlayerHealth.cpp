#include "PlayerHealth.h"
#include <algorithm>

void PlayerHealth::Initialize() {
	health_ = maxHealth_;
}

void PlayerHealth::TakeDamage(float damage) {
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

float PlayerHealth::GetHealthPercent() const {
	if (maxHealth_ <= 0) {
		return 0.0f;
	}
	return health_ / maxHealth_;
}

bool PlayerHealth::IsAlive() const {
	return health_ > 0;
}

void PlayerHealth::SetOnHealthChanged(std::function<void(float)> cb) {
	onHealthChanged_ = cb;
}

void PlayerHealth::SetOnDeath(std::function<void()> cb) {
	onDeath_ = cb;
}
