#include "InspectorUI.h"

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif

namespace KujakuEngine::InspectorUI {

bool DragFloat(const char* label, float* value, float speed, float minValue, float maxValue, const char* format) {
#ifdef USE_IMGUI
	return ImGui::DragFloat(label, value, speed, minValue, maxValue, format);
#else
	(void)label;
	(void)value;
	(void)speed;
	(void)minValue;
	(void)maxValue;
	(void)format;
	return false;
#endif
}

bool DragFloat3(const char* label, float* values, float speed, float minValue, float maxValue, const char* format) {
#ifdef USE_IMGUI
	return ImGui::DragFloat3(label, values, speed, minValue, maxValue, format);
#else
	(void)label;
	(void)values;
	(void)speed;
	(void)minValue;
	(void)maxValue;
	(void)format;
	return false;
#endif
}

bool DragInt(const char* label, int* value, float speed, int minValue, int maxValue) {
#ifdef USE_IMGUI
	return ImGui::DragInt(label, value, speed, minValue, maxValue);
#else
	(void)label;
	(void)value;
	(void)speed;
	(void)minValue;
	(void)maxValue;
	return false;
#endif
}

bool Checkbox(const char* label, bool* value) {
#ifdef USE_IMGUI
	return ImGui::Checkbox(label, value);
#else
	(void)label;
	(void)value;
	return false;
#endif
}

bool ColorEdit4(const char* label, float* values) {
#ifdef USE_IMGUI
	return ImGui::ColorEdit4(label, values);
#else
	(void)label;
	(void)values;
	return false;
#endif
}

bool Combo(const char* label, int* currentItem, const char* const* items, int itemCount) {
#ifdef USE_IMGUI
	return ImGui::Combo(label, currentItem, items, itemCount);
#else
	(void)label;
	(void)currentItem;
	(void)items;
	(void)itemCount;
	return false;
#endif
}

bool InputText(const char* label, char* buffer, std::size_t bufferSize) {
#ifdef USE_IMGUI
	return ImGui::InputText(label, buffer, bufferSize);
#else
	(void)label;
	(void)buffer;
	(void)bufferSize;
	return false;
#endif
}

bool Button(const char* label) {
#ifdef USE_IMGUI
	return ImGui::Button(label);
#else
	(void)label;
	return false;
#endif
}

void TextUnformatted(const char* text) {
#ifdef USE_IMGUI
	ImGui::TextUnformatted(text);
#else
	(void)text;
#endif
}

void TextDisabled(const char* text) {
#ifdef USE_IMGUI
	ImGui::TextDisabled("%s", text);
#else
	(void)text;
#endif
}

} // namespace KujakuEngine::InspectorUI
