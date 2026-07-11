#pragma once

#include "../runtime/KujakuApi.h"
#include "SerializedFieldRegistry.h"

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
#include <string>

namespace KujakuEngine {

class Camera;
class ColliderComponent;
class GameObject;
class Model;
class SerializedFieldRegistry;
struct Collision;

/// <summary>
/// GameObjectに追加するComponentの基底クラス
/// </summary>
class KUJAKU_API Component {
public:
	virtual ~Component();

	virtual void Initialize() {}
	virtual void Update() {}
	virtual void Draw() {}

	/// <summary>
	/// Inspector表示用のComponent名を取得
	/// </summary>
	virtual const char* GetTypeName() const { return "Component"; }

	virtual void DrawInspector();

	/// <summary>
	/// Component固有情報をJSONへ書き出します。
	/// </summary>
	virtual void WriteJson(nlohmann::json& json) const;

	/// <summary>
	/// Component固有情報をJSONから読み込みます。
	/// </summary>
	virtual void ReadJson(const nlohmann::json& json);

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

	/// <summary>
	/// 通常Collider同士が接触開始した時に呼ばれます。
	/// </summary>
	virtual void OnCollisionEnter(const Collision& collision) { (void)collision; }

	/// <summary>
	/// 通常Collider同士が接触中の時に呼ばれます。
	/// </summary>
	virtual void OnCollisionStay(const Collision& collision) { (void)collision; }

	/// <summary>
	/// 通常Collider同士が接触終了した時に呼ばれます。
	/// </summary>
	virtual void OnCollisionExit(const Collision& collision) { (void)collision; }

	/// <summary>
	/// Trigger Colliderとの接触開始時に呼ばれます。
	/// </summary>
	virtual void OnTriggerEnter(ColliderComponent* other) { (void)other; }

	/// <summary>
	/// Trigger Colliderとの接触中に呼ばれます。
	/// </summary>
	virtual void OnTriggerStay(ColliderComponent* other) { (void)other; }

	/// <summary>
	/// Trigger Colliderとの接触終了時に呼ばれます。
	/// </summary>
	virtual void OnTriggerExit(ColliderComponent* other) { (void)other; }

	void SetOwner(GameObject* owner) { owner_ = owner; }

	GameObject* GetOwner() const { return owner_; }

	void SetEnabled(bool enabled) { enabled_ = enabled; }

	bool IsEnabled() const { return enabled_; }

	virtual bool IsTransformComponent() const { return false; }

	virtual Camera* GetSceneCamera() { return nullptr; }

	virtual const Camera* GetSceneCamera() const { return nullptr; }

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

	/// <summary>
	/// ProjectWindowからMaterial Assetをドロップされた時に受け取れるかどうか。
	/// </summary>
	virtual bool ApplyMaterialAsset(const std::string& materialPath) {
		(void)materialPath;
		return false;
	}

	virtual bool UsesMaterialAsset(const std::string& materialPath) const {
		(void)materialPath;
		return false;
	}

protected:
	GameObject* owner_ = nullptr;
	bool enabled_ = true;
};

} // namespace KujakuEngine
