#pragma once
#include "LeafNode.h"

#include <functional>

namespace KujakuEngine {

class ActionNode : public LeafNode {
public:
	using Action = std::function<BTStatus(AIContext&)>;

	ActionNode() = default;
	explicit ActionNode(Action action);

	void SetAction(Action action);

protected:
	BTStatus OnTick(AIContext& context) override;

private:
	Action action_;
};

} // namespace KujakuEngine
