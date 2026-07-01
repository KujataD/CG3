#pragma once

#include "../math/Vector3.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// Play開始時のTransformを保存し、Stop時に復元するComponent
/// </summary>
class TransformSnapshotComponent : public Component {
public:
	/// <summary>
	/// TypeNameを取得します。
	/// </summary>
	const char* GetTypeName() const override { return "TransformSnapshotComponent"; }

	/// <summary>
	/// 初期状態を保存
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// Play開始時の編集状態を保存
	/// </summary>
	void OnPlayStart() override;

	/// <summary>
	/// Play停止時に編集状態へ戻す
	/// </summary>
	void OnPlayStop() override;

	/// <summary>
	/// Component情報をJSON形式で書き出す
	/// </summary>
	void WriteJson(nlohmann::json& json) const override;

	/// <summary>
	/// Component情報をJSON形式で読み込む
	/// </summary>
	void ReadJson(const nlohmann::json& json) override;

	/// <summary>
	/// JSON読み込み後のTransformを復元用スナップショットとして保存
	/// </summary>
	void OnAfterReadJson() override;

	/// <summary>
	/// 現在のTransformを復元用スナップショットとして保存
	/// </summary>
	void Save();

private:
	/// <summary>
	/// TransformSnapshot構造体を表します。
	/// </summary>
	struct TransformSnapshot {
		Vector3 scale;
		Vector3 rotation;
		Vector3 translation;
	};

private:
	TransformSnapshot editSnapshot_{};
};

} // namespace KujakuEngine
