#include "EditorImGuiUtil.h"

#include "../scene/Scene.h"
#include "EditorUndoManager.h"

namespace KujakuEngine {

void CaptureUndo(Scene& scene, const std::string& label) {
	EditorUndoManager::GetInstance()->Capture(scene, label);
}

void CaptureUndo(Scene& scene, const std::string& label, const std::string& sceneJson) {
	EditorUndoManager::GetInstance()->Capture(scene, label, sceneJson);
}

} // namespace KujakuEngine
