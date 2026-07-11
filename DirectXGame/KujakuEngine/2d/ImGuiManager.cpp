#include "ImGuiManager.h"
#include "../../externals/ImGuizmo-1.9/src/ImGuizmo.h"
#include "../Editor/EditorApplication.h"
#include "../Editor/EditorConsole.h"
#include "../Editor/EditorStyle.h"
#include "../Editor/EditorUndoManager.h"
#include "../Editor/SceneJsonExporter.h"
#include "../base/DirectXCommon.h"
#include "../base/WinApp.h"
#include "../scene/Scene.h"
#include <cstdint>
#include <string>

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
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

	EditorStyle::Apply();
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

	// 初期レイアウトはImGui初期化後、最初のEditorDockSpace::Drawで構築する。
	dockSpace_.ResetLayout();
	ClearConsoleLogs();
	AddConsoleLog("Console initialized.");
#endif // USE_IMGUI
}

void ImGuiManager::AddConsoleLog(const std::string& message) { EditorConsole::GetInstance()->AddLog(message); }

void ImGuiManager::ClearConsoleLogs() { EditorConsole::GetInstance()->ClearLogs(); }

void ImGuiManager::HandleEditorShortcuts() {
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontFromFileTTF(
	//	"C:/Windows/Fonts/Arial.ttf", 
	//	24.0f, 
	//	nullptr, 
	//	io.Fonts->GetGlyphRangesJapanese()
	//);

	io.FontGlobalScale = 1.0f;

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (scene && !io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
		bool redo = io.KeyShift;
		if (redo) {
			if (EditorUndoManager::GetInstance()->Redo(*scene)) {
				AddConsoleLog("[Undo] Redo.");
			}
		} else {
			if (EditorUndoManager::GetInstance()->Undo(*scene)) {
				AddConsoleLog("[Undo] Undo.");
			}
		}
		return;
	}
	if (scene && !io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
		if (EditorUndoManager::GetInstance()->Redo(*scene)) {
			AddConsoleLog("[Undo] Redo.");
		}
		return;
	}
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
		if (EditorApplication::GetInstance()->IsPrefabEditing()) {
			EditorApplication::GetInstance()->SavePrefabEditMode();
			return;
		}
		ExportCurrentSceneJson();
	}
	if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_R, false)) {
		EditorApplication::GetInstance()->ReloadGameModule();
	}
#endif // USE_IMGUI
}

void ImGuiManager::ExportCurrentSceneJson() {
	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		AddConsoleLog("[Editor] Scene JSON export failed: No Scene.");
		return;
	}

	SceneJsonExporter::ExportResult exportResult = SceneJsonExporter::ExportScene(*scene, projectWindow_.GetProjectRoot());
	if (exportResult.succeeded) {
		AddConsoleLog("[Editor] Scene JSON exported: " + exportResult.outputDirectory.string());
		projectWindow_.Refresh();
		return;
	}

	AddConsoleLog("[Editor] Scene JSON export failed: " + exportResult.message);
}

void ImGuiManager::DrawEditor() {
#ifdef USE_IMGUI
	// Editor UIを構成する各ウィンドウを毎フレーム描画する。
	// 実際の配置はDockSpace側が管理するため、ここでは各ウィンドウを出すだけでよい。
	HandleEditorShortcuts();
	dockSpace_.Draw();
	sceneView_.Draw(projectWindow_.GetProjectRoot());
	hierarchyWindow_.Draw();
	inspectorWindow_.Draw(projectWindow_);
	// ProjectはDockBuilderでHierarchyとInspectorの間に初期配置される。
	projectWindow_.Draw();
	EditorConsole::GetInstance()->Draw();
#endif // USE_IMGUI
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI

	// エンジンのフレーム先頭処理に対応
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
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
