#pragma once

#include <KujakuEngine.h>
#include <BahamutAI/AI.h>


/// <summary>
/// Enemy用のゲーム固有Component。
/// </summary>
class EnemyComponent : public KujakuEngine::Component {
public:
enum class Fase{
	MoveForward,
	RotateY,
	MoveBackward,
	Forward,
};
public:
	const char* GetTypeName() const override { return "EnemyComponent"; }

	void Initialize() override;
	void OnPlayStart() override;
	void Update() override;

private:
	BahamutAI::BTStatus MoveForward(BahamutAI::AIContext& context);
	BahamutAI::BTStatus RotateY(BahamutAI::AIContext& context);
	BahamutAI::BTStatus MoveBackward(BahamutAI::AIContext& context);
	BahamutAI::BTStatus RotateX(BahamutAI::AIContext& context);

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_FLOAT(speed_, 0.02f);

	// BT作成用
	BahamutAI::BehaviorTreeFactory btFactory_;

	// BT実行本体（毎フレームTickする）
	BahamutAI::BehaviorTreeRuntime btRuntime_;

	// このEnemy固有のBlackboard（AIContextに渡す）
	BahamutAI::Blackboard localBlackboard_;
};
