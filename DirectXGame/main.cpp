#include <KujataEngineEditor.h>

#include <cassert>
#include <fstream>
#include <memory>

using namespace KujataEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	// 画面奥は洞窟の闇。空色のままだと、天井の無い通路から明るい「空」が覗いてしまう。
	KujataEngine::Initialize(L"LE2B_04_オオツカ_ダイチ_BOARD:BORDER", {0.016f, 0.014f, 0.012f, 1.0f});

	EditorApplication* editorApplication = EditorApplication::GetInstance();
	editorApplication->Initialize();

	// ゲームループ
	while (KujataEngine::Update()) {
		editorApplication->BeginFrame();
		editorApplication->Update();
		editorApplication->Draw();
		editorApplication->EndFrame();
	}

	editorApplication->Finalize();

	// エンジンの終了処理
	KujataEngine::Finalize();

	return 0;
}
