#pragma once

#include <filesystem>

namespace KujakuEngine {

// Project Window上で表示するアセット種別。
// 種別を増やす場合は、ここに追加してProjectAssetClassifier::Classifyで判定する。
enum class ProjectItemType {
	FolderEmpty,
	FolderFilled,
	ImageFile,
	ModelFile,
	AudioFile,
	OtherFile,
};

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
	explicit ProjectAssetClassifier(const std::filesystem::path& projectRoot);

	/// <summary>
	/// パスからProject Windowでの見た目情報を決める。
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	ProjectItemViewInfo Classify(const std::filesystem::path& path) const;
	
	/// <summary>
	/// 画像プレビュー対象かどうかを拡張子で判定する。
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	bool IsImageFile(const std::filesystem::path& path) const;

	/// <summary>
	/// モデルプレビュー対象かどうかを拡張子で判定する。
	/// </summary>
	bool IsModelFile(const std::filesystem::path& path) const;
	
	/// <summary>
	/// 音楽ファイルかどうか
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	bool IsAudioFile(const std::filesystem::path& path) const;

private:

	/// <summary>
	/// 空フォルダかどうか
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	bool IsFolderEmpty(const std::filesystem::path& path) const;

	/// <summary>
	/// ファイル・フォルダのアイコン取得
	/// </summary>
	/// <param name="type"></param>
	/// <returns></returns>
	std::filesystem::path GetIconPath(ProjectItemType type) const;

private:
	// アイコンパスをProjectDir基準で作るため保持する。
	std::filesystem::path projectRoot_;
};

} // namespace KujakuEngine
