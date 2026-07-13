#include "PlayState.h"

#include "EngineContext.h"

namespace KujakuEngine {

bool IsGamePlaying() { return GetEngineContext().gamePlaying; }

void SetGamePlaying(bool playing) { GetEngineContext().gamePlaying = playing; }

bool IsSceneViewFocused() { return GetEngineContext().sceneViewFocused; }

void SetSceneViewFocused(bool focused) { GetEngineContext().sceneViewFocused = focused; }

} // namespace KujakuEngine
