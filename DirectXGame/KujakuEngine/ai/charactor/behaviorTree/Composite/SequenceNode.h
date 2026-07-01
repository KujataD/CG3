#pragma once
#include "CompositeNode.h"

namespace KujakuEngine {

/// <summary>
/// SequenceNodeクラスを表します。
/// </summary>
class SequenceNode : public CompositeNode {
protected:
	/// <summary>
	/// OnTickを実行します。
	/// </summary>
	BTStatus OnTick(AIContext& context) override;
};

} // namespace KujakuEngine
