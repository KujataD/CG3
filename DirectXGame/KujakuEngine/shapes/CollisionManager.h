#pragma once
#include <list>
#include "Collider.h"

namespace KujakuEngine{

class CollisionManager {
public:

	void Clear();

	void Update();

	void CheckCollisionPair(Collider* colliderA, Collider* colliderB);

	// setter
	void AddCollider(Collider* collider) { colliders_.push_back(collider); }

private:
	std::list<Collider*> colliders_;
};

}
