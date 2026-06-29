#include "ModelRendererComponent.h"
#include "../3d/Camera.h"
#include "../3d/Model.h"
#include "../scene/GameObject.h"
#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif // USE_IMGUI
#include <array>
#include <cstring>
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

	if (primitiveName == "None") {
		return ModelRendererComponent::PrimitiveType::None;
	}

	return ModelRendererComponent::PrimitiveType::Custom;
}

} // namespace

ModelRendererComponent::ModelRendererComponent() = default;

ModelRendererComponent::ModelRendererComponent(const Camera* camera) : camera_(camera) {}

ModelRendererComponent::ModelRendererComponent(std::unique_ptr<Model> model, const Camera* camera) : model_(std::move(model)), camera_(camera) {}

ModelRendererComponent::~ModelRendererComponent() = default;

void ModelRendererComponent::SetModel(std::unique_ptr<Model> model) {
	model_ = std::move(model);
	primitive_ = PrimitiveType::Custom;
}

void ModelRendererComponent::SetCamera(const Camera* camera) {
	camera_ = camera;
}

void ModelRendererComponent::SetPrimitive(PrimitiveType primitive, const std::string& textureFilePath) {
	primitive_ = primitive;
	textureFilePath_ = textureFilePath;
	RebuildPrimitiveModel();
}

void ModelRendererComponent::Draw() {
	GameObject* owner = GetOwner();
	if (!owner || !model_ || !camera_) {
		return;
	}

	owner->GetTransform().UpdateMatrix(*camera_);
	model_->Draw(owner->GetTransform(), *camera_);
}

void ModelRendererComponent::DrawInspector() {
#ifdef USE_IMGUI
	int primitiveIndex = 0;
	if (primitive_ == PrimitiveType::Cube) {
		primitiveIndex = 1;
	} else if (primitive_ == PrimitiveType::Sphere) {
		primitiveIndex = 2;
	} else if (primitive_ == PrimitiveType::None) {
		primitiveIndex = 3;
	}

	const char* primitiveItems[] = {"Custom", "Cube", "Sphere", "None"};
	if (ImGui::Combo("Primitive", &primitiveIndex, primitiveItems, IM_ARRAYSIZE(primitiveItems))) {
		PrimitiveType selectedPrimitive = PrimitiveType::Custom;
		if (primitiveIndex == 1) {
			selectedPrimitive = PrimitiveType::Cube;
		} else if (primitiveIndex == 2) {
			selectedPrimitive = PrimitiveType::Sphere;
		} else if (primitiveIndex == 3) {
			selectedPrimitive = PrimitiveType::None;
		}
		SetPrimitive(selectedPrimitive, textureFilePath_);
	}

	std::array<char, 256> textureBuffer{};
	strncpy_s(textureBuffer.data(), textureBuffer.size(), textureFilePath_.c_str(), _TRUNCATE);
	if (ImGui::InputText("Texture", textureBuffer.data(), textureBuffer.size())) {
		textureFilePath_ = textureBuffer.data();
	}

	if (ImGui::Button("Apply Texture")) {
		RebuildPrimitiveModel();
	}

	if (model_) {
		ImGui::TextUnformatted("Model: Assigned");
	} else {
		ImGui::TextDisabled("Model: None");
	}

	if (camera_) {
		ImGui::TextUnformatted("Camera: Assigned");
	} else {
		ImGui::TextDisabled("Camera: None");
	}
#endif // USE_IMGUI
}

void ModelRendererComponent::WriteJson(nlohmann::json& json) const {
	json["primitive"] = GetPrimitiveName();
	json["texture"] = textureFilePath_;
	json["modelPath"] = "";
}

void ModelRendererComponent::ReadJson(const nlohmann::json& json) {
	std::string primitiveName = ReadString(json, "primitive", GetPrimitiveName());
	std::string textureFilePath = ReadString(json, "texture", textureFilePath_);
	SetPrimitive(ReadPrimitiveType(primitiveName), textureFilePath);
}

void ModelRendererComponent::RebuildPrimitiveModel() {
	if (primitive_ == PrimitiveType::Cube) {
		model_.reset(Model::CreateCube(textureFilePath_, ShaderModel::kBlingPhongReflection));
		return;
	}

	if (primitive_ == PrimitiveType::Sphere) {
		model_.reset(Model::CreateSphere(textureFilePath_, ShaderModel::kBlingPhongReflection));
		return;
	}

	if (primitive_ == PrimitiveType::None) {
		model_.reset();
	}
}

const char* ModelRendererComponent::GetPrimitiveName() const {
	if (primitive_ == PrimitiveType::Cube) {
		return "Cube";
	}

	if (primitive_ == PrimitiveType::Sphere) {
		return "Sphere";
	}

	if (primitive_ == PrimitiveType::None) {
		return "None";
	}

	return "Custom";
}

} // namespace KujakuEngine
