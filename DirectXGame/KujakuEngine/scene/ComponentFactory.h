#pragma once

#include "../runtime/KujakuApi.h"
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
	static KUJAKU_API ComponentFactory& GetInstance();

	/// <summary>
	/// Component生成関数を登録
	/// </summary>
	KUJAKU_API void Register(const std::string& typeName, CreateFunc createFunc);

	/// <summary>
	/// Component生成関数を登録
	/// </summary>
	KUJAKU_API void Register(const std::string& typeName, const std::string& moduleName, CreateFunc createFunc);

	/// <summary>
	/// Component登録を解除
	/// </summary>
	KUJAKU_API void Unregister(const std::string& typeName);

	/// <summary>
	/// 指定Module由来のComponent登録を解除
	/// </summary>
	KUJAKU_API void UnregisterByModule(const std::string& moduleName);

	/// <summary>
	/// 型名からComponentを生成
	/// </summary>
	KUJAKU_API std::unique_ptr<Component> Create(const std::string& typeName) const;

	/// <summary>
	/// 登録済みComponent名一覧を取得
	/// </summary>
	KUJAKU_API const std::vector<std::string>& GetRegisteredTypeNames() const;
private:
	ComponentFactory() = default;
	~ComponentFactory() = default;
	ComponentFactory(const ComponentFactory&) = delete;
	ComponentFactory& operator=(const ComponentFactory&) = delete;

private:
	/// <summary>
	/// Entry構造体を表します。
	/// </summary>
	struct Entry {
		CreateFunc createFunc;
		std::string moduleName;
	};

	std::unordered_map<std::string, Entry> entries_;
	std::vector<std::string> typeNames_;
};

} // namespace KujakuEngine
