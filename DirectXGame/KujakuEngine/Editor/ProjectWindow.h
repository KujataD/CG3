#pragma once

#include "ProjectAssetClassifier.h"

#include "../3d/Camera.h"
#include "../3d/Model.h"
#include "../3d/WorldTransform.h"

#include <d3d12.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

namespace KujakuEngine {

// UnityのProject Windowに近い、ProjectDir以下のファイル閲覧用ウィンドウ。
/// <summary>
/// ProjectWindowクラスを表します。
/// </summary>
class ProjectWindow {
public:
	/// <summary>
	/// 初期化します。
	/// </summary>
	void Initialize();
	/// <summary>
	/// EnsureInitializedを実行します。
	/// </summary>
	void EnsureInitialized();
	/// <summary>
	/// 描画処理を行います。
	/// </summary>
	void Draw();
	/// <summary>
	/// RenderModelPreviewsを実行します。
	/// </summary>
	void RenderModelPreviews();
	/// <summary>
	/// 表示内容を更新します。
	/// </summary>
	void Refresh();
	/// <summary>
	/// ProjectRootを取得します。
	/// </summary>
	const std::filesystem::path& GetProjectRoot();

private:
	/// <summary>
	/// ProjectItem構造体を表します。
	/// </summary>
	struct ProjectItem {

		std::filesystem::path absolutePath; // <! 実ファイル操作に使う絶対パス。
		std::string displayName;            // <! UI表示用のファイル名。
		std::string assetId;                // <! .metaで管理されるProject内Asset ID。
		ProjectItemViewInfo viewInfo;       // <! 分類結果とアイコン/プレビュー方針。
		uint32_t textureIndex = 0;          // <! TextureManager上のSRVインデックス。
		bool isDirectory = false;
		bool hasTexture = false;			// trueならtextureIndexをImGui::Imageで表示できる。
	};

	/// <summary>
	/// ModelPreview構造体を表します。
	/// </summary>
	struct ModelPreview {
		// 読み込んだモデル本体。ProjectWindowではファイルごとに一度だけ読み込み、以後は再利用する。
		std::unique_ptr<Model> model;
		WorldTransform worldTransform;
		Camera camera;

		// ImGui::Imageで表示するため、モデルはこの小さなRenderTargetへ描画する。
		Microsoft::WRL::ComPtr<ID3D12Resource> colorResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> depthResource;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};
		uint32_t srvIndex = 0;

		bool resourcesCreated = false;
		bool loadFailed = false;
		bool requestedThisFrame = false;
	};

private:
	// 実行時カレントからProjectDirを探す。
	/// <summary>
	/// DetectProjectRootを実行します。
	/// </summary>
	std::filesystem::path DetectProjectRoot() const;
	// パス比較前に表記揺れを減らす。
	/// <summary>
	/// NormalizePathを実行します。
	/// </summary>
	std::filesystem::path NormalizePath(const std::filesystem::path& path) const;
	// ProjectDir外へ移動しないためのガード。
	/// <summary>
	/// InsideProjectRootかどうかを返します。
	/// </summary>
	bool IsInsideProjectRoot(const std::filesystem::path& path) const;
	// UIに出す相対パス文字列を作る。
	/// <summary>
	/// CurrentRelativePathTextを取得します。
	/// </summary>
	std::string GetCurrentRelativePathText() const;

	/// <summary>
	/// MoveToDirectoryを実行します。
	/// </summary>
	void MoveToDirectory(const std::filesystem::path& directory);
	/// <summary>
	/// MoveToParentを実行します。
	/// </summary>
	void MoveToParent();
	/// <summary>
	/// OpenFileInExplorerを実行します。
	/// </summary>
	void OpenFileInExplorer(const std::filesystem::path& filePath) const;
	/// <summary>
	/// Prefabを現在のSceneへ生成します。
	/// </summary>
	bool InstantiatePrefabItem(const std::filesystem::path& prefabPath);
	/// <summary>
	/// 現在のProject WindowディレクトリにMaterialを作成します。
	/// </summary>
	void CreateMaterialInCurrentDirectory();
	/// <summary>
	/// Material用の重複しないファイルパスを作成します。
	/// </summary>
	std::filesystem::path MakeUniqueMaterialPath() const;
	/// <summary>
	/// Material名変更Popupを開く準備をします。
	/// </summary>
	void BeginRenameMaterial(const ProjectItem& item);
	/// <summary>
	/// Material名変更Popupを描画します。
	/// </summary>
	void DrawRenameMaterialPopup();
	/// <summary>
	/// 入力されたMaterial名をファイル名とJSONへ反映します。
	/// </summary>
	bool CommitRenameMaterial();
	/// <summary>
	/// Toolbarを描画します。
	/// </summary>
	void DrawToolbar();
	/// <summary>
	/// Itemを描画します。
	/// </summary>
	void DrawItem(ProjectItem& item, int itemIndex);
	/// <summary>
	/// ResolveTextureを試行します。
	/// </summary>
	bool TryResolveTexture(ProjectItem& item);
	/// <summary>
	/// ResolveModelPreviewを試行します。
	/// </summary>
	bool TryResolveModelPreview(ProjectItem& item);
	/// <summary>
	/// OrCreateModelPreviewを取得します。
	/// </summary>
	ModelPreview* GetOrCreateModelPreview(const std::filesystem::path& path);
	/// <summary>
	/// ModelPreviewResourcesオブジェクトを作成します。
	/// </summary>
	bool CreateModelPreviewResources(ModelPreview& preview);
	/// <summary>
	/// upModelPreviewTransformを設定します。
	/// </summary>
	void SetupModelPreviewTransform(ModelPreview& preview);
	/// <summary>
	/// RenderModelPreviewを実行します。
	/// </summary>
	void RenderModelPreview(ModelPreview& preview);

private:
	// Project Windowで閲覧できる最上位ディレクトリ。
	std::filesystem::path projectRoot_;
	// 現在表示しているディレクトリ。
	std::filesystem::path currentDirectory_;
	std::unique_ptr<ProjectAssetClassifier> classifier_;
	// Refreshで作り直す、現在ディレクトリ直下の表示項目。
	std::vector<ProjectItem> items_;
	// 読み込み成功したアイコン/プレビューのTextureIndexキャッシュ。
	std::unordered_map<std::string, uint32_t> textureCache_;
	// 読み込み失敗した画像を毎回再試行しないためのキャッシュ。
	std::unordered_map<std::string, bool> failedTextureCache_;
	// モデルプレビューは読み込みとRenderTarget作成が重いため、ファイルパスごとに保持する。
	std::unordered_map<std::string, std::unique_ptr<ModelPreview>> modelPreviewCache_;
	// 今フレームProject Window上に見えているモデルだけをOffscreen描画する。
	std::vector<ModelPreview*> modelPreviewsToRender_;
	std::filesystem::path renameMaterialTargetPath_;
	std::array<char, 256> renameMaterialBuffer_{};
	std::string renameMaterialErrorMessage_;
	float iconSize_ = 32.0f;
	uint32_t modelPreviewSize_ = 96;
	bool initialized_ = false;
	bool requestOpenRenameMaterialPopup_ = false;
};

} // namespace KujakuEngine
