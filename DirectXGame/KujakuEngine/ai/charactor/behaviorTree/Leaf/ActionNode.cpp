#include "ActionNode.h"

#include <utility>

namespace KujakuEngine {

ActionNode::ActionNode(Action action) : action_(std::move(action)) {}

void ActionNode::SetAction(Action action) { action_ = std::move(action); }

BTStatus ActionNode::OnTick(AIContext& context) {
	if (!action_) {
		return BTStatus::Failure;
	}

	return action_(context);
}

} // namespace KujakuEngine
