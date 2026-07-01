#pragma once
#include "../BTNode.h"
#include <memory>
#include <vector>

namespace KujakuEngine {

/// <summary>
/// CompositeNodeクラスを表します。
/// </summary>
class CompositeNode : public BTNode {
public:
	/// <summary>
	/// Childを追加します。
	/// </summary>
	void AddChild(std::unique_ptr<BTNode> child);
	/// <summary>
	/// 状態をリセットします。
	/// </summary>
	void Reset() override;
	/// <summary>
	/// Childrenを持つかどうかを返します。
	/// </summary>
	bool HasChildren() const;
	/// <summary>
	/// ChildCountを取得します。
	/// </summary>
	size_t GetChildCount() const;

protected:
	std::vector<std::unique_ptr<BTNode>> children_;
	size_t currentIndex_ = 0;
};

} // namespace KujakuEngine
