#include "BossHitDebugDraw.h"
#include "LineManager.h"
#include <cmath>
#include <type_traits>

namespace BossHitDebug {

namespace {
constexpr float kY = 0.02f; // 地面に埋まらない程度に浮かせる
constexpr float kPi = 3.14159265f;
} // namespace

// ---- 要修正ポイント（唯一のエンジン依存）。LineManager の関数名/引数に合わせる ----
void DrawLine(const Vec3& a, const Vec3& b, const Vec4& color) {
	TuboEngine::LineManager::GetInstance()->DrawLine(a, b, color);
}

Vec4 Color(bool hit) { return hit ? Vec4{1.0f, 0.0f, 0.0f, 1.0f} : Vec4{1.0f, 1.0f, 1.0f, 1.0f}; }

void DrawCircle(const BossHit::Circle& s, const Vec4& c, int seg) {
	for (int i = 0; i < seg; ++i) {
		float a0 = 2.0f * kPi * i / seg, a1 = 2.0f * kPi * (i + 1) / seg;
		DrawLine({s.center.x + std::sin(a0) * s.radius, kY, s.center.z + std::cos(a0) * s.radius},
		         {s.center.x + std::sin(a1) * s.radius, kY, s.center.z + std::cos(a1) * s.radius}, c);
	}
}

void DrawBeam(const BossHit::Beam& s, const Vec4& c, float extra) {
	float hw = s.width * 0.5f + extra;
	// 右方向ベクトル（dir を90度回転）
	float rx = s.dir.z, rz = -s.dir.x;
	Vec3 o = s.origin;
	Vec3 e = {o.x + s.dir.x * s.length, kY, o.z + s.dir.z * s.length};
	Vec3 a = {o.x + rx * hw, kY, o.z + rz * hw};
	Vec3 b = {o.x - rx * hw, kY, o.z - rz * hw};
	Vec3 a2 = {e.x + rx * hw, kY, e.z + rz * hw};
	Vec3 b2 = {e.x - rx * hw, kY, e.z - rz * hw};
	DrawLine(a, a2, c);
	DrawLine(b, b2, c);
	DrawLine(a, b, c);
	DrawLine(a2, b2, c);
}

void DrawFan(const BossHit::Fan& s, const Vec4& c, int seg) {
	float base = std::atan2(s.forward.x, s.forward.z); // yaw（sin→x, cos→z）
	float half = s.angle * 0.5f;
	Vec3 o = {s.origin.x, kY, s.origin.z};
	Vec3 prev{};
	for (int i = 0; i <= seg; ++i) {
		float a = base - half + s.angle * i / seg;
		Vec3 p = {s.origin.x + std::sin(a) * s.radius, kY, s.origin.z + std::cos(a) * s.radius};
		if (i == 0 || i == seg) DrawLine(o, p, c);
		if (i > 0) DrawLine(prev, p, c);
		prev = p;
	}
}

void DrawBullet(const BossHit::Bullet& s, const Vec4& c) { DrawCircle(BossHit::Circle{s.pos, s.radius}, c, 24); }

void DrawShape(const BossHit::Shape& shape, const Vec4& c, float extra) {
	std::visit(
	    [&](const auto& s) {
		    using T = std::decay_t<decltype(s)>;
		    if constexpr (std::is_same_v<T, BossHit::Circle>) DrawCircle(s, c);
		    else if constexpr (std::is_same_v<T, BossHit::Beam>) DrawBeam(s, c, extra);
		    else DrawFan(s, c);
	    },
	    shape);
}

} // namespace BossHitDebug
