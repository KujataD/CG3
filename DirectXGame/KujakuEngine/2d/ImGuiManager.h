#pragma once

namespace KujakuEngine {

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
	/// 終了処理
	/// </summary>
	void Finalize();

private:
	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;
};

} // namespace KujakuEngine