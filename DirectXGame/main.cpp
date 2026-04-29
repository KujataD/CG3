#include <KujakuEngine.h>
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
	camera.translation_ = {0.0f, 0.0f, -20.0f};
	camera.UpdateMatrix();

	DebugCamera debugCamera;
	debugCamera.Initialize(camera.rotation_, camera.translation_);

	auto& light = DirectionalLight::GetInstance()->GetData();
	light.intensity = 0.0f;

	PointLight::GetInstance()->Reset();

	PointLightData pointLights[2];
	pointLights[0].color = {0.0f, 1.0f, 0.0f, 1.0f};
	pointLights[0].position = {0.0f, 0.0f, -1.0f};
	pointLights[0].intensity = 1.0f;
	pointLights[0].radius = 10.0f;
	pointLights[0].decay = 1.0f;
	pointLights[1].color = {1.0f, 0.0f, 1.0f, 1.0f};
	pointLights[1].position = {0.0f, 0.0f, 1.0f};
	pointLights[1].intensity = 1.0f;
	pointLights[1].radius = 10.0f;
	pointLights[1].decay = 1.0f;

	// モンスターボール
	// ------------------------------------------
	Model* modelBall = Model::CreateSphere("resources/monsterBall.png", ShaderModel::kBlingPhongReflection);
	WorldTransform ballTransform;
	ballTransform.Initialize();
	ballTransform.translation_ = {0.0f, -2.0f, 0.0f};

	// テレイン
	// ------------------------------------------
	Model* modelTerrain = Model::CreateFromOBJ("terrain", ShaderModel::kBlingPhongReflection);

	// ゲームループ
	while (KujakuEngine::Update()) {
		Input::Update();

		imguiManager->Begin();

		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

		// カメラ更新
		// --------------------------------------
		debugCamera.Update();
		camera.translation_ = debugCamera.translation_;
		camera.rotation_ = debugCamera.rotation_;
		camera.UpdateMatrix();

		// ライト更新
		PointLight::GetInstance()->SetLight(0, pointLights[0]);
		PointLight::GetInstance()->SetLight(1, pointLights[1]);

		// ボール
		// --------------------------------------
		ballTransform.UpdateMatrix(camera);

		// Imgui
		// --------------------------------------
#ifdef USE_IMGUI
		ImGui::Begin("LightManager");
		ImGui::ColorEdit3("Light Color", &light.color.x);
		ImGui::SliderFloat3("Direction", &light.direction.x, -1.0f, 1.0f);
		ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 10.0f);

		ImGui::ColorEdit3("PointLight Color", &pointLights[0].color.x);
		ImGui::DragFloat("PointLight Intensity", &pointLights[0].intensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat3("PointLight position", &pointLights[0].position.x, 0.01f);
		ImGui::DragFloat("PointLight radius", &pointLights[0].radius, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("PointLight decay", &pointLights[0].decay, 0.01f, 0.0f, 10.0f);

		ImGui::ColorEdit3("PointLight1 Color", &pointLights[1].color.x);
		ImGui::DragFloat("PointLight1 Intensity", &pointLights[1].intensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat3("PointLight1 position", &pointLights[1].position.x, 0.01f);
		ImGui::DragFloat("PointLight1 radius", &pointLights[1].radius, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("PointLight1 decay", &pointLights[1].decay, 0.01f, 0.0f, 10.0f);

		ImGui::DragFloat3("monstarBall", &ballTransform.scale_.x, 0.01f);
		ImGui::End();
#endif // USE_IMGUI

		///
		/// ↑↑↑ 更新処理ここまで ↑↑↑
		///

		PreDraw();

		///
		/// ↓↓↓ 描画処理ここから ↓↓↓
		///

		ParticleModel::PreDraw();
		ParticleModel::PostDraw();

		Model::PreDraw();
		modelBall->Draw(ballTransform, camera);
		modelTerrain->Draw(ballTransform, camera);
		Model::PostDraw();

		Sprite::PreDraw();
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
