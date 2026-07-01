#pragma once
#include "LeafNode.h"

#include <functional>

namespace KujakuEngine {

/// <summary>
/// ActionNodeクラスを表します。
/// </summary>
class ActionNode : public LeafNode {
public:
	using Action = std::function<BTStatus(AIContext&)>;

	/// <summary>
	/// ActionNodeを実行します。
	/// </summary>
	ActionNode() = default;
	/// <summary>
	/// ActionNodeを実行します。
	/// </summary>
	explicit ActionNode(Action action);

	/// <summary>
	/// Actionを設定します。
	/// </summary>
	void SetAction(Action action);

protected:
	/// <summary>
	/// OnTickを実行します。
	/// </summary>
	BTStatus OnTick(AIContext& context) override;

private:
	Action action_;
};

} // namespace KujakuEngine
