#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace KujataEngine {

namespace StringUtil {

std::wstring ToWString(const std::string& str);

std::string ToString(const std::wstring& str);

} // namespace StringUtil

} // namespace KujataEngine