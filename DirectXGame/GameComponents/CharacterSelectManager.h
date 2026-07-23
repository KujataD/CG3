#pragma once
#include <KujakuEngine.h>

/// <summary>
/// キャラ選択シーンの進行役。各キャラの「選択」ボタン(Button.onClick)から
/// SelectPawn / SelectBishop を呼ばれると、選ばれた方をPartySelectionへ記録して
/// 次シーン(SampleScene)へ遷移する。もう片方はPartyManagerが自動的に味方NPCにする。
/// </summary>
class CharacterSelectManager : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "CharacterSelectManager"; }
	bool AllowMultiple() const override { return false; }

	void RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) override;

private:
	/// <summary>leaderNameを操作キャラとして記録し、次シーンへ遷移する。</summary>
	void Select(const std::string& leaderName);

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_STRING_NAMED(pawnName_, "Pawn Name");
		KUJAKU_REGISTER_STRING_NAMED(bishopName_, "Bishop Name");
		KUJAKU_REGISTER_STRING_NAMED(nextSceneName_, "Next Scene");
	}

	// 次シーンでのキャラのGameObject名(PartyManagerのLeader/Ally Nameと一致させる)。
	KUJAKU_FIELD_STRING(pawnName_, "Pawn");
	KUJAKU_FIELD_STRING(bishopName_, "Bishop");
	// 選択後に遷移するシーン名。
	KUJAKU_FIELD_STRING(nextSceneName_, "SampleScene");
};
