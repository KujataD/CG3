#pragma once

#include "Component.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace KujakuEngine {

/// <summary>
/// 登録済みComponentを型名から生成するFactory
/// </summary>
class ComponentFactory {
public:
	using CreateFunc = std::function<std::unique_ptr<Component>()>;

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static ComponentFactory& GetInstance();

	/// <summary>
	/// Component生成関数を登録
	/// </summary>
	void Register(const std::string& typeName, CreateFunc createFunc);

	/// <summary>
	/// 型名からComponentを生成
	/// </summary>
	std::unique_ptr<Component> Create(const std::string& typeName) const;

	/// <summary>
	/// 登録済みComponent名一覧を取得
	/// </summary>
	const std::vector<std::string>& GetRegisteredTypeNames() const;

private:
	ComponentFactory() = default;
	~ComponentFactory() = default;
	ComponentFactory(const ComponentFactory&) = delete;
	ComponentFactory& operator=(const ComponentFactory&) = delete;

private:
	std::unordered_map<std::string, CreateFunc> creators_;
	std::vector<std::string> typeNames_;
};

} // namespace KujakuEngine
