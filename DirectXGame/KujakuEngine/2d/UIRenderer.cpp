#include "UIRenderer.h"

#include "../base/DirectXCommon.h"
#include "../math/MathUtil.h"

namespace KujakuEngine {
namespace {
Matrix4x4 gUIOrtho = MakeIdentity();
float gTargetWidth = 0.0f;
float gTargetHeight = 0.0f;
} // namespace

void UIRenderer::Begin(float targetWidth, float targetHeight) {
	gTargetWidth = targetWidth;
	gTargetHeight = targetHeight;
	// 左上原点のスクリーン空間オルソ(y下方向)。ピクセル座標をそのまま頂点に使う。
	gUIOrtho = MakeOrthographicMatrix(0.0f, 0.0f, targetWidth, targetHeight, 0.0f, 100.0f);

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	ID3D12DescriptorHeap* descriptorHeaps[] = {dxCommon->GetSrvDescriptorHeap()};
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	D3D12_VIEWPORT viewport{};
	viewport.Width = targetWidth;
	viewport.Height = targetHeight;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(targetWidth);
	scissorRect.bottom = static_cast<LONG>(targetHeight);
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void UIRenderer::End() {}

const Matrix4x4& UIRenderer::GetOrtho() { return gUIOrtho; }

float UIRenderer::GetTargetWidth() { return gTargetWidth; }

float UIRenderer::GetTargetHeight() { return gTargetHeight; }

} // namespace KujakuEngine
