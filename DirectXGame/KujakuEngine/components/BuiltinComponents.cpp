#include "BuiltinComponents.h"
#include "AnimatorComponent.h"
#include "AudioSourceComponent.h"
#include "ButtonComponent.h"
#include "CameraComponent.h"
#include "CanvasComponent.h"
#include "ColliderComponent.h"
#include "DebugCameraComponent.h"
#include "DirectionalLightComponent.h"
#include "ImageComponent.h"
#include "ModelRendererComponent.h"
#include "OrbitCameraComponent.h"
#include "PointLightComponent.h"
#include "RectTransformComponent.h"
#include "RigidbodyComponent.h"
#include "SpriteRendererComponent.h"
#include "TextComponent.h"
#include "RotatorComponent.h"
#include "TransformComponent.h"
#include "VolumeComponent.h"
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
	// world空間2D(Sprite方式)。スクリーン空間UIはCanvas方式のImageComponent側。
	factory.RegisterComponent<SpriteRendererComponent>();
	factory.RegisterComponent<CameraComponent>();
	factory.RegisterComponent<DebugCameraComponent>();
	factory.RegisterComponent<OrbitCameraComponent>();
	factory.RegisterComponent<DirectionalLightComponent>();
	factory.RegisterComponent<PointLightComponent>();
	factory.RegisterComponent<RigidbodyComponent>();
	factory.RegisterComponent<AnimatorComponent>();
	factory.RegisterComponent<AudioSourceComponent>();
	factory.RegisterComponent<SphereColliderComponent>();
	factory.RegisterComponent<BoxColliderComponent>();
	factory.RegisterComponent<CapsuleColliderComponent>();
	factory.RegisterComponent<CanvasComponent>();
	factory.RegisterComponent<RectTransformComponent>();
	factory.RegisterComponent<ImageComponent>();
	factory.RegisterComponent<TextComponent>();
	factory.RegisterComponent<ButtonComponent>();
	// ポストエフェクト(Fog/Bloom等)をシーンへ配置するためのVolume。
	factory.RegisterComponent<VolumeComponent>();

	registered = true;
}

} // namespace KujakuEngine
