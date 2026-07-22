#pragma once

#include <KujakuEngine.h>
#include <string>

class PlayerHealth;

namespace KujakuEngine {
class ImageComponent;
}

/// <summary>
/// 画面左上のPlayer用HPバー。自分(HPBarFill)のImageComponentのfillAmountをPlayerのHP率で更新する。
/// </summary>
class PlayerHPBarUpdater : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "PlayerHPBarUpdater"; }

	void Update() override;

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_STRING(playerName_);
	}

	// PlayerHealthを持つGameObjectの名前(シーンから検索する)。
	KUJAKU_FIELD_STRING(playerName_, "Pawn");

	PlayerHealth* health_ = nullptr;
	KujakuEngine::ImageComponent* fillImage_ = nullptr;

	// health_/fillImage_を未取得なら解決する。ロード順に依存しないよう毎フレーム試みる。
	void AcquireRefs();
};
