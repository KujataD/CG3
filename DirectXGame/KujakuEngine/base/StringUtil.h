#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace KujakuEngine {

namespace StringUtil {

/// <summary>
/// ToWStringを実行します。
/// </summary>
std::wstring ToWString(const std::string& str);

/// <summary>
/// ToStringを実行します。
/// </summary>
std::string ToString(const std::wstring& str);

} // namespace StringUtil

} // namespace KujakuEngine