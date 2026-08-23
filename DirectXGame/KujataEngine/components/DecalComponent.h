#pragma once
#include "../3d/GraphicsPipeline.h"
#include "../3d/Model.h"
#include "../runtime/KujataApi.h"
#include "../scene/Component.h"

#include <memory>
#include <string>
#include <vector>

namespace KujataEngine {

class Camera;
class GameObject;

/// <summary>
/// 地形へ貼り付くデカール。**板ポリを空中に置くのではなく、格子の各頂点を真下へ
/// レイキャストして地面の高さに乗せる**ことで、傾斜や段差でも浮かない。
///
/// 板ポリ方式には2つの弱点があった。どちらもこの方式で消える。
///   - Yが地面とずれると中空に浮き、地面との間に隙間が見える
///   - 立体として扱われるので**影を落としてしまう**(地面の模様なのに影が出るのは明確な破綻)
///
/// 影が出ないのは、このコンポーネントが影パス(ModelRendererComponentだけを集める)に
/// 一切現れないため。描画は通常パスのみで、深度書き込みも既定でoffにしてある。
///
/// **地面の見分け方はGuardianGaitと同じ規則**にしてある(動かないコライダーだけを足場とみなし、
/// レイヤーマスクで除外もできる)。歩行とデカールで別々の地面を見て食い違うことがない。
/// </summary>
class KUJATA_API DecalComponent : public Component {
public:
	const char* GetTypeName() const override { return "DecalComponent"; }
	bool AllowMultiple() const override { return true; }

	void OnPlayStart() override;
	void Update() override;
	void Draw() override;

	/// <summary>描画に使うカメラ。シーン側が毎フレーム配る。</summary>
	void SetCamera(const Camera* camera) { camera_ = camera; }

	/// <summary>
	/// 外側の半径を差し替える(衝撃波のように毎フレーム広がるもの用)。
	/// 変えた次のUpdateで格子を組み直す。
	/// </summary>
	void SetRadius(float radius);
	float GetRadius() const { return radius_; }

	/// <summary>色を上書きする。マテリアルを触らずに濃さや色味を変えられる。</summary>
	void SetColorOverride(const Vector4& color);
	void ClearColorOverride() { hasColorOverride_ = false; }

	/// <summary>表示のON/OFF。消しても格子は保持するので、再表示は即座。</summary>
	/// <summary>
	/// この階層を地面判定から除外する(デカールを出した本体を指定する)。
	/// **本体の子ではなくシーン直下に生成されるデカールでは必須**。指定しないと相手の脚や胴に貼り付く。
	/// </summary>
	void SetIgnoreRoot(GameObject* root) { ignoreRoot_ = root; }

	void SetVisible(bool visible) { visible_ = visible; }
	bool IsVisible() const { return visible_; }

private:
	void EnsureModel();
	/// <summary>格子を組み直して頂点を差し替える。</summary>
	void RebuildMesh();
	/// <summary>四角形の格子(中心から半径ぶん広がる正方形)。</summary>
	void BuildQuad(std::vector<VertexData>& outVertices, const Vector3& center) const;
	/// <summary>円環の格子。UVのvが内周0→外周1になるので、リング用シェーダーがそのまま使える。</summary>
	void BuildRing(std::vector<VertexData>& outVertices, const Vector3& center) const;
	/// <summary>指定XZの真下にある地面の高さ。見つからなければ Ground Y。</summary>
	float SampleGroundHeight(float x, float z) const;
	/// <summary>この頂点数を賄うのに必要なバッファ長。</summary>
	uint32_t MaxVertexCount() const;

private:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		KUJATA_REGISTER_INT_NAMED_TIP(shape_, "Shape", 1.0f, 0, 1,
		    "0=四角 / 1=円環。**円環はUVのvが内周0→外周1**になるので、リング用シェーダーがそのまま使える。");
		KUJATA_REGISTER_STRING_NAMED_TIP(texturePath_, "Texture", "貼り付けるテクスチャ。空なら白。");
		KUJATA_REGISTER_INT_NAMED_TIP(shaderModel_, "Shader Model", 1.0f, 0, 7,
		    "描画に使うシェーダー分岐。衝撃波のリングは5。");
		KUJATA_REGISTER_INT_NAMED_TIP(blendMode_, "Blend Mode", 1.0f, 0, 3,
		    "0=通常 / 1=加算 / 2=減算 / 3=乗算。地面の汚れは通常か乗算、光る跡は加算。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(radius_, "Radius", 0.05f, 0.01f, 100.0f,
		    "四角なら中心からの半分の幅、円環なら外周の半径。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(innerRatio_, "Inner Ratio", 0.01f, 0.0f, 0.99f,
		    "円環の内周(外周に対する割合)。0で中心まで埋まった円になる。");
		KUJATA_REGISTER_INT_NAMED_TIP(subdivision_, "Subdivision", 1.0f, 3, 128,
		    "格子の分割数。**多いほど地形に沿うが、頂点ごとにレイキャストするので重くなる**。\n"
		    "平らな床なら8〜16で十分。起伏があるほど増やす。");
		KUJATA_REGISTER_INT_NAMED_TIP(radialSteps_, "Radial Steps", 1.0f, 1, 32,
		    "円環の半径方向の分割数。内周から外周までを何段に割るか。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(groundOffset_, "Ground Offset", 0.001f, 0.0f, 1.0f,
		    "地面からどれだけ浮かせるか。**Zファイティング(地面とちらつく)を避けるための最小限の隙間**。\n"
		    "大きくすると今度は浮いて見えるので、0.02〜0.05程度に留める。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(rayUp_, "Ray Up", 0.1f, 0.0f, 100.0f, "地面を探すレイをどれだけ上から撃つか。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(rayDown_, "Ray Down", 0.1f, 0.1f, 200.0f, "そこから下へ何ユニット探すか。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(groundY_, "Ground Y", 0.05f, -100.0f, 100.0f,
		    "レイが何にも当たらなかったときに使う高さ。");
		KUJATA_REGISTER_BOOL_NAMED_TIP(staticGroundOnly_, "Static Ground Only",
		    "動かないコライダーだけを地面とみなす。offにするとプレイヤーや敵の上にも貼り付いてしまう。");
		KUJATA_REGISTER_UINT32_NAMED_TIP(groundLayerMask_, "Ground Layer Mask", 1.0f, 0u, 4294967295u,
		    "地面とみなすレイヤーのビットマスク。**GuardianGaitと同じ値にしておくこと**。\n"
		    "食い違うと、脚が乗っている面とデカールが貼られる面が別になる。");
		KUJATA_REGISTER_BOOL_NAMED_TIP(followOwner_, "Follow Owner",
		    "onならオーナーが動くたびに格子を組み直して追従する。\n"
		    "焦げ跡のようにその場へ残すものはoffにすると、置いた瞬間の形のまま固定できて軽い。");
	}

	KUJATA_FIELD_INT(shape_, 1);
	KUJATA_FIELD_STRING(texturePath_, "Resources/white1x1.png");
	KUJATA_FIELD_INT(shaderModel_, 0);
	KUJATA_FIELD_INT(blendMode_, 0);
	KUJATA_FIELD_FLOAT(radius_, 3.0f);
	KUJATA_FIELD_FLOAT(innerRatio_, 0.55f);
	KUJATA_FIELD_INT(subdivision_, 32);
	KUJATA_FIELD_INT(radialSteps_, 3);
	KUJATA_FIELD_FLOAT(groundOffset_, 0.03f);
	KUJATA_FIELD_FLOAT(rayUp_, 4.0f);
	KUJATA_FIELD_FLOAT(rayDown_, 12.0f);
	KUJATA_FIELD_FLOAT(groundY_, 0.0f);
	KUJATA_FIELD_BOOL(staticGroundOnly_, true);
	KUJATA_FIELD_UINT32(groundLayerMask_, 0xffffffffu);
	KUJATA_FIELD_BOOL(followOwner_, true);

	// --- 実行状態 ---
	std::unique_ptr<Model> model_;
	const Camera* camera_ = nullptr;
	// 描画用の器。頂点はワールド座標で作るので、この行列は常に単位のまま。
	WorldTransform identityTransform_{};
	bool transformReady_ = false;
	bool visible_ = true;
	bool meshDirty_ = true;
	// 前回組んだときの中心。動いていなければ組み直さない。
	Vector3 lastCenter_ = {0.0f, 0.0f, 0.0f};
	float lastRadius_ = -1.0f;
	bool hasColorOverride_ = false;
	Vector4 colorOverride_ = {1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<VertexData> vertices_;
	// 地面判定から外す階層(デカールを出した本体)。
	GameObject* ignoreRoot_ = nullptr;
};

} // namespace KujataEngine
