#include "PartyManager.h"
#include "AllyAIBrain.h"
#include "PartySelection.h"
#include "Player.h"

#include <components/OrbitCameraComponent.h>

using namespace KujakuEngine;

void PartyManager::OnPlayStart() {
	leader_ = nullptr;
	ally_ = nullptr;
	if (!owner_ || !owner_->GetScene()) {
		return;
	}

	// キャラ選択シーンからの持ち込みがあれば、Inspector設定より優先する。
	// 選ばれた方をリーダーへ、Inspector上のリーダーだった方を味方へ回す。
	std::string selected = PartySelection::ConsumeLeaderName();
	if (!selected.empty() && selected != leaderName_) {
		if (selected == allyName_) {
			allyName_ = leaderName_;
		}
		leaderName_ = selected;
	}

	Scene* scene = owner_->GetScene();
	leader_ = scene->FindGameObjectByName(leaderName_);
	ally_ = scene->FindGameObjectByName(allyName_);

	ApplyRoles();
}

void PartyManager::ApplyRoles() {
	SetBrainMode(leader_, true);
	SetBrainMode(ally_, false);

	// カメラの追従先をリーダーへ向ける(OrbitCameraComponentは毎フレーム名前解決するため即時反映される)。
	if (owner_ && owner_->GetScene()) {
		if (GameObject* cameraObject = owner_->GetScene()->FindGameObjectByName(cameraName_)) {
			if (OrbitCameraComponent* orbitCamera = cameraObject->GetComponent<OrbitCameraComponent>()) {
				orbitCamera->SetTargetName(leaderName_);
			}
		}
	}
}

void PartyManager::SetBrainMode(GameObject* character, bool isLeader) {
	if (!character) {
		return;
	}

	// 入力頭脳とAI頭脳を排他で切り替える。付いていない頭脳は無視する
	// (例: AllyAIBrain未設定のキャラをリーダーにしても問題なく動く)。
	if (Player* inputBrain = character->GetComponent<Player>()) {
		inputBrain->SetEnabled(isLeader);
	}
	if (AllyAIBrain* aiBrain = character->GetComponent<AllyAIBrain>()) {
		aiBrain->SetEnabled(!isLeader);
	}
}
