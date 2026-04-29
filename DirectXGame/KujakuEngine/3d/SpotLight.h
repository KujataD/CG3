#pragma once

#include "../base/DirectXCommon.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include <array>

namespace KujakuEngine {

static const uint32_t kMaxSpotLight = 16;

struct SpotLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // !< ライトの色
	Vector3 position = {0.0f, 0.0f, 0.0f};    // !< ライトの位置
	float intensity = 1.0f;                   // !< 輝度
	Vector3 direction;                        // !< スポットライトの方向
	float distance;                           // !< ライトの届く最大距離
	float decay = 1.0f;                       // !< 減衰率
	float cosAngle;                           // スポットライトの余弦
	float cosFalloffStart = 1.0f;
	float padding[2];
};
//
//struct SpotLightForGPU {
//	std::array<SpotLightData, kMaxSpotLight> lights;
//	int32_t count = 0;
//};

class SpotLight {
public:
	static SpotLight* GetInstance();

	void Initialize();
	void Reset();

	void AddLight(const SpotLightData& light);
	void SetLight(uint32_t index, const SpotLightData& light);
	void SetLight(SpotLightData* light);

	ID3D12Resource* GetResource() const { return resource_.Get(); }
	const SpotLightData& GetData() const { return *map_; }

private:
	SpotLight() = default;
	~SpotLight() = default;
	SpotLight(const SpotLight&) = delete;
	SpotLight& operator=(const SpotLight&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	SpotLightData* map_ = nullptr;
};

} // namespace KujakuEngine