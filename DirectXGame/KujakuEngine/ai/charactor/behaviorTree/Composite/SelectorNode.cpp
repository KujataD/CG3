#include "SelectorNode.h"

namespace KujakuEngine {

BTStatus SelectorNode::OnTick(AIContext& context) {
	// 子がいない場合は失敗とみなす
	if (children_.empty()) {
		return BTStatus::Failure;
	}

	// 順番に子をTickしていく
	while (currentIndex_ < children_.size()) {
		// 子の状態を取得
		const BTStatus status = children_[currentIndex_]->Tick(context);

		// 子が失敗した場合は次の子へ
		if (status == BTStatus::Failure) {
			++currentIndex_;
			continue;
		}

		// 子が実行中の場合はセレクタ全体も実行中
		if (status == BTStatus::Running) {
			return BTStatus::Running;
		}

		// 子が成功した場合はセレクタ全体も成功
		currentIndex_ = 0;
		return BTStatus::Success;
	}

	// 全ての子が失敗した場合はセレクタ全体も失敗
	currentIndex_ = 0;
	return BTStatus::Failure;
}

} // namespace KujakuEngine
