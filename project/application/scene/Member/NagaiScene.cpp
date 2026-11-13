#include "NagaiScene.h"
#include "GameTime.h"
#include "Input.h"
#include "LineManager.h"
#include "TextManager.h"
#include <algorithm>
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
// ステージ（地面の円）をラインで描く
void DrawStageCircle(float radius, const Math::Vector4& color) {
	constexpr int kDivisions = 96;
	constexpr float kTwoPi = 6.28318530718f;
	LineManager* lm = LineManager::GetInstance();
	for (int i = 0; i < kDivisions; ++i) {
		const float a0 = kTwoPi * static_cast<float>(i) / kDivisions;
		const float a1 = kTwoPi * static_cast<float>(i + 1) / kDivisions;
		lm->DrawLine({std::cos(a0) * radius, 0.0f, std::sin(a0) * radius}, {std::cos(a1) * radius, 0.0f, std::sin(a1) * radius}, color);
	}
}
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void NagaiScene::Initialize() {
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({0.0f, 28.0f, -38.0f});
	camera_->setRotation({0.6f, 0.0f, 0.0f});
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_.get());
	axis_->SetPosition({0.0f, 0.0f, 0.0f});

	// ボス（見た目のオブジェクトはボスクラスが持つ）
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(camera_.get(), {0.0f, 0.0f, 0.0f});
	boss_->SetStageRadius(stageRadius_);
	// 攻撃が当たったら仮の自機にダメージ（シールド配分で軽減）
	boss_->SetHitCallback([this](float damage) {
		const float reduction = maxReduction_ * shieldRatio_;
		lastDamage_ = damage * (1.0f - reduction);
		playerHp_ = (std::max)(playerHp_ - lastDamage_, 0.0f);
	});

	// 動作確認用の仮の自機
	dummyPlayer_ = std::make_unique<Object3d>();
	dummyPlayer_->Initialize("sphere/sphere.obj");
	dummyPlayer_->SetCamera(camera_.get());
	dummyPlayer_->SetScale({playerRadius_, playerRadius_, playerRadius_});
	playerHp_ = playerMaxHp_;
}

// =============================================================================
//  Update
// =============================================================================
void NagaiScene::Update() {
	const float dt = TuboEngine::Time::DeltaTime();

	// 仮の自機：矢印キーで 上下=距離 / 左右=角度、1/2 キーでシールド配分を増減
	Input* input = Input::GetInstance();
	if (input->PushKey(DIK_UP))    { playerDistance_ -= 8.0f * dt; }
	if (input->PushKey(DIK_DOWN))  { playerDistance_ += 8.0f * dt; }
	if (input->PushKey(DIK_LEFT))  { playerAngle_ -= 1.5f * dt; }
	if (input->PushKey(DIK_RIGHT)) { playerAngle_ += 1.5f * dt; }
	if (input->PushKey(DIK_1))     { shieldRatio_ -= 1.0f * dt; }
	if (input->PushKey(DIK_2))     { shieldRatio_ += 1.0f * dt; }
	playerDistance_ = std::clamp(playerDistance_, 2.0f, stageRadius_);
	shieldRatio_ = std::clamp(shieldRatio_, 0.0f, 1.0f);

	const Math::Vector3 bossCenter = boss_->GetCenter();
	const Math::Vector3 playerPos = {
		bossCenter.x + std::cos(playerAngle_) * playerDistance_,
		playerRadius_,
		bossCenter.z + std::sin(playerAngle_) * playerDistance_,
	};
	dummyPlayer_->SetPosition(playerPos);
	dummyPlayer_->SetScale({playerRadius_, playerRadius_, playerRadius_});
	// シールド配分が高いほど青く
	dummyPlayer_->SetModelColor({1.0f - 0.7f * shieldRatio_, 1.0f - 0.3f * shieldRatio_, 1.0f, 1.0f});

	// ボス
	boss_->SetStageRadius(stageRadius_);
	boss_->SetTarget(playerPos, playerRadius_);
	boss_->Update(dt); // 攻撃範囲のラインもここで描かれる

	// デバッグライン：ステージの縁
	DrawStageCircle(stageRadius_, {0.6f, 0.6f, 0.6f, 1.0f});

	camera_->Update();
	axis_->Update();
	dummyPlayer_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void NagaiScene::Object3DDraw() {
	axis_->Draw();
	boss_->Draw();
	dummyPlayer_->Draw();
}

void NagaiScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void NagaiScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void NagaiScene::ImGuiDraw() {
#ifdef USE_IMGUI
	axis_->DrawImGui("3D Object : axis");
	boss_->DrawImGui();

	if (ImGui::Begin("Dummy Player")) {
		ImGui::Text("矢印キー: 距離/角度  1/2: シールド配分");
		ImGui::DragFloat("Stage radius", &stageRadius_, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Distance", &playerDistance_, 0.1f, 0.0f, stageRadius_);
		ImGui::SliderAngle("Angle", &playerAngle_, -180.0f, 180.0f);
		ImGui::DragFloat("Radius", &playerRadius_, 0.01f, 0.1f, 3.0f);
		ImGui::SliderFloat("Shield ratio", &shieldRatio_, 0.0f, 1.0f);
		ImGui::SliderFloat("Max reduction", &maxReduction_, 0.0f, 0.99f);
		ImGui::DragFloat("Max HP", &playerMaxHp_, 1.0f, 1.0f, 1000.0f);
		ImGui::Text("HP : %.1f / %.1f", playerHp_, playerMaxHp_);
		ImGui::Text("Last damage : %.1f", lastDamage_);
		if (ImGui::Button("Reset HP")) {
			playerHp_ = playerMaxHp_;
			lastDamage_ = 0.0f;
		}
	}
	ImGui::End();
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void NagaiScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
