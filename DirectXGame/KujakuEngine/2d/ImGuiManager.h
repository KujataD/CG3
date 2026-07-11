#pragma once
#include "../../externals/imgui/imgui.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"
#include "../Editor/EditorDockSpace.h"
#include "../Editor/HierarchyWindow.h"
#include "../Editor/InspectorWindow.h"
#include "../Editor/ProjectWindow.h"
#include "../Editor/SceneViewWindow.h"
#include <cstdint>
#include <string>
#include <vector>

namespace KujakuEngine {

class Camera;
class GameObject;
class Scene;

/// <summary>
/// ImGui管理クラス
/// 初期化・フレーム開始・描画・終了処理を一括管理する
/// </summary>
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

	void DrawConsoleWindow();
	void DrawProjectWindow();
	void HandleEditorShortcuts();
	void ExportCurrentSceneJson();

private:
	EditorDockSpace dockSpace_;
	HierarchyWindow hierarchyWindow_;
	InspectorWindow inspectorWindow_;
	ProjectWindow projectWindow_;
	SceneViewWindow sceneView_;
};

} // namespace KujakuEngine
