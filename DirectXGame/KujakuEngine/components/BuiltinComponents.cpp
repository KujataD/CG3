#include "BuiltinComponents.h"
#include "ModelRendererComponent.h"
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

	registered = true;
}

} // namespace KujakuEngine
