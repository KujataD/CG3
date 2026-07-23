#include "CharacterSelectManager.h"
#include "PartySelection.h"

using namespace KujakuEngine;

void CharacterSelectManager::RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) {
	registry.Add("SelectPawn", [this]() { Select(pawnName_); });
	registry.Add("SelectBishop", [this]() { Select(bishopName_); });
}

void CharacterSelectManager::Select(const std::string& leaderName) {
	PartySelection::SetLeaderName(leaderName);
	ChangeScene(nextSceneName_);
}
