#include "PlayState.h"

namespace KujakuEngine {

namespace {
bool g_gamePlaying = false;
}

bool IsGamePlaying() { return g_gamePlaying; }

void SetGamePlaying(bool playing) { g_gamePlaying = playing; }

} // namespace KujakuEngine
