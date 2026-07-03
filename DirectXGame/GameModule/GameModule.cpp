#include "../KujakuEngine/KujakuEngine.h"
#include "../KujakuEngine/components/CameraComponent.h"
#include "../KujakuEngine/components/ColliderComponent.h"
#include "../KujakuEngine/components/DebugCameraComponent.h"
#include "../KujakuEngine/components/DirectionalLightComponent.h"
#include "../KujakuEngine/components/ModelRendererComponent.h"
#include "../KujakuEngine/components/PointLightComponent.h"
#include "../KujakuEngine/components/RotatorComponent.h"
#include "../KujakuEngine/components/TransformComponent.h"
#include "../KujakuEngine/components/TransformSnapshotComponent.h"
#include "../KujakuEngine/scene/SampleScene.h"
#include <memory>

namespace {

constexpr const char* kGameModuleName = "GameModule";

class GameModuleScene : public KujakuEngine::SampleScene {
public:
	~GameModuleScene() override = default;
};

class MoveForwardComponent : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "MoveForwardComponent"; }

	void Update() override {
		KujakuEngine::GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		owner->GetTransform().translation_.z += speed_;
	}

	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

private:
	KUJAKU_FIELD_FLOAT(speed_, 0.03f);
};

class BlinkComponent : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "BlinkComponent"; }

	void Initialize() override {
		SaveOriginalScale();
	}

	void Update() override {
		KujakuEngine::GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		++frameCount_;
		if (frameCount_ < intervalFrame_) {
			return;
		}

		frameCount_ = 0;
		visible_ = !visible_;

		KujakuEngine::WorldTransform& transform = owner->GetTransform();
		if (visible_) {
			transform.scale_ = originalScale_;
		} else {
			transform.scale_ = originalScale_ * 0.25f;
		}
	}

	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_INT(intervalFrame_, 1.0f, 1, 600);
	}

public:
	void OnAfterReadJson() override {
		SaveOriginalScale();
	}

private:
	void SaveOriginalScale() {
		KujakuEngine::GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		originalScale_ = owner->GetTransform().scale_;
	}

private:
	KUJAKU_FIELD_INT(intervalFrame_, 30);
	int frameCount_ = 0;
	bool visible_ = true;
	KujakuEngine::Vector3 originalScale_ = {1.0f, 1.0f, 1.0f};
};

} // namespace

extern "C" __declspec(dllexport) void RegisterGameComponents(KujakuEngine::ComponentFactory& factory) {
	factory.Register("Transform", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::TransformComponent>();
	});

	factory.Register("RotatorComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::RotatorComponent>();
	});

	factory.Register("TransformSnapshotComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::TransformSnapshotComponent>();
	});

	factory.Register("ModelRendererComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::ModelRendererComponent>();
	});

	factory.Register("CameraComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::CameraComponent>();
	});

	factory.Register("DebugCameraComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::DebugCameraComponent>();
	});

	factory.Register("DirectionalLightComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::DirectionalLightComponent>();
	});

	factory.Register("PointLightComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::PointLightComponent>();
	});

	factory.Register("SphereColliderComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::SphereColliderComponent>();
	});

	factory.Register("BoxColliderComponent", kGameModuleName, []() {
		return std::make_unique<KujakuEngine::BoxColliderComponent>();
	});

	factory.Register("MoveForwardComponent", kGameModuleName, []() {
		return std::make_unique<MoveForwardComponent>();
	});

	factory.Register("BlinkComponent", kGameModuleName, []() {
		return std::make_unique<BlinkComponent>();
	});
}

extern "C" __declspec(dllexport) void UnregisterGameComponents(KujakuEngine::ComponentFactory& factory) {
	factory.UnregisterByModule(kGameModuleName);
}

extern "C" __declspec(dllexport) KujakuEngine::Scene* CreateGameScene() {
	return new GameModuleScene();
}

extern "C" __declspec(dllexport) void DestroyGameScene(KujakuEngine::Scene* scene) {
	delete scene;
}
