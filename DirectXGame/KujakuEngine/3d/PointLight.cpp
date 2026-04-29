#include "PointLight.h"
#include <algorithm>

namespace KujakuEngine {

PointLight* PointLight::GetInstance() {
	static PointLight instance;
	return &instance;
}

void PointLight::Initialize() {
	resource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(PointLightForGPU));
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&map_));
}

void PointLight::Reset() {
	map_->count = 0;
	// arrayの中を空のPointLightで埋める
	for (auto& light : map_->lights) {
		light = PointLightData{};
	}
}

void PointLight::AddLight(const PointLightData& light) {
	if (map_->count >= static_cast<int32_t>(kMaxPointLight)) {
		return;
	}

	map_->lights[map_->count] = light;
	map_->count++;
}

void PointLight::SetLight(uint32_t index, const PointLightData& light) {
	if (index >= kMaxPointLight) {
		return;
	}

	map_->lights[index] = light;

	if (map_->count <= static_cast<int32_t>(index)) {
		map_->count = static_cast<int32_t>(index + 1);
	}
}

} // namespace KujakuEngine