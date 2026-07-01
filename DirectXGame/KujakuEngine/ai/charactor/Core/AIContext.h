#pragma once

namespace KujakuEngine {
class Blackboard;

/// <summary>
/// AIContext構造体を表します。
/// </summary>
struct AIContext {
    Blackboard& localBB;
    Blackboard* worldBB = nullptr;

    /// <summary>
    /// WorldBBを持つかどうかを返します。
    /// </summary>
    bool HasWorldBB() const {
        return worldBB != nullptr;
    }
};
}
