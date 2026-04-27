#pragma once
#include <numbers>
#include <random>
#include <type_traits>

namespace KujakuEngine {

class Random {
public:
	static void Initialize() {
		// 乱数生成エンジン
		std::random_device seedGenerator;
		// メルセンヌ·ツイスターエンジンの初期化
		randomEngine.seed(seedGenerator());
	}

	/// <summary>
	/// 値の取得
	/// </summary>
	template<typename T> static T GetRandom(T min, T max) {
		// 整数型か少数型かどうか
		if constexpr (std::is_integral_v<T>) {
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(randomEngine);
		} else if constexpr (std::is_floating_point_v<T>) {
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(randomEngine);
		}
	}

private:
	// メルセンヌ·ツイスターエンジン(64bit版)
	static std::mt19937_64 randomEngine;
};

} // namespace KujakuEngine