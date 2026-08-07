#pragma once

#include <BahamutAI/AI.h>
#include <KujataEngine.h>

#include "Player.h"

class ChangeSceneManager : public KujataEngine::Component {
public:
	const char* GetTypeName() const override { return "ChangeSceneManager"; }

	// UnityのButton.onClick等から呼べるメソッドを公開する。
	void RegisterInvokableMethods(KujataEngine::InvokableMethodRegistry& registry) override;
private:
	void OnClick();

private:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		// attack_はbool型アニメーションチャンネル。キーフレームで攻撃判定をON/OFFする(ため中はOFF)。
		KUJATA_REGISTER_STRING(sceneName_);
	}
	KUJATA_FIELD_STRING(sceneName_, "SampleScene");
};
