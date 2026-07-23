#pragma once

#include <cstdint>
#include <d3d12.h>
#include <filesystem>
#include <wrl.h>

#include "../base/DirectXCommon.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include "../runtime/KujakuApi.h"
#include "PostEffectPipeline.h"

namespace KujakuEngine {

// ポストプロセス全体の調整パラメータ。
// RenderingWindowから編集し、ProjectSettings/RenderSettings.jsonへ保存する。
struct PostProcessSettings {
	// --- ブルーム(全体設定) ---
	// 閾値/soft kneeはマテリアル別設定(MaterialAssetData)に移行した。ここは全体のON/OFFと強度のみ。
	bool bloomEnabled = true;
	float bloomIntensity = 0.65f; // 合成強度(マテリアル別bloomIntensityに乗算される全体係数)
	// --- 露出/トーンマップ ---
	float exposure = 1.0f; // 露出(トーンマップ前の輝度スケール)
	int32_t tonemapper = 2; // 0=None(検証用) / 1=Reinhard / 2=ACES
	// --- 簡易カラーグレーディング/ビネット ---
	float saturation = 1.0f;
	float contrast = 1.0f;
	Vector4 colorFilter = {1.0f, 1.0f, 1.0f, 1.0f}; // rgbのみ使用
	float vignetteIntensity = 0.0f;
	float vignetteSmoothness = 0.4f;
};

/// <summary>
/// ポストプロセス実行クラス。
/// ビュー毎(Scene/Game)のブルーム中間RT・トーンマップ済みResolve RTを管理し、
/// EndSceneRender/EndGameRender直後にHDRシーンRTへブルーム+トーンマップを適用する。
/// ImGui::Imageで表示するのはここのResolve RT(LDR)になる。
/// </summary>
class PostProcess {
public:
	static KUJAKU_API PostProcess* GetInstance();

	/// <summary>
	/// 初期化。PostEffectPipeline::Initializeの後に呼ぶこと。
	/// </summary>
	void Initialize();

	/// <summary>
	/// ProjectSettings/RenderSettings.jsonから設定を読み込む(無ければデフォルトのまま)。
	/// </summary>
	void LoadSettingsFromProjectRoot(const std::filesystem::path& projectRoot);

	/// <summary>
	/// 現在の設定をProjectSettings/RenderSettings.jsonへ保存する。
	/// </summary>
	KUJAKU_API void SaveSettings() const;

	/// <summary>
	/// sourceに描かれたHDRシーンへブルーム+トーンマップを適用し、ビュー専用のResolve RT(LDR)へ出力する。
	/// EndSceneRender/EndGameRenderの直後に呼ぶこと(sourceはPIXEL_SHADER_RESOURCE状態)。
	/// 終了時に描画先はバックバッファへ戻る。
	/// </summary>
	void Render(uint32_t viewIndex, const RenderTexture& source);

	/// <summary>
	/// エディタ無しビルド用: ブルーム適用後、最終トーンマップをSwapChainバックバッファへ直接出力する。
	/// PreDraw後(バックバッファがRENDER_TARGET状態)に呼ぶこと。
	/// </summary>
	void RenderToBackBuffer(const RenderTexture& source);

	/// <summary>
	/// ImGui::Imageで表示する、ポスト適用済みRT(LDR)のSRVハンドル。
	/// まだ一度もRenderされていないビューは元のシーンRTのSRVを返す(起動直後のフォールバック)。
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetDisplaySrvHandle(uint32_t viewIndex) const;

	KUJAKU_API PostProcessSettings& GetSettings() { return settings_; }

	/// <summary>
	/// 画面フェード(シーン遷移演出用)。amount=0で無効、1で完全にfadeColorになる。
	/// </summary>
	KUJAKU_API void SetFade(const Vector3& color, float amount);
	KUJAKU_API float GetFadeAmount() const { return fadeAmount_; }

private:
	PostProcess() = default;
	~PostProcess() = default;
	PostProcess(const PostProcess&) = delete;
	PostProcess& operator=(const PostProcess&) = delete;

	// ブルームmip/Resolveなど、ポスト用の小さなRT1枚分。
	struct PostTarget {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};
		int32_t width = 0;
		int32_t height = 0;
		bool handlesAllocated = false; // ディスクリプタスロットはリサイズ時も使い回す
	};

	static const uint32_t kMaxBloomMipCount = 4; // 1/2〜1/16

	// 1ビュー(Scene/Game)分のポスト用RT一式。
	struct ViewTargets {
		PostTarget bloomMips[kMaxBloomMipCount];
		uint32_t activeMipCount = 0;
		PostTarget resolve; // トーンマップ後のLDR出力(ImGui::Imageが表示する)
		int32_t sourceWidth = 0; // 生成時のsourceサイズ(リサイズ検知用)
		int32_t sourceHeight = 0;
	};

	// sourceサイズが変わっていたら中間RT一式を作り直す(ハンドルは使い回す)。
	void EnsureTargets(ViewTargets& targets, int32_t width, int32_t height);
	// PostTargetのリソースとViewを(再)生成する。クリアしない運用なのでClearValueはnullptr。
	void RecreateTarget(PostTarget& target, int32_t width, int32_t height, DXGI_FORMAT resourceFormat, DXGI_FORMAT viewFormat);
	// ブルームチェーン(BrightPass→Down→Up)を実行し、結果をbloomMips[0]へ残す。
	void RenderBloomChain(ViewTargets& targets, const RenderTexture& source);
	// 全パス共通のルート定数を現在の設定から作る。
	PostConstants MakeConstants() const;
	// リソースステート遷移(RENDER_TARGET⇔PIXEL_SHADER_RESOURCE)。
	void TransitionTarget(PostTarget& target, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	// 指定サイズのViewport/Scissorを積む。
	void SetViewportScissor(int32_t width, int32_t height);
	// トーンマップパスを積む(出力先RTV/サイズは呼び出し側で設定済みであること)。
	void DrawTonemap(const RenderTexture& source, const ViewTargets& targets, bool bloomActive);

private:
	PostProcessSettings settings_;
	ViewTargets viewTargets_[DirectXCommon::kRenderViewCount];

	// 画面フェード(実行時状態。設定ファイルには保存しない)。
	Vector3 fadeColor_ = {0.0f, 0.0f, 0.0f};
	float fadeAmount_ = 0.0f;

	std::filesystem::path settingsPath_;
	bool hasSettingsPath_ = false;
};

} // namespace KujakuEngine
