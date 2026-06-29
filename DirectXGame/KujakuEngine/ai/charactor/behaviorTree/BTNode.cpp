#include "BTNode.h"

namespace KujakuEngine {

BTStatus BTNode::Tick(AIContext& context) {
	return OnTick(context);
}

void BTNode::Reset() {}

} // namespace KujakuEngine
