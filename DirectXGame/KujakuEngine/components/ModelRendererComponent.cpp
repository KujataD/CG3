#include "ModelRendererComponent.h"
#include "../3d/Camera.h"
#include "../3d/Model.h"
#include "../scene/GameObject.h"
#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif // USE_IMGUI
#include <array>
#include <cstring>
#include <ostream>
#include <string>
#include <utility>

namespace KujakuEngine {

namespace {

std::string EscapeJsonString(const std::string& text) {
	std::string escaped;
	escaped.reserve(text.size());

	for (char character : text) {
		if (character == '\\') {
			escaped += "\\\\";
		} else if (character == '"') {
			escaped += "\\\"";
		} else if (character == '\n') {
			escaped += "\\n";
		} else if (character == '\r') {
			escaped += "\\r";
		} else if (character == '\t') {
			escaped += "\\t";
		} else {
			escaped += character;
		}
	}

	return escaped;
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

void ModelRendererComponent::WriteJson(std::ostream& os, int indent) const {
	const std::string padding(static_cast<size_t>(indent), ' ');
	os << padding << "{\n";
	os << padding << "  \"type\": \"" << GetTypeName() << "\",\n";
	os << padding << "  \"enabled\": ";
	if (IsEnabled()) {
		os << "true,\n";
	} else {
		os << "false,\n";
	}
	os << padding << "  \"primitive\": \"" << GetPrimitiveName() << "\",\n";
	os << padding << "  \"texture\": \"" << EscapeJsonString(textureFilePath_) << "\",\n";
	os << padding << "  \"hasModel\": ";
	if (model_) {
		os << "true,\n";
	} else {
		os << "false,\n";
	}
	os << padding << "  \"hasCamera\": ";
	if (camera_) {
		os << "true\n";
	} else {
		os << "false\n";
	}
	os << padding << "}";
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
