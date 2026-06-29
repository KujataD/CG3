#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

namespace KujakuEngine {

struct Vec2 {
	float x, y;
};

struct Vec3 {
	float x, y, z;
};

struct Vec4 {
	float x, y, z, w;
};

struct Mat4x4 {
	float m[4][4] = {};
};

using BlackboardValue = std::variant<int, float, bool, std::string, Vec2, Vec3, Vec4, Mat4x4>;

class Blackboard {
public:
	void SetValue(const std::string& key, const BlackboardValue& value);

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

	template <typename T>
	const T* TryGetValue(const std::string& key) const {
		const BlackboardValue* value = TryGetRawValue(key);
		if (!value) {
			return nullptr;
		}

		return std::get_if<T>(value);
	}

	bool HasKey(const std::string& key) const;
	void RemoveValue(const std::string& key);
	void Clear();

private:
	const BlackboardValue* TryGetRawValue(const std::string& key) const;

	std::unordered_map<std::string, BlackboardValue> data_;
};

} // namespace KujakuEngine
