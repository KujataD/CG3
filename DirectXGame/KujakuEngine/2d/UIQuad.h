#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

#include "../3d/GraphicsPipeline.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector4.h"
#include "../runtime/KujakuApi.h"

namespace KujakuEngine {

/// <summary>
/// UI用の1枚のテクスチャ付き矩形。要素ごとにGPUバッファを所有する(描画時のCBV/VB上書き衝突を避ける)。
/// 頂点はピクセル座標で保持し、UIRenderer::GetOrtho()でクリップ空間へ変換する。
/// </summary>
class UIQuad {
public:
	KUJAKU_API void Initialize();

	/// <summary>ピクセル矩形(左上x,y・幅・高さ)を設定。</summary>
	KUJAKU_API void SetRect(float x, float y, float width, float height);

	/// <summary>UV範囲(左上・右下)を設定。既定は0..1。</summary>
	KUJAKU_API void SetUV(const Vector2& uvMin, const Vector2& uvMax);

	KUJAKU_API void SetColor(const Vector4& color);
	KUJAKU_API void SetTexture(uint32_t textureIndex) { textureIndex_ = textureIndex; }
	KUJAKU_API void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }

	/// <summary>trueでテクスチャのαのみをカバレッジとして使い、色はSetColorから取る(フォント用)。</summary>
	KUJAKU_API void SetAlphaAsCoverage(bool enabled);

	/// <summary>UIRenderer::Begin後に呼ぶ。kUIパイプラインで矩形を描画する。</summary>
	KUJAKU_API void Draw();

private:
	struct UIQuadMaterial {
		Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		Matrix4x4 uvTransform;
		Vector4 flags = {0.0f, 0.0f, 0.0f, 0.0f}; // x: αマスク使用(1=フォント)
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_;
	Matrix4x4* transformMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	UIQuadMaterial* materialMap_ = nullptr;

	uint32_t textureIndex_ = 0;
	BlendMode blendMode_ = BlendMode::kNormal;
	bool initialized_ = false;
};

} // namespace KujakuEngine
