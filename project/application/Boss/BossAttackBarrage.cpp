#include "BossAttackBarrage.h"
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
constexpr float kBulletHeight = 1.0f; // 弾の飛ぶ高さ（見た目のみ。判定は地面上の距離）

Math::Vector4 LerpColor(const Math::Vector4& a, const Math::Vector4& b, float t) {
	return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

const Math::Vector4 kColorNormal = {1.0f, 1.0f, 1.0f, 1.0f};
const Math::Vector4 kColorDeploy = {1.0f, 0.85f, 0.3f, 1.0f}; // 砲門展開中
const Math::Vector4 kColorStun = {0.5f, 0.5f, 0.6f, 1.0f};    // 硬直中
const Math::Vector4 kColorPort = {1.0f, 0.8f, 0.2f, 1.0f};    // 砲門
const Math::Vector4 kColorWarning = {1.0f, 0.1f, 0.1f, 1.0f}; // 赤い警告表示
const Math::Vector4 kColorBullet = {1.0f, 0.3f, 0.8f, 1.0f};  // 弾
} // namespace

BossAttackBarrage::BossAttackBarrage() {
	// 共通パラメータの初期値（ダメージは弾1発ぶん、硬直は「短い」）
	common_.telegraphTime = 1.2f;
	common_.damage = 10.0f;
	common_.recoveryTime = 0.8f;
}

void BossAttackBarrage::OnEnter(Boss& boss) {
	bullets_.clear();
	firedVolleys_ = 0;
	hitCount_ = 0;
	baseAngle_ = boss.GetFacingAngle();
	stageRadius_ = boss.GetStageRadius();
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

float BossAttackBarrage::GetAttackDuration() const {
	const float lastFire = static_cast<float>((std::max)(params_.volleyCount - 1, 0)) * params_.volleyInterval;
	const float travel = params_.bulletSpeed > 0.0f ? (stageRadius_ + params_.bulletSize) / params_.bulletSpeed : 0.0f;
	return lastFire + travel;
}

float BossAttackBarrage::VolleyBaseAngle(int volley) const {
	const int count = (std::max)(params_.bulletCount, 1);
	const float spacing = 2.0f * kPi / static_cast<float>(count);
	return baseAngle_ + spacing * params_.volleyAngleShift * static_cast<float>(volley);
}

// -----------------------------------------------------------------------------
//  予兆：その場で砲門展開
// -----------------------------------------------------------------------------
void BossAttackBarrage::UpdateTelegraph(Boss& boss, float t) {
	// 砲門が開くのに合わせて少し沈み込み、色が変わる
	boss.SetHeight(-0.2f * Easing::OutCubic(t));
	boss.SetColor(LerpColor(kColorNormal, kColorDeploy, Easing::OutCubic(t)));
}

// -----------------------------------------------------------------------------
//  攻撃：発射間隔ごとに一斉射。撃った弾は飛び続ける
// -----------------------------------------------------------------------------
void BossAttackBarrage::UpdateAttack(Boss& boss, float t) {
	(void)t;
	// 経過時間に応じて、まだ撃っていない一斉射を撃つ（1回目は攻撃開始の瞬間）
	while (firedVolleys_ < params_.volleyCount &&
	       GetStepTimer() >= params_.volleyInterval * static_cast<float>(firedVolleys_)) {
		FireVolley(boss, firedVolleys_);
		++firedVolleys_;
	}

	// 発射の瞬間だけ少し跳ねる
	const float sinceFire = GetStepTimer() - params_.volleyInterval * static_cast<float>((std::max)(firedVolleys_ - 1, 0));
	const float kick = std::clamp(1.0f - sinceFire / 0.15f, 0.0f, 1.0f);
	boss.SetHeight(-0.2f + 0.3f * kick);
	boss.SetColor(LerpColor(kColorDeploy, kColorNormal, kick));

	UpdateBullets(boss);
}

// -----------------------------------------------------------------------------
//  硬直：止まる（残った弾があれば飛ばしきる）
// -----------------------------------------------------------------------------
void BossAttackBarrage::UpdateRecovery(Boss& boss, float t) {
	boss.SetHeight(-0.2f * (1.0f - t));
	const float recoverT = std::clamp((t - 0.5f) / 0.5f, 0.0f, 1.0f);
	boss.SetColor(LerpColor(kColorStun, kColorNormal, recoverT));
	UpdateBullets(boss);
}

void BossAttackBarrage::OnExit(Boss& boss) {
	bullets_.clear();
	boss.SetHeight(0.0f);
	boss.SetColor(kColorNormal);
}

// -----------------------------------------------------------------------------
//  弾
// -----------------------------------------------------------------------------
void BossAttackBarrage::FireVolley(const Boss& boss, int volley) {
	const Math::Vector3& c = boss.GetCenter();
	const int count = (std::max)(params_.bulletCount, 1);
	const float base = VolleyBaseAngle(volley);
	for (int i = 0; i < count; ++i) {
		const float a = base + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(count);
		const float dx = std::cos(a);
		const float dz = std::sin(a);
		bullets_.push_back({
			{c.x, c.y + kBulletHeight, c.z},
			{dx * params_.bulletSpeed, 0.0f, dz * params_.bulletSpeed},
		});
	}
}

void BossAttackBarrage::UpdateBullets(Boss& boss) {
	const float dt = GetDeltaTime();
	const Math::Vector3& c = boss.GetCenter();
	const Math::Vector3& target = boss.GetTargetPosition();
	const float hitDist = params_.bulletSize + boss.GetTargetRadius();
	const float removeDist = boss.GetStageRadius() + params_.bulletSize;

	for (Bullet& b : bullets_) {
		b.position += b.velocity * dt;
	}

	// 当たり判定（地面上の距離）：当たった弾とステージ外に出た弾を消す
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(), [&](const Bullet& b) {
			const float tx = b.position.x - target.x;
			const float tz = b.position.z - target.z;
			if (tx * tx + tz * tz <= hitDist * hitDist) {
				++hitCount_;
				boss.NotifyHitTarget(common_.damage);
				return true;
			}
			const float cx = b.position.x - c.x;
			const float cz = b.position.z - c.z;
			return cx * cx + cz * cz > removeDist * removeDist;
		}),
		bullets_.end());
}

// -----------------------------------------------------------------------------
//  ライン描画：砲門・弾道の警告・弾
// -----------------------------------------------------------------------------
void BossAttackBarrage::DrawLines(const Boss& boss) const {
	LineManager* lm = LineManager::GetInstance();
	const Math::Vector3& c = boss.GetCenter();
	const int count = (std::max)(params_.bulletCount, 1);

	if (GetStep() == Step::Telegraph) {
		const float t = StepProgress(GetTelegraphDuration());
		const float open = Easing::OutBack(t);
		const float base = VolleyBaseAngle(0);
		const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * (2.0f + 6.0f * t) * GetStepTimer());
		Math::Vector4 warn = kColorWarning;
		warn.w = 0.4f + 0.6f * blink;

		for (int i = 0; i < count; ++i) {
			const float a = base + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(count);
			const float dx = std::cos(a);
			const float dz = std::sin(a);
			// 砲門：ボスの周りから外へ伸びる短い線
			const float r0 = 1.2f;
			const float r1 = r0 + params_.portLength * open;
			lm->DrawLine({c.x + dx * r0, c.y + kBulletHeight, c.z + dz * r0}, {c.x + dx * r1, c.y + kBulletHeight, c.z + dz * r1}, kColorPort);
			// 最初の一斉射の弾道を赤く警告表示（中心から端へ伸びていく）
			const float len = stageRadius_ * Easing::OutCubic(t);
			lm->DrawLine({c.x, c.y + 0.05f, c.z}, {c.x + dx * len, c.y + 0.05f, c.z + dz * len}, warn);
		}
		return;
	}

	if (GetStep() == Step::Attack) {
		// 砲門は撃ち終わるまで出しておく
		for (int i = 0; i < count && firedVolleys_ < params_.volleyCount; ++i) {
			const float a = VolleyBaseAngle(firedVolleys_) + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(count);
			const float dx = std::cos(a);
			const float dz = std::sin(a);
			const float r0 = 1.2f;
			const float r1 = r0 + params_.portLength;
			lm->DrawLine({c.x + dx * r0, c.y + kBulletHeight, c.z + dz * r0}, {c.x + dx * r1, c.y + kBulletHeight, c.z + dz * r1}, kColorPort);
		}
	}

	// 弾（仮表示：ワイヤーの球。半径は判定と同じ）
	for (const Bullet& b : bullets_) {
		lm->DrawSphere(b.position, params_.bulletSize, kColorBullet, 8);
	}
}

// -----------------------------------------------------------------------------
//  ImGui（固有パラメータ）
// -----------------------------------------------------------------------------
void BossAttackBarrage::DrawImGuiParams() {
#ifdef USE_IMGUI
	ImGui::DragInt("Bullet count", &params_.bulletCount, 1.0f, 1, 128);
	ImGui::DragFloat("Bullet speed", &params_.bulletSpeed, 0.1f, 0.1f, 50.0f);
	ImGui::DragFloat("Bullet size", &params_.bulletSize, 0.01f, 0.05f, 5.0f);
	ImGui::DragInt("Volley count", &params_.volleyCount, 1.0f, 1, 20);
	ImGui::DragFloat("Volley interval", &params_.volleyInterval, 0.01f, 0.05f, 5.0f);
	ImGui::SliderFloat("Volley angle shift", &params_.volleyAngleShift, 0.0f, 1.0f);
	ImGui::DragFloat("Port length", &params_.portLength, 0.05f, 0.0f, 10.0f);
	ImGui::Text("Bullets : %d  Hits : %d", static_cast<int>(bullets_.size()), hitCount_);
#endif
}
