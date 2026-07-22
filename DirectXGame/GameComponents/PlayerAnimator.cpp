#include "PlayerAnimator.h"
#include "Player.h"

using namespace KujakuEngine;

void PlayerAnimator::OnPlayStart() {
	wasPressed_ = false;
	animator_ = GetComponent<AnimatorComponent>();

	// PlayerはコリジョンとともにPawn最上位へ、本Componentはモデル(子)側に分離されているため、
	// 自身に無ければ親を遡って探す。
	player_ = GetComponent<Player>();
	for (KujakuEngine::GameObject* ancestor = GetOwner() ? GetOwner()->GetParent() : nullptr; !player_ && ancestor; ancestor = ancestor->GetParent()) {
		player_ = ancestor->GetComponent<Player>();
	}
}

void PlayerAnimator::Update() {
	if (!animator_) {
		return;
	}

	// 硬直中(被弾のけぞり中)は攻撃入力を受け付けない(のけぞりアニメーションを上書きしない)。
	if (player_ && player_->IsStunned()) {
		wasPressed_ = false;
		return;
	}

	// R2(右トリガー)は0〜1のアナログ値なので、しきい値でボタン化してエッジ検出する。
	float rightTrigger = Input::GetRightTrigger();
	bool isPressed = rightTrigger >= triggerThreshold_;

	if (isPressed && !wasPressed_) {
		if (!animator_->IsPlaying()) {
			// 押した瞬間に再生。Clip Name指定xxがあればそのクリップへ切り替える。	
				animator_->Play();
			}
			else {
				if (!animator_->PlayByName(clipName_)) {
					// 名前が見つからない場合は選択中クリップを再生する。
					animator_->Play();
				}
			}
		}
	}

	if (stopOnRelease_ && !isPressed && wasPressed_) {
		animator_->Stop();
	}

	wasPressed_ = isPressed;
}
