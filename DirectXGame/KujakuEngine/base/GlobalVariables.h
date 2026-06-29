#pragma once
#include "../../externals/nlohmann/json.hpp"
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

	void Update();

	/// <summary>
	/// グループの作成
	/// </summary>
	/// <param name="groupName">グループ名</param>
	void CreateGroup(const std::string& groupName);

	// 値のセット(int)
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);

	// 値のセット(float)
	void SetValue(const std::string& groupName, const std::string& key, float value);

	// 値のセット(Vector3)
	void SetValue(const std::string& groupName, const std::string& key, const KujakuEngine::Vector3& value);

	// 項目の追加(int)
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);

	// 項目の追加(float)
	void AddItem(const std::string& groupName, const std::string& key, float value);

	// 項目の追加(Vector3)
	void AddItem(const std::string& groupName, const std::string& key, const KujakuEngine::Vector3& value);

	template<typename T> T GetValue(const std::string& groupName, const std::string& key) const {
		assert(datas_.find(groupName) != datas_.end());
		// グループの参照を取得
		const Group& group = datas_.at(groupName);

		// 指定グループに指定のキーが存在する
		assert(group.find(key) != group.end());

		// 指定グループから指定のキーの値を登録
		return std::get<T>(group.at(key));
	}

	/// <summary>
	/// ファイルに書き出し
	/// </summary>
	/// <param name="groupName">グループ</param>
	void SaveFile(const std::string& groupName);

	/// <summary>
	/// ディレクトリの全ファイル読み込み
	/// </summary>
	void LoadFiles();

	/// <summary>
	/// ファイルから読み込む
	/// </summary>
	/// <param name="groupName">グループ</param>
	void LoadFile(const std::string& groupName);

private:
	// シングルトン化a
	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(const GlobalVariables&) = delete;
	const GlobalVariables& operator=(const GlobalVariables&) = delete;

private:
	// 全データ
	std::map<std::string, Group> datas_;

	// グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath = "Resources/GlobalVariables/";
};

} // namespace KujakuEngine