#pragma once

namespace KujakuEngine {
class Blackboard;

struct AIContext {
    Blackboard& localBB;
    Blackboard* worldBB = nullptr;

    bool HasWorldBB() const {
        return worldBB != nullptr;
    }
};
}
