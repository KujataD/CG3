#include "BuiltinComponents.h"
#include "CameraComponent.h"
#include "ColliderComponent.h"
#include "DebugCameraComponent.h"
#include "DirectionalLightComponent.h"
#include "ModelRendererComponent.h"
#include "PointLightComponent.h"
#include "RotatorComponent.h"
#include "TransformComponent.h"
#include "TransformSnapshotComponent.h"
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
	factory.RegisterComponent<TransformSnapshotComponent>();
	factory.RegisterComponent<ModelRendererComponent>();
	factory.RegisterComponent<CameraComponent>();
	factory.RegisterComponent<DebugCameraComponent>();
	factory.RegisterComponent<DirectionalLightComponent>();
	factory.RegisterComponent<PointLightComponent>();
	factory.RegisterComponent<SphereColliderComponent>();
	factory.RegisterComponent<BoxColliderComponent>();

	registered = true;
}

} // namespace KujakuEngine
