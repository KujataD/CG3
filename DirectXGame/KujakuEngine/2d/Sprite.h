#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "../3d/Model.h" // VertexData, MaterialData を共有

#include "../../externals/DirectXTex/DirectXTex.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

namespace KujakuEngine {

/// <summary>
/// スプライト
/// </summary>
class Sprite {
public:
	/// <summary>
	/// スプライトを生成する
	/// </summary>
	/// <param name="textureFilePath">テクスチャのファイルパス</param>
	/// <param name="position">左上の座標（スクリーン座標）</param>
	/// <param name="size">表示サイズ（デフォルトはテクスチャサイズ相当）</param>
	/// <param name="color">色（デフォルトは白・不透明）</param>
	static Sprite* Create(const std::string& textureFilePath, const Vector2& position = {0.0f, 0.0f}, float width = 360.0f, float height = 360.0f, const Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});

	/// <summary>
	/// 描画前処理
	/// </summary>
	static void PreDraw();

	/// <summary>
	/// 描画後処理
	/// </summary>
	static void PostDraw();

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
	void SetVertexMap(float width, float height);


	// --- get ---
	const Vector2& GetPosition() const { return position_; }
	const Vector2& GetSize() const { return size_; }
	float GetRotation() const { return rotation_; }

private:
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
	void CreateVertexBuffer(float width, float height);
	void CreateIndexBuffer();
	void CreateTransformationMatrixBuffer();
	void CreateMaterialBuffer();
	void LoadTexture(const std::string& filePath);

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

	// テクスチャ
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

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
};

} // namespace KujakuEngine
