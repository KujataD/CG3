#pragma once

#include "IAssetResolver.h"
#include "KujakuApi.h"

namespace KujakuEngine {

// 現在のアセットリゾルバを取得する。未設定時は素通しのフォールバックを返す。
KUJAKU_API IAssetResolver& GetAssetResolver();

// アセットリゾルバを設定する(Editor側がAssetDatabaseを注入する)。
KUJAKU_API void SetAssetResolver(IAssetResolver* resolver);

} // namespace KujakuEngine
