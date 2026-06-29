#pragma once
#include "CompositeNode.h"

namespace KujakuEngine {

class SelectorNode : public CompositeNode {
protected:
	BTStatus OnTick(AIContext& context) override;
};

} // namespace KujakuEngine
