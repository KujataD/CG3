#pragma once

#include "../2d/UIQuad.h"
#include "../2d/UIRect.h"
#include "../math/Vector4.h"
#include "../scene/Component.h"
#include <array>
#include <cstdint>
#include <string>

namespace KujakuEngine {

/// <summary>
/// UIのSprite描画(UnityのImage相当)。RectTransformの矩形にテクスチャを色付きで描く。
/// </summary>
class ImageComponent : public Component {
public:
	const char* GetTypeName() const override { return "Image"; }
	bool AllowMultiple() const override { return false; }

	void DrawInspector() override;
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;
	void OnAfterReadJson() override;

	/// <summary>描画パス外で呼ぶ。テクスチャの読み込み(コマンドリスト実行を伴う)を行う。</summary>
	void Prepare();

	/// <summary>UICanvasRendererから呼ばれる。キャンバス単位の矩形をscaleFactorで実ピクセル化して描画。</summary>
	void DrawUI(const UIRect& canvasRect, float scaleFactor);

	bool IsRaycastTarget() const { return raycastTarget_; }

	void SetTexture(const std::string& assetId, const std::string& path);
	void SetColor(const Vector4& color) { color_ = color; }
	const Vector4& GetColor() const { return color_; }

private:
	void EnsureTextureLoaded();
	void SyncPathBuffer();

	std::string textureAssetId_;
	std::string texturePath_ = "Resources/white1x1.png";
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	bool raycastTarget_ = true;

	std::array<char, 256> pathBuffer_{};
	UIQuad quad_;
	bool quadInitialized_ = false;
	uint32_t textureIndex_ = 0;
	std::string loadedPath_;
	bool textureResolved_ = false;
};

} // namespace KujakuEngine
