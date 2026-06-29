#pragma once

namespace KujakuEngine {

template <class TOwner>
class IState {
public:
	virtual ~IState() = default;

	virtual void Enter(TOwner& owner) {}
	virtual void Update(TOwner& owner) {}
	virtual void Exit(TOwner& owner) {}
};

} // namespace KujakuEngine
