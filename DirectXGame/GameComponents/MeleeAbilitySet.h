#pragma once
#include "IAbilitySet.h"
#include <components/AnimatorComponent.h>

class CharacterMotor;

/// <summary>
/// 近接攻撃(Pawn用)のAbilitySet。
/// スロット0=攻撃クリップの再生。攻撃判定は従来通りアニメーションのboolチャンネルが
/// 武器のWeaponComponent.attackをON/OFFすることで発生する(ここでは当たりを持たない)。
/// </summary>
class MeleeAbilitySet : public IAbilitySet {
public:
	const char* GetTypeName() const override { return "MeleeAbilitySet"; }
	bool AllowMultiple() const override { return false; }

	void OnPlayStart() override;

	bool TryUse(int slot) override;
	bool IsBusy() const override;

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_STRING_NAMED(attackClipName_, "Attack Clip");
	}

	// スロット0で再生する攻撃クリップ名。
	KUJAKU_FIELD_STRING(attackClipName_, "PlayerAttack");

	// モデル(子)のAnimator。
	KujakuEngine::AnimatorComponent* animator_ = nullptr;
	// 同じGameObjectのCharacterMotor(行動不能チェック用)。
	CharacterMotor* motor_ = nullptr;
};
