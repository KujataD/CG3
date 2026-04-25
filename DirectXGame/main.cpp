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

	Model* model = Model::CreateFromOBJ("player");
	Camera camera;
	camera.Initialize();
	camera.translation_ = {0.0f, 0.0f, -20.0f};
	camera.UpdateMatrix();
	WorldTransform worldTransform;
	worldTransform.Initialize();

	uint32_t texture = TextureManager::GetInstance()->LoadTexture("resources/uvchecker.png");
	Sprite* sprite = Sprite::Create(texture);

	// ゲームループ
	while (KujakuEngine::Update()) {
		Input::Update();

		imguiManager->Begin();

		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

		worldTransform.rotation_.y += 0.02f;
		worldTransform.UpdateMatrix(camera);

		///
		/// ↑↑↑ 更新処理ここまで ↑↑↑
		///

		PreDraw();

		///
		/// ↓↓↓ 描画処理ここから ↓↓↓
		///

		Model::PreDraw();
		model->Draw(worldTransform, camera);
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
