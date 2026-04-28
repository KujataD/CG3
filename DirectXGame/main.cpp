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

	// パーティクル
	// ------------------------------------------
	ParticleModel* particleModel = ParticleModel::CreatePlane("resources/circle.png", true);
	particleModel->Initialize();
	particleModel->SetBlendMode(BlendMode::kAdd);

	// パーティクルフィールド
	// ------------------------------------------
	AccelerationField acField;
	acField.acceleration = {15.0f, 0.0f, 0.0f};
	acField.area.min = {-1.0f, -1.0f, -1.0f};
	acField.area.max = {1.0f, 1.0f, 1.0f};

	bool useField = true;

	// エミッター
	// ------------------------------------------
	ParticleEmitter emitter;
	emitter.Initialize(particleModel);
	emitter.AddField(acField);
	emitter.Emit();

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

		// パーティクル
		// --------------------------------------
		emitter.Update(kDT, camera);
		emitter.SetIsActiveField(useField);

		// Imgui
		// --------------------------------------
#ifdef USE_IMGUI
		ImGui::Begin("LightManager");
		auto& light = DirectionalLight::GetInstance()->GetData();
		ImGui::ColorEdit3("Light Color", &light.color.x);
		ImGui::SliderFloat3("Direction", &light.direction.x, -1.0f, 1.0f);
		ImGui::DragFloat("Intensity", &light.intensity, 0.01f);
		ImGui::DragFloat3("EmitterTranslate", &emitter.translation_.x, 0.01f, -100.0f, 100.0f);
		ImGui::Checkbox("useField", &useField);

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
		emitter.Draw(camera);
		ParticleModel::PostDraw();

		Sprite::PreDraw();
		Sprite::PostDraw();

		///
		/// ↑↑↑ 描画処理ここまで ↑↑↑
		///
		PostDraw();
	}

	delete particleModel;
	particleModel = nullptr;

	// エンジンの終了処理
	KujakuEngine::Finalize();

	return 0;
}
