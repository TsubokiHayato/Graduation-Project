#pragma once
#include "Camera.h"
#include "Sprite.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <array>
#include <memory>
#include <vector>

/// <summary>
/// 画面中央のレティクル。中心の「＋」と内側の円、その外側に
/// 4部位（近接・遠距離・シールド・ブースター）の出力配分を示す時計回りの円弧ゲージを描く。
/// 円弧の長さは出力（%）に比例する（シールド100%なら青が一周）。
/// 白テクスチャの小さなスプライトを回転させて並べて円弧を作る。
/// </summary>
class Reticle {
public:
	/// <summary>出力を割り振る部位。並び順が円弧の並び順（上から時計回り）</summary>
	enum class Part { Melee, Ranged, Shield, Booster, Count };
	static constexpr int kPartCount = static_cast<int>(Part::Count);

	void Initialize();
	/// <summary>表示を目標の出力配分へなめらかに追従させ、スプライトを配置する</summary>
	void Update(float deltaTime);
	void Draw();
	void DrawImGui();

	/// <summary>
	/// 各部位の出力（%）を設定する。合計が100でなくても割合に直して表示する
	/// </summary>
	void SetOutputs(const std::array<float, kPartCount>& outputs);
	void SetOutput(Part part, float percent);
	float GetOutput(Part part) const { return targets_[static_cast<int>(part)]; }

	/// <summary>ロックオンしていないときの位置（画面座標）。通常は画面中央</summary>
	void SetCenter(const TuboEngine::Math::Vector2& center) { center_ = center; }

	/// <summary>
	/// ロックオン中は毎フレーム呼ぶ。敵のワールド座標を画面座標に直し、レティクルをそこへ寄せる。
	/// 画面外（カメラの後ろを含む）のときは画面の端に張り付く
	/// </summary>
	void SetLockOn(const TuboEngine::Math::Vector3& targetWorld, const TuboEngine::Camera& camera);
	/// <summary>ロックオン解除。レティクルは中央へ戻る</summary>
	void ClearLockOn() { isLockedOn_ = false; }
	bool IsLockedOn() const { return isLockedOn_; }

private:
	/// <summary>ワールド座標 → 画面座標。カメラの後ろなら false</summary>
	static bool WorldToScreen(const TuboEngine::Math::Vector3& world, const TuboEngine::Camera& camera, TuboEngine::Math::Vector2& out);
	/// <summary>ロックオンの角かっこ（4隅の L 字）を置く</summary>
	void PlaceLockOnBrackets();

	/// <summary>中心 center_ から半径 radius、角度 angle（ラジアン、0=右・時計回り）の画面座標</summary>
	TuboEngine::Math::Vector2 PointOnCircle(float radius, float angle) const;
	/// <summary>スプライトを1枚取り出して配置する（足りなければプールに追加）</summary>
	void AcquireSprite(TuboEngine::Math::Vector2 mid, float angle, TuboEngine::Math::Vector2 size, const TuboEngine::Math::Vector4& color);
	/// <summary>線分（中点・向き・長さ・太さ）を描く予定に積む。縁取りは Update の最後にまとめて付ける</summary>
	void PlaceBar(TuboEngine::Math::Vector2 mid, float angle, float length, float thickness, const TuboEngine::Math::Vector4& color);
	/// <summary>円弧を細かいバーの連なりで置く（startAngle から時計回りに sweep）</summary>
	void PlaceArc(float radius, float startAngle, float sweep, float thickness, const TuboEngine::Math::Vector4& color);
	/// <summary>円周上の endAngle に時計回り向きの矢じり（く の字）を置く</summary>
	void PlaceArrowHead(float radius, float endAngle, const TuboEngine::Math::Vector4& color);

private:
	// 使い回すスプライト（毎フレーム先頭から必要な分だけ使う）
	std::vector<std::unique_ptr<TuboEngine::Sprite>> pool_;
	size_t used_ = 0;

	// このフレームに描く線分（縁取り → 本体の順にスプライトへ割り当てる）
	struct Bar {
		TuboEngine::Math::Vector2 mid;
		float angle;
		float length;
		float thickness;
		TuboEngine::Math::Vector4 color;
	};
	std::vector<Bar> bars_;

	// 出力配分（%）。targets_ が目標、displayed_ が表示中（なめらかに追従）
	std::array<float, kPartCount> targets_ = {25.0f, 25.0f, 25.0f, 25.0f};
	std::array<float, kPartCount> displayed_ = {25.0f, 25.0f, 25.0f, 25.0f};

	// 部位ごとの色（近接=黄, 遠距離=緑, シールド=青, ブースター=赤）
	std::array<TuboEngine::Math::Vector4, kPartCount> colors_ = {{
	    {1.00f, 0.80f, 0.10f, 1.0f},
	    {0.20f, 0.85f, 0.30f, 1.0f},
	    {0.20f, 0.60f, 1.00f, 1.0f},
	    {1.00f, 0.25f, 0.20f, 1.0f},
	}};

	// 見た目のパラメータ（ImGui で調整できる）
	TuboEngine::Math::Vector2 center_ = {640.0f, 360.0f};
	float innerRadius_ = 48.0f;    // 内側の円の半径
	float innerThickness_ = 3.0f;  // 内側の円の太さ
	float crossSize_ = 16.0f;      // 中心の「＋」の長さ
	float crossThickness_ = 3.0f;
	float gaugeRadius_ = 64.0f;    // 出力ゲージの円弧の半径
	float gaugeThickness_ = 6.0f;
	float gapDegrees_ = 6.0f;      // 部位と部位のすき間（度）
	float arrowSize_ = 12.0f;      // 矢じりの辺の長さ
	float segmentDegrees_ = 3.0f;  // 円弧を作るバー1本あたりの角度（小さいほどなめらか）
	float followSpeed_ = 12.0f;    // 表示が目標に追従する速さ
	TuboEngine::Math::Vector4 baseColor_ = {1.0f, 1.0f, 1.0f, 0.95f};    // 円と「＋」の色
	TuboEngine::Math::Vector4 trackColor_ = {0.6f, 0.6f, 0.6f, 0.35f};   // 100% 分の土台の輪
	TuboEngine::Math::Vector4 outlineColor_ = {0.0f, 0.0f, 0.0f, 0.7f};  // 縁取りの色（アルファは本体のアルファと掛け合わせる）
	float outlineWidth_ = 1.5f;    // 縁取りの太さ（片側, px）。0 で縁取りなし

	// ロックオン
	bool isLockedOn_ = false;
	TuboEngine::Math::Vector2 lockOnScreen_ = {};   // 敵の画面座標（画面外なら端に寄せた座標）
	bool lockOnOffScreen_ = false;                  // 敵が画面外にいる
	TuboEngine::Math::Vector2 drawCenter_ = {640.0f, 360.0f}; // 実際に描く中心（center_ と敵の間をなめらかに動く）
	float lockOnBlend_ = 0.0f;     // 0 = 解除, 1 = ロック完了（角かっこの演出に使う）
	float moveSpeed_ = 14.0f;      // レティクルが敵へ／中央へ動く速さ
	float lockOnSpeed_ = 8.0f;     // 角かっこが閉じる速さ
	float bracketRadius_ = 84.0f;  // ロック完了時の角かっこの中心からの距離
	float bracketLength_ = 16.0f;  // 角かっこ1辺の長さ
	float bracketThickness_ = 3.0f;
	float screenMargin_ = 90.0f;   // 画面外の敵を指すとき、画面の端から離す距離
	TuboEngine::Math::Vector4 lockOnColor_ = {1.0f, 0.2f, 0.2f, 1.0f};
};
