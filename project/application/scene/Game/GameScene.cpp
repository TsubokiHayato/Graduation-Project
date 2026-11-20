#include "GameScene.h"
#include "GameTime.h"
#include "Input.h"
#include "LineManager.h"
#include "TextManager.h"
#include "EditorObjects.h"
#include <algorithm>
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
// 地面の高さ
constexpr float kGroundHeight = 0.0f;
// プレイヤー（block.obj は 2x2x2 の箱）の中心から足元までの高さ
constexpr float kPlayerHalfHeight = 1.0f;

// ステージの縁をラインで描く（Nagai シーンより）
void DrawStageCircle(float radius, const Math::Vector4& color) {
	constexpr int kDivisions = 96;
	constexpr float kTwoPi = 6.28318530718f;
	LineManager* lm = LineManager::GetInstance();
	for (int i = 0; i < kDivisions; ++i) {
		const float a0 = kTwoPi * static_cast<float>(i) / kDivisions;
		const float a1 = kTwoPi * static_cast<float>(i + 1) / kDivisions;
		lm->DrawLine({std::cos(a0) * radius, kGroundHeight, std::sin(a0) * radius},
		             {std::cos(a1) * radius, kGroundHeight, std::sin(a1) * radius}, color);
	}
}
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void GameScene::Initialize() {
	// ---- カメラ（Tsuboki の UniversalCamera） ----
	camera_ = std::make_unique<TuboEngine::UniversalCamera>();
	if (!camera_->LoadPresets("Resources/Camera/presets.json", /*logIfMissing*/ false)) {
		// 保存が無いときの初期値（ボスとプレイヤーが両方入るよう引き気味）
		auto& behind = camera_->GetPreset(TuboEngine::UniversalCamera::ViewMode::Behind);
		behind.distance = 16.0f;
		behind.pitchDeg = 22.0f;
		behind.lookHeight = 1.0f;
		behind.fovDeg = 35.0f;
	}

	// ---- 地形（Miyamoto） ----
	// terrain.obj は 2x2 の平面（-1〜1）なので、ステージ半径ぶん拡大して円がすべて乗るようにする
	terrain_ = std::make_unique<Object3d>();
	terrain_->Initialize("terrain/terrain.obj");
	terrain_->SetCamera(camera_->GetCamera());
	terrain_->SetPosition({0.0f, kGroundHeight, 0.0f});
	terrain_->SetScale({stageRadius_, 1.0f, stageRadius_});

	// ---- プレイヤー（Onodera） ----
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_->GetCamera());
	{
		// Player.json の初期位置は y=-2 なので、箱の底が地面に付く高さに直す
		Object3d* playerObject = player_->GetPlayerObject();
		Math::Vector3 pos = playerObject->GetPosition();
		pos.y = kGroundHeight + kPlayerHalfHeight;
		playerObject->SetPosition(pos);
	}

	// ---- ブースター（Tsuboki） ----
	booster_ = std::make_unique<PlayerBooster>();
	booster_->SetMountOffset(0, {0.6f, 0.3f, -1.0f});  // 右
	booster_->SetMountOffset(1, {-0.6f, 0.3f, -1.0f}); // 左
	booster_->Initialize(player_->GetPlayerObject(), camera_->GetCamera());
	booster_->SetGroundHeight(kGroundHeight);

	// ---- ボス（Nagai） ----
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(camera_->GetCamera(), {0.0f, kGroundHeight, 0.0f});
	boss_->SetStageRadius(stageRadius_);
	boss_->SetHitCallback([this](float damage) { OnBossHit(damage); });

	// ---- 攻撃予兆（Satou） ----
	warningRenderer_.Initialize(camera_->GetCamera());

	// ---- カメラの追従対象 ----
	// player は足元の位置を渡す（プリセットが足元基準のため）
	camera_->AddTarget("player", [this] {
		Math::Vector3 pos = player_->GetPlayerObject()->GetPosition();
		pos.y -= kPlayerHalfHeight;
		return pos;
	});
	camera_->AddTarget("boss", [this] { return boss_->GetObject3d()->GetPosition(); });
	camera_->SetTarget("player", 0.0f);
	camera_->SetViewMode(TuboEngine::UniversalCamera::ViewMode::Behind, 0.0f);
	camera_->Update(0.0f);
}

// =============================================================================
//  Update
// =============================================================================
void GameScene::Update() {
	const float dt = TuboEngine::Time::DeltaTime();

	// ---- Q でボスにロックオン切り替え（ゼルダの注目） ----
	if (Input::GetInstance()->TriggerKey(DIK_Q)) {
		if (camera_->IsLockedOn()) {
			camera_->ClearLockOn();
		} else {
			camera_->LockOn("boss");
		}
	}
	// ロックオン中は、移動をボス基準（W/S = 近づく/離れる、A/D = 周りを回る）にし、ボスの方を向く
	if (camera_->IsLockedOn()) {
		const Math::Vector3 bossPos = boss_->GetObject3d()->GetPosition();
		player_->GetMovement()->SetLockOnTarget(bossPos);
		booster_->SetFaceTarget(bossPos);
	} else {
		player_->GetMovement()->ClearLockOnTarget();
		booster_->ClearFaceTarget();
	}

	// ---- プレイヤー（WASD 移動 / Space ダッシュ） ----
	player_->Update();
	ClampPlayerToStage();

	// ブースター：出力はエネルギー配分、ダッシュ開始の瞬間にバースト
	booster_->Update(player_->GetEnergyOutput()->GetBoosterOutputRate(), player_->GetMovement()->IsDashing());
	// プレイヤー本体を移動方向に向ける（ノズルが常に背中側に来るように）
	Object3d* playerObject = player_->GetPlayerObject();
	Math::Vector3 playerRotate = playerObject->GetRotation();
	playerRotate.y = booster_->GetYaw();
	playerObject->SetRotation(playerRotate);
	playerObject->Update();

	// ---- ボス ----
	Math::Vector3 playerFoot = playerObject->GetPosition();
	playerFoot.y = kGroundHeight;
	boss_->SetStageRadius(stageRadius_);
	boss_->SetTarget(playerFoot, playerRadius_);
	if (player_->GetState() == PlayerState::Alive) {
		boss_->Update(dt); // 攻撃範囲のラインもここで描かれる
	}

	// カメラはキャラクターを動かした後、カメラに依存する描画物の更新前に更新する
	camera_->Update(dt);

	// ---- 予兆（カメラ更新後） ----
	UpdateWarning(dt);

	terrain_->Update();
	DrawStageCircle(stageRadius_, {0.6f, 0.6f, 0.6f, 1.0f});

	TextManager::GetInstance()->UpdateAll();
}

void GameScene::ClampPlayerToStage() {
	Object3d* playerObject = player_->GetPlayerObject();
	Math::Vector3 pos = playerObject->GetPosition();
	const Math::Vector3 center = boss_ ? boss_->GetCenter() : Math::Vector3{0.0f, 0.0f, 0.0f};
	const float dx = pos.x - center.x;
	const float dz = pos.z - center.z;
	const float limit = (std::max)(stageRadius_ - playerRadius_, 0.0f);
	const float dist = std::sqrt(dx * dx + dz * dz);
	if (dist > limit && dist > 0.0001f) {
		pos.x = center.x + dx / dist * limit;
		pos.z = center.z + dz / dist * limit;
		playerObject->SetPosition(pos);
	}
}

void GameScene::OnBossHit(float damage) {
	// シールド配分（0〜1）で軽減する。式は Nagai シーンと同じ
	const PlayerEnergyData& energy = player_->GetEnergyOutput()->GetEnergyData();
	const float shieldRatio = energy.maxenergy > 0.0f ? std::clamp(energy.shield / energy.maxenergy, 0.0f, 1.0f) : 0.0f;
	const float reduced = damage * (1.0f - maxReduction_ * shieldRatio);
	if (player_->GetHP()->IsInvincible() || player_->GetState() == PlayerState::Dead) {
		return;
	}
	lastDamage_ = reduced;
	player_->TakeDamage(reduced);
}

void GameScene::UpdateWarning(float dt) {
	warning_.Update(dt);
	if (warning_.JustHit()) {
		Math::Vector3 playerFoot = player_->GetPlayerObject()->GetPosition();
		playerFoot.y = kGroundHeight;
		warnLastHit_ = BossHit::Hit(warning_.GetShape(), playerFoot, playerRadius_);
		if (warnLastHit_) {
			OnBossHit(warnDamage_);
		}
	}
	warningRenderer_.Update(warning_);
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void GameScene::Object3DDraw() {
	terrain_->Draw();
	warningRenderer_.Draw(); // 地面に貼る警告範囲
	boss_->Draw();
	player_->Draw();
	booster_->Draw();
}

void GameScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void GameScene::ParticleDraw() {
	booster_->DrawParticles();
}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void GameScene::ImGuiDraw() {
#ifdef USE_IMGUI
	camera_->DrawImGui();
	TuboEngine::Editor::Inspect("terrain", terrain_.get());

	player_->DrawImGui();
	boss_->DrawImGui();

	ImGui::Begin("Booster");
	booster_->DrawImGui();
	ImGui::End();

	if (ImGui::Begin("Game")) {
		ImGui::Text("WASD: 移動  Space: ダッシュ  Q: ボスにロックオン");
		ImGui::Text("ロックオン中: W/S = 近づく/離れる  A/D = ボスの周りを回る");
		ImGui::Text("Lock-on: %s", camera_->IsLockedOn() ? "ON" : "OFF");
		ImGui::DragFloat("Stage radius", &stageRadius_, 0.1f, 5.0f, 100.0f);
		if (ImGui::IsItemEdited()) {
			terrain_->SetScale({stageRadius_, 1.0f, stageRadius_});
		}
		ImGui::DragFloat("Player hit radius", &playerRadius_, 0.01f, 0.1f, 3.0f);
		ImGui::SliderFloat("Max shield reduction", &maxReduction_, 0.0f, 0.99f);
		ImGui::Text("HP : %.1f / %.1f", player_->GetHP()->GetCurrentHP(), player_->GetHP()->GetMaxHP());
		ImGui::Text("Last damage : %.1f", lastDamage_);
		if (player_->GetState() == PlayerState::Dead) {
			ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "DEAD（Player ウィンドウの Reset HP で復帰）");
		}

		if (ImGui::CollapsingHeader("Warning Test (Satou)", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Combo("shape", &warnType_, "Circle\0Beam\0Fan\0");
			static const char* kFanLabels[] = {"30", "45", "60", "90", "120", "180", "270"};
			static_assert(IM_ARRAYSIZE(kFanLabels) == BossHit::WarningRenderer::kFanPresetCount, "プリセット数とラベル数を揃える");
			ImGui::Combo("fan angle(deg)", &warnFanIdx_, kFanLabels, IM_ARRAYSIZE(kFanLabels));
			ImGui::SliderFloat("telegraph(s)", &warnTelegraph_, 0.2f, 5.0f);
			ImGui::SliderFloat("hit window(s)", &warnHitSec_, 0.02f, 0.5f);
			ImGui::DragFloat("damage", &warnDamage_, 1.0f, 0.0f, 200.0f);
			if (ImGui::Button("Start")) {
				// ボス中心から自機の方向へ向けた形状を作る
				const Math::Vector3 c = boss_->GetCenter();
				Math::Vector3 p = player_->GetPlayerObject()->GetPosition();
				Math::Vector3 dir = {p.x - c.x, 0.0f, p.z - c.z};
				const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
				dir = len > 0.0001f ? Math::Vector3{dir.x / len, 0.0f, dir.z / len} : Math::Vector3{0.0f, 0.0f, 1.0f};
				const Math::Vector3 origin = {c.x, kGroundHeight, c.z};
				BossHit::Shape s = (warnType_ == 0) ? BossHit::Shape{BossHit::Circle{origin, stageRadius_ * 0.4f}}
				                 : (warnType_ == 1) ? BossHit::Shape{BossHit::Beam{origin, dir, stageRadius_, 3.0f}}
				                                    : BossHit::Shape{BossHit::Fan{origin, dir, stageRadius_ * 0.6f, BossHit::WarningRenderer::FanPresetRad(warnFanIdx_)}};
				warning_.Start(s, warnTelegraph_, warnHitSec_);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				warning_.Cancel();
			}
			ImGui::Text("progress: %.2f", warning_.GetProgress());
			ImGui::TextColored(warnLastHit_ ? ImVec4(1, 0.2f, 0.2f, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1), "last hit: %s", warnLastHit_ ? "HIT" : "MISS");
		}
	}
	ImGui::End();
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void GameScene::Finalize() {
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
