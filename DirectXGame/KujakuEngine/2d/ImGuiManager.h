#pragma once
#include "../../externals/imgui/imgui.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"
#include <string>
#include <vector>

namespace KujakuEngine {

// エディタ全体の実行状態。
// 将来Scene Cloneを入れる場合も、このモードを見てEditScene/PlaySceneを切り替える想定。
enum class EditorMode {
	Edit,
	Play,
};

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
	/// ゲーム再生中かどうか
	/// main側のゲームUpdateを流すか止めるかを、この戻り値で判断する
	/// </summary>
	bool IsPlaying() const { return editorMode_ == EditorMode::Play; }

	/// <summary>
	/// Editorの現在モードを取得
	/// </summary>
	EditorMode GetEditorMode() const { return editorMode_; }

	/// <summary>
	/// Consoleにログを追加
	/// </summary>
	void AddConsoleLog(const std::string& message);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

private:
	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

	void DrawDockSpace();
	void SetupInitialLayout(ImGuiID dockspaceId);
	void DrawGameWindow();
	void DrawHierarchyWindow();
	void DrawInspectorWindow();
	void DrawConsoleWindow();
	void StartGame();
	void StopGame();

private:
	// 起動時は必ずEdit。Startボタンを押したときだけPlayに移行する。
	EditorMode editorMode_ = EditorMode::Edit;
	// DockBuilderによる初期配置は1回だけ行う。毎フレーム実行するとユーザーが動かしたDock配置を上書きしてしまう。
	bool dockLayoutInitialized_ = false;
	// Consoleウィンドウに表示する簡易ログ。今回は最低限の状態確認用としてメモリ上に保持する。
	std::vector<std::string> consoleLogs_;
};

} // namespace KujakuEngine
