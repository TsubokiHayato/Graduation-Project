#pragma once
#include "Vector3.h"
#include <cmath>

namespace BossHit {
	using Vec3 = TuboEngine::Math::Vector3;

	// 円：ボス中心などの範囲攻撃
	struct Circle { Vec3 center; float radius; };
	// 帯：origin から dir（正規化済み）へ length だけ伸びるビーム
	struct Beam { Vec3 origin; Vec3 dir; float length; float width; };
	// 扇：forward（正規化済み）を中心に、全角 angle[rad]
	struct Fan { Vec3 origin; Vec3 forward; float radius; float angle; };
	// 弾
	struct Bullet { Vec3 pos; float radius; };

	// 円：距離 ≤ 半径 + 自機の大きさ
	inline bool HitCircle(const Circle& s, const Vec3& p, float playerR) {
		float dx = p.x - s.center.x, dz = p.z - s.center.z;
		float r = s.radius + playerR;
		return dx * dx + dz * dz <= r * r;
	}

	// 帯：長方形（幅 × 長さ）と自機の円が重なるか（端も含めて正確）
	inline bool HitBeam(const Beam& s, const Vec3& p, float playerR) {
		float dx = p.x - s.origin.x, dz = p.z - s.origin.z;
		float along = dx * s.dir.x + dz * s.dir.z;
		float side = std::fabs(dx * s.dir.z - dz * s.dir.x);

		// 長方形からはみ出している距離（中にいれば 0）
		float ex = std::fmax(side - s.width * 0.5f, 0.0f);
		float ey = along < 0.0f ? -along : (along > s.length ? along - s.length : 0.0f);
		return ex * ex + ey * ey <= playerR * playerR;
	}

	// 扇：扇形と自機の円が重なるか
	inline bool HitFan(const Fan& s, const Vec3& p, float playerR) {
		float dx = p.x - s.origin.x, dz = p.z - s.origin.z;
		float d2 = dx * dx + dz * dz;
		float outer = s.radius + playerR;
		if (d2 > outer * outer) return false;       // 外側すぎる
		if (d2 < 1e-6f) return true;                // 真上

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

	// 弾：距離 ≤ 弾の大きさ + 自機の大きさ
	inline bool HitBullet(const Bullet& s, const Vec3& p, float playerR) {
		float dx = p.x - s.pos.x, dz = p.z - s.pos.z;
		float r = s.radius + playerR;
		return dx * dx + dz * dz <= r * r;
	}
} // namespace BossHit