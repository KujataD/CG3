#pragma once
#include "BTNode.h"
#include <memory>

namespace KujakuEngine {

/// <summary>
/// BehaviorTreeクラスを表します。
/// </summary>
class BehaviorTree {
public:
	/// <summary>
	/// Rootを設定します。
	/// </summary>
	void SetRoot(std::unique_ptr<BTNode> root);
	/// <summary>
	/// 1回分の処理を実行します。
	/// </summary>
	BTStatus Tick(AIContext& context);
	/// <summary>
	/// 状態をリセットします。
	/// </summary>
	void Reset();
	/// <summary>
	/// Rootを持つかどうかを返します。
	/// </summary>
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
