#pragma once

#include "../runtime/KujakuApi.h"
#include <cstddef>

namespace KujakuEngine::InspectorUI {

// GameModule上のComponentはImGui本体を直接リンクせず、エンジン側のImGuiコンテキストでInspector UIを描画する。
/// <summary>
/// DragFloatを実行します。
/// </summary>
KUJAKU_API bool DragFloat(const char* label, float* value, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
/// <summary>
/// DragFloat3を実行します。
/// </summary>
KUJAKU_API bool DragFloat3(const char* label, float* values, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
/// <summary>
/// DragIntを実行します。
/// </summary>
KUJAKU_API bool DragInt(const char* label, int* value, float speed = 1.0f, int minValue = 0, int maxValue = 0);
/// <summary>
/// Checkboxを実行します。
/// </summary>
KUJAKU_API bool Checkbox(const char* label, bool* value);
/// <summary>
/// ColorEdit4を実行します。
/// </summary>
KUJAKU_API bool ColorEdit4(const char* label, float* values);
/// <summary>
/// Comboを実行します。
/// </summary>
KUJAKU_API bool Combo(const char* label, int* currentItem, const char* const* items, int itemCount);
/// <summary>
/// InputTextを実行します。
/// </summary>
KUJAKU_API bool InputText(const char* label, char* buffer, std::size_t bufferSize);
/// <summary>
/// Buttonを実行します。
/// </summary>
KUJAKU_API bool Button(const char* label);
/// <summary>
/// TextUnformattedを実行します。
/// </summary>
KUJAKU_API void TextUnformatted(const char* text);
/// <summary>
/// TextDisabledを実行します。
/// </summary>
KUJAKU_API void TextDisabled(const char* text);

} // namespace KujakuEngine::InspectorUI
