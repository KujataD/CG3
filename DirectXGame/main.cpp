#include "KujakuEngine/KujakuEngine.h"
#include <cassert>
#include <fstream>
#include <memory>

using namespace KujakuEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	Initialize(L"LC2B_04_オオツカ_ダイチ_AL3", true);
	// タイトルシーンの初期化
	
	// ImGuiManagerインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	// ゲームループ
	while (Update()) {
		Input::Update();

		imguiManager->Begin();
		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

		///
		/// ↑↑↑ 更新処理ここまで ↑↑↑
		///

		PreDraw();

		///
		/// ↓↓↓ 描画処理ここから ↓↓↓
		///

		///
		/// ↑↑↑ 描画処理ここまで ↑↑↑
		///
		PostDraw();
	}

	// エンジンの終了処理
	Finalize();

	return 0;
}
