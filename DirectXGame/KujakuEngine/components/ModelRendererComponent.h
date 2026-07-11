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
class ModelRendererComponent : public Component, public IRaycastTarget, public IMaterialTarget {
public:
	enum class PrimitiveType {
		Custom,
		Cube,
		Sphere,
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
	/// Builtin形状とTextureを設定
	/// </summary>
	void SetPrimitive(PrimitiveType primitive, const std::string& textureFilePath);

	void SetMaterialAsset(const std::string& materialAssetId, const std::string& materialPath);

	void SetMaterialPath(const std::string& materialPath);

	/// <summary>
	/// ProjectWindowからドロップされたMaterial Assetを適用します。
	/// </summary>
	bool ApplyMaterialAsset(const std::string& materialPath) override;

	bool UsesMaterialAsset(const std::string& materialPath) const override;

	PrimitiveType GetPrimitive() const { return primitive_; }

	const std::string& GetTextureFilePath() const { return textureFilePath_; }

	const std::string& GetMaterialAssetId() const { return materialAssetId_; }

	const std::string& GetMaterialPath() const { return materialPath_; }

	const Model* GetModel() const { return model_.get(); }

	const Model* GetRayCastModel() const override { return model_.get(); }

	void Draw() override;

	void DrawInspector() override;

	void WriteJson(nlohmann::json& json) const override;

	void ReadJson(const nlohmann::json& json) override;

private:
	void RebuildPrimitiveModel();
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
	std::string textureFilePath_ = "resources/white1x1.png";
	std::string modelFolderPath_ = "player";
	std::string materialAssetId_;
	std::string materialPath_;
	MaterialAssetData material_ = MaterialAsset::CreateDefault();
};

} // namespace KujakuEngine
