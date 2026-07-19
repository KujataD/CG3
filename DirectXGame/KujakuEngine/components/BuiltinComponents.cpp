#include "BuiltinComponents.h"
#include "ButtonComponent.h"
#include "CameraComponent.h"
#include "CanvasComponent.h"
#include "ColliderComponent.h"
#include "DebugCameraComponent.h"
#include "DirectionalLightComponent.h"
#include "ImageComponent.h"
#include "ModelRendererComponent.h"
#include "PointLightComponent.h"
#include "RectTransformComponent.h"
#include "RigidbodyComponent.h"
#include "TextComponent.h"
#include "RotatorComponent.h"
#include "TransformComponent.h"
#include "../scene/ComponentFactory.h"

namespace KujakuEngine {

void RegisterBuiltinComponents() {
	static bool registered = false;
	if (registered) {
		return;
	}

	ComponentFactory& factory = ComponentFactory::GetInstance();
	factory.RegisterComponent<TransformComponent>();
	factory.RegisterComponent<RotatorComponent>();
	factory.RegisterComponent<ModelRendererComponent>();
	factory.RegisterComponent<CameraComponent>();
	factory.RegisterComponent<DebugCameraComponent>();
	factory.RegisterComponent<DirectionalLightComponent>();
	factory.RegisterComponent<PointLightComponent>();
	factory.RegisterComponent<RigidbodyComponent>();
	factory.RegisterComponent<SphereColliderComponent>();
	factory.RegisterComponent<BoxColliderComponent>();
	factory.RegisterComponent<CapsuleColliderComponent>();
	factory.RegisterComponent<CanvasComponent>();
	factory.RegisterComponent<RectTransformComponent>();
	factory.RegisterComponent<ImageComponent>();
	factory.RegisterComponent<TextComponent>();
	factory.RegisterComponent<ButtonComponent>();

	registered = true;
}

} // namespace KujakuEngine
