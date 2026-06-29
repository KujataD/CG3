#include "KujakuEngine.h"
#include <objbase.h>

namespace KujakuEngine {

void Initialize(const std::wstring& title, Vector4 color, bool enableDebugLayer) {
	// COM(Component Object Model)を使うための初期化を行う。
	// ※ゲーム終了時にUnInitializeが必要。
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// Loggerの初期化
	Logger::Initialize();

	// ウィンドウの生成
	WinApp* winApp = WinApp::GetInstance();
	winApp->CreateGameWindow(title);

	// DirectX の初期化
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	dxCommon->Initialize(winApp, color, WinApp::kWindowWidth, WinApp::kWindowHeight, enableDebugLayer);

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

	// グローバル変数の読み込み
	GlobalVariables::GetInstance()->LoadFiles();

	// 時間管理の初期化
	Time::GetInstance()->Init();
}

void Finalize() { WinApp::GetInstance()->TerminateGameWindow(); }

bool Update() {

	// ウィンドウメッセージを処理し、終了リクエストなら false を返す
	if (WinApp::GetInstance()->ProcessMessage()) {
		return false;
	}

	// 入力処理更新
	Input::Update();

	// Imguiのフレーム開始処理
	ImGuiManager::GetInstance()->Begin();

	// 時間管理の更新
	Time::GetInstance()->Update();

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
