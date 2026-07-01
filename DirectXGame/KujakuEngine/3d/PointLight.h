#pragma once

#include "../base/DirectXCommon.h"
#include "../runtime/KujakuApi.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include <array>

namespace KujakuEngine {

static const uint32_t kMaxPointLight = 16;

/// <summary>
/// PointLightData構造体を表します。
/// </summary>
struct PointLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // !< ライトの色
	Vector3 position = {0.0f, 0.0f, 0.0f};    // !< ライトの位置
	float intensity = 1.0f;                   // !< 輝度
	float radius = 10.0f;                      // !< ライトの届く最大距離
	float decay = 1.0f;                       // !< 減衰率
	float padding[2];
};

/// <summary>
/// PointLightForGPU構造体を表します。
/// </summary>
struct PointLightForGPU {
	std::array<PointLightData, kMaxPointLight> lights;
	int32_t count = 0;
};

/// <summary>
/// PointLightクラスを表します。
/// </summary>
class PointLight {
public:
	/// <summary>
	/// Instanceを取得します。
	/// </summary>
	static KUJAKU_API PointLight* GetInstance();

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
	KUJAKU_API void AddLight(const PointLightData& light);
	/// <summary>
	/// Lightを設定します。
	/// </summary>
	void SetLight(uint32_t index, const PointLightData& light);

	/// <summary>
	/// Resourceを取得します。
	/// </summary>
	ID3D12Resource* GetResource() const { return resource_.Get(); }
	/// <summary>
	/// Dataを取得します。
	/// </summary>
	const PointLightForGPU& GetData() const { return *map_; }

private:
	/// <summary>
	/// PointLightを実行します。
	/// </summary>
	PointLight() = default;
	/// <summary>
	/// PointLightを実行します。
	/// </summary>
	~PointLight() = default;
	/// <summary>
	/// PointLightを実行します。
	/// </summary>
	PointLight(const PointLight&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	PointLight& operator=(const PointLight&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	PointLightForGPU* map_ = nullptr;
};

} // namespace KujakuEngine
