#pragma once

#include "../math/Matrix4x4.h"
#include "../runtime/KujakuApi.h"

namespace KujakuEngine {

/// <summary>
/// スクリーン空間UI描画の共通セットアップ。
/// Begin()で対象RTの解像度に合わせたビューポート/シザー/トポロジ/ヒープを積み、
/// UI用オルソ投影行列を用意する。UIQuad::Draw()がこのオルソを参照する。
/// </summary>
class UIRenderer {
public:
	/// <summary>
	/// UI描画開始。targetWidth/Heightは描画先RT(Scene/Game)のピクセルサイズ。
	/// </summary>
	static KUJAKU_API void Begin(float targetWidth, float targetHeight);

	static KUJAKU_API void End();

	/// <summary>
	/// 現在のUIオルソ投影行列(ピクセル座標→クリップ空間、左上原点)。
	/// </summary>
	static KUJAKU_API const Matrix4x4& GetOrtho();

	static KUJAKU_API float GetTargetWidth();
	static KUJAKU_API float GetTargetHeight();
};

} // namespace KujakuEngine
