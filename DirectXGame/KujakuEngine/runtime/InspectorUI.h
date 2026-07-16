#pragma once

#include "KujakuApi.h"
#include <cstddef>

namespace KujakuEngine::InspectorUI {

// GameModule上のComponentはImGui本体を直接リンクせず、エンジン側のImGuiコンテキストでInspector UIを描画する。

KUJAKU_API bool DragFloat(const char* label, float* value, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
KUJAKU_API bool DragFloat2(const char* label, float* values, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
KUJAKU_API bool DragFloat3(const char* label, float* values, float speed = 1.0f, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f");
KUJAKU_API bool DragInt(const char* label, int* value, float speed = 1.0f, int minValue = 0, int maxValue = 0);
KUJAKU_API bool Checkbox(const char* label, bool* value);
KUJAKU_API bool ColorEdit4(const char* label, float* values);
KUJAKU_API bool Combo(const char* label, int* currentItem, const char* const* items, int itemCount);
KUJAKU_API bool InputText(const char* label, char* buffer, std::size_t bufferSize);
KUJAKU_API bool Button(const char* label);
KUJAKU_API bool ButtonSized(const char* label, float width, float height);

/// <summary>
/// オブジェクト参照フィールド。currentName(未設定は"None")を表示し、HierarchyからGameObjectが
/// ドロップされたらoutDroppedObjectにそのGameObject*を入れてtrueを返す。Clearボタンでは*outClearedをtrue。
/// UnityのInspectorのオブジェクト代入欄に相当。
/// </summary>
KUJAKU_API bool ObjectField(const char* label, const char* currentName, void** outDroppedObject, bool* outCleared);

KUJAKU_API void TextUnformatted(const char* text);
KUJAKU_API void TextDisabled(const char* text);
KUJAKU_API void SameLine();

} // namespace KujakuEngine::InspectorUI
