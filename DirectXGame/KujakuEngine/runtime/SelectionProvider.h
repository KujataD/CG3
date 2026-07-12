#pragma once

#include "ISelectionProvider.h"
#include "KujakuApi.h"

namespace KujakuEngine {

// 現在の選択プロバイダを取得する。未設定時は常にnullptrを返すフォールバックを返す。
KUJAKU_API ISelectionProvider& GetSelectionProvider();

// 選択プロバイダを設定する(Editor側がEditorSelectionを注入する)。
KUJAKU_API void SetSelectionProvider(ISelectionProvider* provider);

} // namespace KujakuEngine
