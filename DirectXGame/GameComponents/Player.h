#pragma once
#include <KujakuEngine.h>

class IAbilitySet;
class CharacterMotor;

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
