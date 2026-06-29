#include "Decorator.h"

namespace KujakuEngine {

void Decorator::SetChild(std::unique_ptr<BTNode> child) {
	child_ = std::move(child);
}

void Decorator::Reset() {
	if (child_) {
		child_->Reset();
	}
}

} // namespace KujakuEngine
