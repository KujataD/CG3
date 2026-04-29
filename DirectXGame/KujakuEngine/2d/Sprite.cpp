#include "Sprite.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include <cassert>

namespace KujakuEngine {

Sprite* Sprite::Create(uint32_t index, const Vector2& position, float width, float height, const Vector4& color) {
	Sprite* sprite = new Sprite();
	sprite->position_ = position;
	sprite->size_ = {1.0f, 1.0f};

	sprite->CreateVertexBuffer(width, height);
	sprite->CreateIndexBuffer();
	sprite->CreateTransformationMatrixBuffer();
	sprite->CreateMaterialBuffer();
	sprite->textureIndex_ = index;

	// 色を反映
	sprite->materialMap_->color = color;

	return sprite;
}

void Sprite::PreDraw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	ID3D12DescriptorHeap* descriptorHeaps[] = {dxCommon->GetSrvDescriptorHeap()};
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = static_cast<float>(WinApp::kWindowWidth);
	viewport.Height = static_cast<float>(WinApp::kWindowHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = WinApp::kWindowWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kWindowHeight;
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Sprite::PostDraw() {}

void Sprite::Draw() {
	// 行列・UVを更新してGPUに転送
	UpdateUVTransform();
	UpdateMatrix();

	ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

	GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kObject3d, blendMode_);

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);

	commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

	auto handle = TextureManager::GetInstance()->GetSrvHandle(textureIndex_);
	commandList->SetGraphicsRootDescriptorTable(2, handle);

	// 描画（インデックス使用）
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::UpdateMatrix() {
	Matrix4x4 worldMatrix = Matrix4x4::MakeAffineMatrix({size_.x, size_.y, 1.0f}, {0.0f, 0.0f, rotation_}, {position_.x, position_.y, 0.0f});

	Matrix4x4 viewMatrix = Matrix4x4::MakeIdentity();
	Matrix4x4 projectionMatrix = Matrix4x4::MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight), 0.0f, 100.0f);

	*transformationMatrixMap_ = worldMatrix * viewMatrix * projectionMatrix;
}

void Sprite::UpdateUVTransform() {

	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScaleMatrix({uvScale_.x, uvScale_.y, 1.0f});
	uvTransformMatrix = uvTransformMatrix * Matrix4x4::MakeRotateZMatrix(uvRotation_);
	uvTransformMatrix = uvTransformMatrix * Matrix4x4::MakeTranslateMatrix({uvTranslate_.x, uvTranslate_.y, 0.0f});
	materialMap_->uvTransform = uvTransformMatrix;
}

void Sprite::SetVertexMap(float width, float height) {
	vertexMap_[0].position = {0.0f, height, 0.0f, 1.0f}; // 左下
	vertexMap_[0].texcoord = {0.0f, 1.0f};
	vertexMap_[0].normal = {0.0f, 0.0f, -1.0f};

	vertexMap_[1].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
	vertexMap_[1].texcoord = {0.0f, 0.0f};
	vertexMap_[1].normal = {0.0f, 0.0f, -1.0f};

	vertexMap_[2].position = {width, height, 0.0f, 1.0f}; // 右下
	vertexMap_[2].texcoord = {1.0f, 1.0f};
	vertexMap_[2].normal = {0.0f, 0.0f, -1.0f};

	vertexMap_[3].position = {width, 0.0f, 0.0f, 1.0f}; // 右上
	vertexMap_[3].texcoord = {1.0f, 0.0f};
	vertexMap_[3].normal = {0.0f, 0.0f, -1.0f};
}

void Sprite::CreateVertexBuffer(float width, float height) {
	vertexResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * 4);
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();

	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// Mapしたままにして頂点を動的に変更できるようにする
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap_));
	SetVertexMap(width, height);
}

void Sprite::CreateIndexBuffer() {
	indexResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * 4);
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();

	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexMap = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexMap));
	indexMap[0] = 0;
	indexMap[1] = 1;
	indexMap[2] = 2;
	indexMap[3] = 1;
	indexMap[4] = 3;
	indexMap[5] = 2;
	indexResource_->Unmap(0, nullptr);
}

void Sprite::CreateTransformationMatrixBuffer() {
	transformationMatrixResource_ = DirectXCommon::GetInstance()->CreateBufferResource((sizeof(Matrix4x4) + 0xFF) & ~0xFF);
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixMap_));
	// 単位行列で初期化
	*transformationMatrixMap_ = Matrix4x4::MakeIdentity();
}

void Sprite::CreateMaterialBuffer() {
	materialResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialData));
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialMap_));

	materialMap_->color = {1.0f, 1.0f, 1.0f, 1.0f};
	materialMap_->enableLighting = 0; // スプライトはライティングなし
	materialMap_->uvTransform = Matrix4x4::MakeIdentity();
}

} // namespace KujakuEngine
