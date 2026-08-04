#pragma once

#include "../runtime/KujakuApi.h"
#include <filesystem>

namespace KujakuEngine {

/// <summary>
/// パス表記揺れを減らした形へ正規化する
/// </summary>
KUJAKU_API std::filesystem::path NormalizeEditorPath(const std::filesystem::path& path);

/// <summary>
/// 実行時カレントからProjectDirを探す
/// </summary>
KUJAKU_API std::filesystem::path DetectEditorProjectRoot();

/// <summary>
/// 実行中exeが置かれているディレクトリを返す(配布パッケージ判定用)。
/// </summary>
KUJAKU_API std::filesystem::path GetExecutableDirectory();

/// <summary>
/// ProjectSettings/SceneJson/Materials/Prefabs/Resources/Animations をまとめて置く
/// "Data"フォルダを返す(DetectEditorProjectRoot()直下)。ソリューション実行/exe単体実行の
/// どちらでも同じ相対構成(Data/配下)からロードできるようにするための唯一の参照点。
/// GameComponents(GameModule.dll)からも参照するためKUJAKU_APIでエクスポートする。
/// </summary>
KUJAKU_API std::filesystem::path GetProjectDataRoot();

} // namespace KujakuEngine
