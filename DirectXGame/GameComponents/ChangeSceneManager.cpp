#include "ChangeSceneManager.h"

using namespace KujataEngine;

void ChangeSceneManager::RegisterInvokableMethods(KujataEngine::InvokableMethodRegistry& registry) {
	registry.Add("OnClick", [this]() { OnClick(); });
}

void ChangeSceneManager::OnClick() {
	ChangeScene(sceneName_.c_str());
}
