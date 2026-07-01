#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

namespace KujakuEngine {

/// <summary>
/// Vec2構造体を表します。
/// </summary>
struct Vec2 {
	float x, y;
};

/// <summary>
/// Vec3構造体を表します。
/// </summary>
struct Vec3 {
	float x, y, z;
};

/// <summary>
/// Vec4構造体を表します。
/// </summary>
struct Vec4 {
	float x, y, z, w;
};

/// <summary>
/// Mat4x4構造体を表します。
/// </summary>
struct Mat4x4 {
	float m[4][4] = {};
};

using BlackboardValue = std::variant<int, float, bool, std::string, Vec2, Vec3, Vec4, Mat4x4>;

/// <summary>
/// Blackboardクラスを表します。
/// </summary>
class Blackboard {
public:
	/// <summary>
	/// Valueを設定します。
	/// </summary>
	void SetValue(const std::string& key, const BlackboardValue& value);

	/// <summary>
	/// Valueを取得します。
	/// </summary>
	template <typename T>
	T GetValue(const std::string& key) const {
		const BlackboardValue* value = TryGetRawValue(key);
		if (!value) {
			throw std::runtime_error("Key not found in blackboard: " + key);
		}

		const T* typedValue = std::get_if<T>(value);
		if (!typedValue) {
			throw std::runtime_error("Blackboard value type mismatch: " + key);
		}

		return *typedValue;
	}

	/// <summary>
	/// GetValueを試行します。
	/// </summary>
	template <typename T>
	const T* TryGetValue(const std::string& key) const {
		const BlackboardValue* value = TryGetRawValue(key);
		if (!value) {
			return nullptr;
		}

		return std::get_if<T>(value);
	}

	/// <summary>
	/// Keyを持つかどうかを返します。
	/// </summary>
	bool HasKey(const std::string& key) const;
	/// <summary>
	/// Valueを削除します。
	/// </summary>
	void RemoveValue(const std::string& key);
	/// <summary>
	/// 状態をクリアします。
	/// </summary>
	void Clear();

private:
	/// <summary>
	/// GetRawValueを試行します。
	/// </summary>
	const BlackboardValue* TryGetRawValue(const std::string& key) const;

	std::unordered_map<std::string, BlackboardValue> data_;
};

} // namespace KujakuEngine
