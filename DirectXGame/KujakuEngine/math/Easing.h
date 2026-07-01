#pragma once

#include <cmath>

namespace KujakuEngine {

namespace EaseUtil {

/// <summary>
/// EaseTypeの種類を表します。
/// </summary>
enum class EaseType { Linear, InQuad, OutQuad, InOutQuad, InBack, OutBack, OutBounce };

/// <summary>
/// EaseCalcを取得します。
/// </summary>
float GetEaseCalc(float t, EaseType easeType);

/// <summary>
/// EaseLerpを実行します。
/// </summary>
inline float EaseLerp(const float start, const float end, float t, EaseType type) {
	float e = GetEaseCalc(t, type);
	return static_cast<float>(static_cast<float>(start) + static_cast<float>(end - start) * e);
}

} // namespace Easing
} // namespace KujakuEngine