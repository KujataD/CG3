#pragma once

namespace KujakuEngine {

class Scene;
class GameObject;

// GameObjectメニューとHierarchy右クリックで共有するUI要素生成。
// いずれもCanvas(無ければ生成)配下に作る。
namespace UIObjectFactory {

GameObject* EnsureCanvas(Scene* scene);
GameObject* CreateImage(Scene* scene);
GameObject* CreateText(Scene* scene);
GameObject* CreateButton(Scene* scene);

} // namespace UIObjectFactory
} // namespace KujakuEngine
