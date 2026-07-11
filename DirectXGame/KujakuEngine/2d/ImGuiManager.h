#pragma once
#include "../../externals/imgui/imgui.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"
#include "../Editor/ProjectWindow.h"
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
	enum class TransformGizmoOperation {
		Translate,
		Rotate,
		Scale,
	};

	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

	void DrawDockSpace();
	void SetupInitialLayout(ImGuiID dockspaceId);
	void DrawGameWindow();
	void DrawHierarchyWindow();
	void DrawHierarchyObject(Scene& scene, GameObject* gameObject, GameObject* selectedObject, bool& selectedObjectExists);
	void DrawInspectorWindow();
	void DrawConsoleWindow();
	void DrawProjectWindow();
	void HandleEditorShortcuts();
	void ExportCurrentSceneJson();
	void LoadGizmoIcons();
	void DrawGizmoToolbar();
	void DrawTransformGizmo(const ImVec2& imagePosition, const ImVec2& imageSize);
	void HandleGameWindowObjectSelection(const ImVec2& imagePosition, const ImVec2& imageSize);
	bool DrawGizmoModeButton(const char* id, const char* fallbackLabel, uint32_t textureIndex, TransformGizmoOperation operation, const char* tooltip);

private:
	// DockBuilderによる初期配置は1回だけ行う。毎フレーム実行するとユーザーが動かしたDock配置を上書きしてしまう。
	bool dockLayoutInitialized_ = false;
	// Consoleウィンドウに表示する簡易ログ。今回は最低限の状態確認用としてメモリ上に保持する。
	std::vector<std::string> consoleLogs_;
	ProjectWindow projectWindow_;
	TransformGizmoOperation gizmoOperation_ = TransformGizmoOperation::Translate;
	bool transformGizmoUsing_ = false;
	bool inspectorEditing_ = false;
	bool gizmoIconsLoaded_ = false;
	uint32_t gizmoTranslateIconIndex_ = 0;
	uint32_t gizmoRotateIconIndex_ = 0;
	uint32_t gizmoScaleIconIndex_ = 0;
};

} // namespace KujakuEngine
