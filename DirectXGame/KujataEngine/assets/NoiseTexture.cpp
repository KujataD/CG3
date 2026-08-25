#include "NoiseTexture.h"
#include "../base/TextureManager.h"
#include "../math/Noise.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace KujataEngine {

namespace {

struct Color3 {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
};

/// <summary>
/// key(サイズ+シード)ごとにSRVインデックスを覚えておく。TextureManager::CreateTextureFromMemory
/// 自体もkeyでGPU生成をスキップするが、それだけだと**CPU側のノイズ計算(4隅ブレンドで4倍重い)は
/// 呼ばれるたびに毎回やり直してしまう**。土埃のように同じ設定のテクスチャを何度も要求される
/// 用途(プールが新しい器を作るたび)でカクつきの原因になっていたため、ここで丸ごとスキップする。
/// </summary>
std::unordered_map<std::string, uint32_t>& GeneratedTextureCache() {
	static std::unordered_map<std::string, uint32_t> cache;
	return cache;
}

Color3 Lerp(const Color3& a, const Color3& b, float t) {
	return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

uint8_t ToByte(float value) { return static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f); }

/// <summary>
/// ノイズが0を跨ぐ場所(尾根)を薄い線として浮かび上がらせる"リッジノイズ"。
/// 1-|noise| は0跨ぎでピーク(≒1)になるので、それをべき乗で先鋭化すると
/// 枝分かれした細いひび割れ網に見える。Voronoi等の専用ノイズが無くても既存のFractalNoiseだけで作れる。
/// </summary>
float CrackMask(float u, float v, float freq, float seedLane, float sharpness) {
	float n = FractalNoise(u * freq + seedLane, v * freq + seedLane, 3);
	float ridge = 1.0f - std::fabs(n);
	return std::pow(std::clamp(ridge, 0.0f, 1.0f), sharpness);
}

/// <summary>
/// colorAt(u,v)を4隅(u,v / u-1,v / u,v-1 / u-1,v-1)で評価し、u,vの重みで混ぜて周期化する。
/// u=0とu=1(v=0とv=1も同様)がぴったり同じ値になるので、単純なWRAP(折り返し)だけで
/// タイルの継ぎ目が完全に連続する。MIRROR(反転)と違って対称の"合わせ目"が見た目に出ない。
/// </summary>
template <class ColorAtFunc>
Color3 MakeTileable(float u, float v, ColorAtFunc colorAt) {
	Color3 c00 = colorAt(u, v);
	Color3 c10 = colorAt(u - 1.0f, v);
	Color3 c01 = colorAt(u, v - 1.0f);
	Color3 c11 = colorAt(u - 1.0f, v - 1.0f);
	Color3 top = Lerp(c00, c10, u);
	Color3 bottom = Lerp(c01, c11, u);
	return Lerp(top, bottom, v);
}

} // namespace

namespace NoiseTexture {

uint32_t GenerateRockAlbedoTexture(uint32_t size, float seedLane) {
	std::string key = "Generated/RockAlbedo/" + std::to_string(size) + "/" + std::to_string(seedLane);
	if (auto cached = GeneratedTextureCache().find(key); cached != GeneratedTextureCache().end()) {
		return cached->second;
	}

	// 低周波の"染み"(base)と高周波の"ざらつき"(grain)を重ねる。
	// FractalNoiseの実測レンジは概ね±0.8なので、0..1へ寄せる係数はそれを踏まえた値にする。
	constexpr Color3 kDark = {0.12f, 0.10f, 0.09f};
	constexpr Color3 kLight = {0.58f, 0.55f, 0.50f};

	auto colorAt = [&](float su, float sv) -> Color3 {
		float base = FractalNoise(su * 4.0f + seedLane, sv * 4.0f + seedLane, 4);
		float grain = FractalNoise(su * 20.0f + seedLane + 7.3f, sv * 20.0f + seedLane + 7.3f, 2);
		float n = base * 0.75f + grain * 0.25f;
		float t = std::clamp(n * 0.65f + 0.5f, 0.0f, 1.0f);
		return Lerp(kDark, kLight, t);
	};

	std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);
	for (uint32_t y = 0; y < size; ++y) {
		float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
		for (uint32_t x = 0; x < size; ++x) {
			float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);

			Color3 color = MakeTileable(u, v, colorAt);
			size_t pixelIndex = (static_cast<size_t>(y) * size + x) * 4;
			pixels[pixelIndex + 0] = ToByte(color.r);
			pixels[pixelIndex + 1] = ToByte(color.g);
			pixels[pixelIndex + 2] = ToByte(color.b);
			pixels[pixelIndex + 3] = 255;
		}
	}

	uint32_t textureIndex = TextureManager::GetInstance()->CreateTextureFromMemory(key, pixels.data(), size, size);
	GeneratedTextureCache()[key] = textureIndex;
	return textureIndex;
}

uint32_t GenerateSoilAlbedoTexture(uint32_t size, float seedLane) {
	std::string key = "Generated/SoilAlbedo/" + std::to_string(size) + "/" + std::to_string(seedLane);
	if (auto cached = GeneratedTextureCache().find(key); cached != GeneratedTextureCache().end()) {
		return cached->second;
	}

	// 岩肌より低コントラスト・低彩度。低周波の"塊"を主体にし、高周波のざらつきは控えめにする
	// (乾いた土の柔らかい表情)。そこへ別レーンのCrackMaskを重ね、乾いてひび割れた地面にする。
	constexpr Color3 kDark = {0.10f, 0.07f, 0.04f};
	constexpr Color3 kLight = {0.42f, 0.30f, 0.18f};
	constexpr Color3 kCrackColor = {0.04f, 0.03f, 0.02f};

	auto colorAt = [&](float su, float sv) -> Color3 {
		float base = FractalNoise(su * 3.0f + seedLane, sv * 3.0f + seedLane, 4);
		float grain = FractalNoise(su * 14.0f + seedLane + 4.1f, sv * 14.0f + seedLane + 4.1f, 2);
		float n = base * 0.85f + grain * 0.15f;
		float t = std::clamp(n * 0.7f + 0.5f, 0.0f, 1.0f);
		Color3 color = Lerp(kDark, kLight, t);

		// ひび割れ: baseとは別レーン(+31.7)にしてブロッチの模様と重ならないようにする。
		float crack = CrackMask(su, sv, 5.0f, seedLane + 31.7f, 10.0f);
		return Lerp(color, kCrackColor, crack * 0.85f);
	};

	std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);
	for (uint32_t y = 0; y < size; ++y) {
		float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
		for (uint32_t x = 0; x < size; ++x) {
			float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);

			Color3 color = MakeTileable(u, v, colorAt);
			size_t pixelIndex = (static_cast<size_t>(y) * size + x) * 4;
			pixels[pixelIndex + 0] = ToByte(color.r);
			pixels[pixelIndex + 1] = ToByte(color.g);
			pixels[pixelIndex + 2] = ToByte(color.b);
			pixels[pixelIndex + 3] = 255;
		}
	}

	uint32_t textureIndex = TextureManager::GetInstance()->CreateTextureFromMemory(key, pixels.data(), size, size);
	GeneratedTextureCache()[key] = textureIndex;
	return textureIndex;
}

uint32_t GenerateDustSpriteTexture(uint32_t size, float seedLane) {
	std::string key = "Generated/DustSprite/" + std::to_string(size) + "/" + std::to_string(seedLane);
	if (auto cached = GeneratedTextureCache().find(key); cached != GeneratedTextureCache().end()) {
		return cached->second;
	}

	// 中心が濃く外周でゼロへ落ちる円形マスクに、fBmで輪郭を崩してもやついた見た目にする。
	constexpr Color3 kDustColor = {0.55f, 0.48f, 0.38f};

	std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);
	for (uint32_t y = 0; y < size; ++y) {
		float v = ((static_cast<float>(y) + 0.5f) / static_cast<float>(size)) * 2.0f - 1.0f;
		for (uint32_t x = 0; x < size; ++x) {
			float u = ((static_cast<float>(x) + 0.5f) / static_cast<float>(size)) * 2.0f - 1.0f;

			float dist = std::sqrt(u * u + v * v);
			float radialMask = std::clamp(1.0f - dist, 0.0f, 1.0f);
			radialMask *= radialMask;

			float noise = FractalNoise(u * 3.0f + seedLane, v * 3.0f + seedLane, 3);
			float wispy = std::clamp(noise * 0.5f + 0.5f, 0.0f, 1.0f);

			float alpha = std::clamp(radialMask * (0.4f + wispy * 0.6f), 0.0f, 1.0f);

			size_t pixelIndex = (static_cast<size_t>(y) * size + x) * 4;
			pixels[pixelIndex + 0] = ToByte(kDustColor.r);
			pixels[pixelIndex + 1] = ToByte(kDustColor.g);
			pixels[pixelIndex + 2] = ToByte(kDustColor.b);
			pixels[pixelIndex + 3] = ToByte(alpha);
		}
	}

	uint32_t textureIndex = TextureManager::GetInstance()->CreateTextureFromMemory(key, pixels.data(), size, size);
	GeneratedTextureCache()[key] = textureIndex;
	return textureIndex;
}

} // namespace NoiseTexture

} // namespace KujataEngine
