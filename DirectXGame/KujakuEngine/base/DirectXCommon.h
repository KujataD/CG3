#pragma once

#include <Windows.h>
#include <chrono>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "WinApp.h"
#include <math/Vector4.h>

namespace KujakuEngine {

/// <summary>
/// DirectX汎用
/// </summary>
class DirectXCommon {
public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static DirectXCommon* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="winApp">ウィンドウ管理クラスのポインタ</param>
	/// <param name="backBufferWidth">バックバッファ幅</param>
	/// <param name="backBufferHeight">バックバッファ高さ</param>
	/// <param name="enableDebugLayer">デバッグレイヤーの有効化</param>
	void Initialize(
	    WinApp* winApp, Vector4 color = {0.1f, 0.25f, 0.5f, 1.0f}, int32_t backBufferWidth = WinApp::kWindowWidth, int32_t backBufferHeight = WinApp::kWindowHeight, bool enableDebugLayer = false);

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// ウィンドウのクライアントサイズに合わせてSwapChainをリサイズする
	/// </summary>
	void ResizeBackBuffer(int32_t width, int32_t height);

	/// <summary>
	/// Gameウィンドウ用描画開始
	/// ここから後の3D/2D描画は、SwapChainではなくGame用RenderTargetへ描く
	/// </summary>
	void BeginGameRender();

	/// <summary>
	/// Gameウィンドウ用描画終了
	/// Game用RenderTargetをImGuiから読める状態へ戻し、描画先をSwapChainへ戻す
	/// </summary>
	void EndGameRender();

	/// <summary>
	/// 描画後処理
	/// </summary>
	void PostDraw();

	/// <summary>
	/// レンダーターゲットのクリア
	/// </summary>
	void ClearRenderTarget();

	/// <summary>
	/// 深度バッファのクリア
	/// </summary>
	void ClearDepthBuffer();

	/// <summary>
	/// リソース作成
	/// </summary>
	/// <param name="sizeInBytes"></param>
	/// <returns></returns>
	ID3D12Resource* CreateBufferResource(size_t sizeInBytes);

	/// <summary>
	/// CommandListにintermediateResourceをTextureResourceに転送するコマンドを積み実行完了まで待つ
	/// </summary>
	void ExecuteCommandAndWait();

	/// <summary>
	/// 初期化済みかどうか
	/// </summary>
	bool IsInitialized() const { return initialized_; }

	// SRV番号をDirectXCommonで一元管理する。
	// 通常テクスチャ、Gameウィンドウ用RenderTarget、ImGuiフォントが同じSRVヒープを共有するため。
	uint32_t AllocateSrvIndex() { return srvIndexCounter_++; }
	uint32_t AllocateRtvIndex() { return rtvIndexCounter_++; }
	uint32_t AllocateDsvIndex() { return dsvIndexCounter_++; }

	// --- get ---
	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetRtvDescriptorHeap() const { return rtvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetDsvDescriptorHeap() const { return dsvDescriptorHeap_.Get(); }
	// ImGui::Imageへ渡すGame用RenderTargetのGPU側SRVハンドル。
	D3D12_GPU_DESCRIPTOR_HANDLE GetGameRenderSrvHandle() const { return gameRenderSrvHandleGPU_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetGameRenderDsvHandle() const { return gameRenderDsvHandle_; }
	uint32_t GetDescriptorSizeRTV() const { return descriptorSizeRTV_; }
	uint32_t GetDescriptorSizeSRV() const { return descriptorSizeSRV_; }
	uint32_t GetDescriptorSizeDSV() const { return descriptorSizeDSV_; }
	int32_t GetBackBufferWidth() const { return backBufferWidth_; }
	int32_t GetBackBufferHeight() const { return backBufferHeight_; }
	int32_t GetGameRenderWidth() const { return gameRenderWidth_; }
	int32_t GetGameRenderHeight() const { return gameRenderHeight_; }
	uint32_t GetSwapChainBufferCount() const { return kSwapChainBufferCount; }
	void SetBackBufferRenderTarget();

	// --- set ---
	void SetClearColor(Vector4 color) {
		clearColor_[0] = color.x;
		clearColor_[1] = color.y;
		clearColor_[2] = color.z;
		clearColor_[3] = color.w;
	}

private:
	DirectXCommon() = default;
	~DirectXCommon() = default;
	DirectXCommon(const DirectXCommon&) = delete;
	const DirectXCommon& operator=(const DirectXCommon&) = delete;

	// --- 初期化フロー ---

	/// <summary>
	/// DXGIDeviceの初期化 (
	///  デバッグレイヤー
	/// / DXGIFactoryの生成
	/// / 使用するアダプタ（GPU）の決定
	/// / D3D12Deviceの生成
	/// / DX12のエラーチェック
	/// )
	/// </summary>
	/// <param name="enableDebugLayer">デバッグレイヤーの有効化</param>
	void InitializeDXGIDevice(bool enableDebugLayer);

	/// <summary>
	/// 各コマンドの初期化 (
	///  コマンドキュー
	/// / コマンドアロケータ
	/// / コマンドリスト
	/// )
	/// </summary>
	void InitializeCommand();

	/// <summary>
	/// ディスクリプタサイズ初期化
	/// </summary>
	void InitializeDescriptorSize();

	/// <summary>
	/// スワップチェーン生成
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	///
	/// </summary>
	void CreateFinalRenderTargets();
	void CreateSwapChainRenderTargetViews();

	void CreateDepthBuffer();
	// Gameウィンドウに表示するためのOffscreen RenderTargetを作成する。
	void CreateGameRenderTarget();
	void CreateGameDepthBuffer();
	void CreateFence();
	void WaitForGpu();
	// 現在のバックバッファに対応するRTVを取得する。
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRtvHandle() const;

private:
	static const uint32_t kSwapChainBufferCount = 3;
	static const uint32_t kGameRenderTargetRtvIndex = kSwapChainBufferCount;
	static const uint32_t kGameRenderDsvIndex = 1;
	static const uint32_t kRtvDescriptorCount = 256;
	static const uint32_t kSrvDescriptorCount = 256;
	static const uint32_t kDsvDescriptorCount = 32;
	WinApp* winApp_ = nullptr;
	bool initialized_ = false;

	// Direct3D関連
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
	// Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[kSwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

	// リソース関連
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[kSwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	// Gameウィンドウへ表示するための描画結果を保持するテクスチャ。
	Microsoft::WRL::ComPtr<ID3D12Resource> gameRenderResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> gameDepthStencilResource_;

	// ディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	// 同期関連
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	uint64_t fenceValues_[kSwapChainBufferCount] = {};
	HANDLE fenceEvent_ = nullptr;

	uint32_t descriptorSizeSRV_;
	uint32_t descriptorSizeRTV_;
	uint32_t descriptorSizeDSV_;

	uint32_t backBufferIndex_ = 0;
	int32_t backBufferWidth_ = 0;
	int32_t backBufferHeight_ = 0;
	int32_t gameRenderWidth_ = WinApp::kWindowWidth;
	int32_t gameRenderHeight_ = WinApp::kWindowHeight;
	// Game用RenderTargetへ描くためのRTV。CPUハンドルはOMSetRenderTargetsで使う。
	D3D12_CPU_DESCRIPTOR_HANDLE gameRenderRtvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE gameRenderDsvHandle_{};
	// Game用RenderTargetをSRVとして作成するためのCPUハンドル。
	D3D12_CPU_DESCRIPTOR_HANDLE gameRenderSrvHandleCPU_{};
	// ImGui::ImageでGame用RenderTargetを表示するためのGPUハンドル。
	D3D12_GPU_DESCRIPTOR_HANDLE gameRenderSrvHandleGPU_{};

	// 画面の色
	float clearColor_[4] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青っぽい色。RGBAの順

	// テクスチャのインデックス管理カウンター
	uint32_t srvIndexCounter_ = 1;
	uint32_t rtvIndexCounter_ = kGameRenderTargetRtvIndex + 1;
	uint32_t dsvIndexCounter_ = kGameRenderDsvIndex + 1;
};

} // namespace KujakuEngine
