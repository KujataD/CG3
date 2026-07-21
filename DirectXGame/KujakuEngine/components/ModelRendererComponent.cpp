#include "ModelRendererComponent.h"
#include "../3d/Camera.h"
#include "../3d/Model.h"
#include "../runtime/AssetResolver.h"
#include "../runtime/InspectorUI.h"
#include "../scene/GameObject.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

namespace KujakuEngine {

namespace {

std::string ReadString(const nlohmann::json& json, const char* key, const std::string& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	if (!json.at(key).is_string()) {
		return defaultValue;
	}

	return json.at(key).get<std::string>();
}

ModelRendererComponent::PrimitiveType ReadPrimitiveType(const std::string& primitiveName) {
	if (primitiveName == "Cube") {
		return ModelRendererComponent::PrimitiveType::Cube;
	}

	if (primitiveName == "Sphere") {
		return ModelRendererComponent::PrimitiveType::Sphere;
	}

	if (primitiveName == "Capsule") {
		return ModelRendererComponent::PrimitiveType::Capsule;
	}

	if (primitiveName == "Plane") {
		return ModelRendererComponent::PrimitiveType::Plane;
	}

	if (primitiveName == "Model") {
		return ModelRendererComponent::PrimitiveType::Model;
	}

	return ModelRendererComponent::PrimitiveType::Custom;
}

std::string GetBaseColorTexturePath(const MaterialAssetData& material) {
	return MaterialAsset::GetTexturePath(material, MaterialTextureSlot::BaseColor);
}

void SetBaseColorTexturePath(MaterialAssetData& material, const std::string& texturePath) {
	MaterialAsset::SetTexture(material, MaterialTextureSlot::BaseColor, "", texturePath);
}

} // namespace

ModelRendererComponent::ModelRendererComponent() = default;

ModelRendererComponent::ModelRendererComponent(const Camera* camera) : camera_(camera) {}

ModelRendererComponent::ModelRendererComponent(std::unique_ptr<Model> model, const Camera* camera) : model_(std::move(model)), camera_(camera) {}

ModelRendererComponent::~ModelRendererComponent() = default;

void ModelRendererComponent::SetModel(std::unique_ptr<Model> model) {
	model_ = std::move(model);
	primitive_ = PrimitiveType::Custom;
	ApplyMaterialToModel();
}

void ModelRendererComponent::SetCamera(const Camera* camera) {
	camera_ = camera;
}

void ModelRendererComponent::SetPrimitive(PrimitiveType primitive, const std::string& textureFilePath) {
	primitive_ = primitive;
	// TextureはMaterialだけで管理する。Material Assetを参照していない場合のみ、
	// 埋め込みMaterialのBaseColorへ反映する(Unityの Instance Material 上書き相当)。
	if (!textureFilePath.empty() && materialAssetId_.empty() && materialPath_.empty()) {
		SetBaseColorTexturePath(material_, textureFilePath);
	}
	RebuildPrimitiveModel();
}

void ModelRendererComponent::SetMaterialAsset(const std::string& materialAssetId, const std::string& materialPath) {
	materialAssetId_ = materialAssetId;
	materialPath_ = materialPath;

	std::filesystem::path resolvedPath = ResolveMaterialPath();
	if (!resolvedPath.empty()) {
		std::string message;
		MaterialAssetData loadedMaterial{};
		if (MaterialAsset::Load(resolvedPath, loadedMaterial, message)) {
			material_ = loadedMaterial;
		}
	}

	ApplyMaterialToModel();
}

void ModelRendererComponent::SetMaterialPath(const std::string& materialPath) {
	if (materialPath.empty()) {
		return;
	}

	IAssetResolver& assetDatabase = GetAssetResolver();
	std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath("", materialPath);
	std::string assetId = assetDatabase.GetOrCreateAssetId(resolvedPath);
	std::string storedPath = assetDatabase.MakeProjectRelativePath(resolvedPath);
	SetMaterialAsset(assetId, storedPath);
}

bool ModelRendererComponent::ApplyMaterialAsset(const std::string& materialPath) {
	SetMaterialPath(materialPath);
	return true;
}

bool ModelRendererComponent::UsesMaterialAsset(const std::string& materialPath) const {
	if (materialAssetId_.empty() && materialPath_.empty()) {
		return false;
	}

	IAssetResolver& assetDatabase = GetAssetResolver();
	std::filesystem::path targetPath = assetDatabase.ResolveAssetPath("", materialPath);
	std::filesystem::path currentPath = ResolveMaterialPath();
	if (targetPath.empty() || currentPath.empty()) {
		return false;
	}

	return targetPath.lexically_normal() == currentPath.lexically_normal();
}

void ModelRendererComponent::Draw() {
	GameObject* owner = GetOwner();
	if (!owner || !model_ || !camera_) {
		return;
	}

	if (billboardEnabled_) {
		// 親階層に追従し、カメラを向くビルボード行列でmatWorld_を更新する。
		// scale_を直接いじらないため、毎フレーム累積してNaN化する不具合は起きない。
		owner->GetTransform().UpdateBillboardMatrix(*camera_, cameraLocalZ_, billboardFaceMode_ == 1);
	} else {
		owner->GetTransform().UpdateMatrix(*camera_);
	}

	model_->Draw(owner->GetTransform(), *camera_);
}

void ModelRendererComponent::DrawInspector() {
#ifdef USE_IMGUI
	int primitiveIndex = 0;
	switch (primitive_) {
	case KujakuEngine::ModelRendererComponent::PrimitiveType::Custom:
		break;
	case KujakuEngine::ModelRendererComponent::PrimitiveType::Cube:
		primitiveIndex = 1;
		break;
	case KujakuEngine::ModelRendererComponent::PrimitiveType::Sphere:
		primitiveIndex = 2;
		break;
	case KujakuEngine::ModelRendererComponent::PrimitiveType::Capsule:
		primitiveIndex = 3;
		break;
	case KujakuEngine::ModelRendererComponent::PrimitiveType::Plane:
		primitiveIndex = 4;
		break;
	case KujakuEngine::ModelRendererComponent::PrimitiveType::Model:
		primitiveIndex = 5;
		break;
	default:
		break;
	}

	const char* primitiveItems[] = {"Custom", "Cube", "Sphere", "Capsule", "Plane", "Model"};
	constexpr int primitiveItemCount = static_cast<int>(sizeof(primitiveItems) / sizeof(primitiveItems[0]));
	if (InspectorUI::Combo("Primitive", &primitiveIndex, primitiveItems, primitiveItemCount)) {
		PrimitiveType selectedPrimitive = PrimitiveType::Custom;
		if (primitiveIndex == 1) {
			selectedPrimitive = PrimitiveType::Cube;
		} else if (primitiveIndex == 2) {
			selectedPrimitive = PrimitiveType::Sphere;
		} else if (primitiveIndex == 3) {
			selectedPrimitive = PrimitiveType::Capsule;
		} else if (primitiveIndex == 4) {
			selectedPrimitive = PrimitiveType::Plane;
		} else if (primitiveIndex == 5) {
			selectedPrimitive = PrimitiveType::Model;
		}
		SetPrimitive(selectedPrimitive, GetBaseColorTexturePath(material_));
	}

	// Texture/ColorはMaterialだけで管理するため、この欄には出さない(Material Inspectorで編集する)。

	// ModelはProject(エクスプローラー)からModelファイルをドロップして差し替える。
	std::array<char, 256> modelDropBuffer{};
	if (InspectorUI::ModelAssetField("Model", modelFolderPath_.c_str(), modelDropBuffer.data(), modelDropBuffer.size())) {
		// CreateFromOBJは "Resources/<name>/<name>.obj" 規約なので、ファイル名(拡張子なし)をModel名として使う。
		std::string modelName = std::filesystem::path(modelDropBuffer.data()).stem().string();
		if (!modelName.empty()) {
			modelFolderPath_ = modelName;
			SetPrimitive(PrimitiveType::Model, GetBaseColorTexturePath(material_));
		}
	}

	InspectorUI::TextUnformatted("Material");
	if (materialPath_.empty()) {
		InspectorUI::TextDisabled("Embedded Material");
	} else {
		InspectorUI::TextUnformatted(materialPath_.c_str());
	}

	InspectorUI::TextUnformatted("--- Billboard ---");
	InspectorUI::Checkbox("Billboard Enabled", &billboardEnabled_);

	if (billboardEnabled_) {
		int faceModeIndex = billboardFaceMode_;
		const char* faceModeItems[] = {"Front", "Back"};
		if (InspectorUI::Combo("Face Mode", &faceModeIndex, faceModeItems, 2)) {
			billboardFaceMode_ = faceModeIndex;
		}

		// カメラの視線方向へのオフセット量。正で手前、負で奥。0で親位置のまま。
		InspectorUI::DragFloat("Camera Local Z", &cameraLocalZ_, 0.01f, -5.0f, 5.0f);
	}
#endif // USE_IMGUI
}

void ModelRendererComponent::WriteJson(nlohmann::json& json) const {
	json["primitive"] = GetPrimitiveName();
	json["modelPath"] = modelFolderPath_;
	json["materialAssetId"] = materialAssetId_;
	json["materialPath"] = materialPath_;
	json["billboardEnabled"] = billboardEnabled_;
	json["billboardFaceMode"] = billboardFaceMode_;
	json["cameraLocalZ"] = cameraLocalZ_;

	nlohmann::json materialJson;
	MaterialAsset::WriteJsonObject(materialJson, material_);
	json["material"] = materialJson;
}

void ModelRendererComponent::ReadJson(const nlohmann::json& json) {
	std::string primitiveName = ReadString(json, "primitive", GetPrimitiveName());
	modelFolderPath_ = ReadString(json, "modelPath", modelFolderPath_);
	materialAssetId_ = ReadString(json, "materialAssetId", materialAssetId_);
	materialPath_ = ReadString(json, "materialPath", materialPath_);

	if (json.contains("billboardEnabled") && json.at("billboardEnabled").is_boolean()) {
		billboardEnabled_ = json.at("billboardEnabled").get<bool>();
	}

	if (json.contains("billboardFaceMode") && json.at("billboardFaceMode").is_number_integer()) {
		billboardFaceMode_ = json.at("billboardFaceMode").get<int>();
	}

	if (json.contains("cameraLocalZ") && json.at("cameraLocalZ").is_number()) {
		cameraLocalZ_ = json.at("cameraLocalZ").get<float>();
	}

	if (json.contains("material") && json.at("material").is_object()) {
		material_ = MaterialAsset::ReadJsonObject(json.at("material"), material_);
	} else {
		// 旧シーン互換: Texture/ColorがMaterialへ移行する前の "texture" を取り込む。
		std::string texturePath = ReadString(json, "texture", GetBaseColorTexturePath(material_));
		SetBaseColorTexturePath(material_, texturePath);
	}

	SetPrimitive(ReadPrimitiveType(primitiveName), GetBaseColorTexturePath(material_));
}

void ModelRendererComponent::RebuildPrimitiveModel() {
	MaterialAssetData activeMaterial = GetActiveMaterial();
	material_ = activeMaterial;
	std::string texturePath = MaterialAsset::ResolveTexturePath(activeMaterial, MaterialTextureSlot::BaseColor).string();

	if (primitive_ == PrimitiveType::Cube) {
		model_.reset(Model::CreateCube(texturePath, ShaderModel::kBlingPhongReflection));
		ApplyMaterialToModel();
		return;
	}

	if (primitive_ == PrimitiveType::Sphere) {
		model_.reset(Model::CreateSphere(texturePath, ShaderModel::kBlingPhongReflection));
		ApplyMaterialToModel();
		return;
	}

	if (primitive_ == PrimitiveType::Capsule) {
		model_.reset(Model::CreateCapsule(texturePath, ShaderModel::kBlingPhongReflection));
		ApplyMaterialToModel();
		return;
	}

	if (primitive_ == PrimitiveType::Plane) {
		model_.reset(Model::CreatePlane(texturePath, ShaderModel::kBlingPhongReflection));
		ApplyMaterialToModel();
		return;
	}

	if (primitive_ == PrimitiveType::Model) {
		model_.reset(Model::CreateFromOBJ(modelFolderPath_, ShaderModel::kBlingPhongReflection));
		ApplyMaterialToModel();
	}
}

void ModelRendererComponent::ApplyMaterialToModel() {
	if (!model_) {
		return;
	}

	// シェーダー方式(ライティング)はMaterialの選択を常に全サブメッシュへ反映する。
	model_->SetShaderModel(static_cast<ShaderModel>(material_.shaderModel));

	// 色/テクスチャの上書き方針(方式B):
	//  - Material Assetを参照している場合は、Unityの共有Material同様に全サブメッシュを一括上書き。
	//  - 参照なし(埋め込み)でも、単一メッシュ(プリミティブや単一OBJ)は従来通りbaseColor/textureを適用。
	//  - 参照なし かつ MultiMesh のときだけ、モデル側のパーツ別マテリアル/テクスチャを尊重する。
	bool hasMaterialAsset = !materialAssetId_.empty() || !materialPath_.empty();
	bool isMultiMesh = model_->GetSubMeshCount() > 1;
	if (hasMaterialAsset || !isMultiMesh) {
		model_->SetColor(material_.baseColor);
		model_->SetTexture(MaterialAsset::ResolveTextureIndex(material_, MaterialTextureSlot::BaseColor));
	}
}

MaterialAssetData ModelRendererComponent::GetActiveMaterial() const {
	std::filesystem::path resolvedPath = ResolveMaterialPath();
	if (!resolvedPath.empty()) {
		std::string message;
		MaterialAssetData loadedMaterial{};
		if (MaterialAsset::Load(resolvedPath, loadedMaterial, message)) {
			return loadedMaterial;
		}
	}

	return material_;
}

std::filesystem::path ModelRendererComponent::ResolveMaterialPath() const {
	if (materialAssetId_.empty() && materialPath_.empty()) {
		return {};
	}

	IAssetResolver& assetDatabase = GetAssetResolver();
	return assetDatabase.ResolveAssetPath(materialAssetId_, materialPath_);
}

const char* ModelRendererComponent::GetPrimitiveName() const {
	if (primitive_ == PrimitiveType::Cube) {
		return "Cube";
	}

	if (primitive_ == PrimitiveType::Sphere) {
		return "Sphere";
	}

	if (primitive_ == PrimitiveType::Capsule) {
		return "Capsule";
	}

	if (primitive_ == PrimitiveType::Plane) {
		return "Plane";
	}

	if (primitive_ == PrimitiveType::Model) {
		return "Model";
	}

	return "Custom";
}

} // namespace KujakuEngine
