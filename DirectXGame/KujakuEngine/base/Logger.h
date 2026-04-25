#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace KujakuEngine {

/// <summary>
/// ログ管理クラス
/// </summary>
class Logger {
public: 
    static void Initialize();
    static void Log(const std::string& message);

private:
    static std::ofstream logStream_;
};

} // namespace KujakuEngine