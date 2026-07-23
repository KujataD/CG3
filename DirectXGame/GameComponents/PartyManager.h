#pragma once
#include <KujakuEngine.h>

/// <summary>
/// パーティ(プレイアブル2人)の役割を一元管理するシーン常駐Component。
/// 「誰を操作するか(リーダー)」を決め、Play開始時に以下を行う:
///   - リーダー: 入力頭脳(Player)を有効化、AI頭脳(AllyAIBrain)を無効化
///   - 味方NPC: 入力頭脳を無効化、AI頭脳を有効化(エルデンリングの霊灰のような参戦)
///   - カメラ(OrbitCameraComponent)の追従先をリーダーへ向ける
///
/// キャラの体(CharacterMotor)は頭脳がどちらでも同じAPIで動くため、
/// リーダーを入れ替えても各キャラの挙動コードは変わらない。
/// Leader Nameを"Bishop"にすればBishop操作+Pawn参戦になる。
/// </summary>
class PartyManager : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "PartyManager"; }
	bool AllowMultiple() const override { return false; }

	void OnPlayStart() override;

	/// <summary>現在のリーダー(操作キャラ)。見つからなければnullptr。</summary>
	KujakuEngine::GameObject* GetLeader() const { return leader_; }

	/// <summary>味方NPC側のキャラ。見つからなければnullptr。</summary>
	KujakuEngine::GameObject* GetAlly() const { return ally_; }

private:
	/// <summary>両キャラの頭脳の有効/無効とカメラ追従先を、現在のリーダー設定に合わせて適用する。</summary>
	void ApplyRoles();
	/// <summary>characterの頭脳Componentを切り替える(isLeader=trueなら入力、falseならAI)。</summary>
	void SetBrainMode(KujakuEngine::GameObject* character, bool isLeader);

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_STRING_NAMED(leaderName_, "Leader Name");
		KUJAKU_REGISTER_STRING_NAMED(allyName_, "Ally Name");
		KUJAKU_REGISTER_STRING_NAMED(cameraName_, "Camera Name");
	}

	// 操作するキャラのGameObject名。
	KUJAKU_FIELD_STRING(leaderName_, "Pawn");
	// 味方NPCとして参戦するキャラのGameObject名。
	KUJAKU_FIELD_STRING(allyName_, "Bishop");
	// 追従カメラ(OrbitCameraComponent持ち)のGameObject名。
	KUJAKU_FIELD_STRING(cameraName_, "Main Camera");

	KujakuEngine::GameObject* leader_ = nullptr;
	KujakuEngine::GameObject* ally_ = nullptr;
};
