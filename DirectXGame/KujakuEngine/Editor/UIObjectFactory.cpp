#include "UIObjectFactory.h"

#include "../scene/Component.h"
#include "../scene/ComponentFactory.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include <cstring>
#include <memory>

namespace KujakuEngine {
namespace UIObjectFactory {
namespace {

GameObject* FindFirstCanvas(Scene* scene) {
	for (const std::unique_ptr<GameObject>& gameObject : scene->GetGameObjects()) {
		if (!gameObject) {
			continue;
		}
		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (component && std::strcmp(component->GetTypeName(), "Canvas") == 0) {
				return gameObject.get();
			}
		}
	}
	return nullptr;
}

void AddComponentByType(Scene* scene, GameObject* gameObject, const char* typeName) {
	Component* added = gameObject->AddComponent(ComponentFactory::GetInstance().Create(typeName));
	scene->OnEditorComponentAdded(gameObject, added);
}

} // namespace

GameObject* EnsureCanvas(Scene* scene) {
	GameObject* existing = FindFirstCanvas(scene);
	if (existing) {
		return existing;
	}
	GameObject* canvas = scene->CreateGameObject("Canvas");
	AddComponentByType(scene, canvas, "Canvas");
	return canvas;
}

GameObject* CreateImage(Scene* scene) {
	GameObject* canvas = EnsureCanvas(scene);
	GameObject* image = scene->CreateGameObject("Image");
	AddComponentByType(scene, image, "RectTransform");
	AddComponentByType(scene, image, "Image");
	image->SetParent(canvas);
	return image;
}

GameObject* CreateText(Scene* scene) {
	GameObject* canvas = EnsureCanvas(scene);
	GameObject* text = scene->CreateGameObject("Text");
	AddComponentByType(scene, text, "RectTransform");
	AddComponentByType(scene, text, "Text");
	text->SetParent(canvas);
	return text;
}

GameObject* CreateButton(Scene* scene) {
	GameObject* canvas = EnsureCanvas(scene);
	GameObject* button = scene->CreateGameObject("Button");
	AddComponentByType(scene, button, "RectTransform");
	AddComponentByType(scene, button, "Image");
	AddComponentByType(scene, button, "Button");
	button->SetParent(canvas);

	GameObject* label = scene->CreateGameObject("Text");
	AddComponentByType(scene, label, "RectTransform");
	AddComponentByType(scene, label, "Text");
	label->SetParent(button);
	return button;
}

} // namespace UIObjectFactory
} // namespace KujakuEngine
