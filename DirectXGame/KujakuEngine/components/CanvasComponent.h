#pragma once

#include "../math/Vector2.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// UIルート。Screen Space - Overlay + Canvas Scaler(基準解像度・Scale With Screen Size)。
/// 配下のUI要素は「キャンバス単位」でレイアウトされ、描画時にscaleFactorで実ピクセルへ拡縮される。
/// </summary>
class CanvasComponent : public Component {
public:
	struct Layout {
		float scaleFactor = 1.0f;
		float canvasWidth = 0.0f;  // キャンバス単位での横幅
		float canvasHeight = 0.0f; // キャンバス単位での縦幅
	};

	const char* GetTypeName() const override { return "Canvas"; }
	bool AllowMultiple() const override { return false; }

	void DrawInspector() override;
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;

	/// <summary>対象RT解像度からscaleFactorとキャンバス単位のサイズを算出する。</summary>
	Layout GetLayout(float targetWidth, float targetHeight) const;

	int GetSortOrder() const { return sortOrder_; }

private:
	Vector2 referenceResolution_ = {1280.0f, 720.0f};
	float matchWidthHeight_ = 0.5f; // 0=幅基準, 1=高さ基準
	bool scaleWithScreenSize_ = true;
	int sortOrder_ = 0;
};

} // namespace KujakuEngine
