#include "SampleScene.h"
#include "../2d/Sprite.h"
#include "../3d/Camera.h"
#include "../3d/DirectionalLight.h"
#include "../3d/Model.h"
#include "../3d/PointLight.h"
#include "../3d/SpotLight.h"
#include "../runtime/PlayState.h"
#include "../components/CameraComponent.h"
#include "../components/DebugCameraComponent.h"
#include "../components/DirectionalLightComponent.h"
#include "../components/ModelRendererComponent.h"
#include "../components/PointLightComponent.h"
#include "../components/RotatorComponent.h"
#include "../math/MathUtil.h"
#include "../vfx/ParticleModel.h"
#include <memory>
#include <numbers>

namespace KujakuEngine {

namespace {

const char* kMainCameraName = "Main Camera";
const char* kEditorDebugCameraName = "Editor Debug Camera";
const char* kDirectionalLightName = "Directional Light";
const char* kPointLightName = "Point Light";

} // namespace

void SampleScene::Initialize() {
	EnsureSceneServiceObjects();

	PointLight::GetInstance()->Reset();
	SpotLight::GetInstance()->Reset();

	Camera* renderCamera = GetCurrentViewCamera();

	GameObject* ball = CreateGameObject("MonsterBall");
	ball->GetTransform().translation_ = {0.0f, -2.0f, 0.0f};
	ball->GetTransform().rotation_.x = std::numbers::pi_v<float> * -0.5f;
	ball->AddComponent<RotatorComponent>();
	ModelRendererComponent* ballRenderer = ball->AddComponent<ModelRendererComponent>(renderCamera);
	ballRenderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Sphere, "resources/monsterBall.png");

	GameObject* terrain = CreateGameObject("Terrain");
	terrain->AddComponent<ModelRendererComponent>(std::unique_ptr<Model>(Model::CreateFromGlTF("plane", ShaderModel::kBlingPhongReflection)), renderCamera);

	Scene::Initialize();
	ApplyRenderCameraToModelRenderers(GetCurrentViewCamera());
}

void SampleScene::Update() {
	Scene::Update();
}

void SampleScene::Draw() {
	UpdateSceneView();
	ApplySceneLights();

	ParticleModel::PreDraw();
	ParticleModel::PostDraw();

	Model::PreDraw();
	ApplyRenderCameraToModelRenderers(GetCurrentViewCamera());
	Scene::Draw();
	Model::PostDraw();

	Sprite::PreDraw();
	Sprite::PostDraw();
}

Camera* SampleScene::GetEditorCamera() {
	return GetCurrentViewCamera();
}

void SampleScene::UpdateSceneView() {
	EnsureSceneServiceObjects();

	if (IsGamePlaying()) {
		currentViewCameraComponent_ = gameCameraComponent_;
	} else {
		if (editorDebugCameraComponent_) {
			editorDebugCameraComponent_->UpdateEditorCamera();
		}
		currentViewCameraComponent_ = editorCameraComponent_;
	}

	if (currentViewCameraComponent_) {
		currentViewCameraComponent_->SyncFromOwnerTransform();
	}
}

void SampleScene::EnsureSceneServiceObjects() {
	GameObject* mainCamera = FindGameObjectByName(kMainCameraName);
	if (!mainCamera) {
		mainCamera = CreateGameObject(kMainCameraName);
		mainCamera->GetTransform().translation_ = {0.0f, 0.0f, -20.0f};
	}
	gameCameraComponent_ = mainCamera->GetComponent<CameraComponent>();
	if (!gameCameraComponent_) {
		gameCameraComponent_ = mainCamera->AddComponent<CameraComponent>();
	}

	GameObject* editorCamera = FindGameObjectByName(kEditorDebugCameraName);
	if (!editorCamera) {
		editorCamera = CreateGameObject(kEditorDebugCameraName);
		editorCamera->GetTransform().translation_ = {0.0f, 0.0f, -20.0f};
	}
	editorCameraComponent_ = editorCamera->GetComponent<CameraComponent>();
	if (!editorCameraComponent_) {
		editorCameraComponent_ = editorCamera->AddComponent<CameraComponent>();
	}
	editorDebugCameraComponent_ = editorCamera->GetComponent<DebugCameraComponent>();
	if (!editorDebugCameraComponent_) {
		editorDebugCameraComponent_ = editorCamera->AddComponent<DebugCameraComponent>();
	}

	GameObject* directionalLight = FindGameObjectByName(kDirectionalLightName);
	if (!directionalLight) {
		directionalLight = CreateGameObject(kDirectionalLightName);
	}
	DirectionalLightComponent* directionalLightComponent = directionalLight->GetComponent<DirectionalLightComponent>();
	if (!directionalLightComponent) {
		directionalLightComponent = directionalLight->AddComponent<DirectionalLightComponent>();
		directionalLightComponent->GetData().intensity = 0.0f;
	}

	GameObject* pointLight = FindGameObjectByName(kPointLightName);
	if (!pointLight) {
		pointLight = CreateGameObject(kPointLightName);
		pointLight->GetTransform().translation_ = {0.0f, 5.0f, 0.0f};
	}
	PointLightComponent* pointLightComponent = pointLight->GetComponent<PointLightComponent>();
	if (!pointLightComponent) {
		pointLightComponent = pointLight->AddComponent<PointLightComponent>();
		pointLightComponent->GetData().intensity = 0.0f;
	}

	if (IsGamePlaying()) {
		currentViewCameraComponent_ = gameCameraComponent_;
	} else {
		currentViewCameraComponent_ = editorCameraComponent_;
	}
}

GameObject* SampleScene::FindGameObjectByName(const std::string& name) {
	for (const std::unique_ptr<GameObject>& gameObject : GetGameObjects()) {
		if (!gameObject) {
			continue;
		}
		if (gameObject->GetName() == name) {
			return gameObject.get();
		}
	}

	return nullptr;
}

Camera* SampleScene::GetCurrentViewCamera() {
	EnsureSceneServiceObjects();

	if (currentViewCameraComponent_) {
		currentViewCameraComponent_->SyncFromOwnerTransform();
		return &currentViewCameraComponent_->GetCamera();
	}

	return nullptr;
}

void SampleScene::ApplySceneLights() {
	DirectionalLight::GetInstance()->Reset();
	PointLight::GetInstance()->Reset();
	SpotLight::GetInstance()->Reset();

	for (const std::unique_ptr<GameObject>& gameObject : GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (!component || !component->IsEnabled()) {
				continue;
			}

			DirectionalLightComponent* directionalLight = dynamic_cast<DirectionalLightComponent*>(component.get());
			if (directionalLight) {
				directionalLight->Apply();
				continue;
			}

			PointLightComponent* pointLight = dynamic_cast<PointLightComponent*>(component.get());
			if (pointLight) {
				pointLight->Apply();
			}
		}
	}
}

void SampleScene::ApplyRenderCameraToModelRenderers(const Camera* camera) {
	for (const std::unique_ptr<GameObject>& gameObject : GetGameObjects()) {
		if (!gameObject) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (!component) {
				continue;
			}

			ModelRendererComponent* renderer = dynamic_cast<ModelRendererComponent*>(component.get());
			if (!renderer) {
				continue;
			}

			renderer->SetCamera(camera);
		}
	}
}

GameObject* SampleScene::CreateEditorCube() {
	GameObject* cube = CreateGameObject("Cube");
	if (!cube) {
		return nullptr;
	}

	ModelRendererComponent* renderer = cube->AddComponent<ModelRendererComponent>(GetCurrentViewCamera());
	if (renderer) {
		renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Cube, "resources/monsterBall.png");
	}

	return cube;
}

GameObject* SampleScene::CreateEditorSphere() {
	GameObject* sphere = CreateGameObject("Sphere");
	if (!sphere) {
		return nullptr;
	}

	ModelRendererComponent* renderer = sphere->AddComponent<ModelRendererComponent>(GetCurrentViewCamera());
	if (renderer) {
		renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Sphere, "resources/monsterBall.png");
	}

	return sphere;
}

void SampleScene::OnEditorComponentAdded(GameObject* gameObject, Component* component) {
	Scene::OnEditorComponentAdded(gameObject, component);

	ModelRendererComponent* renderer = dynamic_cast<ModelRendererComponent*>(component);
	if (renderer) {
		renderer->SetCamera(GetCurrentViewCamera());
		return;
	}

	DebugCameraComponent* debugCamera = dynamic_cast<DebugCameraComponent*>(component);
	if (debugCamera && gameObject) {
		if (!gameObject->GetComponent<CameraComponent>()) {
			CameraComponent* addedCamera = gameObject->AddComponent<CameraComponent>();
			Scene::OnEditorComponentAdded(gameObject, addedCamera);
		}
		debugCamera->OnAfterReadJson();
	}

	CameraComponent* camera = dynamic_cast<CameraComponent*>(component);
	if (camera || debugCamera) {
		EnsureSceneServiceObjects();
	}
}

} // namespace KujakuEngine
