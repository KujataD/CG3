#pragma once

#include "GraphicsPipeline.h"
#include <cstdint>
#include <string>

namespace KujakuEngine::ModelUtil {

/// <summary>
/// OBJファイルを読み込み、エンジン用のモデルデータに変換する
/// </summary>
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

/// <summary>
/// MTLファイルからマテリアル情報を読み込む
/// </summary>
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

/// <summary>
/// マテリアルのテクスチャパスからSRVインデックスを設定する
/// </summary>
void ResolveTextureIndex(MaterialData& material);

/// <summary>
/// 指定テクスチャを使う基本マテリアルを作成する
/// </summary>
MaterialData CreateTexturedMaterial(const std::string& textureFilePath, int32_t enableLighting);

/// <summary>
/// 3Dモデル描画で共通のビューポート・シザー・トポロジを設定する
/// </summary>
void SetCommonRenderState();

} // namespace KujakuEngine::ModelUtil
