#pragma once
#include <KujakuEngine.h>

/// <summary>
/// 魔法弾(MagicAbilitySetがランタイム生成する)。
/// Fire()で発射され、直進しながら敵(EnemyHealth持ち)に接触するとダメージを与えて消える。
/// 寿命切れでも消える。「消える」はSetActive(false)で、オブジェクトはプールとして再利用される
/// (Update中のオブジェクト破棄を避けるため。MagicAbilitySet側が非アクティブ弾を再発射する)。
/// 味方(PlayerHealth持ち)には当たらない(フレンドリーファイアなし)。
/// </summary>
class MagicProjectile : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "MagicProjectile"; }
	bool AllowMultiple() const override { return false; }

	void Update() override;
	void OnTriggerStay(KujakuEngine::ColliderComponent* other) override;

	/// <summary>発射する(位置は呼び出し側がTransformへ設定済みであること)。</summary>
	void Fire(const KujakuEngine::Vector3& direction, float speed, float lifetime, float damage);

private:
	// 進行方向(正規化済み)。
	KujakuEngine::Vector3 direction_ = {0.0f, 0.0f, 1.0f};
	float speed_ = 15.0f;
	// 残り寿命[s]。0以下で消える。
	float lifetime_ = 0.0f;
	float damage_ = 10.0f;
};
