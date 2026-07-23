#include "../KujakuEngine/KujakuEngine.h"
#include "../KujakuEngine/scene/SampleScene.h"
#include "../GameComponents/AllyAIBrain.h"
#include "../GameComponents/CharacterMotor.h"
#include "../GameComponents/CharacterSelectManager.h"
#include "../GameComponents/EnemyComponent.h"
#include "../GameComponents/MagicAbilitySet.h"
#include "../GameComponents/MagicProjectile.h"
#include "../GameComponents/MeleeAbilitySet.h"
#include "../GameComponents/PartyManager.h"
#include "../GameComponents/Player.h"
#include "../GameComponents/PlayerAnimator.h"
#include "../GameComponents/PlayerMoveComponent.h"
#include "../GameComponents/WeaponComponent.h"
#include "../GameComponents/EnemyHealth.h"
#include "../GameComponents/PlayerHealth.h"
#include "../GameComponents/EnemyWeapon.h"
#include "../GameComponents/HPBarUpdater.h"
#include "../GameComponents/ChangeSceneManager.h"
#include "../GameComponents/HammerEnemyComponent.h"
#include "../GameComponents/PlayerHPBarUpdater.h"
#include <memory>

namespace {

constexpr const char* kGameModuleName = "GameModule";

class GameModuleScene : public KujakuEngine::SampleScene {
public:
	~GameModuleScene() override = default;
};

class MoveForwardComponent : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "MoveForwardComponent"; }

	void Update() override {
		KujakuEngine::GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		owner->GetTransform().translation_.z += speed_;
	}

	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

private:
	KUJAKU_FIELD_FLOAT(speed_, 0.03f);
};

class BlinkComponent : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "BlinkComponent"; }

	void Initialize() override {
		SaveOriginalScale();
	}

	void Update() override {
		KujakuEngine::GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		++frameCount_;
		if (frameCount_ < intervalFrame_) {
			return;
		}

		frameCount_ = 0;
		visible_ = !visible_;

		KujakuEngine::WorldTransform& transform = owner->GetTransform();
		if (visible_) {
			transform.scale_ = originalScale_;
		} else {
			transform.scale_ = originalScale_ * 0.25f;
		}
	}

	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_INT(intervalFrame_, 1.0f, 1, 600);
	}

public:
	void OnAfterReadJson() override {
		SaveOriginalScale();
	}

private:
	void SaveOriginalScale() {
		KujakuEngine::GameObject* owner = GetOwner();
		if (!owner) {
			return;
		}

		originalScale_ = owner->GetTransform().scale_;
	}

private:
	KUJAKU_FIELD_INT(intervalFrame_, 30);
	int frameCount_ = 0;
	bool visible_ = true;
	KujakuEngine::Vector3 originalScale_ = {1.0f, 1.0f, 1.0f};
};

} // namespace

extern "C" __declspec(dllexport) void RegisterGameComponents(KujakuEngine::ComponentFactory& factory) {
	// GameModuleはゲーム固有Componentだけを登録する。
	// TransformやModelRendererなどの標準ComponentはEngine初期化時に登録される。
	factory.RegisterComponent<MoveForwardComponent>(kGameModuleName);
	factory.RegisterComponent<BlinkComponent>(kGameModuleName);
	factory.RegisterComponent<EnemyComponent>(kGameModuleName);
	factory.RegisterComponent<CharacterMotor>(kGameModuleName);
	factory.RegisterComponent<Player>(kGameModuleName);
	factory.RegisterComponent<AllyAIBrain>(kGameModuleName);
	factory.RegisterComponent<PartyManager>(kGameModuleName);
	factory.RegisterComponent<CharacterSelectManager>(kGameModuleName);
	factory.RegisterComponent<MeleeAbilitySet>(kGameModuleName);
	factory.RegisterComponent<MagicAbilitySet>(kGameModuleName);
	factory.RegisterComponent<MagicProjectile>(kGameModuleName);
	factory.RegisterComponent<PlayerAnimator>(kGameModuleName);
	factory.RegisterComponent<PlayerMoveComponent>(kGameModuleName);
	factory.RegisterComponent<WeaponComponent>(kGameModuleName);
	factory.RegisterComponent<EnemyHealth>(kGameModuleName);
	factory.RegisterComponent<PlayerHealth>(kGameModuleName);
	factory.RegisterComponent<HPBarUpdater>(kGameModuleName);
	factory.RegisterComponent<EnemyWeapon>(kGameModuleName);
	factory.RegisterComponent<ChangeSceneManager>(kGameModuleName);
	factory.RegisterComponent<HammerEnemyComponent>(kGameModuleName);
	factory.RegisterComponent<PlayerHPBarUpdater>(kGameModuleName);
}

extern "C" __declspec(dllexport) void UnregisterGameComponents(KujakuEngine::ComponentFactory& factory) {
	factory.UnregisterByModule(kGameModuleName);
}

extern "C" __declspec(dllexport) KujakuEngine::Scene* CreateGameScene() {
	return new GameModuleScene();
}

extern "C" __declspec(dllexport) void DestroyGameScene(KujakuEngine::Scene* scene) {
	delete scene;
}
