#pragma once

#include "../runtime/InspectorUI.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
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

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace KujakuEngine {

/// <summary>
/// アニメーション可能なfloatチャンネル1つ分。pathはComponent内のチャンネル名
/// (例: "speed", "velocity.x")。Component種別のprefixはAnimator側が付ける。
/// </summary>
struct AnimatableChannel {
	std::string path;
	float* value = nullptr;
};

/// <summary>
/// 登録された調整項目をInspector表示、JSON保存、JSON読み込みへ流すRegistry。
/// </summary>
class SerializedFieldRegistry {
public:
	enum class Mode {
		DrawInspector,
		WriteJson,
		ReadJson,
		CollectAnimatables,
	};

	/// <summary>
	/// Inspector描画用Registryを作成します。
	/// </summary>
	SerializedFieldRegistry() : mode_(Mode::DrawInspector) {}

	/// <summary>
	/// JSON保存用Registryを作成します。
	/// </summary>
	explicit SerializedFieldRegistry(nlohmann::json& json) : mode_(Mode::WriteJson), writeJson_(&json) {}

	/// <summary>
	/// JSON読み込み用Registryを作成します。
	/// </summary>
	explicit SerializedFieldRegistry(const nlohmann::json& json) : mode_(Mode::ReadJson), readJson_(&json) {}

	/// <summary>
	/// アニメーションチャンネル収集用Registryを作成します。
	/// float/Vector3/Vector4フィールドをチャンネルとして列挙します。
	/// </summary>
	explicit SerializedFieldRegistry(std::vector<AnimatableChannel>& channels) : mode_(Mode::CollectAnimatables), channels_(&channels) {}

	void Float(const char* memberName, float& value, float dragSpeed, float minValue, float maxValue) {
		std::string key = MakeJsonKey(memberName);
		FloatNamed(key.c_str(), MakeDisplayName(key).c_str(), value, dragSpeed, minValue, maxValue);
	}

	void FloatNamed(const char* jsonKey, const char* label, float& value, float dragSpeed, float minValue, float maxValue) {
		if (mode_ == Mode::CollectAnimatables) {
			channels_->push_back({pathPrefix_ + jsonKey, &value});
			return;
		}
		if (mode_ == Mode::DrawInspector) {
			bool changed = InspectorUI::DragFloat(label, &value, dragSpeed, minValue, maxValue);
			InspectorUI::AnimationFieldHook(jsonKey, &value, 1, changed);
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = value;
			return;
		}
		if (!HasReadableKey(jsonKey)) {
			return;
		}
		if (!readJson_->at(jsonKey).is_number()) {
			return;
		}
		value = readJson_->at(jsonKey).get<float>();
		if (minValue < maxValue) {
			value = std::clamp(value, minValue, maxValue);
		}
	}

	void Int(const char* memberName, int& value, float dragSpeed, int minValue, int maxValue) {
		std::string key = MakeJsonKey(memberName);
		IntNamed(key.c_str(), MakeDisplayName(key).c_str(), value, dragSpeed, minValue, maxValue);
	}

	void IntNamed(const char* jsonKey, const char* label, int& value, float dragSpeed, int minValue, int maxValue) {
		if (mode_ == Mode::DrawInspector) {
			InspectorUI::DragInt(label, &value, dragSpeed, minValue, maxValue);
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = value;
			return;
		}
		if (!HasReadableKey(jsonKey)) {
			return;
		}
		if (!readJson_->at(jsonKey).is_number_integer()) {
			return;
		}
		value = readJson_->at(jsonKey).get<int>();
		if (minValue < maxValue) {
			value = std::clamp(value, minValue, maxValue);
		}
	}

	void UInt32(const char* memberName, uint32_t& value, float dragSpeed, uint32_t minValue, uint32_t maxValue) {
		std::string key = MakeJsonKey(memberName);
		UInt32Named(key.c_str(), MakeDisplayName(key).c_str(), value, dragSpeed, minValue, maxValue);
	}

	void UInt32Named(const char* jsonKey, const char* label, uint32_t& value, float dragSpeed, uint32_t minValue, uint32_t maxValue) {
		if (mode_ == Mode::DrawInspector) {
			int intValue = static_cast<int>(std::min<uint32_t>(value, static_cast<uint32_t>(0x7fffffff)));
			int intMin = static_cast<int>(std::min<uint32_t>(minValue, static_cast<uint32_t>(0x7fffffff)));
			int intMax = static_cast<int>(std::min<uint32_t>(maxValue, static_cast<uint32_t>(0x7fffffff)));
			if (InspectorUI::DragInt(label, &intValue, dragSpeed, intMin, intMax)) {
				if (intValue < 0) {
					value = 0;
				} else {
					value = static_cast<uint32_t>(intValue);
				}
			}
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = value;
			return;
		}
		if (!HasReadableKey(jsonKey)) {
			return;
		}
		if (!readJson_->at(jsonKey).is_number_unsigned() && !readJson_->at(jsonKey).is_number_integer()) {
			return;
		}
		int64_t readValue = readJson_->at(jsonKey).get<int64_t>();
		if (readValue < 0) {
			return;
		}
		value = static_cast<uint32_t>(readValue);
		if (minValue < maxValue) {
			value = std::clamp(value, minValue, maxValue);
		}
	}

	void Bool(const char* memberName, bool& value) {
		std::string key = MakeJsonKey(memberName);
		BoolNamed(key.c_str(), MakeDisplayName(key).c_str(), value);
	}

	void BoolNamed(const char* jsonKey, const char* label, bool& value) {
		if (mode_ == Mode::DrawInspector) {
			InspectorUI::Checkbox(label, &value);
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = value;
			return;
		}
		if (!HasReadableKey(jsonKey)) {
			return;
		}
		if (!readJson_->at(jsonKey).is_boolean()) {
			return;
		}
		value = readJson_->at(jsonKey).get<bool>();
	}

	// ラベル見出しの下に X/Y/Z の3チェックボックスを横並びで描く(Unityのconstraints風)。
	// JSONは各軸を個別キーで保存/読込する。
	void BoolAxes(const char* label, const char* keyX, bool& x, const char* keyY, bool& y, const char* keyZ, bool& z) {
		if (mode_ == Mode::DrawInspector) {
			InspectorUI::TextUnformatted(label);
			// 表示は "X"/"Y"/"Z"、IDはキーで一意化(##で不可視化)。
			std::string labelX = std::string("X##") + keyX;
			std::string labelY = std::string("Y##") + keyY;
			std::string labelZ = std::string("Z##") + keyZ;
			InspectorUI::Checkbox(labelX.c_str(), &x);
			InspectorUI::SameLine();
			InspectorUI::Checkbox(labelY.c_str(), &y);
			InspectorUI::SameLine();
			InspectorUI::Checkbox(labelZ.c_str(), &z);
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[keyX] = x;
			(*writeJson_)[keyY] = y;
			(*writeJson_)[keyZ] = z;
			return;
		}
		if (HasReadableKey(keyX) && readJson_->at(keyX).is_boolean()) {
			x = readJson_->at(keyX).get<bool>();
		}
		if (HasReadableKey(keyY) && readJson_->at(keyY).is_boolean()) {
			y = readJson_->at(keyY).get<bool>();
		}
		if (HasReadableKey(keyZ) && readJson_->at(keyZ).is_boolean()) {
			z = readJson_->at(keyZ).get<bool>();
		}
	}

	void Vector3Field(const char* memberName, Vector3& value, float dragSpeed, float minValue, float maxValue) {
		std::string key = MakeJsonKey(memberName);
		Vector3Named(key.c_str(), MakeDisplayName(key).c_str(), value, dragSpeed, minValue, maxValue);
	}

	void Vector3Named(const char* jsonKey, const char* label, Vector3& value, float dragSpeed, float minValue, float maxValue) {
		if (mode_ == Mode::CollectAnimatables) {
			std::string basePath = pathPrefix_ + jsonKey;
			channels_->push_back({basePath + ".x", &value.x});
			channels_->push_back({basePath + ".y", &value.y});
			channels_->push_back({basePath + ".z", &value.z});
			return;
		}
		if (mode_ == Mode::DrawInspector) {
			bool changed = InspectorUI::DragFloat3(label, &value.x, dragSpeed, minValue, maxValue);
			InspectorUI::AnimationFieldHook(jsonKey, &value.x, 3, changed);
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = {value.x, value.y, value.z};
			return;
		}
		if (!CanReadArray(jsonKey, 3)) {
			return;
		}
		const nlohmann::json& values = readJson_->at(jsonKey);
		if (values.at(0).is_number()) {
			value.x = values.at(0).get<float>();
		}
		if (values.at(1).is_number()) {
			value.y = values.at(1).get<float>();
		}
		if (values.at(2).is_number()) {
			value.z = values.at(2).get<float>();
		}
	}

	void Vector4Field(const char* memberName, Vector4& value, float dragSpeed, float minValue, float maxValue) {
		std::string key = MakeJsonKey(memberName);
		Vector4Named(key.c_str(), MakeDisplayName(key).c_str(), value, dragSpeed, minValue, maxValue);
	}

	void Vector4Named(const char* jsonKey, const char* label, Vector4& value, float dragSpeed, float minValue, float maxValue) {
		if (mode_ == Mode::CollectAnimatables) {
			std::string basePath = pathPrefix_ + jsonKey;
			channels_->push_back({basePath + ".x", &value.x});
			channels_->push_back({basePath + ".y", &value.y});
			channels_->push_back({basePath + ".z", &value.z});
			channels_->push_back({basePath + ".w", &value.w});
			return;
		}
		if (mode_ == Mode::DrawInspector) {
			float values[4] = {value.x, value.y, value.z, value.w};
			bool changed = InspectorUI::ColorEdit4(label, values);
			if (changed) {
				value = {values[0], values[1], values[2], values[3]};
			}
			InspectorUI::AnimationFieldHook(jsonKey, &value.x, 4, changed);
			(void)dragSpeed;
			(void)minValue;
			(void)maxValue;
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = {value.x, value.y, value.z, value.w};
			return;
		}
		if (!CanReadArray(jsonKey, 4)) {
			return;
		}
		const nlohmann::json& values = readJson_->at(jsonKey);
		if (values.at(0).is_number()) {
			value.x = values.at(0).get<float>();
		}
		if (values.at(1).is_number()) {
			value.y = values.at(1).get<float>();
		}
		if (values.at(2).is_number()) {
			value.z = values.at(2).get<float>();
		}
		if (values.at(3).is_number()) {
			value.w = values.at(3).get<float>();
		}
	}

	void String(const char* memberName, std::string& value) {
		std::string key = MakeJsonKey(memberName);
		StringNamed(key.c_str(), MakeDisplayName(key).c_str(), value);
	}

	void StringNamed(const char* jsonKey, const char* label, std::string& value) {
		if (mode_ == Mode::DrawInspector) {
			std::array<char, 256> buffer{};
			size_t copyLength = (std::min)(value.size(), buffer.size() - 1);
			std::copy_n(value.data(), copyLength, buffer.data());
			buffer[copyLength] = '\0';
			if (InspectorUI::InputText(label, buffer.data(), buffer.size())) {
				value = buffer.data();
			}
			return;
		}
		if (mode_ == Mode::WriteJson) {
			(*writeJson_)[jsonKey] = value;
			return;
		}
		if (!HasReadableKey(jsonKey)) {
			return;
		}
		if (!readJson_->at(jsonKey).is_string()) {
			return;
		}
		value = readJson_->at(jsonKey).get<std::string>();
	}

	template <class TObject>
	void Object(const char* memberName, TObject& value) {
		std::string key = MakeJsonKey(memberName);
		ObjectNamed(key.c_str(), MakeDisplayName(key).c_str(), value);
	}

	template <class TObject>
	void ObjectNamed(const char* jsonKey, const char* label, TObject& value) {
		if (mode_ == Mode::CollectAnimatables) {
			SerializedFieldRegistry childRegistry(*channels_);
			childRegistry.pathPrefix_ = pathPrefix_ + jsonKey + ".";
			value.RegisterSerializedFields(childRegistry);
			return;
		}
		if (mode_ == Mode::DrawInspector) {
			InspectorUI::TextUnformatted(label);
			SerializedFieldRegistry childRegistry;
			value.RegisterSerializedFields(childRegistry);
			return;
		}
		if (mode_ == Mode::WriteJson) {
			nlohmann::json childJson = nlohmann::json::object();
			SerializedFieldRegistry childRegistry(childJson);
			value.RegisterSerializedFields(childRegistry);
			(*writeJson_)[jsonKey] = childJson;
			return;
		}
		if (!HasReadableKey(jsonKey)) {
			return;
		}
		if (!readJson_->at(jsonKey).is_object()) {
			return;
		}
		SerializedFieldRegistry childRegistry(readJson_->at(jsonKey));
		value.RegisterSerializedFields(childRegistry);
	}

	static std::string MakeJsonKey(const char* memberName) {
		std::string key = memberName;
		size_t dotPosition = key.find_last_of('.');
		if (dotPosition != std::string::npos) {
			key = key.substr(dotPosition + 1);
		}
		while (!key.empty() && key.back() == '_') {
			key.pop_back();
		}
		return key;
	}

	static std::string MakeDisplayName(const std::string& key) {
		std::string label;
		label.reserve(key.size() + 4);

		for (size_t index = 0; index < key.size(); ++index) {
			char character = key[index];
			if (character == '_') {
				label.push_back(' ');
				continue;
			}

			bool shouldInsertSpace = false;
			if (index > 0 && std::isupper(static_cast<unsigned char>(character)) != 0) {
				char previous = key[index - 1];
				if (previous != '_' && std::islower(static_cast<unsigned char>(previous)) != 0) {
					shouldInsertSpace = true;
				}
			}
			if (shouldInsertSpace) {
				label.push_back(' ');
			}

			if (label.empty() || (!label.empty() && label.back() == ' ')) {
				label.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
			} else {
				label.push_back(character);
			}
		}

		return label;
	}

private:
	bool HasReadableKey(const char* jsonKey) const {
		if (!readJson_) {
			return false;
		}
		return readJson_->contains(jsonKey);
	}

	bool CanReadArray(const char* jsonKey, size_t requiredSize) const {
		if (!HasReadableKey(jsonKey)) {
			return false;
		}
		if (!readJson_->at(jsonKey).is_array()) {
			return false;
		}
		return readJson_->at(jsonKey).size() >= requiredSize;
	}

	Mode mode_ = Mode::DrawInspector;
	nlohmann::json* writeJson_ = nullptr;
	const nlohmann::json* readJson_ = nullptr;
	std::vector<AnimatableChannel>* channels_ = nullptr;
	std::string pathPrefix_;
};

} // namespace KujakuEngine

#define KUJAKU_FIELD_FLOAT(name, defaultValue) float name = defaultValue
#define KUJAKU_FIELD_INT(name, defaultValue) int name = defaultValue
#define KUJAKU_FIELD_UINT32(name, defaultValue) uint32_t name = defaultValue
#define KUJAKU_FIELD_BOOL(name, defaultValue) bool name = defaultValue
#define KUJAKU_FIELD_VECTOR3(name, defaultValue) KujakuEngine::Vector3 name = defaultValue
#define KUJAKU_FIELD_VECTOR4(name, defaultValue) KujakuEngine::Vector4 name = defaultValue
#define KUJAKU_FIELD_STRING(name, defaultValue) std::string name = defaultValue
#define KUJAKU_FIELD_OBJECT(type, name) type name{}

#define KUJAKU_SERIALIZED_FIELDS_BEGIN() \
public: \
	void DrawInspector() override { \
		KujakuEngine::SerializedFieldRegistry registry; \
		RegisterSerializedFields(registry); \
	} \
	void WriteJson(nlohmann::json& json) const override { \
		KujakuEngine::SerializedFieldRegistry registry(json); \
		auto& mutableSelf = const_cast<std::remove_const_t<std::remove_reference_t<decltype(*this)>>&>(*this); \
		mutableSelf.RegisterSerializedFields(registry); \
	} \
	void ReadJson(const nlohmann::json& json) override { \
		KujakuEngine::SerializedFieldRegistry registry(json); \
		RegisterSerializedFields(registry); \
	} \
	void CollectAnimatableChannels(std::vector<KujakuEngine::AnimatableChannel>& channels) override { \
		KujakuEngine::SerializedFieldRegistry registry(channels); \
		RegisterSerializedFields(registry); \
	} \
private: \
	void RegisterSerializedFields(KujakuEngine::SerializedFieldRegistry& registry)

#define KUJAKU_REGISTER_FLOAT(member, dragSpeed, minValue, maxValue) registry.Float(#member, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_FLOAT_NAMED(member, label, dragSpeed, minValue, maxValue) registry.FloatNamed(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_INT(member, dragSpeed, minValue, maxValue) registry.Int(#member, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_INT_NAMED(member, label, dragSpeed, minValue, maxValue) registry.IntNamed(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_UINT32(member, dragSpeed, minValue, maxValue) registry.UInt32(#member, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_UINT32_NAMED(member, label, dragSpeed, minValue, maxValue) registry.UInt32Named(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_BOOL(member) registry.Bool(#member, member)
#define KUJAKU_REGISTER_BOOL_NAMED(member, label) registry.BoolNamed(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member)
#define KUJAKU_REGISTER_BOOL_AXES(label, memberX, memberY, memberZ)                                    \
	registry.BoolAxes(label, KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#memberX).c_str(), memberX, \
	                  KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#memberY).c_str(), memberY,        \
	                  KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#memberZ).c_str(), memberZ)
#define KUJAKU_REGISTER_VECTOR3(member, dragSpeed, minValue, maxValue) registry.Vector3Field(#member, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_VECTOR3_NAMED(member, label, dragSpeed, minValue, maxValue) registry.Vector3Named(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_VECTOR4(member, dragSpeed, minValue, maxValue) registry.Vector4Field(#member, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_VECTOR4_NAMED(member, label, dragSpeed, minValue, maxValue) registry.Vector4Named(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member, dragSpeed, minValue, maxValue)
#define KUJAKU_REGISTER_STRING(member) registry.String(#member, member)
#define KUJAKU_REGISTER_STRING_NAMED(member, label) registry.StringNamed(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member)
#define KUJAKU_REGISTER_OBJECT(member) registry.Object(#member, member)
#define KUJAKU_REGISTER_OBJECT_NAMED(member, label) registry.ObjectNamed(KujakuEngine::SerializedFieldRegistry::MakeJsonKey(#member).c_str(), label, member)
