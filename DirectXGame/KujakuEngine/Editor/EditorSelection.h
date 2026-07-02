#pragma once

#include "AssetDatabase.h"
#include <filesystem>

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
	/// Project Window上のAssetを選択します。
	/// </summary>
	void SetSelectedAsset(const std::filesystem::path& assetPath, AssetType assetType);

	/// <summary>
	/// 選択中GameObjectを取得
	/// </summary>
	GameObject* GetSelectedGameObject() const;

	/// <summary>
	/// 選択中Assetのパスを取得します。
	/// </summary>
	const std::filesystem::path& GetSelectedAssetPath() const;

	/// <summary>
	/// 選択中Asset種別を取得します。
	/// </summary>
	AssetType GetSelectedAssetType() const;

	/// <summary>
	/// 選択状態を解除
	/// </summary>
	void Clear();

private:
	/// <summary>
	/// EditorSelectionを実行します。
	/// </summary>
	EditorSelection() = default;
	/// <summary>
	/// EditorSelectionを実行します。
	/// </summary>
	~EditorSelection() = default;
	/// <summary>
	/// EditorSelectionを実行します。
	/// </summary>
	EditorSelection(const EditorSelection&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	EditorSelection& operator=(const EditorSelection&) = delete;

private:
	// 所有権はSceneが持つ。Scene破棄時はEditorApplication側でClearする。
	GameObject* selectedGameObject_ = nullptr;
	// Project Window上で選んだAsset。Asset自体の所有権はファイルシステムが持つ。
	std::filesystem::path selectedAssetPath_;
	AssetType selectedAssetType_ = AssetType::Unknown;
};

} // namespace KujakuEngine
