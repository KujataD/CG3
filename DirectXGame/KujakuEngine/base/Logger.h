#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

#include "../runtime/KujakuApi.h"

namespace KujakuEngine {

/// <summary>
/// ログ管理クラス
/// </summary>
class KUJAKU_API Logger {
public: 
    static void Initialize();
    static void Log(const std::string& message);
};

} // namespace KujakuEngine