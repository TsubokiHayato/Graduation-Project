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

// =============================================================================
//  進行
// =============================================================================
void BossAttackState::Enter(Boss& boss) {
	OnEnter(boss);
	ChangeStep(boss, Step::Telegraph);
}

void BossAttackState::Update(Boss& boss, float deltaTime) {
	if (step_ == Step::Finished) {
		return;
	}
	deltaTime_ = deltaTime;
	stepTimer_ += deltaTime;

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
		switch (step_) {
		case Step::Telegraph: ChangeStep(boss, Step::Attack);   break;
		case Step::Attack:    ChangeStep(boss, Step::Recovery); break;
		case Step::Recovery:
			ChangeStep(boss, Step::Finished);
			OnExit(boss);
			break;
		default: break;
		}
	}
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
