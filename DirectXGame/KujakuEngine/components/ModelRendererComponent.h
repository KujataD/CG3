#pragma once

#include "../scene/Component.h"
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
	/// 現在のBuiltin形状を取得
	/// </summary>
	PrimitiveType GetPrimitive() const { return primitive_; }

	/// <summary>
	/// Textureファイルパスを取得
	/// </summary>
	const std::string& GetTextureFilePath() const { return textureFilePath_; }

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
	void RebuildPrimitiveModel();
	const char* GetPrimitiveName() const;

	std::unique_ptr<Model> model_;
	const Camera* camera_ = nullptr;
	PrimitiveType primitive_ = PrimitiveType::Custom;
	std::string textureFilePath_ = "resources/white1x1.png";
	std::string modelFolderPath_ = "player";
};

} // namespace KujakuEngine
