#include "EnemyWeapon.h"
#include "CharacterMotor.h"
#include "PlayerHealth.h"

using namespace KujakuEngine;

namespace {

// 武器の最上位親(敵本体)を返す。ノックバック方向の基準にする。
GameObject* GetRootObject(GameObject* object) {
	while (object && object->GetParent()) {
		object = object->GetParent();
	}
	return object;
}

} // namespace

void EnemyWeapon::Initialize() {
}

void EnemyWeapon::Update() {
	// attackがOFF→ON(新しい攻撃振りの開始)になった瞬間はヒット履歴をリセットし、即ヒットできるようにする。
	if (attack_ && !prevAttack_) {
		hitCooldowns_.clear();
	}
	prevAttack_ = attack_;

	// 対象ごとの再ヒットクールダウンを進める。
	float deltaTime = Time::GetDeltaTime();
	for (auto it = hitCooldowns_.begin(); it != hitCooldowns_.end();) {
		it->second -= deltaTime;
		if (it->second <= 0.0f) {
			it = hitCooldowns_.erase(it);
		} else {
			++it;
		}
	}
}

void EnemyWeapon::OnTriggerStay(KujakuEngine::ColliderComponent* other) {
	if (!other || !attack_) {
		return;
	}

	KujakuEngine::GameObject* otherObj = other->GetOwner();
	if (!otherObj) {
		return;
	}

	// 再ヒット間隔中はダメージを与えない(アニメーション中でも間隔が空けば何度でも当たる)。
	if (hitCooldowns_.count(otherObj) > 0) {
		return;
	}

	// 無敵中はヒット不成立(履歴にも残さず、無敵が切れたら即座に当たり得る)。
	if (ApplyDamageToPlayer(otherObj)) {
		hitCooldowns_[otherObj] = hitInterval_;
	}
}

bool EnemyWeapon::ApplyDamageToPlayer(KujakuEngine::GameObject* target) {
	// 敵の武器なのでダメージ対象はPlayer(PlayerHealth持ち)のみ。
	auto health = target->GetComponent<PlayerHealth>();
	if (!health) {
		return false;
	}

	// 無敵中(回避中など)はダメージもノックバックも入らない。
	if (health->IsInvincible()) {
		return false;
	}

	health->TakeDamage(damageValue_);

	// ノックバック: 敵本体→プレイヤーの水平方向へ吹き飛ばし、しばらく行動不能にする。
	// 体(CharacterMotor)へ直接与える。頭脳が入力かAIかに関わらず同じ挙動になる。
	CharacterMotor* motor = target->GetComponent<CharacterMotor>();
	if (!motor) {
		return true;
	}

	GameObject* weaponOwner = GetOwner();
	GameObject* enemyRoot = GetRootObject(weaponOwner);

	Vector3 direction = {0.0f, 0.0f, 1.0f};
	if (enemyRoot) {
		Vector3 toTarget = target->GetTransform().translation_ - enemyRoot->GetTransform().translation_;
		toTarget.y = 0.0f;
		float length = Length(toTarget);
		if (length > 0.0001f) {
			direction = toTarget / length;
		}
	}

	motor->ApplyKnockback(direction * knockback_, stunDuration_);
	return true;
}
