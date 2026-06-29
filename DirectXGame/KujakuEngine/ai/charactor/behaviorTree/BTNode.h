#pragma once
#include "../Core/AIContext.h"
#include "BTStatus.h"

namespace KujakuEngine {

class BTNode {
public:
	virtual ~BTNode() = default;

	BTStatus Tick(AIContext& context);
	virtual void Reset();

protected:
	virtual BTStatus OnTick(AIContext& context) = 0;
};

} // namespace KujakuEngine
