#include "EnemyHealth.h"

#include "HateTable.h"
#include <algorithm>

using namespace KujataEngine;

namespace {

// 名前で子孫を探す(自分自身も対象)。
GameObject* FindDescendantByName(GameObject* object, const std::string& name) {
	if (!object) {
		return nullptr;
	}
	if (object->GetName() == name) {
		return object;
	}
	for (GameObject* child : object->GetChildren()) {
		if (GameObject* found = FindDescendantByName(child, name)) {
			return found;
		}
	}
	return nullptr;
}

} // namespace

Vector3 EnemyHealth::GetLockOnPoint() const {
	if (!owner_) {
		return lockOnOffset_;
	}

	// 基準にする部位。指定が無ければ自分自身。
	GameObject* anchor = owner_;
	if (!lockOnObjectName_.empty()) {
		if (GameObject* found = FindDescendantByName(owner_, lockOnObjectName_)) {
			anchor = found;
		}
	}

	// 子オブジェクトのtranslation_は親からの相対なので、必ずワールド行列から取る。
	// (matWorld_は全Update後に更新されるため1フレーム古いが、狙い点の用途では問題にならない)
	Vector3 base = (anchor == owner_) ? owner_->GetTransform().translation_ : anchor->GetTransform().GetWorldPosition();
	return base + lockOnOffset_;
}

void EnemyHealth::Initialize() {
	health_ = maxHealth_;
	poise_ = 0.0f;
	poiseDecayTimer_ = 0.0f;
	stunTimer_ = 0.0f;
}

void EnemyHealth::OnPlayStart() {
	poise_ = 0.0f;
	poiseDecayTimer_ = 0.0f;
	stunTimer_ = 0.0f;
}

void EnemyHealth::Update() {
	float deltaTime = Time::GetDeltaTime();

	// スタン中: 時間だけ進める(蓄積は0のまま)。
	if (stunTimer_ > 0.0f) {
		stunTimer_ -= deltaTime;
		if (stunTimer_ <= 0.0f) {
			stunTimer_ = 0.0f;
			if (onStaggerEnd_) {
				onStaggerEnd_();
			}
		}
		return;
	}

	// 蓄積の減衰(最後のヒットから待ち時間経過後)。
	if (poise_ <= 0.0f) {
		return;
	}
	if (poiseDecayTimer_ > 0.0f) {
		poiseDecayTimer_ -= deltaTime;
		return;
	}
	poise_ = std::max(0.0f, poise_ - poiseDecayPerSecond_ * deltaTime);
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

void EnemyHealth::TakeDamage(float damage, float poiseDamage, KujataEngine::GameObject* attacker) {
	TakeDamage(damage);
	AddPoise(poiseDamage);

	// **ヘイトの入口はここ1箇所にまとめる。**
	// 各ダメージ源(武器・魔法弾・致命)がそれぞれヘイトを積む作りにすると、
	// 新しい攻撃手段を足すたびに積み忘れが起きる。ダメージが通った場所で一括して通知する。
	if (attacker) {
		if (HateTable* hate = GetComponent<HateTable>()) {
			hate->AddDamageHate(attacker, damage);
		}
	}
}

void EnemyHealth::AddPoise(float poiseDamage) {
	if (!IsAlive() || poiseDamage <= 0.0f || IsStaggered()) {
		return;
	}

	poise_ += poiseDamage;
	poiseDecayTimer_ = poiseDecayDelay_;

	if (poise_ < poiseMax_) {
		return;
	}

	// 閾値到達: ゲージを空にしてスタン開始。頭脳へ通知(モーション中断はそちらで)。
	poise_ = 0.0f;
	poiseDecayTimer_ = 0.0f;
	stunTimer_ = stunDuration_;
	if (stunTimer_ > 0.0f && onStagger_) {
		onStagger_();
	}
}

void EnemyHealth::Flinch() {
	if (!IsAlive() || IsStaggered()) {
		return;
	}
	if (onFlinch_) {
		onFlinch_();
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
