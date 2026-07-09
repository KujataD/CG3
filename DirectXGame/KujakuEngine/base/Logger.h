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
    /// <summary>
    /// 初期化します。
    /// </summary>
    static void Initialize();
    /// <summary>
    /// Logを実行します。
    /// </summary>
    static void Log(const std::string& message);

private:
    static std::ofstream logStream_;
};

} // namespace KujakuEngine