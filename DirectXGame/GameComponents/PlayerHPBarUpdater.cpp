#include "PlayerHPBarUpdater.h"
#include "../KujataEngine/components/ImageComponent.h"
#include "PlayerHealth.h"

void PlayerHPBarUpdater::Update() {
	AcquireRefs();
	if (!health_ || !fillImage_) {
		return;
	}

	// コールバックに頼らず毎フレームHP率を反映する(確実に追従させる)。
	fillImage_->SetFillAmount(health_->GetHealthPercent());
}

void PlayerHPBarUpdater::AcquireRefs() {
	KujataEngine::GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	if (!fillImage_) {
		fillImage_ = owner->GetComponent<KujataEngine::ImageComponent>();
	}

	if (!health_ && owner->GetScene()) {
		KujataEngine::GameObject* player = owner->GetScene()->FindGameObjectByName(playerName_);
		if (player) {
			health_ = player->GetComponent<PlayerHealth>();
		}
	}
}
