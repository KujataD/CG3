#pragma once

namespace KujakuEngine {

class GameObject;

/// <summary>
/// Editor内の選択状態を管理する
/// </summary>
class EditorSelection {
public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static EditorSelection* GetInstance();

	/// <summary>
	/// 選択中GameObjectを設定
	/// </summary>
	void SetSelectedGameObject(GameObject* object);

	/// <summary>
	/// 選択中GameObjectを取得
	/// </summary>
	GameObject* GetSelectedGameObject() const;

	/// <summary>
	/// 選択状態を解除
	/// </summary>
	void Clear();

private:
	EditorSelection() = default;
	~EditorSelection() = default;
	EditorSelection(const EditorSelection&) = delete;
	EditorSelection& operator=(const EditorSelection&) = delete;

private:
	// 所有権はSceneが持つ。Scene破棄時はEditorApplication側でClearする。
	GameObject* selectedGameObject_ = nullptr;
};

} // namespace KujakuEngine
