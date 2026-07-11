#pragma once

namespace KujakuEngine {

class Scene;
class GameObject;

// Hierarchyウィンドウ（Scene内のGameObjectツリー表示・生成・D&D・削除）を描画する。
class HierarchyWindow {
public:
	void Draw();

private:
	void DrawObject(Scene& scene, GameObject* gameObject, GameObject* selectedObject, bool& selectedObjectExists);
};

} // namespace KujakuEngine
