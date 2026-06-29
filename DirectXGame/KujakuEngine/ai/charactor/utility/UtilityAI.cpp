#include "UtilityAI.h"

namespace KujakuEngine {

void UtilityAI::AddOption(const UtilityOption& option) {
	options_.push_back(option);
	lastSelected_ = nullptr;
}

void UtilityAI::ClearOptions() {
	options_.clear();
	lastSelected_ = nullptr;
}

const UtilityOption* UtilityAI::SelectBest(AIContext& context) {
	UtilityOption* bestOption = nullptr;

	// スコアを計算して、最も高いスコアのオプションを選択
	for (UtilityOption& option : options_) {
		// スコア関数がない場合はスコア0とみなす
		if (!option.scoreFunc) {
			option.lastScore = 0.0f;
			continue;
		}

		// スコアを計算
		option.lastScore = option.scoreFunc(context);

		// 最小スコアを満たさないオプションは選択肢から除外
		if (option.lastScore < option.minScore) {
			continue;
		}

		// 最も高いスコアのオプションを更新
		if (!bestOption || option.lastScore > bestOption->lastScore) {
			bestOption = &option;
		}
	}

	// 最も高いスコアのオプションを保存
	lastSelected_ = bestOption;
	return lastSelected_;
}

} // namespace KujakuEngine
