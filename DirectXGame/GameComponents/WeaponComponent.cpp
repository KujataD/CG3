#include "WeaponComponent.h"
#include "EnemyHealth.h"

void WeaponComponent::Initialize() {
}

void WeaponComponent::Update() {
	// attackがOFF→ON(新しい攻撃振りの開始)になった瞬間だけヒット履歴をクリアする。
	// これにより「1振りで敵1体につき1回だけ」ダメージが入る(多段ヒット防止)。
	if (attack_ && !prevAttack_) {
		hitThisSwing_.clear();
	}
	prevAttack_ = attack_;
}

void WeaponComponent::OnTriggerStay(KujakuEngine::ColliderComponent* other) {
	if (!other || !attack_) {
		return;
	}

	KujakuEngine::GameObject* otherObj = other->GetOwner();
	if (!otherObj) {
		return;
	}

	// この攻撃振りで既にダメージ済みなら二重に与えない。
	if (hitThisSwing_.count(otherObj) > 0) {
		return;
	}

	ApplyDamageToEnemy(otherObj);
	hitThisSwing_.insert(otherObj);
}

void WeaponComponent::ApplyDamageToEnemy(KujakuEngine::GameObject* enemy) {
	auto health = enemy->GetComponent<EnemyHealth>();
	if (!health) {
		return;
	}

	health->TakeDamage(damageValue_);
}
