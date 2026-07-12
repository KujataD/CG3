#pragma once

#include "KujakuApi.h"

namespace KujakuEngine {

class IAssetResolver;
class ISelectionProvider;

// エンジンのランタイムサービスを集約する注入コンテキスト(DIPの土台)。
// Editor/アプリが起動時に各サービスを設定し、ランタイムはここ経由で参照する。
// 個々のアクセサ(IsGamePlaying / GetAssetResolver / GetSelectionProvider)は
// このコンテキストの薄いラッパー。将来サービスを増やす時はここへフィールドを追加する。
struct EngineContext {
	// Play(実行)中かどうか。
	bool gamePlaying = false;
	// アセットID↔パスの解決(未設定時は各アクセサがフォールバックを使う)。
	IAssetResolver* assetResolver = nullptr;
	// Editorの選択状態の問い合わせ。
	ISelectionProvider* selectionProvider = nullptr;
};

// プロセス全体で唯一のEngineContextを取得する。
KUJAKU_API EngineContext& GetEngineContext();

} // namespace KujakuEngine
