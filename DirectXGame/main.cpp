#include "KujakuEngine/KujakuEngine.h"
#include <cassert>
#include <fstream>
#include <memory>

using namespace KujakuEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	KujakuEngine::Initialize(L"LC2B_04_オオツカ_ダイチ_AL3", true);

	// ImGuiManagerインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	// カメラ
	// ------------------------------------------
	Camera camera;
	camera.Initialize();
	camera.translation_ = {0.0f, 0.0f, -15.0f};
	camera.UpdateMatrix();

	// モデル
	// ------------------------------------------
	Model* model = Model::CreatePlane("resources/uvchecker.png", true);
	Vector4 modelColor = Vector4(1.0f, 1.0f, 1.0f, 0.5f);
	model->SetColor(modelColor);
	WorldTransform modelWorldTransform;
	modelWorldTransform.Initialize();
	modelWorldTransform.rotation_.y = std::numbers::pi_v<float>;
	modelWorldTransform.UpdateMatrix(camera);

	// パーティクル
	// ------------------------------------------
	Particle* particle = Particle::CreatePlane("resources/uvchecker.png", true);
	particle->Initialize();
	particle->SetBlendMode(BlendMode::kAdd);

	const uint32_t kNumParticle = 10u;
	WorldTransform transforms[kNumParticle];

	for (uint32_t i = 0; i < kNumParticle; i++) {
		transforms[i].Initialize();
		transforms[i].translation_ = {i * 0.1f, i * 0.1f, i * 0.1f};
		transforms[i].rotation_.y = std::numbers::pi_v<float>;
		transforms[i].UpdateMatrix(camera);
	}

	// スプライト
	// ------------------------------------------
	uint32_t texture1 = TextureManager::GetInstance()->LoadTexture("resources/uvchecker.png");
	uint32_t texture2 = TextureManager::GetInstance()->LoadTexture("resources/white1x1.png");
	Sprite* sprite = Sprite::Create(texture1);
	sprite->SetTexture(texture2);

	bool isUvChecker = true;

	// ゲームループ
	while (KujakuEngine::Update()) {
		Input::Update();

		imguiManager->Begin();

		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

		if (isUvChecker) {
			sprite->SetTexture(texture1);
		} else {
			sprite->SetTexture(texture2);
		}

		for (uint32_t i = 0; i < kNumParticle; i++) {
			particle->AddInstanceTransform(transforms[i], camera);
		}
		particle->UpdateBuffer();

#ifdef USE_IMGUI
		ImGui::Begin("ObjectManager");
		ImGui::Checkbox("isUvChecker", &isUvChecker);
		ImGui::DragFloat3("Model.translation", &modelWorldTransform.translation_.x, 0.01f);
		ImGui::ColorEdit4("ModelColor", &modelColor.x);
	particle->SetColor(modelColor);
		ImGui::End();

		ImGui::Begin("LightManager");
		auto& light = DirectionalLight::GetInstance()->GetData();
		ImGui::ColorEdit3("Light Color", &light.color.x);
		ImGui::SliderFloat3("Direction", &light.direction.x, -1.0f, 1.0f);
		ImGui::DragFloat("Intensity", &light.intensity, 0.01f);
		ImGui::End();
#endif // USE_IMGUI

		///
		/// ↑↑↑ 更新処理ここまで ↑↑↑
		///

		PreDraw();

		///
		/// ↓↓↓ 描画処理ここから ↓↓↓
		///

		Particle::PreDraw();
		particle->Draw(camera);
		Particle::PostDraw();

		Sprite::PreDraw();
		sprite->Draw();
		Sprite::PostDraw();

		///
		/// ↑↑↑ 描画処理ここまで ↑↑↑
		///
		PostDraw();
	}

	// エンジンの終了処理
	KujakuEngine::Finalize();

	return 0;
}
