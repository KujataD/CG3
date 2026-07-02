#include "BuiltinComponents.h"
#include "CameraComponent.h"
#include "ColliderComponent.h"
#include "DebugCameraComponent.h"
#include "DirectionalLightComponent.h"
#include "ModelRendererComponent.h"
#include "PointLightComponent.h"
#include "RotatorComponent.h"
#include "TransformSnapshotComponent.h"
#include "../scene/ComponentFactory.h"
#include <memory>

namespace KujakuEngine {

void RegisterBuiltinComponents() {
	static bool registered = false;
	if (registered) {
		return;
	}

	ComponentFactory::GetInstance().Register("RotatorComponent", []() {
		return std::make_unique<RotatorComponent>();
	});

	ComponentFactory::GetInstance().Register("TransformSnapshotComponent", []() {
		return std::make_unique<TransformSnapshotComponent>();
	});

	ComponentFactory::GetInstance().Register("ModelRendererComponent", []() {
		return std::make_unique<ModelRendererComponent>();
	});

	ComponentFactory::GetInstance().Register("CameraComponent", []() {
		return std::make_unique<CameraComponent>();
	});

	ComponentFactory::GetInstance().Register("DebugCameraComponent", []() {
		return std::make_unique<DebugCameraComponent>();
	});

	ComponentFactory::GetInstance().Register("DirectionalLightComponent", []() {
		return std::make_unique<DirectionalLightComponent>();
	});

	ComponentFactory::GetInstance().Register("PointLightComponent", []() {
		return std::make_unique<PointLightComponent>();
	});

	ComponentFactory::GetInstance().Register("SphereColliderComponent", []() {
		return std::make_unique<SphereColliderComponent>();
	});

	ComponentFactory::GetInstance().Register("BoxColliderComponent", []() {
		return std::make_unique<BoxColliderComponent>();
	});

	registered = true;
}

} // namespace KujakuEngine
