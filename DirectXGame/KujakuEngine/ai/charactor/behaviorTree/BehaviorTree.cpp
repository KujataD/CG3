#include "BehaviorTree.h"

namespace KujakuEngine {

void BehaviorTree::SetRoot(std::unique_ptr<BTNode> root) {
	root_ = std::move(root);
}

BTStatus BehaviorTree::Tick(AIContext& context) {
	// ルートがいない場合は失敗とみなす
	if (!root_) {
		return BTStatus::Failure;
	}

	// ルートノードをTickする
	return root_->Tick(context);
}

void BehaviorTree::Reset() {
	if (root_) {
		root_->Reset();
	}
}

bool BehaviorTree::HasRoot() const {
	return root_ != nullptr;
}

} // namespace KujakuEngine
