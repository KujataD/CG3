#pragma once
#include "../math/Matrix4x4.h"
#include "../shapes/ShapeUtil.h"

namespace KujakuEngine {

/// <summary>
/// AccelerationField構造体を表します。
/// </summary>
struct AccelerationField {
	Vector3 acceleration; // !< 加速度
	AABB area;            // !< 範囲
};

} // namespace KujakuEngine