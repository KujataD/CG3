#pragma once

#include "../math/Vector3.h"
#include "../math/Vector4.h"

namespace KujakuEngine{

struct PointLight {
	Vector4 color;		// !< ライトの色
	Vector3 position;	// !< ライトの位置
	float intensity;	// !< 輝度
};

}