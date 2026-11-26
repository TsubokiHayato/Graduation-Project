#include "BossAttackState.h"
#include "Boss.h"
#include "LineManager.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// 共通パラメータの JSON 変換
#define BOSS_ATTACK_COMMON_FIELDS(X)       \
	X("telegraphTime", telegraphTime)      \
	X("damage", damage)                    \
	X("recoveryTime", recoveryTime)
TUBO_JSON_DEFINE(BossAttackState::CommonParams, BOSS_ATTACK_COMMON_FIELDS)

namespace {
// この攻撃がボスの向き・高さ・色を動かしてよいかを、処理の間だけボスに設定する
// （同時攻撃の従が、主の動きを上書きしないようにする）
class BodyControlScope {
public:
	BodyControlScope(Boss& boss, bool enabled) : boss_(boss), prev_(boss.IsBodyControlEnabled()) { boss_.SetBodyControlEnabled(enabled); }
	~BodyControlScope() { boss_.SetBodyControlEnabled(prev_); }
	BodyControlScope(const BodyControlScope&) = delete;
	BodyControlScope& operator=(const BodyControlScope&) = delete;

private:
	Boss& boss_;
	bool prev_;
};
} // namespace

// =============================================================================
//  進行
// =============================================================================
void BossAttackState::Enter(Boss& boss) {
	BodyControlScope scope(boss, controlsBody_);
	holdReleased_ = false;
	OnEnter(boss);
	ChangeStep(boss, Step::Telegraph);
}

void BossAttackState::Update(Boss& boss, float deltaTime) {
	if (step_ == Step::Finished) {
		return;
	}
	BodyControlScope scope(boss, controlsBody_);
	deltaTime_ = deltaTime;
	stepTimer_ += deltaTime;

	// 同時攻撃の主：ほかの攻撃が終わるまで姿勢を保って待つ
	if (step_ == Step::Hold) {
		UpdateHold(boss);
		if (holdReleased_) {
			ChangeStep(boss, Step::Recovery);
		}
		return;
	}

	const float duration = GetStepDuration(step_);
	const float t = StepProgress(duration);

	switch (step_) {
	case Step::Telegraph: UpdateTelegraph(boss, t); break;
	case Step::Attack:    UpdateAttack(boss, t);    break;
	case Step::Recovery:  UpdateRecovery(boss, t);  break;
	default: break;
	}

	// 段階の時間が過ぎたら次へ
	if (stepTimer_ >= duration) {
		AdvanceStep(boss);
	}
}

void BossAttackState::AdvanceStep(Boss& boss) {
	switch (step_) {
	case Step::Telegraph:
		ChangeStep(boss, Step::Attack);
		break;
	case Step::Attack:
		if (skipRecovery_) {
			// 連続技の途中・同時攻撃の従：硬直せずに終わる
			Finish(boss);
		} else if (holdBeforeRecovery_ && !holdReleased_) {
			// 同時攻撃の主：ほかの攻撃が終わるまで待ってから硬直する
			ChangeStep(boss, Step::Hold);
		} else {
			ChangeStep(boss, Step::Recovery);
		}
		break;
	case Step::Recovery:
		Finish(boss);
		break;
	default:
		break;
	}
}

void BossAttackState::Cancel(Boss& boss) {
	if (step_ == Step::Finished) {
		return;
	}
	BodyControlScope scope(boss, controlsBody_);
	Finish(boss);
}

void BossAttackState::Finish(Boss& boss) {
	ChangeStep(boss, Step::Finished);
	OnExit(boss);
}

void BossAttackState::ChangeStep(Boss& boss, Step next) {
	step_ = next;
	stepTimer_ = 0.0f;
	if (next != Step::Finished) {
		OnStepEnter(boss, next);
	}
}

float BossAttackState::GetStepDuration(Step step) const {
	switch (step) {
	case Step::Telegraph: return GetTelegraphDuration();
	case Step::Attack:    return GetAttackDuration();
	case Step::Recovery:  return common_.recoveryTime;
	default:              return 0.0f;
	}
}

// =============================================================================
//  パラメータの保存・読み込み
// =============================================================================
void BossAttackState::SaveParams(JsonManager::Json& json) const {
	json["common"] = common_;
}

void BossAttackState::LoadParams(const JsonManager::Json& json) {
	JsonManager::Read(json, "common", common_);
}

float BossAttackState::StepProgress(float duration) const {
	if (duration <= 0.0f) {
		return 1.0f;
	}
	return std::clamp(stepTimer_ / duration, 0.0f, 1.0f);
}

// =============================================================================
//  補助
// =============================================================================
void BossAttackState::DrawGroundCircle(const Math::Vector3& center, float radius, const Math::Vector4& color, int divisions) {
	if (radius <= 0.0f || divisions <= 0) {
		return;
	}
	constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
	LineManager* lm = LineManager::GetInstance();
	for (int i = 0; i < divisions; ++i) {
		const float a0 = kTwoPi * static_cast<float>(i) / divisions;
		const float a1 = kTwoPi * static_cast<float>(i + 1) / divisions;
		lm->DrawLine(
			{center.x + std::cos(a0) * radius, center.y, center.z + std::sin(a0) * radius},
			{center.x + std::cos(a1) * radius, center.y, center.z + std::sin(a1) * radius},
			color);
	}
}

// =============================================================================
//  ImGui
// =============================================================================
void BossAttackState::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::PushID(this);
	if (ImGui::CollapsingHeader(GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
		if (UsesCommonTelegraphTime()) {
			ImGui::DragFloat("Telegraph time", &common_.telegraphTime, 0.05f, 0.1f, 10.0f);
		}
		ImGui::DragFloat("Damage", &common_.damage, 1.0f, 0.0f, 1000.0f);
		ImGui::DragFloat("Recovery time", &common_.recoveryTime, 0.05f, 0.1f, 10.0f);
		DrawImGuiParams();
	}
	ImGui::PopID();
#endif
}
