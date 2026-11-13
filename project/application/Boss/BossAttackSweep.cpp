#include "BossAttackSweep.h"
#include "Boss.h"
#include "Easing.h"
#include "LineManager.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kDegToRad = kPi / 180.0f;
constexpr float kArmHeight = 1.0f; // 腕の線を描く高さ

Math::Vector4 LerpColor(const Math::Vector4& a, const Math::Vector4& b, float t) {
	return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

// 角度の差を -π〜π に収める
float WrapAngle(float a) {
	while (a > kPi) { a -= 2.0f * kPi; }
	while (a < -kPi) { a += 2.0f * kPi; }
	return a;
}

const Math::Vector4 kColorNormal = {1.0f, 1.0f, 1.0f, 1.0f};
const Math::Vector4 kColorWindUp = {1.0f, 0.4f, 0.2f, 1.0f};  // 振りかぶり
const Math::Vector4 kColorSwing = {1.0f, 1.0f, 0.8f, 1.0f};   // 振り抜き
const Math::Vector4 kColorStun = {0.4f, 0.4f, 0.5f, 1.0f};    // 硬直中
const Math::Vector4 kColorWarning = {1.0f, 0.1f, 0.1f, 1.0f}; // 赤い警告表示
const Math::Vector4 kColorArm = {1.0f, 0.8f, 0.3f, 1.0f};     // 腕
} // namespace

BossAttackSweep::BossAttackSweep() {
	// 共通パラメータの初期値（硬直は「中程度」）
	common_.telegraphTime = 1.0f;
	common_.damage = 30.0f;
	common_.recoveryTime = 1.5f;
}

void BossAttackSweep::OnEnter(Boss& boss) {
	hasHit_ = false;
	lastHitResult_ = false;
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

void BossAttackSweep::OnStepEnter(Boss& boss, Step step) {
	(void)boss;
	if (step == Step::Attack) {
		hasHit_ = false;
		lastHitResult_ = false;
	}
}

// -----------------------------------------------------------------------------
//  腕の角度：振りかぶりで片側の後ろへ引き、攻撃で反対側の端まで振り抜く
// -----------------------------------------------------------------------------
float BossAttackSweep::ArmOffset() const {
	const float half = params_.angleDeg * 0.5f * kDegToRad;
	const float windUp = half + params_.windUpDeg * kDegToRad; // 振りかぶった位置
	switch (GetStep()) {
	case Step::Telegraph: return windUp * Easing::OutCubic(StepProgress(GetTelegraphDuration()));
	case Step::Attack:    return windUp + (-half - windUp) * Easing::OutCubic(StepProgress(params_.swingTime));
	case Step::Recovery:  return -half;
	default:              return 0.0f;
	}
}

// -----------------------------------------------------------------------------
//  予兆：振りかぶりながら自機の方へ旋回（旋回速度に上限あり）
// -----------------------------------------------------------------------------
void BossAttackSweep::UpdateTelegraph(Boss& boss, float t) {
	boss.TurnTowardTarget(GetDeltaTime());

	// 振りかぶりで少し沈み、色が変わる
	boss.SetHeight(-0.3f * Easing::OutCubic(t));
	boss.SetColor(LerpColor(kColorNormal, kColorWindUp, Easing::OutCubic(t)));
}

// -----------------------------------------------------------------------------
//  攻撃：向きは固定。振っている間に扇形の中にいたら1回だけダメージ
// -----------------------------------------------------------------------------
void BossAttackSweep::UpdateAttack(Boss& boss, float t) {
	boss.SetHeight(-0.3f + 0.5f * Easing::OutCubic(t));
	boss.SetColor(LerpColor(kColorSwing, kColorWindUp, t));

	if (!hasHit_ && IsHit(boss)) {
		hasHit_ = true;
		lastHitResult_ = true;
		boss.NotifyHitTarget(common_.damage);
	}
}

// -----------------------------------------------------------------------------
//  硬直：振り抜いた姿勢のまま止まる（旋回もしない）
// -----------------------------------------------------------------------------
void BossAttackSweep::UpdateRecovery(Boss& boss, float t) {
	const float landT = std::clamp(t / 0.3f, 0.0f, 1.0f);
	boss.SetHeight(0.2f * (1.0f - Easing::OutBounce(landT)));
	const float recoverT = std::clamp((t - 0.7f) / 0.3f, 0.0f, 1.0f);
	boss.SetColor(LerpColor(kColorStun, kColorNormal, recoverT));
}

void BossAttackSweep::OnExit(Boss& boss) {
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

// -----------------------------------------------------------------------------
//  当たり判定：距離 ≤ 扇の半径、かつボスの向きとの角度差 ≤ 扇の角度の半分
// -----------------------------------------------------------------------------
bool BossAttackSweep::IsHit(const Boss& boss) const {
	const Math::Vector3& c = boss.GetCenter();
	const Math::Vector3& p = boss.GetTargetPosition();
	const float dx = p.x - c.x;
	const float dz = p.z - c.z;
	if (std::sqrt(dx * dx + dz * dz) > params_.radius) {
		return false;
	}
	const float diff = WrapAngle(std::atan2(dz, dx) - boss.GetFacingAngle());
	return std::abs(diff) <= params_.angleDeg * 0.5f * kDegToRad;
}

// -----------------------------------------------------------------------------
//  ライン描画
// -----------------------------------------------------------------------------
void BossAttackSweep::DrawFan(const Boss& boss, float facing, float radius, const Math::Vector4& color) const {
	const Math::Vector3& c = boss.GetCenter();
	const float y = c.y + 0.05f;
	const float half = params_.angleDeg * 0.5f * kDegToRad;
	const int divisions = (std::max)(static_cast<int>(params_.angleDeg / 5.0f), 2);
	LineManager* lm = LineManager::GetInstance();

	const float a0 = facing - half;
	const float a1 = facing + half;
	// 両端の半径
	lm->DrawLine({c.x, y, c.z}, {c.x + std::cos(a0) * radius, y, c.z + std::sin(a0) * radius}, color);
	lm->DrawLine({c.x, y, c.z}, {c.x + std::cos(a1) * radius, y, c.z + std::sin(a1) * radius}, color);
	// 弧
	for (int i = 0; i < divisions; ++i) {
		const float b0 = a0 + (a1 - a0) * static_cast<float>(i) / divisions;
		const float b1 = a0 + (a1 - a0) * static_cast<float>(i + 1) / divisions;
		lm->DrawLine({c.x + std::cos(b0) * radius, y, c.z + std::sin(b0) * radius},
		             {c.x + std::cos(b1) * radius, y, c.z + std::sin(b1) * radius}, color);
	}
}

void BossAttackSweep::DrawLines(const Boss& boss) const {
	const Math::Vector3& c = boss.GetCenter();
	const float facing = boss.GetFacingAngle();
	LineManager* lm = LineManager::GetInstance();

	switch (GetStep()) {
	case Step::Telegraph: {
		const float t = StepProgress(GetTelegraphDuration());
		// 扇形（＝実際の判定範囲）を赤く点滅表示。ボスの旋回に合わせて動く
		const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * (3.0f + 7.0f * t) * GetStepTimer());
		Math::Vector4 warn = kColorWarning;
		warn.w = 0.5f + 0.5f * blink;
		DrawFan(boss, facing, params_.radius, warn);
		// 中心から外へ広がる弧で攻撃までの残り時間を示す
		DrawFan(boss, facing, params_.radius * t, {1.0f, 0.4f, 0.1f, 1.0f});
		break;
	}
	case Step::Attack: {
		// 振り抜いた軌跡：振り始めから今の腕の位置まで扇を塗るように線を引く
		const float half = params_.angleDeg * 0.5f * kDegToRad;
		const float arm = ArmOffset();
		const float from = (std::min)(half, (std::max)(arm, -half));
		for (float a = half; a >= from; a -= 4.0f * kDegToRad) {
			const float wa = facing + a;
			lm->DrawLine({c.x, c.y + 0.05f, c.z}, {c.x + std::cos(wa) * params_.radius, c.y + 0.05f, c.z + std::sin(wa) * params_.radius}, kColorSwing);
		}
		break;
	}
	default:
		break;
	}

	// 腕（仮表示：ボスから扇の半径まで伸びる線）
	if (GetStep() != Step::Finished) {
		const float a = facing + ArmOffset();
		const float dx = std::cos(a);
		const float dz = std::sin(a);
		const float r0 = 1.0f;
		const float r1 = params_.radius;
		for (int i = 0; i < 3; ++i) {
			const float h = kArmHeight + 0.15f * static_cast<float>(i);
			lm->DrawLine({c.x + dx * r0, c.y + h, c.z + dz * r0}, {c.x + dx * r1, c.y + h, c.z + dz * r1}, kColorArm);
		}
	}
}

// -----------------------------------------------------------------------------
//  ImGui（固有パラメータ）
// -----------------------------------------------------------------------------
void BossAttackSweep::DrawImGuiParams() {
#ifdef USE_IMGUI
	ImGui::DragFloat("Fan radius", &params_.radius, 0.1f, 0.5f, 50.0f);
	ImGui::DragFloat("Fan angle (deg)", &params_.angleDeg, 1.0f, 5.0f, 360.0f);
	ImGui::DragFloat("Swing time", &params_.swingTime, 0.01f, 0.05f, 3.0f);
	ImGui::DragFloat("Wind-up (deg)", &params_.windUpDeg, 1.0f, 0.0f, 180.0f);
	ImGui::Text("Last hit : %s", lastHitResult_ ? "HIT" : "-");
#endif
}
