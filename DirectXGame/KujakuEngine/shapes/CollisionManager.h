#pragma once
#include <list>
#include "Collider.h"

namespace KujakuEngine{

/// <summary>
/// CollisionManagerクラスを表します。
/// </summary>
class CollisionManager {
public:

	/// <summary>
	/// 状態をクリアします。
	/// </summary>
	void Clear();

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update();

	/// <summary>
	/// CheckCollisionPairを実行します。
	/// </summary>
	void CheckCollisionPair(Collider* colliderA, Collider* colliderB);

	// setter
	void AddCollider(Collider* collider) { colliders_.push_back(collider); }

private:
	std::list<Collider*> colliders_;
};

}
