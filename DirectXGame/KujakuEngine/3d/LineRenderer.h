#pragma once

#include "../math/Matrix4x4.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include "../runtime/KujakuApi.h"
#include "../shapes/ShapeUtil.h"
#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

namespace KujakuEngine {

class Camera;

/// <summary>
/// デバッグやゲーム用の一時Line描画を管理する。
/// DrawLineで登録した線はRender後にクリアされる。
/// </summary>
class LineRenderer {
public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static KUJAKU_API LineRenderer* GetInstance();

	/// <summary>
	/// 始点と終点からLineを登録します。
	/// </summary>
	static KUJAKU_API void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);

	/// <summary>
	/// SegmentからLineを登録します。
	/// </summary>
	static KUJAKU_API void DrawLine(const Segment& segment, const Vector4& color);

	/// <summary>
	/// 登録済みLineをCameraで描画し、描画後にクリアします。
	/// </summary>
	KUJAKU_API void Render(const Camera& camera);

	/// <summary>
	/// 登録済みLineを破棄します。
	/// </summary>
	KUJAKU_API void Clear();

private:
	/// <summary>
	/// LineRendererを実行します。
	/// </summary>
	LineRenderer() = default;
	/// <summary>
	/// LineRendererを実行します。
	/// </summary>
	~LineRenderer() = default;
	/// <summary>
	/// LineRendererを実行します。
	/// </summary>
	LineRenderer(const LineRenderer&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	LineRenderer& operator=(const LineRenderer&) = delete;

	struct LineVertex {
		Vector3 position;
		Vector4 color;
	};

	/// <summary>
	/// Line頂点を保持できる頂点バッファを確保します。
	/// </summary>
	void EnsureVertexCapacity(uint32_t vertexCount);

	/// <summary>
	/// WVP用定数バッファを確保します。
	/// </summary>
	void EnsureConstantBuffer();

	std::vector<LineVertex> vertices_;
	uint32_t vertexCapacity_ = 0;
	LineVertex* vertexMap_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Matrix4x4* wvpMap_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
};

/// <summary>
/// 始点と終点からLineを登録します。
/// </summary>
KUJAKU_API void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);

/// <summary>
/// SegmentからLineを登録します。
/// </summary>
KUJAKU_API void DrawLine(const Segment& segment, const Vector4& color);

} // namespace KujakuEngine
