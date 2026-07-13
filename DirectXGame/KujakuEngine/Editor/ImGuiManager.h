#pragma once
#include "../../externals/imgui/imgui.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"
#include "EditorDockSpace.h"
#include "GameViewWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "PerformanceWindow.h"
#include "ProjectWindow.h"
#include "SceneViewWindow.h"
#include <string>

namespace KujakuEngine {

// ImGuiバックエンド（DX12/Win32）の初期化・フレーム制御・終了と、
// Editor各ウィンドウの毎フレーム描画の統括を行う。
class ImGuiManager {
public:
	static ImGuiManager* GetInstance();

	void Initialize();

	/// <summary>
	/// フレーム開始処理（更新処理の先頭で呼ぶ）
	/// </summary>
	void Begin();

	/// <summary>
	/// フレーム終了・描画処理（描画処理の最後に呼ぶ）
	/// </summary>
	void End();

	void DrawEditor();

	void AddConsoleLog(const std::string& message);

	void ClearConsoleLogs();

	void Finalize();

private:
	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

	void HandleEditorShortcuts();
	void ExportCurrentSceneJson();

private:
	EditorDockSpace dockSpace_;
	HierarchyWindow hierarchyWindow_;
	InspectorWindow inspectorWindow_;
	ProjectWindow projectWindow_;
	SceneViewWindow sceneView_;
	GameViewWindow gameView_;
	PerformanceWindow performanceWindow_;
};

} // namespace KujakuEngine
