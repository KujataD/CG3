#include "ChangeSceneManager.h"

using namespace KujakuEngine;

void ChangeSceneManager::RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) {
	registry.Add("OnClick", [this]() { OnClick(); });
}

void ChangeSceneManager::OnClick() {
	ChangeScene(sceneName_.c_str());
}
