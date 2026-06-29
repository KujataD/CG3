#include "CompositeNode.h"

namespace KujakuEngine {

void CompositeNode::AddChild(std::unique_ptr<BTNode> child) {
	if (!child) {
		return;
	}

	children_.push_back(std::move(child));
}

void CompositeNode::Reset() {
	currentIndex_ = 0;
	for (auto& child : children_) {
		child->Reset();
	}
}

bool CompositeNode::HasChildren() const {
	return !children_.empty();
}

size_t CompositeNode::GetChildCount() const {
	return children_.size();
}

} // namespace KujakuEngine
