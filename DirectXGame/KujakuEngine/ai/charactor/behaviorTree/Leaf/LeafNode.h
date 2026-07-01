#pragma once
#include "../BTNode.h"

namespace KujakuEngine {

/// <summary>
/// LeafNodeクラスを表します。
/// </summary>
class LeafNode : public BTNode {
public:
	/// <summary>
	/// LeafNodeを実行します。
	/// </summary>
	~LeafNode() override = default;
};

} // namespace KujakuEngine
