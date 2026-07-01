#pragma once
#include "../Core/AIContext.h"
#include "BTStatus.h"

namespace KujakuEngine {

/// <summary>
/// BTNodeクラスを表します。
/// </summary>
class BTNode {
public:
	/// <summary>
	/// BTNodeを実行します。
	/// </summary>
	virtual ~BTNode() = default;

	/// <summary>
	/// 1回分の処理を実行します。
	/// </summary>
	BTStatus Tick(AIContext& context);
	/// <summary>
	/// 状態をリセットします。
	/// </summary>
	virtual void Reset();

protected:
	/// <summary>
	/// OnTickを実行します。
	/// </summary>
	virtual BTStatus OnTick(AIContext& context) = 0;
};

} // namespace KujakuEngine
