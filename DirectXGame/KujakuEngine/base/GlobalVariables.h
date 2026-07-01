#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26819)
#endif
#include "../../externals/nlohmann/json.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "../math/Vector3.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <variant>

namespace KujakuEngine {

/// <summary>
/// グローバル変数
/// </summary>
class GlobalVariables {
public:
	/// <summary>
	/// Instanceを取得します。
	/// </summary>
	static GlobalVariables* GetInstance();

	// 構造体宣言
	// ------------------------------------------

	// 項目
	using Item = std::variant<int32_t, float, KujakuEngine::Vector3>;

	// グループ
	using Group = std::map<std::string, Item>;

	// ------------------------------------------

	// 各機能
	// ------------------------------------------

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update();

	/// <param name="groupName">グループ名</param>
	/// <summary>
	/// Groupオブジェクトを作成します。
	/// </summary>
	void CreateGroup(const std::string& groupName);

	// 値のセット(int)
	/// <summary>
	/// Valueを設定します。
	/// </summary>
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);

	// 値のセット(float)
	/// <summary>
	/// Valueを設定します。
	/// </summary>
	void SetValue(const std::string& groupName, const std::string& key, float value);

	// 値のセット(Vector3)
	/// <summary>
	/// Valueを設定します。
	/// </summary>
	void SetValue(const std::string& groupName, const std::string& key, const KujakuEngine::Vector3& value);

	// 項目の追加(int)
	/// <summary>
	/// Itemを追加します。
	/// </summary>
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);

	// 項目の追加(float)
	/// <summary>
	/// Itemを追加します。
	/// </summary>
	void AddItem(const std::string& groupName, const std::string& key, float value);

	// 項目の追加(Vector3)
	/// <summary>
	/// Itemを追加します。
	/// </summary>
	void AddItem(const std::string& groupName, const std::string& key, const KujakuEngine::Vector3& value);

	/// <summary>
	/// Valueを取得します。
	/// </summary>
	template<typename T> T GetValue(const std::string& groupName, const std::string& key) const {
		assert(datas_.find(groupName) != datas_.end());
		// グループの参照を取得
		const Group& group = datas_.at(groupName);

		// 指定グループに指定のキーが存在する
		assert(group.find(key) != group.end());

		// 指定グループから指定のキーの値を登録
		return std::get<T>(group.at(key));
	}

	/// <param name="groupName">グループ</param>
	/// <summary>
	/// SaveFileを実行します。
	/// </summary>
	void SaveFile(const std::string& groupName);

	/// <summary>
	/// ディレクトリの全ファイル読み込み
	/// </summary>
	void LoadFiles();

	/// <param name="groupName">グループ</param>
	/// <summary>
	/// LoadFileを実行します。
	/// </summary>
	void LoadFile(const std::string& groupName);

private:
	// シングルトン化a
	/// <summary>
	/// GlobalVariablesを実行します。
	/// </summary>
	GlobalVariables() = default;
	/// <summary>
	/// GlobalVariablesを実行します。
	/// </summary>
	~GlobalVariables() = default;
	/// <summary>
	/// GlobalVariablesを実行します。
	/// </summary>
	GlobalVariables(const GlobalVariables&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	const GlobalVariables& operator=(const GlobalVariables&) = delete;

private:
	// 全データ
	std::map<std::string, Group> datas_;

	// グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath = "Resources/GlobalVariables/";
};

} // namespace KujakuEngine
