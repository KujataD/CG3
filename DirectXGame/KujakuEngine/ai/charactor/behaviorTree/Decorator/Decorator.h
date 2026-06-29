#pragma once
#include "../BTNode.h"
#include <memory>

namespace KujakuEngine {

class Decorator : public BTNode {
public:
	void SetChild(std::unique_ptr<BTNode> child);
	void Reset() override;

protected:
	std::unique_ptr<BTNode> child_;
};

} // namespace KujakuEngine
