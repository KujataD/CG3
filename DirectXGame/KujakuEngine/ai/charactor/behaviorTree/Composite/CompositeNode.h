#pragma once
#include "../BTNode.h"
#include <memory>
#include <vector>

namespace KujakuEngine {

class CompositeNode : public BTNode {
public:
	void AddChild(std::unique_ptr<BTNode> child);
	void Reset() override;
	bool HasChildren() const;
	size_t GetChildCount() const;

protected:
	std::vector<std::unique_ptr<BTNode>> children_;
	size_t currentIndex_ = 0;
};

} // namespace KujakuEngine
