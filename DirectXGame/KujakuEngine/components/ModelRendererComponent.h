#pragma once

#include "../assets/MaterialAsset.h"
#include "../scene/Component.h"
#include "../scene/IMaterialTarget.h"
#include "../scene/IRaycastTarget.h"
#include <filesystem>
#include <memory>
#include <string>

namespace KujakuEngine {

class Camera;
class Model;

/// <summary>
/// ModelをGameObjectのTransformで描画するComponent
/// </summary>
/// <remarks>ゲームDLL(GameModule)からランタイム生成できるようKUJAKU_APIでエクスポートする。</remarks>
class KUJAKU_API ModelRendererComponent : public Component, public IRaycastTarget, public IMaterialTarget {
public:
	enum class PrimitiveType {
		Custom,
		Cube,
		Sphere,
		Capsule,
		Plane,
		Model,
	};

	ModelRendererComponent();
	explicit ModelRendererComponent(const Camera* camera);
	ModelRendererComponent(std::unique_ptr<Model> model, const Camera* camera);
	~ModelRendererComponent() override;

	const char* GetTypeName() const override { return "ModelRendererComponent"; }
	bool AllowMultiple() const override { return false; }

	void SetModel(std::unique_ptr<Model> model);

	void SetCamera(const Camera* camera);

	/// <summary>
	/// Builtin形状を設定します。textureFilePathは、Material Assetを参照していない場合のみ
	/// 埋め込みMaterialのBaseColorへ反映されます(Texture/ColorはMaterialだけで管理します)。
	/// </summary>
	void SetPrimitive(PrimitiveType primitive, const std::string& textureFilePath);

	/// <summary>
	/// 表示するModelをパス(プロジェクト相対)で設定します。存在するファイルならassetIdを補完し、
	/// 以後のリネーム/移動に追従します。旧形式の名前だけの指定("bishop")は
	/// "Resources/&lt;name&gt;/&lt;name&gt;.obj" 規約でパスへ展開されます。
	/// </summary>
	void SetModelPath(const std::string& modelPathOrName);

	const std::string& GetModelPath() const { return modelPath_; }

	void SetMaterialAsset(const std::string& materialAssetId, const std::string& materialPath);

	void SetMaterialPath(const std::string& materialPath);

	/// <summary>
	/// ProjectWindowからドロップされたMaterial Assetを適用します。
	/// </summary>
	bool ApplyMaterialAsset(const std::string& materialPath) override;

	bool UsesMaterialAsset(const std::string& materialPath) const override;

	PrimitiveType GetPrimitive() const { return primitive_; }

	const std::string& GetMaterialAssetId() const { return materialAssetId_; }

	const std::string& GetMaterialPath() const { return materialPath_; }

	const Model* GetModel() const { return model_.get(); }

	const Model* GetRayCastModel() const override { return model_.get(); }

	void Update() override;

	void Draw() override;

	/// <summary>
	/// アニメーション可能チャンネル(emissiveIntensity/emissiveColor.r,g,b)を公開する。
	/// AnimationWindowでカーブを打つと発光の明滅などが作れる。
	/// </summary>
	void CollectAnimatableChannels(std::vector<AnimatableChannel>& channels) override;

	/// <summary>
	/// ランタイム発光上書き(被弾フラッシュ等の演出用)。マテリアルアセットには影響しない。
	/// Clearするまで毎フレームこの値がモデルへ適用される。
	/// </summary>
	void SetEmissiveOverride(const Vector3& color, float intensity) {
		emissiveOverrideColor_ = color;
		emissiveOverrideIntensity_ = intensity;
		emissiveOverrideActive_ = true;
	}
	void ClearEmissiveOverride() { emissiveOverrideActive_ = false; }
	bool HasEmissiveOverride() const { return emissiveOverrideActive_; }

	void DrawInspector() override;

	void WriteJson(nlohmann::json& json) const override;

	void ReadJson(const nlohmann::json& json) override;

private:
	void RebuildPrimitiveModel();
	std::filesystem::path ResolveModelFilePath() const;
	void ApplyMaterialToModel();
	/// <summary>
	/// 参照中のMaterial Assetを読み込み、失敗時はComponent内Materialを返します。
	/// </summary>
	MaterialAssetData GetActiveMaterial() const;
	std::filesystem::path ResolveMaterialPath() const;
	const char* GetPrimitiveName() const;

	std::unique_ptr<Model> model_;
	const Camera* camera_ = nullptr;
	PrimitiveType primitive_ = PrimitiveType::Custom;
	// Modelアセットへの参照。assetId優先・path(プロジェクト相対)はfallback。
	// 旧シーン互換のため名前だけの値("player"等)も保持しうる(使用時に規約パスへ展開)。
	std::string modelAssetId_;
	std::string modelPath_ = "player";
	std::string materialAssetId_;
	std::string materialPath_;
	MaterialAssetData material_ = MaterialAsset::CreateDefault();
	bool billboardEnabled_ = false;
	int billboardFaceMode_ = 0;
	float cameraLocalZ_ = 1.0f;
	// ランタイム発光上書き(演出用の一時値。シリアライズしない)。
	Vector3 emissiveOverrideColor_ = {0.0f, 0.0f, 0.0f};
	float emissiveOverrideIntensity_ = 1.0f;
	bool emissiveOverrideActive_ = false;
};

} // namespace KujakuEngine
