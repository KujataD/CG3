#pragma once

#include "IState.h"
#include <memory>
#include <unordered_map>

namespace KujakuEngine {

/// <summary>
/// StateMachineクラスを表します。
/// </summary>
template <class TStateId, class TOwner>
class StateMachine {
public:
	/// <summary>
	/// Stateを追加します。
	/// </summary>
	void AddState(TStateId id, std::unique_ptr<IState<TOwner>> state) {
		states_[id] = std::move(state);
	}

	/// <summary>
	/// ChangeStateを実行します。
	/// </summary>
	void ChangeState(TStateId nextId, TOwner& owner) {
		if (currentState_ && currentId_ == nextId) {
			return;
		}

		if (currentState_) {
			currentState_->Exit(owner);
		}

		currentId_ = nextId;

		auto it = states_.find(nextId);
		currentState_ = it != states_.end() ? it->second.get() : nullptr;

		if (currentState_) {
			currentState_->Enter(owner);
		}
	}

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update(TOwner& owner) {
		if (currentState_) {
			currentState_->Update(owner);
		}
	}

	/// <summary>
	/// CurrentIdを取得します。
	/// </summary>
	TStateId GetCurrentId() const { return currentId_; }

private:
	std::unordered_map<TStateId, std::unique_ptr<IState<TOwner>>> states_;
	IState<TOwner>* currentState_ = nullptr;
	TStateId currentId_{};
};

} // namespace KujakuEngine
