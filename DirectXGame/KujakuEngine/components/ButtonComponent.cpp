#include "ButtonComponent.h"

#include "../runtime/InspectorUI.h"
#include "../runtime/UIEventBus.h"
#include "../scene/GameObject.h"
#include "../scene/InvokableMethod.h"
#include "ImageComponent.h"
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace KujakuEngine {
namespace {

Vector4 ReadVector4(const nlohmann::json& json, const char* key, const Vector4& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	const nlohmann::json& value = json.at(key);
	if (!value.is_array() || value.size() < 4) {
		return defaultValue;
	}
	for (int i = 0; i < 4; ++i) {
		if (!value[i].is_number()) {
			return defaultValue;
		}
	}
	return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>()};
}

std::string ReadString(const nlohmann::json& json, const char* key, const std::string& defaultValue) {
	if (!json.contains(key) || !json.at(key).is_string()) {
		return defaultValue;
	}
	return json.at(key).get<std::string>();
}

} // namespace

void ButtonComponent::SyncEventBuffer() {
	std::memset(eventBuffer_.data(), 0, eventBuffer_.size());
	strncpy_s(eventBuffer_.data(), eventBuffer_.size(), onClickEvent_.c_str(), _TRUNCATE);
}

void ButtonComponent::ApplyVisualState(VisualState state) {
	if (!owner_) {
		return;
	}
	ImageComponent* image = owner_->GetComponent<ImageComponent>();
	if (!image) {
		return;
	}
	if (!interactable_) {
		state = VisualState::Disabled;
	}
	switch (state) {
	case VisualState::Highlighted:
		image->SetColor(highlightedColor_);
		break;
	case VisualState::Pressed:
		image->SetColor(pressedColor_);
		break;
	case VisualState::Disabled:
		image->SetColor(disabledColor_);
		break;
	case VisualState::Normal:
	default:
		image->SetColor(normalColor_);
		break;
	}
}

void ButtonComponent::FireOnClick() {
	if (!interactable_) {
		return;
	}
	// 旧来の名前付きイベント(後方互換)。
	UIEventBus::Publish(onClickEvent_);
	// UnityのButton.onClick相当。対象Componentのメソッドを直接呼ぶ。
	onClick_.Invoke();
}

void ButtonComponent::DrawInspector() {
#ifdef USE_IMGUI
	if (!eventBufferSynced_) {
		SyncEventBuffer();
		eventBufferSynced_ = true;
	}
	InspectorUI::Checkbox("Interactable", &interactable_);
	InspectorUI::ColorEdit4("Normal", &normalColor_.x);
	InspectorUI::ColorEdit4("Highlighted", &highlightedColor_.x);
	InspectorUI::ColorEdit4("Pressed", &pressedColor_.x);
	InspectorUI::ColorEdit4("Disabled", &disabledColor_.x);
	if (InspectorUI::InputText("On Click Event", eventBuffer_.data(), eventBuffer_.size())) {
		onClickEvent_ = eventBuffer_.data();
	}
	DrawOnClickInspector();
#endif // USE_IMGUI
}

void ButtonComponent::DrawOnClickInspector() {
#ifdef USE_IMGUI
	InspectorUI::TextUnformatted("On Click ()");

	int removeIndex = -1;
	for (int i = 0; i < static_cast<int>(onClick_.calls.size()); ++i) {
		PersistentCall& call = onClick_.calls[i];
		const std::string suffix = std::string("##onClick") + std::to_string(i);

		// 対象GameObject(HierarchyからD&D)。
		void* dropped = nullptr;
		bool cleared = false;
		const std::string targetName = GameObjectDisplayName(call.target.value);
		if (InspectorUI::ObjectField((std::string("Target") + suffix).c_str(), targetName.c_str(), &dropped, &cleared)) {
			if (cleared) {
				call.target.Clear();
				call.componentType.clear();
				call.methodName.clear();
			} else if (dropped) {
				call.target.Assign(static_cast<GameObject*>(dropped));
				call.componentType.clear();
				call.methodName.clear();
			}
		}

		if (call.target.value) {
			// 対象GameObjectが持つComponentの型名一覧をドロップダウンにする。
			std::vector<std::string> componentNames;
			for (const std::unique_ptr<Component>& component : call.target.value->GetComponents()) {
				if (component) {
					componentNames.push_back(component->GetTypeName());
				}
			}
			std::vector<const char*> componentItems;
			componentItems.reserve(componentNames.size());
			for (const std::string& name : componentNames) {
				componentItems.push_back(name.c_str());
			}
			int componentIndex = -1;
			for (int k = 0; k < static_cast<int>(componentNames.size()); ++k) {
				if (componentNames[k] == call.componentType) {
					componentIndex = k;
					break;
				}
			}
			if (InspectorUI::Combo((std::string("Component") + suffix).c_str(), &componentIndex, componentItems.data(), static_cast<int>(componentItems.size()))) {
				if (componentIndex >= 0 && componentIndex < static_cast<int>(componentNames.size())) {
					call.componentType = componentNames[componentIndex];
					call.methodName.clear();
				}
			}

			// 選択中Componentが公開するメソッド名一覧をドロップダウンにする。
			if (!call.componentType.empty()) {
				Component* targetComponent = nullptr;
				for (const std::unique_ptr<Component>& component : call.target.value->GetComponents()) {
					if (component && call.componentType == component->GetTypeName()) {
						targetComponent = component.get();
						break;
					}
				}

				std::vector<std::string> methodNames;
				if (targetComponent) {
					InvokableMethodRegistry registry;
					targetComponent->RegisterInvokableMethods(registry);
					for (const InvokableMethodRegistry::Entry& entry : registry.Entries()) {
						methodNames.push_back(entry.name);
					}
				}

				if (methodNames.empty()) {
					InspectorUI::TextDisabled("(no invokable methods)");
				} else {
					std::vector<const char*> methodItems;
					methodItems.reserve(methodNames.size());
					for (const std::string& name : methodNames) {
						methodItems.push_back(name.c_str());
					}
					int methodIndex = -1;
					for (int k = 0; k < static_cast<int>(methodNames.size()); ++k) {
						if (methodNames[k] == call.methodName) {
							methodIndex = k;
							break;
						}
					}
					if (InspectorUI::Combo((std::string("Method") + suffix).c_str(), &methodIndex, methodItems.data(), static_cast<int>(methodItems.size()))) {
						if (methodIndex >= 0 && methodIndex < static_cast<int>(methodNames.size())) {
							call.methodName = methodNames[methodIndex];
						}
					}
				}
			}
		}

		if (InspectorUI::Button((std::string("Remove") + suffix).c_str())) {
			removeIndex = i;
		}
	}

	if (removeIndex >= 0) {
		onClick_.calls.erase(onClick_.calls.begin() + removeIndex);
	}

	if (InspectorUI::Button("Add On Click ()")) {
		onClick_.calls.emplace_back();
	}
#endif // USE_IMGUI
}

void ButtonComponent::WriteJson(nlohmann::json& json) const {
	json["interactable"] = interactable_;
	json["normalColor"] = {normalColor_.x, normalColor_.y, normalColor_.z, normalColor_.w};
	json["highlightedColor"] = {highlightedColor_.x, highlightedColor_.y, highlightedColor_.z, highlightedColor_.w};
	json["pressedColor"] = {pressedColor_.x, pressedColor_.y, pressedColor_.z, pressedColor_.w};
	json["disabledColor"] = {disabledColor_.x, disabledColor_.y, disabledColor_.z, disabledColor_.w};
	json["onClickEvent"] = onClickEvent_;

	nlohmann::json callsJson = nlohmann::json::array();
	for (const PersistentCall& call : onClick_.calls) {
		nlohmann::json callJson;
		callJson["target"] = call.target.targetInstanceId;
		callJson["component"] = call.componentType;
		callJson["method"] = call.methodName;
		callsJson.push_back(std::move(callJson));
	}
	json["onClick"] = nlohmann::json::object();
	json["onClick"]["calls"] = std::move(callsJson);
}

void ButtonComponent::ReadJson(const nlohmann::json& json) {
	if (json.contains("interactable") && json.at("interactable").is_boolean()) {
		interactable_ = json.at("interactable").get<bool>();
	}
	normalColor_ = ReadVector4(json, "normalColor", normalColor_);
	highlightedColor_ = ReadVector4(json, "highlightedColor", highlightedColor_);
	pressedColor_ = ReadVector4(json, "pressedColor", pressedColor_);
	disabledColor_ = ReadVector4(json, "disabledColor", disabledColor_);
	onClickEvent_ = ReadString(json, "onClickEvent", onClickEvent_);

	onClick_.calls.clear();
	if (json.contains("onClick") && json.at("onClick").is_object()) {
		const nlohmann::json& onClickJson = json.at("onClick");
		if (onClickJson.contains("calls") && onClickJson.at("calls").is_array()) {
			for (const nlohmann::json& callJson : onClickJson.at("calls")) {
				if (!callJson.is_object()) {
					continue;
				}
				PersistentCall call;
				call.target.targetInstanceId = ReadString(callJson, "target", "");
				call.componentType = ReadString(callJson, "component", "");
				call.methodName = ReadString(callJson, "method", "");
				onClick_.calls.push_back(std::move(call));
			}
		}
	}
}

void ButtonComponent::OnAfterReadJson() {
	SyncEventBuffer();
	eventBufferSynced_ = true;
}

void ButtonComponent::ResolveReferences(IObjectResolver& resolver) {
	// onClickの各target(instanceId)を実GameObject*へ解決する。
	onClick_.Resolve(resolver);
}

} // namespace KujakuEngine
