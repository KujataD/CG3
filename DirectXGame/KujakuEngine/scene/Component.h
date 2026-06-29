#pragma once

#include "../../externals/nlohmann/json.hpp"
#include <iosfwd>

namespace KujakuEngine {

class GameObject;

/// <summary>
/// GameObjectに追加するComponentの基底クラス
/// </summary>
class Component {
public:
	virtual ~Component();

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize() {}

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update() {}

	/// <summary>
	/// 描画処理
	/// </summary>
	virtual void Draw() {}

	/// <summary>
	/// Inspector表示用のComponent名を取得
	/// </summary>
	virtual const char* GetTypeName() const { return "Component"; }

	/// <summary>
	/// Inspector上のComponent固有UIを描画
	/// </summary>
	virtual void DrawInspector() {}

	/// <summary>
	/// Component固有の保存情報をJSONへ書き出す
	/// </summary>
	virtual void WriteJson(nlohmann::json& json) const { (void)json; }

	/// <summary>
	/// Component固有の保存情報をJSONから読み込む
	/// </summary>
	virtual void ReadJson(const nlohmann::json& json) { (void)json; }

	/// <summary>
	/// JSON読み込み後、他Componentの復元結果を参照して整えるための後処理
	/// </summary>
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

	/// <summary>
	/// EditからPlayへ入る直前の処理
	/// </summary>
	virtual void OnPlayStart() {}

	/// <summary>
	/// PlayからEditへ戻る時の処理
	/// </summary>
	virtual void OnPlayStop() {}

	/// <summary>
	/// 所有GameObjectを設定
	/// </summary>
	void SetOwner(GameObject* owner) { owner_ = owner; }

	/// <summary>
	/// 所有GameObjectを取得
	/// </summary>
	GameObject* GetOwner() const { return owner_; }

	/// <summary>
	/// Componentの有効状態を設定
	/// </summary>
	void SetEnabled(bool enabled) { enabled_ = enabled; }

	/// <summary>
	/// Componentが有効かどうか
	/// </summary>
	bool IsEnabled() const { return enabled_; }

protected:
	GameObject* owner_ = nullptr;
	bool enabled_ = true;
};

} // namespace KujakuEngine
