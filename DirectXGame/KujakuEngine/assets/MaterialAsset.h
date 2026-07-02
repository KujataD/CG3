#pragma once

#include "../math/Vector4.h"
#include "../runtime/KujakuApi.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26819)
#endif
#include "../../externals/nlohmann/json.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cstdint>
#include <filesystem>
#include <string>

namespace KujakuEngine {

/// <summary>
/// Material Assetが保持する描画パラメータ。
/// </summary>
struct MaterialAssetData {
	std::string name = "New Material";
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	std::string textureAssetId;
	std::string texturePath = "resources/white1x1.png";
};

/// <summary>
/// Unity風のProject管理Materialを読み書きするUtility。
/// </summary>
class MaterialAsset {
public:
	/// <summary>
	/// 標準Material設定を作成します。
	/// </summary>
	static KUJAKU_API MaterialAssetData CreateDefault();

	/// <summary>
	/// Material JSONファイルかどうかを返します。
	/// </summary>
	static KUJAKU_API bool IsMaterialFile(const std::filesystem::path& path);

	/// <summary>
	/// Material JSONからMaterial設定を読み込みます。
	/// </summary>
	static KUJAKU_API bool Load(const std::filesystem::path& path, MaterialAssetData& outMaterial, std::string& message);

	/// <summary>
	/// Material設定をMaterial JSONへ保存します。
	/// </summary>
	static KUJAKU_API bool Save(const std::filesystem::path& path, const MaterialAssetData& material, std::string& message);

	/// <summary>
	/// 標準Material JSONを新規作成します。
	/// </summary>
	static KUJAKU_API bool CreateDefaultFile(const std::filesystem::path& path, std::string& message);

	/// <summary>
	/// JSON ObjectからMaterial設定を読みます。
	/// </summary>
	static KUJAKU_API MaterialAssetData ReadJsonObject(const nlohmann::json& json, const MaterialAssetData& defaultMaterial);

	/// <summary>
	/// Material設定をJSON Objectへ書きます。
	/// </summary>
	static KUJAKU_API void WriteJsonObject(nlohmann::json& json, const MaterialAssetData& material);

	/// <summary>
	/// Materialが参照するTextureパスを解決します。
	/// </summary>
	static KUJAKU_API std::filesystem::path ResolveTexturePath(const MaterialAssetData& material);

	/// <summary>
	/// Materialが参照するTextureをTextureManagerへ読み込み、SRVインデックスを返します。
	/// </summary>
	static KUJAKU_API uint32_t ResolveTextureIndex(const MaterialAssetData& material);
};

} // namespace KujakuEngine
