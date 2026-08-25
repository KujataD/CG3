#pragma once

#include "../runtime/KujataApi.h"
#include "../scene/Component.h"

namespace KujataEngine {

/// <summary>
/// FractalNoise(math/Noise.h)から岩肌/土埃のテクスチャを起動時にメモリ生成し、
/// 同じGameObjectのModelRendererComponentまたはParticleSystemComponentへ直接差し込む。
/// ファイルには保存しない一回限りの手続きテクスチャ(NoiseTexture.h参照)。
/// </summary>
class KUJATA_API NoiseTextureComponent : public Component {
public:
	enum class Kind {
		// ModelRendererComponentのBaseColorへ適用する岩肌テクスチャ。
		RockAlbedo = 0,
		// ParticleSystemComponentの粒テクスチャへ適用する土埃スプライト。
		DustSprite = 1,
		// ModelRendererComponentのBaseColorへ適用する土(地面)テクスチャ。
		SoilAlbedo = 2,
	};

	const char* GetTypeName() const override { return "NoiseTextureComponent"; }
	bool AllowMultiple() const override { return false; }

	void Initialize() override;

	/// <summary>
	/// プレハブ経由でInstantiateされた場合、Initialize()はAddComponent直後(=まだJSONの
	/// 値が反映される前)に走ってしまう。デフォルト値(kind=RockAlbedo等)で一度実行され、
	/// 対応するコンポーネントが無いと何も適用されないまま終わる。
	/// JSON反映後に必ず呼ばれるこのフックでも実行し直すことで取りこぼしを防ぐ。
	/// </summary>
	void OnAfterReadJson() override { RegenerateAndApply(); }

	/// <summary>
	/// 今の設定でテクスチャを作り直し、同じGameObjectのレンダラーへ適用し直す。
	/// InspectorのRegenerateボタン、または外部から見た目を変えたい時に呼ぶ。
	/// </summary>
	void RegenerateAndApply();

	void RegisterInvokableMethods(InvokableMethodRegistry& registry) override;

private:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		KUJATA_REGISTER_INT_NAMED_TIP(kind_, "Kind", 1.0f, 0, 2,
		    "0 = 岩肌(ModelRendererComponentへ) / 1 = 土埃(ParticleSystemComponentへ) / 2 = 土(ModelRendererComponentへ)。\n"
		    "対応するコンポーネントが同じGameObjectに無ければ何も起きない。");
		KUJATA_REGISTER_INT_NAMED_TIP(textureSize_, "Texture Size", 1.0f, 8, 1024,
		    "生成するテクスチャの一辺のピクセル数。大きいほど高精細だが生成が重くなる。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(seedLane_, "Seed", 1.0f, -1000.0f, 1000.0f,
		    "ノイズを読み出す位置をずらす値。同じ設定のまま模様だけ変えたい時に使う。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(uvTiling_, "UV Tiling", 0.5f, 1.0f, 200.0f,
		    "岩肌/土(ModelRendererComponent)にのみ効く。プリミティブのUVは常に0..1固定で\n"
		    "Transform.scaleでは伸びないため、地面のような巨大なオブジェクトでは大きくしないと\n"
		    "模様が1枚だけ間延びして一色に見える。値だけ繰り返し敷き詰められる。");
	}

	KUJATA_FIELD_INT(kind_, 0);
	KUJATA_FIELD_INT(textureSize_, 256);
	KUJATA_FIELD_FLOAT(seedLane_, 0.0f);
	KUJATA_FIELD_FLOAT(uvTiling_, 1.0f);
};

} // namespace KujataEngine
