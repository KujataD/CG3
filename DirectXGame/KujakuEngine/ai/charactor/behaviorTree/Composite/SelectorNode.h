#pragma once
#include "CompositeNode.h"

namespace KujakuEngine {

/// <summary>
/// SelectorNodeクラスを表します。
/// </summary>
class SelectorNode : public CompositeNode {
protected:
	/// <summary>
	/// OnTickを実行します。
	/// </summary>
	BTStatus OnTick(AIContext& context) override;
};

} // namespace KujakuEngine
