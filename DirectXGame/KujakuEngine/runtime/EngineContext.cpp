#include "EngineContext.h"

namespace KujakuEngine {

EngineContext& GetEngineContext() {
	static EngineContext context;
	return context;
}

} // namespace KujakuEngine
