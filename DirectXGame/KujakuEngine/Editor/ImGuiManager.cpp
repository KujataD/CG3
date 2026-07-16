#include "ImGuiManager.h"
#include "../../externals/ImGuizmo-1.9/src/ImGuizmo.h"
#include "../../externals/imsearch/imsearch.h"
#include "EditorApplication.h"
#include "EditorConsole.h"
#include "EditorImGuiUtil.h"
#include "EditorSelection.h"
#include "EditorStyle.h"
#include "EditorUndoManager.h"
#include "SceneJsonExporter.h"
#include "../base/DirectXCommon.h"
#include "../base/WinApp.h"
#include "../runtime/PlayState.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include <cstdint>
#include <filesystem>
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

	// フォントを既定より少し大きく、きれいなサンサリフ(Segoe UI)にする。
	// 日本語グリフは存在すれば別フォントからマージする。無ければImGui既定フォントにフォールバック。
	{
		const float fontSize = 18.0f;
		const char* latinFontPath = "C:/Windows/Fonts/segoeui.ttf";
		if (std::filesystem::exists(latinFontPath)) {
			io.Fonts->AddFontFromFileTTF(latinFontPath, fontSize);

			const char* japaneseFontPaths[] = {
			    "C:/Windows/Fonts/YuGothR.ttc",
			    "C:/Windows/Fonts/meiryo.ttc",
			    "C:/Windows/Fonts/msgothic.ttc",
			};
			for (const char* japaneseFontPath : japaneseFontPaths) {
				if (std::filesystem::exists(japaneseFontPath)) {
					ImFontConfig mergeConfig;
					mergeConfig.MergeMode = true;
					io.Fonts->AddFontFromFileTTF(japaneseFontPath, fontSize, &mergeConfig, io.Fonts->GetGlyphRangesJapanese());
					break;
				}
			}
		}
	}

	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
	// ImSearch(AddComponent等の検索)のコンテキストをImGuiコンテキスト生成直後に作る。
	ImSearch::CreateContext();

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
	dockSpace_.Draw([this]() { DrawMainMenuBar(); }, [this]() { DrawToolbar(); });

	// Windowメニューの表示フラグに応じて各ウィンドウを描画する。
	// 各ウィンドウのBeginにp_openを渡すことで、閉じるボタン[x]でもフラグが下りる。
	if (windowVisibility_.scene) {
		sceneView_.Draw(projectWindow_.GetProjectRoot(), &windowVisibility_.scene);
	} else {
		// 非表示のビューは描画パスをスキップさせる。
		SetSceneViewVisible(false);
	}
	if (windowVisibility_.game) {
		gameView_.Draw(&windowVisibility_.game);
	} else {
		SetGameViewVisible(false);
	}
	if (windowVisibility_.hierarchy) {
		hierarchyWindow_.Draw(&windowVisibility_.hierarchy);
	}
	if (windowVisibility_.inspector) {
		inspectorWindow_.Draw(projectWindow_, &windowVisibility_.inspector);
	}
	// ProjectはDockBuilderでHierarchyとInspectorの間に初期配置される。
	if (windowVisibility_.project) {
		projectWindow_.Draw(&windowVisibility_.project);
	}
	if (windowVisibility_.performance) {
		performanceWindow_.Draw(&windowVisibility_.performance);
	}
	if (windowVisibility_.console) {
		EditorConsole::GetInstance()->Draw(&windowVisibility_.console);
	}
#endif // USE_IMGUI
}

namespace {
#ifdef USE_IMGUI
// メニューバー内の再生コントロール用アイコンボタン。
// isTriangle=true で再生(▶ 三角)、false で編集(■ 四角)を描く。activeなら強調表示。クリックでtrue。
bool DrawModeIconButton(const char* id, bool active, bool isTriangle) {
	const float height = ImGui::GetFrameHeight();
	ImGui::InvisibleButton(id, ImVec2(height, height));
	const bool clicked = ImGui::IsItemClicked();

	const ImVec2 pMin = ImGui::GetItemRectMin();
	const ImVec2 pMax = ImGui::GetItemRectMax();

	ImU32 color;
	if (active) {
		color = ImGui::GetColorU32(ImGuiCol_ButtonActive);
	} else if (ImGui::IsItemHovered()) {
		color = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
	} else {
		color = ImGui::GetColorU32(ImGuiCol_Text);
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float pad = height * 0.28f;
	if (isTriangle) {
		const ImVec2 a(pMin.x + pad, pMin.y + pad);
		const ImVec2 b(pMin.x + pad, pMax.y - pad);
		const ImVec2 c(pMax.x - pad, (pMin.y + pMax.y) * 0.5f);
		drawList->AddTriangleFilled(a, b, c, color);
	} else {
		drawList->AddRectFilled(ImVec2(pMin.x + pad, pMin.y + pad), ImVec2(pMax.x - pad, pMax.y - pad), color);
	}
	return clicked;
}
#endif // USE_IMGUI
} // namespace

void ImGuiManager::DrawMainMenuBar() {
#ifdef USE_IMGUI
	EditorApplication* app = EditorApplication::GetInstance();
	Scene* scene = app->GetCurrentScene();
	const bool hasScene = scene != nullptr;

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
			if (app->IsPrefabEditing()) {
				app->SavePrefabEditMode();
			} else {
				ExportCurrentSceneJson();
			}
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Exit")) {
			PostQuitMessage(0);
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit")) {
		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, hasScene)) {
			if (EditorUndoManager::GetInstance()->Undo(*scene)) {
				AddConsoleLog("[Undo] Undo.");
			}
		}
		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, hasScene)) {
			if (EditorUndoManager::GetInstance()->Redo(*scene)) {
				AddConsoleLog("[Undo] Redo.");
			}
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("GameObject")) {
		if (ImGui::MenuItem("Create Empty", nullptr, false, hasScene)) {
			CaptureUndo(*scene, "Create Entity");
			EditorSelection::GetInstance()->SetSelectedGameObject(scene->CreateEditorEntity());
		}
		if (ImGui::MenuItem("Create Cube", nullptr, false, hasScene)) {
			CaptureUndo(*scene, "Create Cube");
			EditorSelection::GetInstance()->SetSelectedGameObject(scene->CreateEditorCube());
		}
		if (ImGui::MenuItem("Create Sphere", nullptr, false, hasScene)) {
			CaptureUndo(*scene, "Create Sphere");
			EditorSelection::GetInstance()->SetSelectedGameObject(scene->CreateEditorSphere());
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window")) {
		ImGui::MenuItem("Scene", nullptr, &windowVisibility_.scene);
		ImGui::MenuItem("Game", nullptr, &windowVisibility_.game);
		ImGui::MenuItem("Hierarchy", nullptr, &windowVisibility_.hierarchy);
		ImGui::MenuItem("Inspector", nullptr, &windowVisibility_.inspector);
		ImGui::MenuItem("Project", nullptr, &windowVisibility_.project);
		ImGui::MenuItem("Console", nullptr, &windowVisibility_.console);
		ImGui::MenuItem("Performance", nullptr, &windowVisibility_.performance);
		ImGui::Separator();
		if (ImGui::MenuItem("Reset Layout")) {
			dockSpace_.ResetLayout();
		}
		ImGui::EndMenu();
	}
#endif // USE_IMGUI
}

void ImGuiManager::DrawToolbar() {
#ifdef USE_IMGUI
	EditorApplication* app = EditorApplication::GetInstance();

	// 縦位置を揃えるため、テキスト系はフレーム内でセンタリングする。
	ImGui::AlignTextToFramePadding();

	// --- 再生・編集コントロール。Edit=四角/Start=三角のアイコンで表す ---
	const bool isPlaying = app->IsPlaying();
	// Edit(四角): Playを止めて編集へ戻る。編集中は強調表示。
	if (DrawModeIconButton("##EditModeButton", !isPlaying, false)) {
		app->Stop();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Edit");
	}
	ImGui::SameLine();
	// Start(三角): Playを開始。再生中は強調表示。
	if (DrawModeIconButton("##StartModeButton", isPlaying, true)) {
		app->Start();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Start");
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload DLL")) {
		app->ReloadGameModule();
	}
	ImGui::SameLine();

	if (app->IsPrefabEditing()) {
		if (ImGui::Button("Save Prefab")) {
			app->SavePrefabEditMode();
		}
		ImGui::SameLine();
		if (ImGui::Button("Back")) {
			app->ClosePrefabEditMode(false);
		}
		ImGui::SameLine();
		ImGui::Text("Mode: PrefabEdit (%s)", app->GetPrefabEditPath().filename().string().c_str());
	} else if (isPlaying) {
		ImGui::TextUnformatted("Mode: Play");
	} else {
		ImGui::TextUnformatted("Mode: Edit");
	}
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
	// ImSearchコンテキストはImGuiコンテキスト破棄の前に解放する。
	ImSearch::DestroyContext();
	ImGui::DestroyContext();
#endif // USE_IMGUI
}

} // namespace KujakuEngine
