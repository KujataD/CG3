#pragma once
#include "../BTNode.h"
#include <memory>

namespace KujakuEngine {

/// <summary>
/// Decoratorクラスを表します。
/// </summary>
class Decorator : public BTNode {
public:
	/// <summary>
	/// Childを設定します。
	/// </summary>
	void SetChild(std::unique_ptr<BTNode> child);
	/// <summary>
	/// 状態をリセットします。
	/// </summary>
	void Reset() override;

protected:
	std::unique_ptr<BTNode> child_;
};

} // namespace KujakuEngine
