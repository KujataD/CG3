#include <KujakuEngine.h>

#include <cassert>
#include <fstream>
#include <memory>

using namespace KujakuEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	KujakuEngine::Initialize(L"LC2B_04_オオツカ_ダイチ_AL3");

	EditorApplication* editorApplication = EditorApplication::GetInstance();
	editorApplication->Initialize();
	editorApplication->SetCurrentScene(std::make_unique<SampleScene>());

	// ゲームループ
	while (KujakuEngine::Update()) {
		editorApplication->BeginFrame();
		editorApplication->Update();
		editorApplication->Draw();
		editorApplication->EndFrame();
	}

	editorApplication->Finalize();

	// エンジンの終了処理
	KujakuEngine::Finalize();

	return 0;
}
