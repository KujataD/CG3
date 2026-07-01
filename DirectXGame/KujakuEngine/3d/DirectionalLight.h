#pragma once
#include "../runtime/KujakuApi.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include <d3d12.h>
#include <wrl.h>

namespace KujakuEngine {

/// <summary>
/// DirectionalLightData構造体を表します。
/// </summary>
struct DirectionalLightData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 direction = {0.0f, -1.0f, 0.0f};
	float intensity = 1.0f;
};

/// <summary>
/// DirectionalLightクラスを表します。
/// </summary>
class DirectionalLight {
public:
	/// <summary>
	/// Instanceを取得します。
	/// </summary>
	static KUJAKU_API DirectionalLight* GetInstance();

	/// <summary>
	/// 初期化します。
	/// </summary>
	void Initialize();
	/// <summary>
	/// 状態をリセットします。
	/// </summary>
	KUJAKU_API void Reset();
	KUJAKU_API void Update(); // ImGuiでの編集後にGPUへ反映

	// Drawから呼ぶ用
	/// <summary>
	/// Resourceを取得します。
	/// </summary>
	ID3D12Resource* GetResource() const { return lightResource_.Get(); }

	// ImGuiや外部から値を変える用
	/// <summary>
	/// Dataを取得します。
	/// </summary>
	DirectionalLightData& GetData() { return *lightMap_; }

private:
	/// <summary>
	/// DirectionalLightを実行します。
	/// </summary>
	DirectionalLight() = default;
	/// <summary>
	/// DirectionalLightを実行します。
	/// </summary>
	~DirectionalLight() = default;
	/// <summary>
	/// DirectionalLightを実行します。
	/// </summary>
	DirectionalLight(const DirectionalLight&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	DirectionalLight& operator=(const DirectionalLight&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
	DirectionalLightData* lightMap_ = nullptr;
};

} // namespace KujakuEngine
