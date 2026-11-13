#pragma once
#include "Vector3.h"
#include <variant>

// =============================================================================
//  ボス攻撃の当たり判定（円・帯・扇・弾）
//  - 地面は XZ 平面（Y が上）
//  - 判定は「形状」と「半径つきの自機（円）」が重なるか
//  - 警告表示（GP-12）も同じ形状データから作る
// =============================================================================
namespace BossHit {

using Vec3 = TuboEngine::Math::Vector3;

// 円：ボス中心などの範囲攻撃
struct Circle {
	Vec3 center;
	float radius;
};
// 帯：origin から dir（正規化済み）へ length だけ伸びるビーム（幅 width）
struct Beam {
	Vec3 origin;
	Vec3 dir;
	float length;
	float width;
};
// 扇：forward（正規化済み）を中心に、全角 angle[rad]
struct Fan {
	Vec3 origin;
	Vec3 forward;
	float radius;
	float angle;
};
// 弾
struct Bullet {
	Vec3 pos;
	float radius;
};

// 警告・判定で形状を種類を問わず扱うための型
using Shape = std::variant<Circle, Beam, Fan>;

// 円：距離 ≤ 半径 + 自機の大きさ
bool HitCircle(const Circle& s, const Vec3& p, float playerR);
// 帯：長方形（幅 × 長さ）と自機の円が重なるか（端も含めて正確）
bool HitBeam(const Beam& s, const Vec3& p, float playerR);
// 扇：扇形と自機の円が重なるか
bool HitFan(const Fan& s, const Vec3& p, float playerR);
// 弾：距離 ≤ 弾の大きさ + 自機の大きさ
bool HitBullet(const Bullet& s, const Vec3& p, float playerR);

// 形状の種類を問わず自機との判定を行う
bool Hit(const Shape& shape, const Vec3& playerPos, float playerR);

// 形状を「中心から」t 倍に縮めたものを返す（t: 0〜1）。円・扇は半径、帯は長さが変わる
Shape ScaleShape(const Shape& shape, float t);

} // namespace BossHit
