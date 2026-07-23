#pragma once
#include <KujakuEngine.h>

/// <summary>
/// キャラクターの攻撃手段(技)の共通インターフェース。
/// 頭脳(Player=入力 / AllyAIBrain=BT)は「スロットNを使え」としか言わず、
/// それが剣振りか魔法弾かは実装側(MeleeAbilitySet / MagicAbilitySet)だけが知る。
/// GetComponent&lt;IAbilitySet&gt;()で実装型を問わず取得できる(dynamic_castベース)。
/// </summary>
class IAbilitySet : public KujakuEngine::Component {
public:
	/// <summary>
	/// スロットslotの技を使う。使えたらtrue(クールダウン中・行動不能中などはfalse)。
	/// </summary>
	virtual bool TryUse(int slot) = 0;

	/// <summary>技の実行中(モーション中・詠唱中など)か。頭脳側の次行動判断に使う。</summary>
	virtual bool IsBusy() const = 0;
};
