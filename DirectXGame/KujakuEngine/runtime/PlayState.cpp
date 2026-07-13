#include "PlayState.h"

#include "EngineContext.h"

namespace KujakuEngine {

bool IsGamePlaying() { return GetEngineContext().gamePlaying; }

void SetGamePlaying(bool playing) { GetEngineContext().gamePlaying = playing; }

bool IsSceneViewFocused() { return GetEngineContext().sceneViewFocused; }

void SetSceneViewFocused(bool focused) { GetEngineContext().sceneViewFocused = focused; }

bool IsSceneViewVisible() { return GetEngineContext().sceneViewVisible; }

void SetSceneViewVisible(bool visible) { GetEngineContext().sceneViewVisible = visible; }

bool IsGameViewVisible() { return GetEngineContext().gameViewVisible; }

void SetGameViewVisible(bool visible) { GetEngineContext().gameViewVisible = visible; }

} // namespace KujakuEngine
