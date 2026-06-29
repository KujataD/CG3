#pragma once
#include "CompositeNode.h"

namespace KujakuEngine {

class SequenceNode : public CompositeNode {
protected:
	BTStatus OnTick(AIContext& context) override;
};

} // namespace KujakuEngine
