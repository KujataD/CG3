#pragma once
#include <KujakuEngine.h>

class IAbilitySet;
class CharacterMotor;

/// <summary>
/// 手動操作の「頭脳」。パッド/キーボード入力を読んで、同じGameObjectの
/// CharacterMotor(体)とIAbilitySet(技)を動かす。実処理はすべてそちら側にあり、
/// ここは入力の変換だけを行う。
///   移動: 左スティック / WASD    回避: Aボタン / Space    攻撃: R2(右トリガー)
///
/// NPC(味方AI)として動かすときは、このComponentを無効化してAllyAIBrainを有効化する
/// (PartyManagerが切り替える)。体・技のAPIは共通なのでキャラの挙動は変わらない。
/// </summary>
class Player : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "Player"; }
	bool AllowMultiple() const override { return false; }

	void OnPlayStart() override;
	void Update() override;

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT_NAMED(triggerThreshold_, "Trigger Threshold", 0.01f, 0.05f, 1.0f);
	}

	// この値以上R2を引いたら攻撃入力とみなす。
	KUJAKU_FIELD_FLOAT(triggerThreshold_, 0.5f);

	// 同じGameObjectのCharacterMotor(操作対象の体)。
	CharacterMotor* motor_ = nullptr;
	// 同じGameObjectのIAbilitySet(攻撃手段。Melee/Magicどちらでもよい)。
	IAbilitySet* abilitySet_ = nullptr;

	// R2のエッジ検出用。
	bool wasAttackPressed_ = false;
};
