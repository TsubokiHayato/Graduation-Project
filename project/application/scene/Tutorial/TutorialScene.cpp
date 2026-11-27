#include "TutorialScene.h"
#include "GameScenes.h"
#include "GameTime.h"
#include "Input.h"
#include "LineManager.h"
#include "SceneManager.h"
#include "TextManager.h"
#include "TextObject.h"
#include <algorithm>
#include <cmath>
#include <string>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
// テキストのレイアウト（TextManager の JSON）
constexpr char kTextLayoutPath[] = "Resources/Text/Tutorial.json";

// ステップの id（テキストの name「step_<id>_title」「step_<id>_body」に使う。enum Step の並びと対応）
constexpr const char* kStepIds[] = {"move", "melee", "ranged", "power", "shield", "core"};

// ボスの攻撃をゆっくりにする倍率（仕様書 6-2：チュートリアル中の速度倍率）
constexpr float kTutorialBossSpeed = 0.6f;

// テキストのフェードインの長さ（秒）
constexpr float kTextFadeTime = 0.3f;

constexpr float kPi = 3.14159265f;

// ステージの縁をラインで描く
void DrawStageCircle(float radius, const Math::Vector4& color) {
	constexpr int kDivisions = 96;
	LineManager* lm = LineManager::GetInstance();
	for (int i = 0; i < kDivisions; ++i) {
		const float a0 = 2.0f * kPi * static_cast<float>(i) / kDivisions;
		const float a1 = 2.0f * kPi * static_cast<float>(i + 1) / kDivisions;
		lm->DrawLine({std::cos(a0) * radius, 0.0f, std::sin(a0) * radius}, {std::cos(a1) * radius, 0.0f, std::sin(a1) * radius}, color);
	}
}
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void TutorialScene::Initialize() {
	// カメラ：自機の後ろから、ボスの方を向く（NagaiScene と同じ）
	camera_ = std::make_unique<TuboEngine::UniversalCamera>();
	SetupCamera();

	// ボス：自分では攻撃を選ばず、ステップに合わせて攻撃を予約する。チュートリアル中はゆっくり
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(camera_->GetCamera(), {0.0f, 0.0f, 0.0f});
	boss_->SetStageRadius(stageRadius_);
	boss_->SetAutoAttack(false);
	boss_->SetSpeedScale(kTutorialBossSpeed);

	// 自機：ボスを中心に旋回する
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_->GetCamera());
	player_->SetOrbitCenter(boss_->GetCenter());
	player_->SetDistanceRange(minDistance_, stageRadius_);

	// ボスの攻撃が当たったら自機にダメージ。ただしチュートリアルでは倒れないようにする
	boss_->SetHitCallback([this](float damage) {
		if (player_->GetHP()->GetCurrentHP() - damage > 0.0f) {
			player_->TakeDamage(damage);
		}
	});

	camera_->AddTarget("player", [this] { return player_->GetPosition(); });
	camera_->AddTarget("boss", [this] { return boss_->GetCenter(); });
	camera_->SetTarget("player", 0.0f);
	camera_->SetViewMode(TuboEngine::UniversalCamera::ViewMode::Behind, 0.0f);
	camera_->LockOn("boss", 0.0f);
	camera_->Update(0.0f);

	// テキスト（すべて JSON に定義してある）
	TextManager::GetInstance()->LoadTextLayout(kTextLayoutPath);
	RefreshTextRefs();

	EnterStep(kStepMove);
}

void TutorialScene::SetupCamera() {
	// 自機の後方 10m、注視点は自機から 45% ボス寄り（NagaiScene と同じ考え方）
	constexpr float kBackDistance = 10.0f;
	constexpr float kPitchDeg = 20.0f;
	constexpr float kLookRatio = 0.45f;
	const float cosPitch = std::cos(kPitchDeg * kPi / 180.0f);

	TuboEngine::UniversalCamera::Preset& preset = camera_->GetPreset(TuboEngine::UniversalCamera::ViewMode::Behind);
	preset.distance = kBackDistance / cosPitch;
	preset.pitchDeg = kPitchDeg;
	preset.yawOffsetDeg = 0.0f;
	preset.lookHeight = 2.5f;
	preset.shoulder = 0.0f;
	preset.lookAhead = 0.0f;
	preset.followYaw = true;
	preset.followSpeed = 8.0f;
	preset.fovDeg = 60.0f;
	camera_->lockOnLookWeight = kLookRatio;
	camera_->lockOnExtraDistance = kLookRatio / cosPitch;
	camera_->lockOnMaxExtraDistance = 1000.0f;
}

// =============================================================================
//  Update
// =============================================================================
void TutorialScene::Update() {
	const float dt = TuboEngine::Time::DeltaTime();
	stepTimer_ += dt;

	// ImGui の TextManager パネルでレイアウトを読み込み直したら、テキストを取り直す
	if (TextManager::GetInstance()->GetLayoutRevision() != textRevision_) {
		RefreshTextRefs();
	}

	// ---- 進行 ----
	if (step_ < kStepCount) {
		if (IsStepCompleted(step_)) {
			EnterStep(step_ + 1);
		}
	} else if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		// 完了の表示中に Enter でタイトルへ
		SceneManager::GetInstance()->ChangeScene(TITLE);
		return;
	}

	// ---- キャラクター ----
	camera_->Update(dt);

	player_->SetOrbitCenter(boss_->GetCenter());
	player_->SetDistanceRange(minDistance_, stageRadius_);
	player_->Update();

	UpdateBossForStep();
	boss_->SetStageRadius(stageRadius_);
	boss_->SetTarget(player_->GetPosition(), player_->GetRadius());
	boss_->Update(dt);

	DrawStageCircle(stageRadius_, {0.6f, 0.6f, 0.6f, 1.0f});

	// ---- テキスト ----
	UpdateTextVisual(dt);
	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  進行
// =============================================================================
void TutorialScene::EnterStep(int step) {
	step_ = (std::min)(step, static_cast<int>(kStepComplete));
	stepTimer_ = 0.0f;
	bossRequestPending_ = false;

	// 防御・弱点のステップ以外では、ボスは攻撃しない（出している攻撃も止めて待機に戻す）
	if (step_ != kStepShieldGuard && step_ != kStepCoreAttack) {
		boss_->ForcePhase(boss_->GetPhaseIndex());
	}
}

bool TutorialScene::IsStepCompleted(int step) const {
	// 各要素ができるまでは、すべて「Enter キーが押されたか」で仮に判定する。
	// 要素ができたら、case ごとに本物の判定に差し替える（下のコメントは判定の案）。
	const bool enterPressed = Input::GetInstance()->TriggerKey(DIK_RETURN);

	switch (step) {
	case kStepMove:
		// 案：自機がボスの周りを一定の角度（例：90°）回り込み、前後にも動いた
		return enterPressed;
	case kStepMelee:
		// 案：近距離攻撃がボスに当たった（弾かれてもよい）
		return enterPressed;
	case kStepRanged:
		// 案：遠距離攻撃がボスに当たった
		return enterPressed;
	case kStepPowerLever:
		// 案：レバーで出力の配分を変えた（どこかの部位を一定以上にした）
		return enterPressed;
	case kStepShieldGuard:
		// 案：全方位爆撃をバリアで受けきった（爆撃を受けても HP が減らなかった）
		return enterPressed;
	case kStepCoreAttack:
		// 案：ボスの硬直中に、背中のコアへ攻撃が当たった（Boss::TakeDamage が Hit を返した）
		return enterPressed;
	default:
		return false;
	}
}

void TutorialScene::UpdateBossForStep() {
	if (step_ != kStepShieldGuard && step_ != kStepCoreAttack) {
		return;
	}
	// 待機に戻ったら全方位爆撃を予約する（防御の練習と、その後の硬直で弱点を狙う練習をくり返せるように）
	if (boss_->GetState() == Boss::State::Acting) {
		bossRequestPending_ = false;
	} else if (boss_->GetState() == Boss::State::Idle && !bossRequestPending_) {
		boss_->RequestPattern("OmniBomb");
		bossRequestPending_ = true;
	}
}

// =============================================================================
//  テキスト
// =============================================================================
void TutorialScene::RefreshTextRefs() {
	TextManager* tm = TextManager::GetInstance();
	auto fetch = [tm](const std::string& name) {
		TextRef ref;
		ref.text = tm->GetTextByName(name);
		if (ref.text) {
			ref.baseColor = ref.text->GetColor();
		}
		return ref;
	};
	for (int i = 0; i < kStepCount; ++i) {
		stepTitles_[i] = fetch(std::string("step_") + kStepIds[i] + "_title");
		stepBodies_[i] = fetch(std::string("step_") + kStepIds[i] + "_body");
	}
	label_ = fetch("tutorial_label");
	hint_ = fetch("hint");
	completeTitle_ = fetch("complete_title");
	completeBody_ = fetch("complete_body");
	textRevision_ = tm->GetLayoutRevision();
}

void TutorialScene::UpdateTextVisual(float deltaTime) {
	(void)deltaTime;
	const float fade = std::clamp(stepTimer_ / kTextFadeTime, 0.0f, 1.0f);

	// visible なら JSON の色で表示（切り替え直後はフェードイン）、そうでなければ透明にする
	auto show = [](TextRef& ref, bool visible, float alpha) {
		if (!ref.text) {
			return;
		}
		Math::Vector4 color = ref.baseColor;
		color.w = visible ? ref.baseColor.w * alpha : 0.0f;
		ref.text->SetColor(color);
	};

	const bool complete = step_ >= kStepCount;
	for (int i = 0; i < kStepCount; ++i) {
		show(stepTitles_[i], i == step_, fade);
		show(stepBodies_[i], i == step_, fade);
	}
	show(label_, !complete, 1.0f);
	// 「Enter で次へ」はゆっくり明滅させる
	show(hint_, !complete, 0.55f + 0.45f * std::sin(stepTimer_ * 3.0f));
	show(completeTitle_, complete, fade);
	show(completeBody_, complete, fade);
}

// =============================================================================
//  描画
// =============================================================================
void TutorialScene::Object3DDraw() {
	boss_->Draw();
	player_->Draw();
}

void TutorialScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void TutorialScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void TutorialScene::ImGuiDraw() {
#ifdef USE_IMGUI
	if (ImGui::Begin("Tutorial")) {
		static const char* kStepNames[] = {"1. 移動", "2. 近距離攻撃", "3. 遠距離攻撃", "4. 出力の調整", "5. シールドで防御", "6. 弱点を攻撃", "完了"};
		ImGui::Text("Step : %s", kStepNames[step_]);
		ImGui::TextDisabled("達成の判定は仮：Enter キーで次へ");
		if (ImGui::Button("< 前へ") && step_ > 0) {
			EnterStep(step_ - 1);
		}
		ImGui::SameLine();
		if (ImGui::Button("次へ >") && step_ < kStepCount) {
			EnterStep(step_ + 1);
		}
		ImGui::SameLine();
		if (ImGui::Button("最初から")) {
			boss_->Restart();
			boss_->SetAutoAttack(false);
			EnterStep(kStepMove);
		}
		ImGui::Text("テキスト : %s（TextManager パネルで調整して Save）", kTextLayoutPath);
	}
	ImGui::End();

	camera_->DrawImGui();
	boss_->DrawImGui();
	if (ImGui::Begin("Player")) {
		player_->DrawImGui();
	}
	ImGui::End();
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void TutorialScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
