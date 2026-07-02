#pragma once

#include "../assets/MaterialAsset.h"
#include "../scene/Component.h"
#include <filesystem>
#include <memory>
#include <string>

namespace KujakuEngine {

class Camera;
class Model;

/// <summary>
/// ModelをGameObjectのTransformで描画するComponent
/// </summary>
class ModelRendererComponent : public Component {
public:
	/// <summary>
	/// PrimitiveTypeの種類を表します。
	/// </summary>
	enum class PrimitiveType {
		Custom,
		Cube,
		Sphere,
		Model,
	};

	/// <summary>
	/// ModelRendererComponentを実行します。
	/// </summary>
	ModelRendererComponent();
	/// <summary>
	/// ModelRendererComponentを実行します。
	/// </summary>
	explicit ModelRendererComponent(const Camera* camera);
	/// <summary>
	/// ModelRendererComponentを実行します。
	/// </summary>
	ModelRendererComponent(std::unique_ptr<Model> model, const Camera* camera);
	/// <summary>
	/// ModelRendererComponentを実行します。
	/// </summary>
	~ModelRendererComponent() override;

	/// <summary>
	/// TypeNameを取得します。
	/// </summary>
	const char* GetTypeName() const override { return "ModelRendererComponent"; }
	/// <summary>
	/// AllowMultipleを実行します。
	/// </summary>
	bool AllowMultiple() const override { return false; }

	/// <summary>
	/// 描画に使用するModelを設定
	/// </summary>
	void SetModel(std::unique_ptr<Model> model);

	/// <summary>
	/// 描画に使用するCameraを設定
	/// </summary>
	void SetCamera(const Camera* camera);

	/// <summary>
	/// Builtin形状とTextureを設定
	/// </summary>
	void SetPrimitive(PrimitiveType primitive, const std::string& textureFilePath);

	/// <summary>
	/// Project管理Materialを設定
	/// </summary>
	void SetMaterialAsset(const std::string& materialAssetId, const std::string& materialPath);

	/// <summary>
	/// Project管理Materialをファイルパスから設定
	/// </summary>
	void SetMaterialPath(const std::string& materialPath);

	/// <summary>
	/// ProjectWindowからドロップされたMaterial Assetを適用します。
	/// </summary>
	bool ApplyMaterialAsset(const std::string& materialPath) override;

	/// <summary>
	/// 現在のBuiltin形状を取得
	/// </summary>
	PrimitiveType GetPrimitive() const { return primitive_; }

	/// <summary>
	/// Textureファイルパスを取得
	/// </summary>
	const std::string& GetTextureFilePath() const { return textureFilePath_; }

	/// <summary>
	/// 参照中Material Asset IDを取得
	/// </summary>
	const std::string& GetMaterialAssetId() const { return materialAssetId_; }

	/// <summary>
	/// 参照中Materialファイルパスを取得
	/// </summary>
	const std::string& GetMaterialPath() const { return materialPath_; }

	/// <summary>
	/// 描画に使用するModelを取得
	/// </summary>
	const Model* GetModel() const { return model_.get(); }

	/// <summary>
	/// RayCast対象にできるModelを取得
	/// </summary>
	const Model* GetRayCastModel() const override { return model_.get(); }

	/// <summary>
	/// Model描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// Inspector表示
	/// </summary>
	void DrawInspector() override;

	/// <summary>
	/// Component情報をJSON形式で書き出す
	/// </summary>
	void WriteJson(nlohmann::json& json) const override;

	/// <summary>
	/// Component情報をJSON形式で読み込む
	/// </summary>
	void ReadJson(const nlohmann::json& json) override;

private:
	/// <summary>
	/// RebuildPrimitiveModelを実行します。
	/// </summary>
	void RebuildPrimitiveModel();
	/// <summary>
	/// 現在のMaterial設定をModelへ反映します。
	/// </summary>
	void ApplyMaterialToModel();
	/// <summary>
	/// 参照中のMaterial Assetを読み込み、失敗時はComponent内Materialを返します。
	/// </summary>
	MaterialAssetData GetActiveMaterial() const;
	/// <summary>
	/// Material Assetのパスを解決します。
	/// </summary>
	std::filesystem::path ResolveMaterialPath() const;
	/// <summary>
	/// PrimitiveNameを取得します。
	/// </summary>
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
