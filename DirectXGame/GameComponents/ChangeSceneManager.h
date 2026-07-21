#pragma once

#include <BahamutAI/AI.h>
#include <KujakuEngine.h>

#include "Player.h"

class ChangeSceneManager : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "ChangeSceneManager"; }

	// UnityのButton.onClick等から呼べるメソッドを公開する。
	void RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) override;
private:
	void OnClick();

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		// attack_はbool型アニメーションチャンネル。キーフレームで攻撃判定をON/OFFする(ため中はOFF)。
		KUJAKU_REGISTER_STRING(sceneName_);
	}
	KUJAKU_FIELD_STRING(sceneName_, "SampleScene");
};
