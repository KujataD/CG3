#include "KujakuEngine.h"
#include <objbase.h>

namespace KujakuEngine {

void Initialize(const std::wstring& title, bool enableDebugLayer) {
	// COM(Component Object Model)を使うための初期化を行う。
	// ※ゲーム終了時にUnInitializeが必要。
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウの生成
	WinApp* winApp = WinApp::GetInstance();
	winApp->CreateGameWindow(title);

	// DirectX の初期化
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	dxCommon->Initialize(winApp, WinApp::kWindowWidth, WinApp::kWindowHeight, enableDebugLayer);

#ifdef USE_IMGUI
	// ImGui の初期化
	ImGuiManager::GetInstance()->Initialize();
#endif // USE_IMGUI

	// グラフィックスパイプラインの初期化
	GraphicsPipeline::GetInstance()->Initialize();

	// Light初期化
	DirectionalLight::GetInstance()->Initialize();
	PointLight::GetInstance()->Initialize();
	SpotLight::GetInstance()->Initialize();
	
	// texture初期化
	TextureManager::GetInstance()->Initialize();

	Input::Initialize();

	Random::Initialize();
}

void Finalize() { WinApp::GetInstance()->TerminateGameWindow(); }

bool Update() {
	// ウィンドウメッセージを処理し、終了リクエストなら false を返す
	if (WinApp::GetInstance()->ProcessMessage()) {
		return false;
	}
	return true;
}

void PreDraw() {
	// バックバッファクリア＆バリア設定
	DirectXCommon::GetInstance()->PreDraw();
}

void PostDraw() {

#ifdef USE_IMGUI
	// ImGuiの描画コマンドを積む（描画処理の最後に呼ぶ）
	ImGuiManager::GetInstance()->End();
#endif // USE_IMGUI

	// プレゼント・フェンス待機・コマンドリストリセット
	DirectXCommon::GetInstance()->PostDraw();
}

} // namespace KujakuEngine
