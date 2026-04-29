#include "SpotLight.h"
#include <algorithm>

namespace KujakuEngine {

SpotLight* SpotLight::GetInstance() {
	static SpotLight instance;
	return &instance;
}

void SpotLight::Initialize() {
	resource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(SpotLightData));
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&map_));
}

void SpotLight::Reset() {
	//map_->count = 0;
	//// arrayの中を空のSpotLightで埋める
	//for (auto& light : map_->lights) {
	//	light = SpotLightData{};
	//}
}

void SpotLight::AddLight(const SpotLightData& light) {
	//if (map_->count >= static_cast<int32_t>(kMaxSpotLight)) {
	//	return;
	//}

	//map_->lights[map_->count] = light;
	//map_->count++;
}

void SpotLight::SetLight(uint32_t index, const SpotLightData& light) {
	
	//if (index >= kMaxSpotLight) {
	//	return;
	//}

	//map_->lights[index] = light;

	//if (map_->count <= static_cast<int32_t>(index)) {
	//	map_->count = static_cast<int32_t>(index + 1);
	//}
}

void SpotLight::SetLight(SpotLightData* light) { *map_ = *light; }

} // namespace KujakuEngine