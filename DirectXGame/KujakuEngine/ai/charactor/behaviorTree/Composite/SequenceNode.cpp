#include "SequenceNode.h"

namespace KujakuEngine {

BTStatus SequenceNode::OnTick(AIContext& context) {
	// 子がいない場合は成功とみなす
	if (children_.empty()) {
		return BTStatus::Success;
	}

	// 順番に子をTickしていく
	while (currentIndex_ < children_.size()) {
		// 子の状態を取得
		const BTStatus status = children_[currentIndex_]->Tick(context);

		// 子が成功した場合は次の子へ
		if (status == BTStatus::Success) {
			++currentIndex_;
			continue;
		}

		// 子が実行中の場合はシーケンス全体も実行中
		if (status == BTStatus::Running) {
			return BTStatus::Running;
		}

		// 子が失敗した場合はシーケンス全体が失敗
		currentIndex_ = 0;
		return BTStatus::Failure;
	}

	// 全ての子が成功した場合はシーケンス全体も成功
	currentIndex_ = 0;
	return BTStatus::Success;
}

} // namespace KujakuEngine
