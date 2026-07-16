#include "ImageComponent.h"

#include "../base/TextureManager.h"
#include "../runtime/InspectorUI.h"
#include <cstring>

namespace KujakuEngine {
namespace {

Vector4 ReadVector4(const nlohmann::json& json, const char* key, const Vector4& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	const nlohmann::json& value = json.at(key);
	if (!value.is_array() || value.size() < 4) {
		return defaultValue;
	}
	for (int i = 0; i < 4; ++i) {
		if (!value[i].is_number()) {
			return defaultValue;
		}
	}
	return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>()};
}

std::string ReadString(const nlohmann::json& json, const char* key, const std::string& defaultValue) {
	if (!json.contains(key) || !json.at(key).is_string()) {
		return defaultValue;
	}
	return json.at(key).get<std::string>();
}

} // namespace

void ImageComponent::SyncPathBuffer() {
	std::memset(pathBuffer_.data(), 0, pathBuffer_.size());
	strncpy_s(pathBuffer_.data(), pathBuffer_.size(), texturePath_.c_str(), _TRUNCATE);
}

void ImageComponent::SetTexture(const std::string& assetId, const std::string& path) {
	textureAssetId_ = assetId;
	texturePath_ = path;
	textureResolved_ = false;
	SyncPathBuffer();
}

void ImageComponent::EnsureTextureLoaded() {
	if (textureResolved_ && loadedPath_ == texturePath_) {
		return;
	}
	TextureManager* textureManager = TextureManager::GetInstance();
	uint32_t index = textureManager->GetDefaultWhiteTexture();
	if (!texturePath_.empty()) {
		uint32_t loaded = 0;
		if (textureManager->TryLoadTexture(texturePath_, loaded)) {
			index = loaded;
		}
	}
	textureIndex_ = index;
	loadedPath_ = texturePath_;
	textureResolved_ = true;
}

void ImageComponent::Prepare() {
	// テクスチャ読み込み(コマンドリスト実行を伴う)は描画パス外のここで行う。
	EnsureTextureLoaded();
}

void ImageComponent::DrawUI(const UIRect& canvasRect, float scaleFactor) {
	if (!quadInitialized_) {
		quad_.Initialize();
		quadInitialized_ = true;
		SyncPathBuffer();
	}
	// 未Prepareでも安全なようにキャッシュ済み白テクスチャへフォールバック(新規読み込みはしない)。
	if (!textureResolved_) {
		textureIndex_ = TextureManager::GetInstance()->GetDefaultWhiteTexture();
	}

	quad_.SetRect(canvasRect.x * scaleFactor, canvasRect.y * scaleFactor, canvasRect.width * scaleFactor, canvasRect.height * scaleFactor);
	quad_.SetColor(color_);
	quad_.SetTexture(textureIndex_);
	quad_.Draw();
}

void ImageComponent::DrawInspector() {
#ifdef USE_IMGUI
	InspectorUI::ColorEdit4("Color", &color_.x);
	if (InspectorUI::InputText("Texture", pathBuffer_.data(), pathBuffer_.size())) {
		texturePath_ = pathBuffer_.data();
		textureAssetId_.clear();
		textureResolved_ = false;
	}
	InspectorUI::Checkbox("Raycast Target", &raycastTarget_);
#endif // USE_IMGUI
}

void ImageComponent::WriteJson(nlohmann::json& json) const {
	json["textureAssetId"] = textureAssetId_;
	json["texturePath"] = texturePath_;
	json["color"] = {color_.x, color_.y, color_.z, color_.w};
	json["raycastTarget"] = raycastTarget_;
}

void ImageComponent::ReadJson(const nlohmann::json& json) {
	textureAssetId_ = ReadString(json, "textureAssetId", textureAssetId_);
	texturePath_ = ReadString(json, "texturePath", texturePath_);
	color_ = ReadVector4(json, "color", color_);
	if (json.contains("raycastTarget") && json.at("raycastTarget").is_boolean()) {
		raycastTarget_ = json.at("raycastTarget").get<bool>();
	}
	textureResolved_ = false;
}

void ImageComponent::OnAfterReadJson() {
	SyncPathBuffer();
	textureResolved_ = false;
}

} // namespace KujakuEngine
