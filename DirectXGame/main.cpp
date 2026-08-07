#include <KujataEngineEditor.h>

#include <cassert>
#include <fstream>
#include <memory>

using namespace KujataEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	KujataEngine::Initialize(L"Kujata Engine");

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
