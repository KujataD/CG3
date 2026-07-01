#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>

#include <3d/GraphicsPipeline.h>
#include <3d/Model.h>
#include "../runtime/KujakuApi.h"

#include "../../externals/DirectXTex/DirectXTex.h"
#include <math/MathUtil.h>

namespace KujakuEngine {

/// <summary>
/// スプライト
/// </summary>
class Sprite {

public:
	/// <param name="textureFilePath">テクスチャのファイルパス</param>
	/// <param name="position">左上の座標（スクリーン座標）</param>
	/// <param name="size">表示サイズ（デフォルトはテクスチャサイズ相当）</param>
	/// <param name="color">色（デフォルトは白・不透明）</param>
	/// <summary>
	/// オブジェクトを作成します。
	/// </summary>
	static Sprite* Create(uint32_t index, const Vector2& position = { 0.0f, 0.0f }, float width = 360.0f, float height = 360.0f, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, Vector2 anchorPoint = {0.0f, 0.0f});

	/// <summary>
	/// 描画前処理
	/// </summary>
	static KUJAKU_API void PreDraw();

	/// <summary>
	/// 描画後処理
	/// </summary>
	static KUJAKU_API void PostDraw();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// --- set ---
	void SetPosition(const Vector2& position) { position_ = position; }
	void SetSize(const Vector2& size) { size_ = size; }
	void SetColor(const Vector4& color) { materialMap_->color = color; }
	void SetRotation(float rotation) { rotation_ = rotation; }
	void SetUVTranslate(const Vector2& translate) { uvTranslate_ = translate; }
	void SetUVScale(const Vector2& scale) { uvScale_ = scale; }
	void SetUVRotation(float rotation) { uvRotation_ = rotation; }
	/// <summary>
	/// VertexMapを設定します。
	/// </summary>
	void SetVertexMap(float width, float height, Vector2 anchorPoint);


	void SetTexture(uint32_t index) { textureIndex_ = index; }
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
	
	// --- get ---
	/// <summary>
	/// Positionを取得します。
	/// </summary>
	const Vector2& GetPosition() const { return position_; }
	/// <summary>
	/// Sizeを取得します。
	/// </summary>
	const Vector2& GetSize() const { return size_; }
	/// <summary>
	/// Rotationを取得します。
	/// </summary>
	float GetRotation() const { return rotation_; }

private:
	/// <summary>
	/// Spriteを実行します。
	/// </summary>
	Sprite() = default;

	/// <summary>
	/// 行列を更新してGPUに転送する
	/// main.cpp のループ内 Sprite 行列計算に対応
	/// </summary>
	void UpdateMatrix();

	/// <summary>
	/// UVトランスフォーム行列を更新してマテリアルに書き込む
	/// main.cpp の uvTransformMatrix 計算に対応
	/// </summary>
	void UpdateUVTransform();


	// --- GPU リソース生成 ---
	/// <summary>
	/// VertexBufferオブジェクトを作成します。
	/// </summary>
	void CreateVertexBuffer(float width, float height, Vector2 anchorPoint);
	/// <summary>
	/// IndexBufferオブジェクトを作成します。
	/// </summary>
	void CreateIndexBuffer();
	/// <summary>
	/// TransformationMatrixBufferオブジェクトを作成します。
	/// </summary>
	void CreateTransformationMatrixBuffer();
	/// <summary>
	/// MaterialBufferオブジェクトを作成します。
	/// </summary>
	void CreateMaterialBuffer();

private:
	// 頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexMap_ = nullptr;

	// インデックスバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	// トランスフォーム行列定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	Matrix4x4* transformationMatrixMap_ = nullptr;

	// マテリアル定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	MaterialData* materialMap_ = nullptr;

	// トランスフォーム
	Vector2 position_ = {0.0f, 0.0f};
	Vector2 size_ = {640.0f, 360.0f};
	float rotation_ = 0.0f;

	// UVトランスフォーム
	Vector2 uvTranslate_ = {0.0f, 0.0f};
	Vector2 uvScale_ = {1.0f, 1.0f};
	float uvRotation_ = 0.0f;

	float width_ = 360.0f;
	float height_ = 360.0f;

	// srvの場所を示すIndex
	uint32_t textureIndex_;

	// ブレンドモード
	BlendMode blendMode_ = BlendMode::kNormal;
};

} // namespace KujakuEngine
