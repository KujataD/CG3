#pragma once

#include <filesystem>

namespace KujakuEngine {

// Project Window上で表示するアセット種別。
// 種別を増やす場合は、ここに追加してProjectAssetClassifier::Classifyで判定する。
/// <summary>
/// ProjectItemTypeの種類を表します。
/// </summary>
enum class ProjectItemType {
	FolderEmpty,
	FolderFilled,
	PrefabFile,
	MaterialFile,
	ImageFile,
	ModelFile,
	AudioFile,
	OtherFile,
};

/// <summary>
/// ProjectItemViewInfo構造体を表します。
/// </summary>
struct ProjectItemViewInfo {
	// 表示対象の分類結果。
	ProjectItemType type = ProjectItemType::OtherFile;

	// trueの場合は固定アイコンではなく、対象ファイル自体を画像プレビューとして表示する。
	bool usePreview = false;

	// trueの場合はモデルを小さなRenderTargetへ描画してプレビューとして表示する。
	bool useModelPreview = false;

	// 固定アイコンを使う場合の画像パス。
	std::filesystem::path iconPath;
};

/// <summary>
/// プロジェクトアセットのファイル型によって分類します。
/// </summary>
class ProjectAssetClassifier {
public:
	/// <summary>
	/// ProjectAssetClassifierを実行します。
	/// </summary>
	explicit ProjectAssetClassifier(const std::filesystem::path& projectRoot);

	/// <param name="path"></param>
	/// <returns></returns>
	/// <summary>
	/// Classifyを実行します。
	/// </summary>
	ProjectItemViewInfo Classify(const std::filesystem::path& path) const;
	
	/// <param name="path"></param>
	/// <returns></returns>
	/// <summary>
	/// PrefabFileかどうかを返します。
	/// </summary>
	bool IsPrefabFile(const std::filesystem::path& path) const;

	/// <summary>
	/// MaterialFileかどうかを返します。
	/// </summary>
	bool IsMaterialFile(const std::filesystem::path& path) const;

	/// <summary>
	/// ImageFileかどうかを返します。
	/// </summary>
	bool IsImageFile(const std::filesystem::path& path) const;

	/// <summary>
	/// モデルプレビュー対象かどうかを拡張子で判定する。
	/// </summary>
	bool IsModelFile(const std::filesystem::path& path) const;
	
	/// <param name="path"></param>
	/// <returns></returns>
	/// <summary>
	/// AudioFileかどうかを返します。
	/// </summary>
	bool IsAudioFile(const std::filesystem::path& path) const;

private:

	/// <param name="path"></param>
	/// <returns></returns>
	/// <summary>
	/// FolderEmptyかどうかを返します。
	/// </summary>
	bool IsFolderEmpty(const std::filesystem::path& path) const;

	/// <param name="type"></param>
	/// <returns></returns>
	/// <summary>
	/// IconPathを取得します。
	/// </summary>
	std::filesystem::path GetIconPath(ProjectItemType type) const;

private:
	// アイコンパスをProjectDir基準で作るため保持する。
	std::filesystem::path projectRoot_;
};

} // namespace KujakuEngine
