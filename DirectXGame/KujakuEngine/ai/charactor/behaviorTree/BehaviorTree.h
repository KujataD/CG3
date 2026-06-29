#pragma once
#include "BTNode.h"
#include <memory>

namespace KujakuEngine {

class BehaviorTree {
public:
	void SetRoot(std::unique_ptr<BTNode> root);
	BTStatus Tick(AIContext& context);
	void Reset();
	bool HasRoot() const;

private:
	std::unique_ptr<BTNode> root_;
};

// 使用例
// Blackboard blackboard;
// AIContext context{.blackboard = &blackboard};
// auto root = std::make_unique<SelectorNode>();
// root->AddChild(std::make_unique<AttackNode>());
// BehaviorTree tree;
// tree.SetRoot(std::move(root));
// tree.Tick(context);

} // namespace KujakuEngine
