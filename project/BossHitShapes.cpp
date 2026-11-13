#include "BossHitShapes.h"
#include <cmath>
#include <type_traits>

namespace BossHit {

bool HitCircle(const Circle& s, const Vec3& p, float playerR) {
	float dx = p.x - s.center.x, dz = p.z - s.center.z;
	float r = s.radius + playerR;
	return dx * dx + dz * dz <= r * r;
}

bool HitBeam(const Beam& s, const Vec3& p, float playerR) {
	float dx = p.x - s.origin.x, dz = p.z - s.origin.z;
	float along = dx * s.dir.x + dz * s.dir.z;            // 前方向の距離
	float side = std::fabs(dx * s.dir.z - dz * s.dir.x); // 横ズレ

	// 長方形からはみ出している距離（中にいれば 0）
	float ex = std::fmax(side - s.width * 0.5f, 0.0f);
	float ey = along < 0.0f ? -along : (along > s.length ? along - s.length : 0.0f);
	return ex * ex + ey * ey <= playerR * playerR;
}

bool HitFan(const Fan& s, const Vec3& p, float playerR) {
	float dx = p.x - s.origin.x, dz = p.z - s.origin.z;
	float d2 = dx * dx + dz * dz;
	float outer = s.radius + playerR;
	if (d2 > outer * outer) return false; // 外側すぎる
	if (d2 < 1e-6f) return true;          // 真上は当たり扱い（0除算回避）

	// 角度が範囲内なら、距離チェックだけで確定
	float cosTheta = (dx * s.forward.x + dz * s.forward.z) / std::sqrt(d2);
	if (cosTheta >= std::cos(s.angle * 0.5f)) return true;

	// 角度の外側：扇の両端の辺（線分）との距離で判定
	float base = std::atan2(s.forward.x, s.forward.z);
	for (int sgn = -1; sgn <= 1; sgn += 2) {
		float a = base + sgn * s.angle * 0.5f;
		float ex = std::sin(a), ez = std::cos(a);
		float t = dx * ex + dz * ez;
		t = t < 0.0f ? 0.0f : (t > s.radius ? s.radius : t);
		float qx = dx - ex * t, qz = dz - ez * t;
		if (qx * qx + qz * qz <= playerR * playerR) return true;
	}
	return false;
}

bool HitBullet(const Bullet& s, const Vec3& p, float playerR) {
	float dx = p.x - s.pos.x, dz = p.z - s.pos.z;
	float r = s.radius + playerR;
	return dx * dx + dz * dz <= r * r;
}

bool Hit(const Shape& shape, const Vec3& playerPos, float playerR) {
	return std::visit(
	    [&](const auto& s) -> bool {
		    using T = std::decay_t<decltype(s)>;
		    if constexpr (std::is_same_v<T, Circle>) return HitCircle(s, playerPos, playerR);
		    else if constexpr (std::is_same_v<T, Beam>) return HitBeam(s, playerPos, playerR);
		    else return HitFan(s, playerPos, playerR);
	    },
	    shape);
}

Shape ScaleShape(const Shape& shape, float t) {
	Shape out = shape;
	std::visit(
	    [&](auto& s) {
		    using T = std::decay_t<decltype(s)>;
		    if constexpr (std::is_same_v<T, Circle>) s.radius *= t;
		    else if constexpr (std::is_same_v<T, Beam>) s.length *= t; // 根元から先端へ伸びる
		    else s.radius *= t;                                        // 扇は角度固定で半径だけ伸びる
	    },
	    out);
	return out;
}

} // namespace BossHit
