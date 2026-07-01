#pragma once

namespace KujakuEngine {

/// <summary>
/// IStateクラスを表します。
/// </summary>
template <class TOwner>
class IState {
public:
	/// <summary>
	/// IStateを実行します。
	/// </summary>
	virtual ~IState() = default;

	virtual void Enter(TOwner& owner) {}
	virtual void Update(TOwner& owner) {}
	virtual void Exit(TOwner& owner) {}
};

} // namespace KujakuEngine
