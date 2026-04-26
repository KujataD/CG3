#include "KujakuEngine/KujakuEngine.h"
#include <cassert>
#include <fstream>
#include <memory>

using namespace KujakuEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	KujakuEngine::Initialize(L"LC2B_04_オオツカ_ダイチ_AL3", true);
	// タイトルシーンの初期化

	// ImGuiManagerインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();
	TextureManager::GetInstance()->Initialize();

	// カメラ
	// ------------------------------------------
	Camera camera;
	camera.Initialize();
	camera.translation_ = {0.0f, 0.0f, -20.0f};
	camera.UpdateMatrix();

	// モデル
	// ------------------------------------------
	Model* model = Model::CreateFromOBJ("player", true);
	Vector4 modelColor = Vector4(1.0f, 1.0f, 1.0f, 0.5f);
	model->SetColor(modelColor);
	WorldTransform modelWorldTransform;
	modelWorldTransform.Initialize();

	Model* model2 = Model::CreateFromOBJ("enemy", true);
	WorldTransform modelWorldTransform2;
	modelWorldTransform2.Initialize();
	modelWorldTransform2.UpdateMatrix(camera);


	// スプライト
	// ------------------------------------------
	uint32_t texture1 = TextureManager::GetInstance()->LoadTexture("resources/uvchecker.png");
	uint32_t texture2 = TextureManager::GetInstance()->LoadTexture("resources/white1x1.png");
	Sprite* sprite = Sprite::Create(texture1);
	sprite->SetTexture(texture2);
	sprite->SetColor(modelColor);

	bool isUvChecker = true;

	// ゲームループ
	while (KujakuEngine::Update()) {
		Input::Update();

		imguiManager->Begin();

		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

		modelWorldTransform.rotation_.y += 0.02f;
		modelWorldTransform.UpdateMatrix(camera);

		modelWorldTransform2.rotation_.y -= 0.02f;
		modelWorldTransform2.UpdateMatrix(camera);

		if (isUvChecker) {
			sprite->SetTexture(texture1);
		} else {
			sprite->SetTexture(texture2);
		}

#ifdef USE_IMGUI
		ImGui::Begin("ObjectManager");
		ImGui::Checkbox("isUvChecker", &isUvChecker);
		ImGui::DragFloat3("Model.translation", &modelWorldTransform.translation_.x, 0.01f);
		ImGui::ColorEdit4("ModelColor", &modelColor.x);
		model->SetColor(modelColor);
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

		Model::PreDraw();
		model2->Draw(modelWorldTransform2, camera);
		model->Draw(modelWorldTransform, camera);
		Model::PostDraw();

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
