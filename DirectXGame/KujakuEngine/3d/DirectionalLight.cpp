#include "DirectionalLight.h"
#include "../base/DirectXCommon.h"
#include <cassert>

namespace KujakuEngine {

DirectionalLight* DirectionalLight::GetInstance() {
	static DirectionalLight instance;
	return &instance;
}

void DirectionalLight::Initialize() {
	lightResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(DirectionalLightData));

	lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightMap_));

	// デフォルト値を書き込む
	*lightMap_ = DirectionalLightData{};
}

void DirectionalLight::Update() {
	// Mapしたままなので特に何もしなくて良い
	// ImGuiで GetData() を経由して値を変えれば即反映される
}

} // namespace KujakuEngine