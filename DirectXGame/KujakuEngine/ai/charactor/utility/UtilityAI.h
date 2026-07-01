#pragma once

#include "../Core/AIContext.h"
#include <functional>
#include <string>
#include <vector>

namespace KujakuEngine {

/// <summary>
/// UtilityOption構造体を表します。
/// </summary>
struct UtilityOption {
	std::string name;
	std::function<float(AIContext&)> scoreFunc;
	float lastScore = 0.0f;
	float minScore = 0.0f;
};

/// <summary>
/// UtilityAIクラスを表します。
/// </summary>
class UtilityAI {
public:
	/// <summary>
	/// Optionを追加します。
	/// </summary>
	void AddOption(const UtilityOption& option);
	/// <summary>
	/// ClearOptionsを実行します。
	/// </summary>
	void ClearOptions();

	/// <summary>
	/// SelectBestを実行します。
	/// </summary>
	const UtilityOption* SelectBest(AIContext& context);
	/// <summary>
	/// LastSelectedを取得します。
	/// </summary>
	const UtilityOption* GetLastSelected() const { return lastSelected_; }
	/// <summary>
	/// Optionsを取得します。
	/// </summary>
	const std::vector<UtilityOption>& GetOptions() const { return options_; }

private:
	std::vector<UtilityOption> options_;
	const UtilityOption* lastSelected_ = nullptr;
};

} // namespace KujakuEngine
