#pragma once

#include <Windows.h>
#include <cassert>
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

	/// <param name="sizeInBytes"></param>
	/// <returns></returns>
	/// <summary>
	/// BufferResourceオブジェクトを作成します。
	/// </summary>
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
	/// <summary>
	/// AllocateSrvIndexを実行します。
	/// </summary>
	uint32_t AllocateSrvIndex() {
		// kSrvDescriptorCountを超えるとSRVヒープの外側へ書き込むことになり、
		// 同じヒープを共有するImGuiフォント等のメモリを破壊して無関係な箇所で落ちる。
		assert(srvIndexCounter_ < kSrvDescriptorCount && "SRV descriptor heap overflow: increase kSrvDescriptorCount.");
		return srvIndexCounter_++;
	}
	/// <summary>
	/// AllocateRtvIndexを実行します。
	/// </summary>
	uint32_t AllocateRtvIndex() { return rtvIndexCounter_++; }
	/// <summary>
	/// AllocateDsvIndexを実行します。
	/// </summary>
	uint32_t AllocateDsvIndex() { return dsvIndexCounter_++; }

	// --- get ---
	/// <summary>
	/// Deviceを取得します。
	/// </summary>
	ID3D12Device* GetDevice() const { return device_.Get(); }
	/// <summary>
	/// CommandListを取得します。
	/// </summary>
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	/// <summary>
	/// CommandQueueを取得します。
	/// </summary>
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
	/// <summary>
	/// SrvDescriptorHeapを取得します。
	/// </summary>
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	/// <summary>
	/// RtvDescriptorHeapを取得します。
	/// </summary>
	ID3D12DescriptorHeap* GetRtvDescriptorHeap() const { return rtvDescriptorHeap_.Get(); }
	/// <summary>
	/// DsvDescriptorHeapを取得します。
	/// </summary>
	ID3D12DescriptorHeap* GetDsvDescriptorHeap() const { return dsvDescriptorHeap_.Get(); }
	// ImGui::Imageへ渡すGame用RenderTargetのGPU側SRVハンドル。
	/// <summary>
	/// GameRenderSrvHandleを取得します。
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGameRenderSrvHandle() const { return gameRenderSrvHandleGPU_; }
	/// <summary>
	/// GameRenderDsvHandleを取得します。
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetGameRenderDsvHandle() const { return gameRenderDsvHandle_; }
	/// <summary>
	/// DescriptorSizeRTVを取得します。
	/// </summary>
	uint32_t GetDescriptorSizeRTV() const { return descriptorSizeRTV_; }
	/// <summary>
	/// DescriptorSizeSRVを取得します。
	/// </summary>
	uint32_t GetDescriptorSizeSRV() const { return descriptorSizeSRV_; }
	/// <summary>
	/// DescriptorSizeDSVを取得します。
	/// </summary>
	uint32_t GetDescriptorSizeDSV() const { return descriptorSizeDSV_; }
	/// <summary>
	/// BackBufferWidthを取得します。
	/// </summary>
	int32_t GetBackBufferWidth() const { return backBufferWidth_; }
	/// <summary>
	/// BackBufferHeightを取得します。
	/// </summary>
	int32_t GetBackBufferHeight() const { return backBufferHeight_; }
	/// <summary>
	/// GameRenderWidthを取得します。
	/// </summary>
	int32_t GetGameRenderWidth() const { return gameRenderWidth_; }
	/// <summary>
	/// GameRenderHeightを取得します。
	/// </summary>
	int32_t GetGameRenderHeight() const { return gameRenderHeight_; }
	/// <summary>
	/// SwapChainBufferCountを取得します。
	/// </summary>
	uint32_t GetSwapChainBufferCount() const { return kSwapChainBufferCount; }
	/// <summary>
	/// BackBufferRenderTargetを設定します。
	/// </summary>
	void SetBackBufferRenderTarget();

	// --- set ---
	/// <summary>
	/// ClearColorを設定します。
	/// </summary>
	void SetClearColor(Vector4 color) {
		clearColor_[0] = color.x;
		clearColor_[1] = color.y;
		clearColor_[2] = color.z;
		clearColor_[3] = color.w;
	}

private:
	/// <summary>
	/// DirectXCommonを実行します。
	/// </summary>
	DirectXCommon() = default;
	/// <summary>
	/// DirectXCommonを実行します。
	/// </summary>
	~DirectXCommon() = default;
	/// <summary>
	/// DirectXCommonを実行します。
	/// </summary>
	DirectXCommon(const DirectXCommon&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
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
	/// <summary>
	/// InitializeDXGIDeviceを実行します。
	/// </summary>
	void InitializeDXGIDevice(bool enableDebugLayer);

	/// <summary>
	/// 各コマンドの初期化 (
	///  コマンドキュー
	/// / コマンドアロケータ
	/// / コマンドリスト
	/// )
	/// </summary>
	/// <summary>
	/// InitializeCommandを実行します。
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
	/// <summary>
	/// SwapChainRenderTargetViewsオブジェクトを作成します。
	/// </summary>
	void CreateSwapChainRenderTargetViews();

	/// <summary>
	/// DepthBufferオブジェクトを作成します。
	/// </summary>
	void CreateDepthBuffer();
	// Gameウィンドウに表示するためのOffscreen RenderTargetを作成する。
	/// <summary>
	/// GameRenderTargetオブジェクトを作成します。
	/// </summary>
	void CreateGameRenderTarget();
	/// <summary>
	/// GameDepthBufferオブジェクトを作成します。
	/// </summary>
	void CreateGameDepthBuffer();
	/// <summary>
	/// Fenceオブジェクトを作成します。
	/// </summary>
	void CreateFence();
	/// <summary>
	/// WaitForGpuを実行します。
	/// </summary>
	void WaitForGpu();
	// 現在のバックバッファに対応するRTVを取得する。
	/// <summary>
	/// BackBufferRtvHandleを取得します。
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRtvHandle() const;

private:
	static const uint32_t kSwapChainBufferCount = 3;
	static const uint32_t kGameRenderTargetRtvIndex = kSwapChainBufferCount;
	static const uint32_t kGameRenderDsvIndex = 1;
	static const uint32_t kRtvDescriptorCount = 256;
	// 通常テクスチャ+Project Windowのプレビュー+ImGuiフォントが同じヒープを共有するため、
	// フォルダ数やアセット数が増えても余裕を持たせておく。
	static const uint32_t kSrvDescriptorCount = 4096;
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
