#pragma once

#include "../3d/DebugCamera.h"
#include "../math/Vector3.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// Edit中にGameObjectのCameraをDebugCamera操作で動かすComponent
/// </summary>
class DebugCameraComponent : public Component {
public:
	/// <summary>
	/// TypeNameを取得します。
	/// </summary>
	const char* GetTypeName() const override { return "DebugCameraComponent"; }
	/// <summary>
	/// AllowMultipleを実行します。
	/// </summary>
	bool AllowMultiple() const override { return false; }

	/// <summary>
	/// DebugCameraをTransformの現在値で初期化する
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// Inspector表示
	/// </summary>
	void DrawInspector() override;

	/// <summary>
	/// Edit中のDebugCamera操作をTransformとCameraComponentへ反映する
	/// </summary>
	void UpdateEditorCamera();

	/// <summary>
	/// JSON復元後にDebugCameraの基準値をTransformへ合わせる
	/// </summary>
	void OnAfterReadJson() override;

private:
	/// <summary>
	/// InitializeFromOwnerTransformを実行します。
	/// </summary>
	void InitializeFromOwnerTransform();
	/// <summary>
	/// SyncDebugCameraFromExternalTransformを実行します。
	/// </summary>
	void SyncDebugCameraFromExternalTransform();
	/// <summary>
	/// SyncOwnerTransformFromDebugCameraを実行します。
	/// </summary>
	void SyncOwnerTransformFromDebugCamera();
	/// <summary>
	/// TransformChangedExternallyを持つかどうかを返します。
	/// </summary>
	bool HasTransformChangedExternally(const Vector3& translation, const Vector3& rotation) const;

private:
	DebugCamera debugCamera_;
	Vector3 lastAppliedTranslation_ = {0.0f, 0.0f, 0.0f};
	Vector3 lastAppliedRotation_ = {0.0f, 0.0f, 0.0f};
	bool initialized_ = false;
};

} // namespace KujakuEngine
