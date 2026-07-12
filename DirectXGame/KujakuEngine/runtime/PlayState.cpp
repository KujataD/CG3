#include "PlayState.h"

#include "EngineContext.h"

namespace KujakuEngine {

bool IsGamePlaying() { return GetEngineContext().gamePlaying; }

void SetGamePlaying(bool playing) { GetEngineContext().gamePlaying = playing; }

} // namespace KujakuEngine
