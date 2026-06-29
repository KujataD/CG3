#pragma once

#include "../Core/AIContext.h"
#include <functional>
#include <string>
#include <vector>

namespace KujakuEngine {

struct UtilityOption {
	std::string name;
	std::function<float(AIContext&)> scoreFunc;
	float lastScore = 0.0f;
	float minScore = 0.0f;
};

class UtilityAI {
public:
	void AddOption(const UtilityOption& option);
	void ClearOptions();

	const UtilityOption* SelectBest(AIContext& context);
	const UtilityOption* GetLastSelected() const { return lastSelected_; }
	const std::vector<UtilityOption>& GetOptions() const { return options_; }

private:
	std::vector<UtilityOption> options_;
	const UtilityOption* lastSelected_ = nullptr;
};

} // namespace KujakuEngine
