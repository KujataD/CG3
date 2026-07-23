#include "MagicProjectile.h"
#include "EnemyHealth.h"

using namespace KujakuEngine;

void MagicProjectile::Fire(const Vector3& direction, float speed, float lifetime, float damage) {
	Vector3 normalized = direction;
	float length = Length(normalized);
	if (length > 0.0001f) {
		normalized = normalized / length;
	} else {
		normalized = {0.0f, 0.0f, 1.0f};
	}
	direction_ = normalized;
	speed_ = speed;
	lifetime_ = lifetime;
	damage_ = damage;
}

void MagicProjectile::Update() {
	if (!owner_ || lifetime_ <= 0.0f) {
		return;
	}

	float deltaTime = Time::GetDeltaTime();
	owner_->GetTransform().translation_ += direction_ * (speed_ * deltaTime);

	lifetime_ -= deltaTime;
	if (lifetime_ <= 0.0f) {
		lifetime_ = 0.0f;
		owner_->SetActive(false);
	}
}

void MagicProjectile::OnTriggerStay(KujakuEngine::ColliderComponent* other) {
	if (!other || lifetime_ <= 0.0f) {
		return;
	}

	KujakuEngine::GameObject* otherObj = other->GetOwner();
	if (!otherObj) {
		return;
	}

	// 敵(EnemyHealth持ち)にだけ当たる。味方や地形はすり抜ける。
	EnemyHealth* health = otherObj->GetComponent<EnemyHealth>();
	if (!health) {
		return;
	}

	health->TakeDamage(damage_);
	lifetime_ = 0.0f;
	if (owner_) {
		owner_->SetActive(false);
	}
}
