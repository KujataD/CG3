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
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static ImGuiManager* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// フレーム開始処理（更新処理の先頭で呼ぶ）
	/// </summary>
	void Begin();

	/// <summary>
	/// フレーム終了・描画処理（描画処理の最後に呼ぶ）
	/// </summary>
	void End();

	/// <summary>
	/// Editor UIを描画
	/// </summary>
	void DrawEditor();

	/// <summary>
	/// Consoleにログを追加
	/// </summary>
	void AddConsoleLog(const std::string& message);

	/// <summary>
	/// Consoleログを消去
	/// </summary>
	void ClearConsoleLogs();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

private:
	/// <summary>
	/// TransformGizmoOperationの種類を表します。
	/// </summary>
	enum class TransformGizmoOperation {
		Translate,
		Rotate,
		Scale,
	};

	/// <summary>
	/// ImGuiManagerを実行します。
	/// </summary>
	ImGuiManager() = default;
	/// <summary>
	/// ImGuiManagerを実行します。
	/// </summary>
	~ImGuiManager() = default;
	/// <summary>
	/// ImGuiManagerを実行します。
	/// </summary>
	ImGuiManager(const ImGuiManager&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	ImGuiManager& operator=(const ImGuiManager&) = delete;

	/// <summary>
	/// DockSpaceを描画します。
	/// </summary>
	void DrawDockSpace();
	/// <summary>
	/// upInitialLayoutを設定します。
	/// </summary>
	void SetupInitialLayout(ImGuiID dockspaceId);
	/// <summary>
	/// GameWindowを描画します。
	/// </summary>
	void DrawGameWindow();
	/// <summary>
	/// HierarchyWindowを描画します。
	/// </summary>
	void DrawHierarchyWindow();
	/// <summary>
	/// HierarchyObjectを描画します。
	/// </summary>
	void DrawHierarchyObject(Scene& scene, GameObject* gameObject, GameObject* selectedObject, bool& selectedObjectExists);
	/// <summary>
	/// InspectorWindowを描画します。
	/// </summary>
	void DrawInspectorWindow();
	/// <summary>
	/// ConsoleWindowを描画します。
	/// </summary>
	void DrawConsoleWindow();
	/// <summary>
	/// ProjectWindowを描画します。
	/// </summary>
	void DrawProjectWindow();
	/// <summary>
	/// HandleEditorShortcutsを実行します。
	/// </summary>
	void HandleEditorShortcuts();
	/// <summary>
	/// ExportCurrentSceneJsonを実行します。
	/// </summary>
	void ExportCurrentSceneJson();
	/// <summary>
	/// LoadGizmoIconsを実行します。
	/// </summary>
	void LoadGizmoIcons();
	/// <summary>
	/// GizmoToolbarを描画します。
	/// </summary>
	void DrawGizmoToolbar();
	/// <summary>
	/// TransformGizmoを描画します。
	/// </summary>
	void DrawTransformGizmo(const ImVec2& imagePosition, const ImVec2& imageSize);
	/// <summary>
	/// HandleGameWindowObjectSelectionを実行します。
	/// </summary>
	void HandleGameWindowObjectSelection(const ImVec2& imagePosition, const ImVec2& imageSize);
	/// <summary>
	/// GizmoModeButtonを描画します。
	/// </summary>
	bool DrawGizmoModeButton(const char* id, const char* fallbackLabel, uint32_t textureIndex, TransformGizmoOperation operation, const char* tooltip);

private:
	// DockBuilderによる初期配置は1回だけ行う。毎フレーム実行するとユーザーが動かしたDock配置を上書きしてしまう。
	bool dockLayoutInitialized_ = false;
	// Consoleウィンドウに表示する簡易ログ。今回は最低限の状態確認用としてメモリ上に保持する。
	std::vector<std::string> consoleLogs_;
	ProjectWindow projectWindow_;
	TransformGizmoOperation gizmoOperation_ = TransformGizmoOperation::Translate;
	bool gizmoIconsLoaded_ = false;
	uint32_t gizmoTranslateIconIndex_ = 0;
	uint32_t gizmoRotateIconIndex_ = 0;
	uint32_t gizmoScaleIconIndex_ = 0;
};

} // namespace KujakuEngine
