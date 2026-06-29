#include "EditorSelection.h"

namespace KujakuEngine {

EditorSelection* EditorSelection::GetInstance() {
	static EditorSelection instance;
	return &instance;
}

void EditorSelection::SetSelectedGameObject(GameObject* object) {
	selectedGameObject_ = object;
}

GameObject* EditorSelection::GetSelectedGameObject() const {
	return selectedGameObject_;
}

void EditorSelection::Clear() {
	selectedGameObject_ = nullptr;
}

} // namespace KujakuEngine
