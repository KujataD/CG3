#pragma once

#include "../runtime/KujakuApi.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26819)
#endif
#include "../../externals/nlohmann/json.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <iosfwd>

namespace KujakuEngine {

class Camera;
class GameObject;
class Model;

/// <summary>
/// GameObjectに追加するComponentの基底クラス
/// </summary>
class KUJAKU_API Component {
public:
	/// <summary>
	/// Componentを実行します。
	/// </summary>
	virtual ~Component();

	virtual void Initialize() {}

	virtual void Update() {}

	virtual void Draw() {}

	/// <summary>
	/// Inspector表示用のComponent名を取得
	/// </summary>
	virtual const char* GetTypeName() const { return "Component"; }

	virtual void DrawInspector() {}

	virtual void WriteJson(nlohmann::json& json) const { (void)json; }

	virtual void ReadJson(const nlohmann::json& json) { (void)json; }

	virtual void OnAfterReadJson() {}

	/// <summary>
	/// Component情報を共通形式のJSONとして書き出す
	/// </summary>
	void WriteJson(std::ostream& os, int indent) const;

	/// <summary>
	/// Editor上で削除可能かどうか
	/// </summary>
	virtual bool CanRemove() const { return true; }

	/// <summary>
	/// 同じ種類のComponentを複数追加できるかどうか
	/// </summary>
	virtual bool AllowMultiple() const { return true; }

	virtual void OnPlayStart() {}

	virtual void OnPlayStop() {}

	void SetOwner(GameObject* owner) { owner_ = owner; }

	/// <summary>
	/// 所有GameObjectを取得
	/// </summary>
	GameObject* GetOwner() const { return owner_; }

	void SetEnabled(bool enabled) { enabled_ = enabled; }

	/// <summary>
	/// Componentが有効かどうか
	/// </summary>
	bool IsEnabled() const { return enabled_; }

	/// <summary>
	/// 必須Transform Componentかどうか
	/// </summary>
	virtual bool IsTransformComponent() const { return false; }

	/// <summary>
	/// Scene表示に使えるCameraを取得
	/// </summary>
	virtual Camera* GetSceneCamera() { return nullptr; }

	/// <summary>
	/// Scene表示に使えるCameraを取得
	/// </summary>
	virtual const Camera* GetSceneCamera() const { return nullptr; }

	/// <summary>
	/// RayCast対象にできるModelを取得
	/// </summary>
	virtual const Model* GetRayCastModel() const { return nullptr; }

	/// <summary>
	/// Edit中のScene表示で、実体を持たないComponentの位置を示すアイコンを描画するかどうか。
	/// </summary>
	virtual bool HasEditorBillboard() const;

	/// <summary>
	/// Editor用ビルボードに使う画像ファイル名。KujakuEngine/resources/images から読み込む。
	/// </summary>
	virtual const char* GetEditorBillboardIconName() const;

	/// <summary>
	/// Editor用ビルボードをRayCastで選択するためのワールド半径。
	/// </summary>
	virtual float GetEditorBillboardPickRadius() const;

protected:
	GameObject* owner_ = nullptr;
	bool enabled_ = true;
};

} // namespace KujakuEngine
