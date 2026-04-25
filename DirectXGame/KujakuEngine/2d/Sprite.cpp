#include "Sprite.h"
#include "../3d/GraphicsPipeline.h"
#include "../base/DirectXCommon.h"
#include "../base/WinApp.h"
#include <cassert>

namespace KujakuEngine {

Sprite* Sprite::Create(const std::string& textureFilePath, const Vector2& position, float width, float height, const Vector4& color) {
	Sprite* sprite = new Sprite();
	sprite->position_ = position;
	sprite->size_ = {1.0f, 1.0f};

	sprite->CreateVertexBuffer(width, height);
	sprite->CreateIndexBuffer();
	sprite->CreateTransformationMatrixBuffer();
	sprite->CreateMaterialBuffer();
	sprite->LoadTexture(textureFilePath);

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

	GraphicsPipeline::GetInstance()->SetCommandList();
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Sprite::PostDraw() {}

void Sprite::Draw() {
	// 行列・UVを更新してGPUに転送
	UpdateUVTransform();
	UpdateMatrix();

	ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);

	commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_);

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

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = sizeof(VertexData) * 4;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource_));
	assert(SUCCEEDED(hr));

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// Mapしたままにして頂点を動的に変更できるようにする
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap_));
	SetVertexMap(width, height);
}

void Sprite::CreateIndexBuffer() {

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = sizeof(uint32_t) * 6;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexResource_));
	assert(SUCCEEDED(hr));

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
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = (sizeof(Matrix4x4) + 0xFF) & ~0xFF;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformationMatrixResource_));
	assert(SUCCEEDED(hr));

	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixMap_));
	// 単位行列で初期化
	*transformationMatrixMap_ = Matrix4x4::MakeIdentity();
}

void Sprite::CreateMaterialBuffer() {
	// main.cpp の materialResourceSprite 生成に対応

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = sizeof(MaterialData);
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
	assert(SUCCEEDED(hr));

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialMap_));

	materialMap_->color = {1.0f, 1.0f, 1.0f, 1.0f};
	materialMap_->enableLighting = 0; // スプライトはライティングなし
	materialMap_->uvTransform = Matrix4x4::MakeIdentity();
}

void Sprite::LoadTexture(const std::string& filePath) {
	// Model::LoadTexture と同じ処理
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();

	DirectX::ScratchImage image{};
	std::wstring filePathW(filePath.begin(), filePath.end());
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
	// 1x1など最小サイズの場合はミップ生成をスキップ
	if (image.GetMetadata().width > 1 && image.GetMetadata().height > 1) {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
		assert(SUCCEEDED(hr));
	} else {
		// そのままコピー
		hr = mipImages.InitializeFromImage(*image.GetImages());
		assert(SUCCEEDED(hr));
	}

	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureResource_));
	assert(SUCCEEDED(hr));

	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		hr = textureResource_->WriteToSubresource(UINT(mipLevel), nullptr, img->pixels, UINT(img->rowPitch), UINT(img->slicePitch));
		assert(SUCCEEDED(hr));
	}

	// SRV生成
	ID3D12DescriptorHeap* srvHeap = DirectXCommon::GetInstance()->GetSrvDescriptorHeap();
	const UINT descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// テクスチャの数に応じて、場所を変える
	UINT srvIndex = DirectXCommon::GetInstance()->AllocateSrvIndex();

	textureSrvHandleCPU_ = srvHeap->GetCPUDescriptorHandleForHeapStart();
	textureSrvHandleCPU_.ptr += descriptorSizeSRV * srvIndex;
	textureSrvHandleGPU_ = srvHeap->GetGPUDescriptorHandleForHeapStart();
	textureSrvHandleGPU_.ptr += descriptorSizeSRV * srvIndex;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	device->CreateShaderResourceView(textureResource_.Get(), &srvDesc, textureSrvHandleCPU_);
}

} // namespace KujakuEngine
