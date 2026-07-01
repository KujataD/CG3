#pragma once
#include <numbers>
#include <random>
#include <type_traits>
#include <algorithm>

namespace KujakuEngine {

/// <summary>
/// Randomクラスを表します。
/// </summary>
class Random {
public:
	/// <summary>
	/// 初期化します。
	/// </summary>
	static void Initialize() {
		// 乱数生成エンジン
		std::random_device seedGenerator;
		// メルセンヌ·ツイスターエンジンの初期化
		randomEngine.seed(seedGenerator());
	}

	/// <summary>
	/// Randomを取得します。
	/// </summary>
	template<typename T> static T GetRandom(T min, T max) {
		// スワップ
		if (min > max) {
			std::swap(min, max);
		}

		// 整数型か少数型かどうか
		if constexpr (std::is_integral_v<T>) {
			/// <summary>
			/// distributionを実行します。
			/// </summary>
			std::uniform_int_distribution<T> distribution(min, max);
			return distribution(randomEngine);
		/// <summary>
		/// constexprを実行します。
		/// </summary>
		} else if constexpr (std::is_floating_point_v<T>) {
			/// <summary>
			/// distributionを実行します。
			/// </summary>
			std::uniform_real_distribution<T> distribution(min, max);
			return distribution(randomEngine);
		} else {
			return min;
		}
	}

private:
	// メルセンヌ·ツイスターエンジン(64bit版)
	static std::mt19937_64 randomEngine;
};

} // namespace KujakuEngine