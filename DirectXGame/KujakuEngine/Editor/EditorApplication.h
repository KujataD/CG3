#pragma once

#include "../scene/Scene.h"
#include <memory>
#include <string>

namespace KujakuEngine {

// エディタ全体の実行状態。
// 将来Scene Cloneを入れる場合も、このモードを見てEditScene/PlaySceneを切り替える想定。
enum class EditorMode {
	Edit,
	Play,
};

/// <summary>
/// Editor全体の状態と実行制御を管理する
/// </summary>
class EditorApplication {
public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static EditorApplication* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// Editorフレーム開始処理
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// Editor UI更新とゲームUpdate制御
	/// </summary>
	void Update();

	/// <summary>
	/// ゲーム描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// Editorフレーム終了処理
	/// </summary>
	void EndFrame();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// Playを開始する
	/// </summary>
	void Start();

	/// <summary>
	/// Playを停止してEditへ戻る
	/// </summary>
	void Stop();

	/// <summary>
	/// ゲーム再生中かどうか
	/// </summary>
	bool IsPlaying() const;

	/// <summary>
	/// Editorの現在モードを取得
	/// </summary>
	EditorMode GetEditorMode() const;

	/// <summary>
	/// ゲームUpdateを流すかどうか
	/// </summary>
	bool ShouldUpdateGame() const;

	/// <summary>
	/// 現在操作対象にするSceneを設定し、所有権をEditorApplicationへ移す
	/// </summary>
	void SetCurrentScene(std::unique_ptr<Scene> scene);

	/// <summary>
	/// 現在操作対象のSceneを取得
	/// </summary>
	Scene* GetCurrentScene() const;

private:
	EditorApplication() = default;
	~EditorApplication() = default;
	EditorApplication(const EditorApplication&) = delete;
	EditorApplication& operator=(const EditorApplication&) = delete;

	void AddConsoleLog(const std::string& message);

private:
	// 起動時は必ずEdit。Startを押したときだけPlayに移行する。
	EditorMode editorMode_ = EditorMode::Edit;

	// 将来のEditScene/PlayScene差し替え口。現時点では1つのSceneを直接制御する。
	std::unique_ptr<Scene> currentScene_;
};

} // namespace KujakuEngine
