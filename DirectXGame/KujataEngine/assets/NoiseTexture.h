#pragma once

#include "../runtime/KujataApi.h"
#include <cstdint>
#include <string>

namespace KujataEngine {

/// <summary>
/// FractalNoise(math/Noise.h)からRGBA8テクスチャを起動時にメモリ生成する。
/// ファイルには保存しない(TextureManager::CreateTextureFromMemoryで都度アップロード)。
/// 同じkeyへは1回しか実際の生成をしない(内部でSRVインデックスをキャッシュする)。
/// </summary>
namespace NoiseTexture {

/// <summary>
/// 岩肌用のアルベドテクスチャを生成し、SRVインデックスを返す。
/// 低周波の"染み"と高周波の"ざらつき"を重ねたfBmを、グレーがかった岩色へマッピングする。
/// </summary>
/// <param name="size">一辺のピクセル数(正方形)。</param>
/// <param name="seedLane">Noiseの第2引数に使うレーン。値を変えると別の模様になる。</param>
KUJATA_API uint32_t GenerateRockAlbedoTexture(uint32_t size = 256, float seedLane = 0.0f);

/// <summary>
/// 土(地面)用のアルベドテクスチャを生成し、SRVインデックスを返す。
/// 岩肌より暗く・彩度低めの茶色で、低周波の"塊"を主体にした柔らかい表情にする。
/// </summary>
/// <param name="size">一辺のピクセル数(正方形)。</param>
/// <param name="seedLane">Noiseの第2引数に使うレーン。値を変えると別の模様になる。</param>
KUJATA_API uint32_t GenerateSoilAlbedoTexture(uint32_t size = 256, float seedLane = 50.0f);

/// <summary>
/// 土埃パーティクル用のスプライトテクスチャを生成し、SRVインデックスを返す。
/// 円形のソフトなアルファ減衰にfBmで揺らぎを加え、もやついた埃の塊に見せる。
/// </summary>
/// <param name="size">一辺のピクセル数(正方形)。</param>
/// <param name="seedLane">Noiseの第2引数に使うレーン。値を変えると別の模様になる。</param>
KUJATA_API uint32_t GenerateDustSpriteTexture(uint32_t size = 128, float seedLane = 100.0f);

} // namespace NoiseTexture

} // namespace KujataEngine
