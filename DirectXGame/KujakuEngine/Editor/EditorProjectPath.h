#pragma once

#include <filesystem>

namespace KujakuEngine {

/// <summary>
/// パス表記揺れを減らした形へ正規化する
/// </summary>
std::filesystem::path NormalizeEditorPath(const std::filesystem::path& path);

/// <summary>
/// 実行時カレントからProjectDirを探す
/// </summary>
std::filesystem::path DetectEditorProjectRoot();

} // namespace KujakuEngine
