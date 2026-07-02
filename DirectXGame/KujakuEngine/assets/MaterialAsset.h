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
#include <vector>

namespace KujakuEngine {

/// <summary>
/// Materialが持つTexture用途。
/// </summary>
enum class MaterialTextureSlot {
	BaseColor,
	Normal,
	Environment,
};

/// <summary>
/// Material内のTexture参照情報。
/// </summary>
struct MaterialTexture {
	MaterialTextureSlot slot = MaterialTextureSlot::BaseColor;
	std::string assetId;
	std::string path;
};

/// <summary>
/// Material Assetが保持する描画パラメータ。
/// </summary>
struct MaterialAssetData {
	std::string name = "New Material";
	Vector4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<MaterialTexture> textures;
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
	/// Materialファイル名から表示名を取得します。
	/// </summary>
	static KUJAKU_API std::string GetDisplayName(const std::filesystem::path& path);

	/// <summary>
	/// ファイル名として使えるMaterial名へ整形します。
	/// </summary>
	static KUJAKU_API std::string SanitizeName(const std::string& name);

	/// <summary>
	/// Material名を変更し、Material JSONと.metaも合わせて更新します。
	/// </summary>
	static KUJAKU_API bool Rename(const std::filesystem::path& path, const std::string& newName, std::filesystem::path& outNewPath, std::string& message);

	/// <summary>
	/// TextureSlotをJSON保存用の文字列へ変換します。
	/// </summary>
	static KUJAKU_API const char* ToString(MaterialTextureSlot slot);

	/// <summary>
	/// 文字列からTextureSlotへ変換します。
	/// </summary>
	static KUJAKU_API bool TryGetTextureSlotFromString(const std::string& slotName, MaterialTextureSlot& outSlot);

	/// <summary>
	/// 指定SlotのTexture参照を取得します。存在しない場合はnullptrを返します。
	/// </summary>
	static KUJAKU_API const MaterialTexture* GetTexture(const MaterialAssetData& material, MaterialTextureSlot slot);

	/// <summary>
	/// 指定SlotのTextureパスを取得します。
	/// </summary>
	static KUJAKU_API std::string GetTexturePath(const MaterialAssetData& material, MaterialTextureSlot slot);

	/// <summary>
	/// 指定SlotのTexture参照を設定します。
	/// </summary>
	static KUJAKU_API void SetTexture(MaterialAssetData& material, MaterialTextureSlot slot, const std::string& assetId, const std::string& path);

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
	static KUJAKU_API std::filesystem::path ResolveTexturePath(const MaterialAssetData& material, MaterialTextureSlot slot = MaterialTextureSlot::BaseColor);

	/// <summary>
	/// Materialが参照するTextureをTextureManagerへ読み込み、SRVインデックスを返します。
	/// </summary>
	static KUJAKU_API uint32_t ResolveTextureIndex(const MaterialAssetData& material, MaterialTextureSlot slot = MaterialTextureSlot::BaseColor);
};

} // namespace KujakuEngine
