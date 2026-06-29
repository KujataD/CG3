#include "SampleScene.h"
#include "../2d/Sprite.h"
#include "../3d/DirectionalLight.h"
#include "../3d/Model.h"
#include "../3d/PointLight.h"
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

	debugCamera_.Initialize(camera_.rotation_, camera_.translation_);

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
	Scene::Draw();
	Model::PostDraw();

	Sprite::PreDraw();
	Sprite::PostDraw();
}

void SampleScene::UpdateSceneView() {
	debugCamera_.Update();
	camera_.translation_ = debugCamera_.translation_;
	camera_.rotation_ = debugCamera_.rotation_;
	camera_.UpdateMatrix();

	spotLight_.direction = Normalize(spotLight_.direction);
	SpotLight::GetInstance()->SetLight(&spotLight_);
}

GameObject* SampleScene::CreateEditorCube() {
	GameObject* cube = CreateGameObject("Cube");
	if (!cube) {
		return nullptr;
	}

	ModelRendererComponent* renderer = cube->AddComponent<ModelRendererComponent>(&camera_);
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

	ModelRendererComponent* renderer = sphere->AddComponent<ModelRendererComponent>(&camera_);
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

	renderer->SetCamera(&camera_);
}

} // namespace KujakuEngine
