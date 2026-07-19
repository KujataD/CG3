#pragma once

#include <string>

namespace KujakuEngine {

class Scene;

// Editor UI 全体で共有するドラッグ&ドロップのペイロード種別。
constexpr const char* kHierarchyDragPayloadType = "KujakuHierarchyGameObject";
constexpr const char* kProjectPrefabDragPayloadType = "KujakuProjectPrefab";
constexpr const char* kProjectMaterialDragPayloadType = "KujakuProjectMaterial";
constexpr const char* kProjectTextureDragPayloadType = "KujakuProjectTexture";
constexpr const char* kProjectModelDragPayloadType = "KujakuProjectModel";
constexpr const char* kProjectAnimClipDragPayloadType = "KujakuProjectAnimClip";

// 現在のSceneの状態をUndo履歴へ積む。
void CaptureUndo(Scene& scene, const std::string& label);

// 指定したScene JSONをUndo履歴へ積む（編集前スナップショットを渡す用途）。
void CaptureUndo(Scene& scene, const std::string& label, const std::string& sceneJson);

} // namespace KujakuEngine
