#pragma once

#include "../base/DirectXCommon.h"
#include "../runtime/KujakuApi.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include <array>

namespace KujakuEngine {

static const uint32_t kMaxSpotLight = 16;

/// <summary>
/// SpotLightData構造体を表します。
/// </summary>
struct SpotLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // !< ライトの色
	Vector3 position = {0.0f, 0.0f, 0.0f};    // !< ライトの位置
	float intensity = 0.0f;                   // !< 輝度
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

/// <summary>
/// SpotLightクラスを表します。
/// </summary>
class SpotLight {
public:
	/// <summary>
	/// Instanceを取得します。
	/// </summary>
	static KUJAKU_API SpotLight* GetInstance();

	/// <summary>
	/// 初期化します。
	/// </summary>
	void Initialize();
	/// <summary>
	/// 状態をリセットします。
	/// </summary>
	KUJAKU_API void Reset();

	/// <summary>
	/// Lightを追加します。
	/// </summary>
	void AddLight(const SpotLightData& light);
	/// <summary>
	/// Lightを設定します。
	/// </summary>
	void SetLight(uint32_t index, const SpotLightData& light);
	/// <summary>
	/// Lightを設定します。
	/// </summary>
	KUJAKU_API void SetLight(SpotLightData* light);

	/// <summary>
	/// Resourceを取得します。
	/// </summary>
	ID3D12Resource* GetResource() const { return resource_.Get(); }
	/// <summary>
	/// Dataを取得します。
	/// </summary>
	const SpotLightData& GetData() const { return *map_; }

private:
	/// <summary>
	/// SpotLightを実行します。
	/// </summary>
	SpotLight() = default;
	/// <summary>
	/// SpotLightを実行します。
	/// </summary>
	~SpotLight() = default;
	/// <summary>
	/// SpotLightを実行します。
	/// </summary>
	SpotLight(const SpotLight&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	SpotLight& operator=(const SpotLight&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	SpotLightData* map_ = nullptr;
};

} // namespace KujakuEngine
