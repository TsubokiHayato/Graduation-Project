#include "BossEditScene.h"
#include "GameTime.h"
#include "LineManager.h"
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr float kPi = 3.14159265f;

// 地面に円をラインで描く
void DrawGroundCircle(const Math::Vector3& center, float radius, const Math::Vector4& color, int divisions = 96) {
	LineManager* lm = LineManager::GetInstance();
	for (int i = 0; i < divisions; ++i) {
		const float a0 = 2.0f * kPi * static_cast<float>(i) / divisions;
		const float a1 = 2.0f * kPi * static_cast<float>(i + 1) / divisions;
		lm->DrawLine({center.x + std::cos(a0) * radius, center.y, center.z + std::sin(a0) * radius},
		             {center.x + std::cos(a1) * radius, center.y, center.z + std::sin(a1) * radius}, color);
	}
}
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void BossEditScene::Initialize() {
	camera_ = std::make_unique<TuboEngine::Camera>();
	ApplyCamera();

	// ボス（自分では攻撃を選ばず、タイムラインのプレビューだけを再生する）
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(camera_.get(), {0.0f, 0.0f, 0.0f});
	boss_->SetStageRadius(stageRadius_);
	boss_->SetAutoAttack(false);

	// 的（自機の代わり）。ボスと同じモデルだと色がボスに引っぱられるので球にする
	target_ = std::make_unique<Object3d>();
	target_->Initialize("sphere/sphere.obj");
	target_->SetCamera(camera_.get());

	editor_.Initialize(boss_.get());
}

// =============================================================================
//  Update
// =============================================================================
void BossEditScene::Update() {
	const float dt = TuboEngine::Time::DeltaTime();

	// 的の位置を先に渡してから、プレビューの時刻を反映する（向きや照準は的の位置で決まる）
	const Math::Vector3 targetPos = editor_.GetTargetPosition();
	boss_->SetTarget(targetPos, editor_.GetTargetRadius());
	boss_->SetStageRadius(stageRadius_);
	editor_.Update();

	// ボス（一時停止中も攻撃範囲のラインは描く）
	boss_->Update(dt);

	// 的
	const float r = editor_.GetTargetRadius();
	target_->SetPosition({targetPos.x, targetPos.y + r, targetPos.z});
	target_->SetScale({r, r, r});
	target_->SetModelColor({0.3f, 1.0f, 0.4f, 1.0f});
	DrawGroundCircle(targetPos, r, {0.3f, 1.0f, 0.4f, 1.0f}, 24);

	// ステージの縁と、5m ごとの目安の円
	DrawGroundCircle(boss_->GetCenter(), stageRadius_, {0.6f, 0.6f, 0.6f, 1.0f});
	for (float radius = 5.0f; radius < stageRadius_; radius += 5.0f) {
		DrawGroundCircle(boss_->GetCenter(), radius, {0.3f, 0.3f, 0.3f, 1.0f});
	}

	ApplyCamera();
	target_->Update();
}

void BossEditScene::ApplyCamera() {
	// ステージ中央を、手前（-Z）の斜め上から見下ろす
	const float pitch = cameraPitchDeg_ * kPi / 180.0f;
	camera_->SetTranslate({0.0f, std::sin(pitch) * cameraDistance_, -std::cos(pitch) * cameraDistance_});
	camera_->setRotation({pitch, 0.0f, 0.0f});
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();
}

// =============================================================================
//  描画
// =============================================================================
void BossEditScene::Object3DDraw() {
	boss_->Draw();
	target_->Draw();
}

void BossEditScene::SpriteDraw() {}

void BossEditScene::ParticleDraw() {}

void BossEditScene::ImGuiDraw() {
#ifdef USE_IMGUI
	editor_.DrawImGui();
	boss_->DrawImGui(); // 攻撃の標準の数値・形態の重みなど

	if (ImGui::Begin("Boss Edit Camera")) {
		ImGui::SliderFloat("Distance", &cameraDistance_, 10.0f, 80.0f);
		ImGui::SliderFloat("Pitch (deg)", &cameraPitchDeg_, 20.0f, 89.0f);
		ImGui::DragFloat("Stage radius", &stageRadius_, 0.1f, 5.0f, 100.0f);
	}
	ImGui::End();
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void BossEditScene::Finalize() {}
