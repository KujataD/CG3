#include "PartyManager.h"
#include "GameFx.h"
#include "AllyAIBrain.h"
#include "CharacterMotor.h"
#include "GameInput.h"
#include "PartySelection.h"
#include "Player.h"
#include "PlayerHealth.h"

#include <components/OrbitCameraComponent.h>
#include <utility>

using namespace KujataEngine;

void PartyManager::OnPlayStart() {
	leader_ = nullptr;
	ally_ = nullptr;
	swapCooldownTimer_ = 0.0f;
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

void PartyManager::Update() {
	if (swapCooldownTimer_ > 0.0f) {
		swapCooldownTimer_ -= Time::GetDeltaTime();
		if (swapCooldownTimer_ < 0.0f) {
			swapCooldownTimer_ = 0.0f;
		}
	}

	if (swapEnabled_ && GameInput::IsSwapCharacterTriggered()) {
		SwapLeader();
	}
}

bool PartyManager::SwapLeader() {
	if (!leader_ || !ally_ || swapCooldownTimer_ > 0.0f) {
		return false;
	}

	// 相方が倒れていたら切り替えられない。
	if (PlayerHealth* allyHealth = ally_->GetComponent<PlayerHealth>()) {
		if (!allyHealth->IsAlive()) {
			return false;
		}
	}
	// どちらかが被弾硬直中なら切り替えられない(硬直を切替で逃げられないように)。
	for (GameObject* character : {leader_, ally_}) {
		if (CharacterMotor* motor = character->GetComponent<CharacterMotor>()) {
			if (motor->IsStunned()) {
				return false;
			}
		}
	}

	// これまで味方として動いていたAI頭脳(=これからリーダーになる側)は、監視オブザーバーと
	// 実行中の分岐を手放す。手放さないと、新しく味方になる側がオブザーバーを作れず
	// BahamutAIEditorへ実行フローが届かない(同一キーは最初の登録者だけが送信する)。
	if (AllyAIBrain* allyBrain = ally_->GetComponent<AllyAIBrain>()) {
		allyBrain->OnRelievedFromDuty();
	}

	std::swap(leader_, ally_);
	std::swap(leaderName_, allyName_);

	// **入れ替わる2人の足元それぞれから魂の炎が立つ。** 「魂が移った」ことを2点で示す。
	// 色は魂の色で上書きするので、Prefab側の色を変えても揃ったままになる。
	for (GameObject* character : {leader_, ally_}) {
		if (character) {
			GameFx::Burst(character->GetScene(), GameFx::Prefab::kSoulBurst, character->GetTransform().translation_, 1.0f,
			    &GameFx::kSoulColor);
		}
	}
	ApplyRoles();
	swapCooldownTimer_ = swapCooldown_;
	return true;
}

PartyManager* PartyManager::FindInScene(Scene* scene) {
	if (!scene) {
		return nullptr;
	}
	for (const std::unique_ptr<GameObject>& object : scene->GetGameObjects()) {
		if (!object) {
			continue;
		}
		if (PartyManager* manager = object->GetComponent<PartyManager>()) {
			return manager;
		}
	}
	return nullptr;
}

GameObject* PartyManager::FindLeaderInScene(Scene* scene, const std::string& fallbackName) {
	if (!scene) {
		return nullptr;
	}
	if (PartyManager* manager = FindInScene(scene)) {
		if (manager->GetLeader()) {
			return manager->GetLeader();
		}
	}
	return fallbackName.empty() ? nullptr : scene->FindGameObjectByName(fallbackName);
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
