#pragma once
#include "BossHitShapes.h"
#include "Vector4.h"

// =============================================================================
//  BossHit 形状のデバッグ描画（LineManager を使う）
//  LineManager への依存は BossHitDebugDraw.cpp の DrawLine 1か所だけ
// =============================================================================
namespace BossHitDebug {

using Vec3 = TuboEngine::Math::Vector3;
using Vec4 = TuboEngine::Math::Vector4;

// 線を1本引く（唯一のエンジン依存）
void DrawLine(const Vec3& a, const Vec3& b, const Vec4& color);

// 当たっていれば赤、外れていれば白
Vec4 Color(bool hit);

// 円（XZ平面）
void DrawCircle(const BossHit::Circle& s, const Vec4& c, int seg = 48);
// 帯（長方形）。extra に自機半径を渡すと「判定が実際に効く幅」で描ける
void DrawBeam(const BossHit::Beam& s, const Vec4& c, float extra = 0.0f);
// 扇（円弧 + 両端の線）
void DrawFan(const BossHit::Fan& s, const Vec4& c, int seg = 32);
// 弾（XZ平面の円）
void DrawBullet(const BossHit::Bullet& s, const Vec4& c);
// Shape（variant）をまとめて描く
void DrawShape(const BossHit::Shape& shape, const Vec4& c, float extra = 0.0f);

} // namespace BossHitDebug
