#include "UIInput.h"

namespace KujataEngine {
namespace {
UIPointerState gPointer;
} // namespace

void SetUIPointer(const UIPointerState& state) { gPointer = state; }
const UIPointerState& GetUIPointer() { return gPointer; }

} // namespace KujataEngine
