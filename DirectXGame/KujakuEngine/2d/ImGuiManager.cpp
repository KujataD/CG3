#include "ImGuiManager.h"
#include "../../externals/imgui/imgui_internal.h"
#include "../base/DirectXCommon.h"
#include "../base/WinApp.h"

namespace KujakuEngine {
namespace {

void AllocateImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	// ImGui 1.92系のDX12バックエンドは、フォントや追加テクスチャ用のSRVをバックエンド側へ要求してくる。
	// KujakuEngine側のSRV採番関数を使うことで、Gameウィンドウ用SRVや通常テクスチャと番号が衝突しないようにする。
	DirectXCommon* dxCommon = static_cast<DirectXCommon*>(info->UserData);
	uint32_t srvIndex = dxCommon->AllocateSrvIndex();
	ID3D12DescriptorHeap* srvHeap = dxCommon->GetSrvDescriptorHeap();
	uint32_t descriptorSize = dxCommon->GetDescriptorSizeSRV();

	// CPUハンドルはCreateShaderResourceViewなど、CPU側からDescriptorを書き込む用途で使う。
	*outCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	outCpuHandle->ptr += descriptorSize * srvIndex;

	// GPUハンドルはシェーダーやImGui描画時にGPU側から参照する用途で使う。
	*outGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	outGpuHandle->ptr += descriptorSize * srvIndex;
}

void FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {
	// 現状はSRVヒープをフレーム中に再利用しないため解放処理は不要。
}

} // namespace

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize() {
#ifdef USE_IMGUI

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	WinApp* winApp = WinApp::GetInstance();

	// ImGui初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Dockingを有効化する。これによりDockSpaceや各ウィンドウのドッキングが使える。
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// キーボード操作を有効化して、エディタUIとして最低限扱いやすくする。
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// 1.92系のDX12バックエンドはInitInfoで初期化する。
	// 旧式の初期化ではフォントアトラス用テクスチャが正しく作られず、assertの原因になる。
	ImGui_ImplDX12_InitInfo initInfo{};
	initInfo.Device = dxCommon->GetDevice();
	initInfo.CommandQueue = dxCommon->GetCommandQueue();
	initInfo.NumFramesInFlight = dxCommon->GetSwapChainBufferCount();
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();
	initInfo.UserData = dxCommon;
	initInfo.SrvDescriptorAllocFn = AllocateImGuiSrvDescriptor;
	initInfo.SrvDescriptorFreeFn = FreeImGuiSrvDescriptor;
	ImGui_ImplDX12_Init(&initInfo);

	// エディタは必ず編集状態から始める。Startを押すまではゲームロジックを進めない。
	editorMode_ = EditorMode::Edit;
	// 初期レイアウトはImGui初期化後、最初のDrawDockSpaceで構築する。
	dockLayoutInitialized_ = false;
	consoleLogs_.clear();
	AddConsoleLog("Console initialized.");
	AddConsoleLog("Editor Mode: Edit");
#endif // USE_IMGUI
}

void ImGuiManager::AddConsoleLog(const std::string& message) {
	consoleLogs_.push_back(message);
	// 無制限にログを溜めるとメモリと描画負荷が増えるため、簡易Consoleでは古いログから捨てる。
	if (consoleLogs_.size() > 128) {
		consoleLogs_.erase(consoleLogs_.begin());
	}
}

void ImGuiManager::StartGame() {
	// Play中にStartを連打しても状態遷移やログ追加を重複させない。
	if (editorMode_ == EditorMode::Play) {
		return;
	}

	// ここでPlayへ切り替える。main.cpp側はIsPlaying()を見てゲームUpdateを実行する。
	editorMode_ = EditorMode::Play;
	consoleLogs_.clear();
	AddConsoleLog("[Editor] Start");
	AddConsoleLog("Editor Mode: Play");
}

void ImGuiManager::StopGame() {
	// Edit中のStop/Edit操作は何もしない。状態遷移はPlayからEditへ戻す時だけ行う。
	if (editorMode_ == EditorMode::Edit) {
		return;
	}

	// ここでEditへ戻す。main.cpp側のゲームUpdateは次フレームから止まる。
	editorMode_ = EditorMode::Edit;
	AddConsoleLog("[Editor] Stop");
	AddConsoleLog("Editor Mode: Edit");
}

void ImGuiManager::DrawDockSpace() {
#ifdef USE_IMGUI
	// MainViewport全体を覆う透明な親ウィンドウを作り、その中にDockSpaceを置く。
	// これによりGame/Hierarchy/Inspector/ConsoleをUnity風に分割配置できる。
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	// DockSpace用の親ウィンドウは移動・リサイズ・タイトルバー表示をさせず、画面全体の土台として使う。
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// 親ウィンドウの余白や角丸を消して、アプリ全体がエディタ領域になるようにする。
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("KujakuEditorDockSpace", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	if (ImGui::BeginMenuBar()) {
		// EditはStopと同じ扱い。今はPlayを止めて編集状態に戻すだけにしている。
		if (ImGui::Button("Edit")) {
			StopGame();
		}
		ImGui::SameLine();
		// Startを押すとPlayへ入り、main.cpp側でゲームロジックが進む。
		if (ImGui::Button("Start")) {
			StartGame();
		}
		ImGui::SameLine();
		// Stopを押すとEditへ戻り、main.cpp側でゲームロジックが止まる。
		if (ImGui::Button("Stop")) {
			StopGame();
		}
		ImGui::SameLine();

		// 現在のモードをメニューバー上にも出して、Start/Stopの結果をすぐ確認できるようにする。
		if (editorMode_ == EditorMode::Play) {
			ImGui::TextUnformatted("Mode: Play");
		} else {
			ImGui::TextUnformatted("Mode: Edit");
		}
		ImGui::EndMenuBar();
	}

	// DockSpaceのIDは毎フレーム同じ値にする必要がある。IDが変わるとドッキング状態を維持できない。
	ImGuiID dockspaceId = ImGui::GetID("KujakuEditorMainDockSpace");
	ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

	// DockBuilderによる初期配置は初回だけ。以降はユーザーがドラッグした配置を尊重する。
	if (!dockLayoutInitialized_) {
		SetupInitialLayout(dockspaceId);
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::SetupInitialLayout(ImGuiID dockspaceId) {
#ifdef USE_IMGUI
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// 既存ノードを一度消してから、Unityの2 by 3 Layoutに近い初期Dock構成を作る。
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
	ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

	// gameNodeを少しずつ分割して、Game / Hierarchy / Project / Inspector / Consoleを作る。
	ImGuiID hierarchyNode = 0;
	ImGuiID projectNode = 0;
	ImGuiID inspectorNode = 0;
	ImGuiID consoleNode = 0;
	ImGuiID gameNode = dockspaceId;

	// 右側にInspector、Project、Hierarchyの順で列を作る。
	// 画面上ではGame | Hierarchy | Project | Inspectorとなり、ProjectがHierarchyとInspectorの間に入る。
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.25f, &inspectorNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.20f, &projectNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.20f, &hierarchyNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Down, 0.28f, &consoleNode, &gameNode);

	// ウィンドウ名とDock先を紐づける。名前は各ImGui::Beginの文字列と一致させる。
	ImGui::DockBuilderDockWindow("Inspector", inspectorNode);
	ImGui::DockBuilderDockWindow("Project", projectNode);
	ImGui::DockBuilderDockWindow("Hierarchy", hierarchyNode);
	ImGui::DockBuilderDockWindow("Console", consoleNode);
	ImGui::DockBuilderDockWindow("Game", gameNode);
	ImGui::DockBuilderFinish(dockspaceId);

	// ここをtrueにして、次フレーム以降は初期配置を作り直さない。
	dockLayoutInitialized_ = true;
#endif // USE_IMGUI
}

void ImGuiManager::DrawGameWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Game");
	// DockされたGameウィンドウの内側サイズを取得する。タブや枠の分を除いた描画可能領域。
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	// Imageに0以下のサイズを渡さないように最低サイズを保証する。
	if (contentSize.x < 1.0f) {
		contentSize.x = 1.0f;
	}
	if (contentSize.y < 1.0f) {
		contentSize.y = 1.0f;
	}

	// DirectXCommonが作ったGame用RenderTargetのSRVをImGuiへ渡す。
	// 描画本体はmain.cppでBeginGameRenderからEndGameRenderの間に行われる。
	D3D12_GPU_DESCRIPTOR_HANDLE handle = DirectXCommon::GetInstance()->GetGameRenderSrvHandle();
	// Gameウィンドウの現在サイズに合わせてRenderTargetを表示する。
	ImGui::Image(static_cast<ImTextureID>(handle.ptr), contentSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawHierarchyWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Hierarchy");
	// 今回はScene管理をまだ入れないため、最低限の仮表示だけにしている。
	ImGui::TextUnformatted("Camera");
	ImGui::TextUnformatted("Terrain");
	ImGui::TextUnformatted("MonsterBall");
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawInspectorWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Inspector");
	// 今回は選択オブジェクトやComponent編集をまだ扱わないため空欄にしている。
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawConsoleWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Console");
	// ログとは別に、現在モードを常に先頭へ表示する。
	if (editorMode_ == EditorMode::Play) {
		ImGui::TextUnformatted("Editor Mode: Play");
	} else {
		ImGui::TextUnformatted("Editor Mode: Edit");
	}
	ImGui::Separator();
	for (const std::string& log : consoleLogs_) {
		ImGui::TextUnformatted(log.c_str());
	}
	// Consoleが末尾までスクロールされている場合は、新しいログへ追従する。
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawProjectWindow() {
#ifdef USE_IMGUI
	// ProjectDir以下のフォルダ/ファイルを閲覧するProject Windowを描画する。
	projectWindow_.Draw();
#endif // USE_IMGUI
}

void ImGuiManager::DrawEditor() {
#ifdef USE_IMGUI
	// Editor UIを構成する各ウィンドウを毎フレーム描画する。
	// 実際の配置はDockSpace側が管理するため、ここでは各ウィンドウを出すだけでよい。
	DrawDockSpace();
	DrawGameWindow();
	DrawHierarchyWindow();
	DrawInspectorWindow();
	// ProjectはDockBuilderでHierarchyとInspectorの間に初期配置される。
	DrawProjectWindow();
	DrawConsoleWindow();
#endif // USE_IMGUI
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI

	// エンジンのフレーム先頭処理に対応
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif // USE_IMGUI
}

void ImGuiManager::End() {
#ifdef USE_IMGUI

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	// Project Windowで要求されたモデルサムネイルを、ImGui本体の描画前にOffscreenへ描く。
	projectWindow_.RenderModelPreviews();
	ImGui::Render();
	// ImGuiで作ったUIをDirectX12のCommandListに描画命令として積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());
#endif // USE_IMGUI
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // USE_IMGUI
}

} // namespace KujakuEngine
