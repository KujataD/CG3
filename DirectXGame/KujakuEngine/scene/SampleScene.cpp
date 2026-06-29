#include "SampleScene.h"
#include "../2d/Sprite.h"
#include "../3d/DirectionalLight.h"
#include "../3d/Model.h"
#include "../3d/PointLight.h"
#include "../Editor/EditorApplication.h"
#include "../components/ModelRendererComponent.h"
#include "../components/RotatorComponent.h"
#include "../components/TransformSnapshotComponent.h"
#include "../math/MathUtil.h"
#include "../vfx/ParticleModel.h"
#include <cmath>
#include <memory>
#include <numbers>

namespace KujakuEngine {

void SampleScene::Initialize() {
	camera_.Initialize();
	camera_.translation_ = {0.0f, 0.0f, -20.0f};
	camera_.UpdateMatrix();

	editorCamera_.Initialize();
	editorCamera_.translation_ = camera_.translation_;
	editorCamera_.rotation_ = camera_.rotation_;
	editorCamera_.UpdateMatrix();
	currentViewCamera_ = &editorCamera_;

	debugCamera_.Initialize(editorCamera_.rotation_, editorCamera_.translation_);

	DirectionalLight::GetInstance()->GetData().intensity = 0.0f;

	PointLight::GetInstance()->Reset();
	SpotLight::GetInstance()->Reset();

	spotLight_.position = {0.0f, 5.0f, 0.0f};
	spotLight_.distance = 50.0f;
	spotLight_.direction = Normalize({0.0f, -1.0f, 0.0f});
	spotLight_.intensity = 1.0f;
	spotLight_.decay = 1.0f;
	spotLight_.cosAngle = std::cos(std::numbers::pi_v<float> / 2.0f);
	spotLight_.cosFalloffStart = std::cos(std::numbers::pi_v<float> / 3.0f);

	GameObject* ball = CreateGameObject("MonsterBall");
	ball->GetTransform().translation_ = {0.0f, -2.0f, 0.0f};
	ball->GetTransform().rotation_.x = std::numbers::pi_v<float> * -0.5f;
	ball->AddComponent<TransformSnapshotComponent>();
	ball->AddComponent<RotatorComponent>();
	ModelRendererComponent* ballRenderer = ball->AddComponent<ModelRendererComponent>(&camera_);
	ballRenderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Sphere, "resources/monsterBall.png");

	GameObject* terrain = CreateGameObject("Terrain");
	terrain->AddComponent<ModelRendererComponent>(std::unique_ptr<Model>(Model::CreateFromGlTF("plane", ShaderModel::kBlingPhongReflection)), &camera_);

	Scene::Initialize();
}

void SampleScene::Update() {
	Scene::Update();
}

void SampleScene::Draw() {
	UpdateSceneView();

	ParticleModel::PreDraw();
	ParticleModel::PostDraw();

	Model::PreDraw();
	ApplyRenderCameraToModelRenderers(currentViewCamera_);
	Scene::Draw();
	Model::PostDraw();

	Sprite::PreDraw();
	Sprite::PostDraw();
}

void SampleScene::UpdateSceneView() {
	if (EditorApplication::GetInstance()->IsPlaying()) {
		currentViewCamera_ = &camera_;
		camera_.UpdateMatrix();
	} else {
		debugCamera_.Update();
		editorCamera_.translation_ = debugCamera_.translation_;
		editorCamera_.rotation_ = debugCamera_.rotation_;
		editorCamera_.UpdateMatrix();
		currentViewCamera_ = &editorCamera_;
	}

	spotLight_.direction = Normalize(spotLight_.direction);
	SpotLight::GetInstance()->SetLight(&spotLight_);
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

	ModelRendererComponent* renderer = cube->AddComponent<ModelRendererComponent>(currentViewCamera_);
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

	ModelRendererComponent* renderer = sphere->AddComponent<ModelRendererComponent>(currentViewCamera_);
	if (renderer) {
		renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Sphere, "resources/monsterBall.png");
	}

	return sphere;
}

void SampleScene::OnEditorComponentAdded(GameObject* gameObject, Component* component) {
	(void)gameObject;

	ModelRendererComponent* renderer = dynamic_cast<ModelRendererComponent*>(component);
	if (!renderer) {
		return;
	}

	renderer->SetCamera(currentViewCamera_);
}

} // namespace KujakuEngine
