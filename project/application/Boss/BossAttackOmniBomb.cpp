#include "BossAttackOmniBomb.h"
#include "Boss.h"
#include "Easing.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr float kPi = std::numbers::pi_v<float>;

Math::Vector4 LerpColor(const Math::Vector4& a, const Math::Vector4& b, float t) {
	return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

const Math::Vector4 kColorNormal = {1.0f, 1.0f, 1.0f, 1.0f};
const Math::Vector4 kColorGlow = {1.0f, 0.55f, 0.1f, 1.0f};   // 予兆の発光色
const Math::Vector4 kColorBlast = {1.0f, 1.0f, 0.85f, 1.0f};  // 爆発の瞬間
const Math::Vector4 kColorStun = {0.35f, 0.35f, 0.45f, 1.0f}; // 硬直中（出力が落ちた見た目）
const Math::Vector4 kColorWarning = {1.0f, 0.1f, 0.1f, 1.0f}; // 赤い警告表示
} // namespace

BossAttackOmniBomb::BossAttackOmniBomb() {
	// 共通パラメータの初期値（硬直は「長い」）
	common_.telegraphTime = 2.0f;
	common_.damage = 60.0f;
	common_.recoveryTime = 3.0f;
}

void BossAttackOmniBomb::OnEnter(Boss& boss) {
	lastHitResult_ = false;
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

void BossAttackOmniBomb::OnStepEnter(Boss& boss, Step step) {
	if (step == Step::Attack) {
		// 判定は爆発の瞬間に一度だけ（予兆の警告範囲と同じ半径）
		lastHitResult_ = IsHit(boss);
		if (lastHitResult_) {
			boss.NotifyHitTarget(common_.damage);
		}
	}
}

// -----------------------------------------------------------------------------
//  予兆：光りながら上昇
// -----------------------------------------------------------------------------
void BossAttackOmniBomb::UpdateTelegraph(Boss& boss, float t) {
	boss.SetHeight(params_.riseHeight * Easing::OutCubic(t));

	// 点滅しながら、発射が近づくほど速く・強く光る
	const float freq = 2.0f + 8.0f * t; // 2Hz → 10Hz
	const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * freq * GetStepTimer());
	const float glow = (0.3f + 0.7f * t) * (0.4f + 0.6f * blink);
	boss.SetColor(LerpColor(kColorNormal, kColorGlow, glow));
}

// -----------------------------------------------------------------------------
//  攻撃：爆発（判定は OnStepEnter で済ませている）
// -----------------------------------------------------------------------------
void BossAttackOmniBomb::UpdateAttack(Boss& boss, float t) {
	boss.SetHeight(params_.riseHeight);
	boss.SetColor(LerpColor(kColorBlast, kColorStun, Easing::InQuad(t)));
}

// -----------------------------------------------------------------------------
//  硬直：暗くなって着地し、止まる
// -----------------------------------------------------------------------------
void BossAttackOmniBomb::UpdateRecovery(Boss& boss, float t) {
	const float landDuration = (std::min)(params_.landTime, common_.recoveryTime);
	const float landT = landDuration > 0.0f ? std::clamp(GetStepTimer() / landDuration, 0.0f, 1.0f) : 1.0f;
	boss.SetHeight(params_.riseHeight * (1.0f - Easing::OutSine(landT)));

	// 硬直の終わり際（残り3割）で元の色へ戻す
	const float recoverT = std::clamp((t - 0.7f) / 0.3f, 0.0f, 1.0f);
	boss.SetColor(LerpColor(kColorStun, kColorNormal, recoverT));
}

void BossAttackOmniBomb::OnExit(Boss& boss) {
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

// -----------------------------------------------------------------------------
//  当たり判定：地面上（XZ平面）でのボス中心からの距離 ≤ 爆撃の半径
//  円の内側全域が範囲なので自機の大きさは足さない（警告表示と一致させる）
// -----------------------------------------------------------------------------
bool BossAttackOmniBomb::IsHit(const Boss& boss) const {
	const Math::Vector3& center = boss.GetCenter();
	const Math::Vector3& target = boss.GetTargetPosition();
	const float dx = target.x - center.x;
	const float dz = target.z - center.z;
	return std::sqrt(dx * dx + dz * dz) <= params_.radius;
}

// -----------------------------------------------------------------------------
//  ライン描画：警告範囲と衝撃波
// -----------------------------------------------------------------------------
void BossAttackOmniBomb::DrawLines(const Boss& boss) const {
	const Math::Vector3 c = boss.GetCenter();
	const Math::Vector3 ground = {c.x, c.y + 0.05f, c.z};

	switch (GetStep()) {
	case Step::Telegraph: {
		const float t = StepProgress(common_.telegraphTime);
		// 警告範囲（＝実際の判定範囲）を赤く点滅表示
		const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * (2.0f + 6.0f * t) * GetStepTimer());
		Math::Vector4 warn = kColorWarning;
		warn.w = 0.5f + 0.5f * blink;
		DrawGroundCircle(ground, params_.radius, warn);
		DrawGroundCircle(ground, params_.radius * 0.995f, warn);
		// 中心から外へ広がる輪で発射までの残り時間を示す
		DrawGroundCircle(ground, params_.radius * t, {1.0f, 0.4f, 0.1f, 1.0f});
		break;
	}
	case Step::Attack: {
		const float t = StepProgress(params_.blastTime);
		// 衝撃波：中心から爆撃の半径まで一気に広がる
		const float r = params_.radius * Easing::OutCubic(t);
		const Math::Vector4 col = {1.0f, 0.9f - 0.6f * t, 0.3f, 1.0f};
		DrawGroundCircle(ground, r, col);
		break;
	}
	default:
		break;
	}
}

// -----------------------------------------------------------------------------
//  ImGui（固有パラメータ）
// -----------------------------------------------------------------------------
void BossAttackOmniBomb::DrawImGuiParams() {
#ifdef USE_IMGUI
	ImGui::DragFloat("Radius", &params_.radius, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Rise height", &params_.riseHeight, 0.05f, 0.0f, 20.0f);
	ImGui::DragFloat("Blast time", &params_.blastTime, 0.01f, 0.05f, 3.0f);
	ImGui::DragFloat("Land time", &params_.landTime, 0.01f, 0.05f, 3.0f);
	ImGui::Text("Last hit : %s", lastHitResult_ ? "HIT" : "-");
#endif
}
