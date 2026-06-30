#pragma once

#include "../runtime/GameModule.h"
#include "../runtime/GameModuleLoader.h"
#include "../scene/Scene.h"
#include <filesystem>
#include <functional>
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
	static KUJAKU_API EditorApplication* GetInstance();

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
	KUJAKU_API bool IsPlaying() const;

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
	/// Game DLLをReloadする
	/// </summary>
	bool ReloadGameModule();

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
	void DestroyCurrentScene();
	void SetCurrentSceneRaw(Scene* scene, GameModuleApi::DestroySceneFunc destroySceneFunc);
	void InitializeCurrentSceneAndImportJson();
	/// <summary>
	/// HotReload用の世代別一時ディレクトリへGameModuleをビルドする
	/// </summary>
	bool BuildGameModuleForHotReload(std::filesystem::path& outDllPath);
	/// <summary>
	/// 起動時に標準配置のGameModuleを読み込み、Componentを登録する
	/// </summary>
	bool LoadGameModuleComponentsForEditor();
	/// <summary>
	/// DLL差し替え前に現在SceneをJSONへ退避する
	/// </summary>
	bool SaveCurrentSceneJsonForHotReload();
	/// <summary>
	/// GameModule由来のComponent登録を解除し、読み込み済みDLLを解放する
	/// </summary>
	void UnregisterAndUnloadGameModule();
	std::filesystem::path GetGameModuleProjectPath() const;
	std::filesystem::path GetGameModuleDllPath() const;
	/// <summary>
	/// HotReload世代別ビルドのルートディレクトリを取得する
	/// </summary>
	std::filesystem::path GetGameModuleHotReloadBuildRoot() const;
	std::filesystem::path GetGameModuleCopyDirectory() const;
	void RestoreFallbackSceneAfterHotReloadFailure();

private:
	// 起動時は必ずEdit。Startを押したときだけPlayに移行する。
	EditorMode editorMode_ = EditorMode::Edit;

	// 将来のEditScene/PlayScene差し替え口。DLL由来SceneはDLL側DestroySceneで破棄する。
	Scene* currentScene_ = nullptr;
	GameModuleApi::DestroySceneFunc destroyCurrentSceneFunc_ = nullptr;
	GameModuleLoader gameModuleLoader_;
};

} // namespace KujakuEngine
