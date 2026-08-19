#pragma once

#include <KujataEngine.h>
#include <unordered_map>

class EnemyWeapon : public KujataEngine::Component {
public:
	const char* GetTypeName() const override { return "EnemyWeapon"; }

	void Initialize() override;
	void Update() override;
	void OnTriggerStay(KujataEngine::ColliderComponent* other) override;

	/// <summary>
	/// 攻撃判定のON/OFF。通常はアニメーションクリップのboolチャンネルで駆動するが、
	/// 攻撃をコードで組み立てる場合(ガーディアンの踏みつけ等)はここから直接切り替える。
	/// </summary>
	void SetAttack(bool attack) { attack_ = attack; }
	bool IsAttacking() const { return attack_; }

private:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		// attack_はbool型アニメーションチャンネル。キーフレームで攻撃判定をON/OFFする(ため中はOFF)。
		KUJATA_REGISTER_BOOL(attack_);
		KUJATA_REGISTER_FLOAT(damageValue_, 1.0f, 0.0f, 0.0f);
		KUJATA_REGISTER_FLOAT(knockback_, 0.1f, 0.0f, 0.0f);
		KUJATA_REGISTER_FLOAT(hitInterval_, 0.05f, 0.0f, 0.0f);
		KUJATA_REGISTER_FLOAT(stunDuration_, 0.05f, 0.0f, 0.0f);
	}

	KUJATA_FIELD_BOOL(attack_, false);
	KUJATA_FIELD_FLOAT(damageValue_, 10);
	// ノックバックの初速[unit/s]。
	KUJATA_FIELD_FLOAT(knockback_, 5);
	// 同一対象への再ヒット間隔[s]。1回のアニメーション中でも、この間隔ごとに当たるたびダメージが入る。
	KUJATA_FIELD_FLOAT(hitInterval_, 0.5f);
	// ヒット時にプレイヤーが動けなくなる時間[s]。
	KUJATA_FIELD_FLOAT(stunDuration_, 0.6f);

	// 対象ごとの再ヒットまでの残り時間。OnTriggerStayは毎フレーム呼ばれるため、
	// これが無いと接触中に毎フレームダメージが入ってしまう。攻撃振り開始時にクリアする。
	std::unordered_map<KujataEngine::GameObject*, float> hitCooldowns_;
	bool prevAttack_ = false;

	// ダメージ+ノックバックを適用する。無敵中などでヒット不成立ならfalse(ヒット履歴に残さない)。
	bool ApplyDamageToPlayer(KujataEngine::GameObject* target);
};
