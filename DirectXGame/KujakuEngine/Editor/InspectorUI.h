#pragma once

#include "../runtime/KujakuApi.h"
#include <cstddef>

namespace KujakuEngine::InspectorUI {

// GameModule上のComponentはImGui本体を直接リンクせず、エンジン側のImGuiコンテキストでInspector UIを描画する。
KUJAKU_API bool DragFloat(const char* label, float* value, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
KUJAKU_API bool DragFloat3(const char* label, float* values, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
KUJAKU_API bool ColorEdit4(const char* label, float* values);
KUJAKU_API bool Combo(const char* label, int* currentItem, const char* const* items, int itemCount);
KUJAKU_API bool InputText(const char* label, char* buffer, std::size_t bufferSize);
KUJAKU_API bool Button(const char* label);
KUJAKU_API void TextUnformatted(const char* text);
KUJAKU_API void TextDisabled(const char* text);

} // namespace KujakuEngine::InspectorUI
