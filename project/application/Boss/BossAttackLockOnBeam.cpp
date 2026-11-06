#include "BossAttackLockOnBeam.h"
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
const Math::Vector4 kColorCharge = {1.0f, 0.3f, 0.3f, 1.0f};  // 追尾中（赤みがかる）
const Math::Vector4 kColorLocked = {1.0f, 0.1f, 0.1f, 1.0f};  // ロック確定
const Math::Vector4 kColorFire = {0.8f, 1.0f, 1.0f, 1.0f};    // 照射中
const Math::Vector4 kColorStun = {0.4f, 0.4f, 0.5f, 1.0f};    // 硬直中
const Math::Vector4 kColorAim = {1.0f, 0.6f, 0.1f, 1.0f};     // 追尾中の照準線
const Math::Vector4 kColorWarning = {1.0f, 0.1f, 0.1f, 1.0f}; // 赤い警告表示
const Math::Vector4 kColorBeam = {0.4f, 0.9f, 1.0f, 1.0f};    // ビーム
} // namespace

BossAttackLockOnBeam::BossAttackLockOnBeam() {
	// 共通パラメータの初期値（硬直は「中程度」）。予兆時間は 追尾時間＋回避猶予 で決まる
	common_.damage = 40.0f;
	common_.recoveryTime = 1.5f;
}

bool BossAttackLockOnBeam::IsLocked() const {
	return GetStep() == Step::Telegraph && GetStepTimer() >= params_.trackingTime;
}

float BossAttackLockOnBeam::TargetAngle(const Boss& boss) {
	const Math::Vector3& c = boss.GetCenter();
	const Math::Vector3& p = boss.GetTargetPosition();
	return std::atan2(p.z - c.z, p.x - c.x);
}

void BossAttackLockOnBeam::OnEnter(Boss& boss) {
	hasHit_ = false;
	lastHitResult_ = false;
	// 照準はボスの今の向きから追い始める
	aimAngle_ = boss.GetFacingAngle();
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

void BossAttackLockOnBeam::OnStepEnter(Boss& boss, Step step) {
	(void)boss;
	if (step == Step::Attack) {
		hasHit_ = false;
		lastHitResult_ = false;
	}
}

// -----------------------------------------------------------------------------
//  予兆：前半は追尾、後半はロック確定（照準が止まる）
// -----------------------------------------------------------------------------
void BossAttackLockOnBeam::UpdateTelegraph(Boss& boss, float t) {
	(void)t;

	if (!IsLocked()) {
		// 追尾：照準が自機の角度を追いかける（追う速さに上限あり）
		const float diff = WrapAngle(TargetAngle(boss) - aimAngle_);
		const float maxStep = params_.trackingSpeed * GetDeltaTime();
		aimAngle_ = WrapAngle(aimAngle_ + std::clamp(diff, -maxStep, maxStep));

		const float trackT = params_.trackingTime > 0.0f ? GetStepTimer() / params_.trackingTime : 1.0f;
		boss.SetColor(LerpColor(kColorNormal, kColorCharge, std::clamp(trackT, 0.0f, 1.0f) * 0.6f));
	} else {
		// ロック確定：照準は止まったまま。発射が近づくほど速く点滅
		const float lockT = params_.evadeTime > 0.0f ? (GetStepTimer() - params_.trackingTime) / params_.evadeTime : 1.0f;
		const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * (4.0f + 8.0f * lockT) * GetStepTimer());
		boss.SetColor(LerpColor(kColorCharge, kColorLocked, blink));
	}
	// ボスは照準の方向を向く
	boss.SetFacingAngle(aimAngle_);
}

// -----------------------------------------------------------------------------
//  攻撃：照射。照射中は判定が続き、当たったら1回だけダメージ
// -----------------------------------------------------------------------------
void BossAttackLockOnBeam::UpdateAttack(Boss& boss, float t) {
	(void)t;
	boss.SetFacingAngle(aimAngle_);
	boss.SetColor(kColorFire);

	if (!hasHit_ && IsHit(boss)) {
		hasHit_ = true;
		lastHitResult_ = true;
		boss.NotifyHitTarget(common_.damage);
	}
}

// -----------------------------------------------------------------------------
//  硬直：暗くなって止まる（旋回もしない）
// -----------------------------------------------------------------------------
void BossAttackLockOnBeam::UpdateRecovery(Boss& boss, float t) {
	const float recoverT = std::clamp((t - 0.7f) / 0.3f, 0.0f, 1.0f);
	boss.SetColor(LerpColor(kColorStun, kColorNormal, recoverT));
}

void BossAttackLockOnBeam::OnExit(Boss& boss) {
	boss.SetColor(kColorNormal);
}

// -----------------------------------------------------------------------------
//  当たり判定：ロックした方向に沿った距離（前後）と、横のずれで判定する
// -----------------------------------------------------------------------------
bool BossAttackLockOnBeam::IsHit(const Boss& boss) const {
	const Math::Vector3& c = boss.GetCenter();
	const Math::Vector3& p = boss.GetTargetPosition();
	const float r = boss.GetTargetRadius();

	const float dirX = std::cos(aimAngle_);
	const float dirZ = std::sin(aimAngle_);
	const float relX = p.x - c.x;
	const float relZ = p.z - c.z;

	const float along = relX * dirX + relZ * dirZ;           // 方向に沿った距離
	const float lateral = std::abs(relX * dirZ - relZ * dirX); // 横のずれ

	// ビームは中央からステージ端まで
	if (along < -r || along > boss.GetStageRadius() + r) {
		return false;
	}
	return lateral <= params_.beamWidth * 0.5f + r;
}

// -----------------------------------------------------------------------------
//  ライン描画
// -----------------------------------------------------------------------------
void BossAttackLockOnBeam::DrawBand(const Boss& boss, float width, float y, const Math::Vector4& color) const {
	const Math::Vector3& c = boss.GetCenter();
	const float len = boss.GetStageRadius();
	const float dirX = std::cos(aimAngle_);
	const float dirZ = std::sin(aimAngle_);
	// 横方向（方向に垂直）
	const float sideX = -dirZ * width * 0.5f;
	const float sideZ = dirX * width * 0.5f;

	const Math::Vector3 a = {c.x + sideX, c.y + y, c.z + sideZ};
	const Math::Vector3 b = {c.x - sideX, c.y + y, c.z - sideZ};
	const Math::Vector3 aEnd = {a.x + dirX * len, a.y, a.z + dirZ * len};
	const Math::Vector3 bEnd = {b.x + dirX * len, b.y, b.z + dirZ * len};

	LineManager* lm = LineManager::GetInstance();
	lm->DrawLine(a, aEnd, color);
	lm->DrawLine(b, bEnd, color);
	lm->DrawLine(a, b, color);
	lm->DrawLine(aEnd, bEnd, color);
}

void BossAttackLockOnBeam::DrawLines(const Boss& boss) const {
	const Math::Vector3& c = boss.GetCenter();
	const float len = boss.GetStageRadius();
	const float dirX = std::cos(aimAngle_);
	const float dirZ = std::sin(aimAngle_);
	LineManager* lm = LineManager::GetInstance();

	switch (GetStep()) {
	case Step::Telegraph:
		if (!IsLocked()) {
			// 追尾中：照準線（細い線）
			lm->DrawLine({c.x, c.y + 0.05f, c.z}, {c.x + dirX * len, c.y + 0.05f, c.z + dirZ * len}, kColorAim);
		} else {
			// ロック確定：ビームの範囲（＝実際の判定範囲）を赤く点滅表示
			const float lockT = params_.evadeTime > 0.0f ? (GetStepTimer() - params_.trackingTime) / params_.evadeTime : 1.0f;
			const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * (4.0f + 8.0f * lockT) * GetStepTimer());
			Math::Vector4 warn = kColorWarning;
			warn.w = 0.5f + 0.5f * blink;
			DrawBand(boss, params_.beamWidth, 0.05f, warn);
			// 中心線が端へ伸びていき、発射までの残り時間を示す
			const float l = len * std::clamp(lockT, 0.0f, 1.0f);
			lm->DrawLine({c.x, c.y + 0.05f, c.z}, {c.x + dirX * l, c.y + 0.05f, c.z + dirZ * l}, {1.0f, 0.4f, 0.1f, 1.0f});
		}
		break;
	case Step::Attack: {
		// 照射：帯の中を何本もの線で埋めて太いビームに見せる。出始めに少し太く
		const float t = StepProgress(params_.fireTime);
		const float pulse = 1.0f + 0.15f * std::sin(2.0f * kPi * 12.0f * GetStepTimer());
		const float width = params_.beamWidth * pulse * (1.0f + 0.3f * (1.0f - Easing::OutCubic((std::min)(t * 4.0f, 1.0f))));
		constexpr int kLayers = 5;
		for (int i = 0; i < kLayers; ++i) {
			const float w = width * (static_cast<float>(i + 1) / kLayers);
			DrawBand(boss, w, 0.5f, LerpColor(kColorBeam, kColorNormal, static_cast<float>(kLayers - 1 - i) / kLayers));
			DrawBand(boss, w, 1.0f, kColorBeam);
		}
		break;
	}
	default:
		break;
	}
}

// -----------------------------------------------------------------------------
//  ImGui（固有パラメータ）
// -----------------------------------------------------------------------------
void BossAttackLockOnBeam::DrawImGuiParams() {
#ifdef USE_IMGUI
	ImGui::DragFloat("Tracking time", &params_.trackingTime, 0.05f, 0.0f, 10.0f);
	ImGui::DragFloat("Evade time", &params_.evadeTime, 0.05f, 0.0f, 10.0f);
	ImGui::DragFloat("Beam width", &params_.beamWidth, 0.05f, 0.1f, 20.0f);
	ImGui::DragFloat("Fire time", &params_.fireTime, 0.05f, 0.05f, 10.0f);
	ImGui::DragFloat("Tracking speed", &params_.trackingSpeed, 0.05f, 0.1f, 20.0f);
	ImGui::Text("Last hit : %s", lastHitResult_ ? "HIT" : "-");
#endif
}
