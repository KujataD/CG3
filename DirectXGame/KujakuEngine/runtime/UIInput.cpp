#include "UIInput.h"

namespace KujakuEngine {
namespace {
UIPointerState gPointer;
} // namespace

void SetUIPointer(const UIPointerState& state) { gPointer = state; }
const UIPointerState& GetUIPointer() { return gPointer; }

} // namespace KujakuEngine
