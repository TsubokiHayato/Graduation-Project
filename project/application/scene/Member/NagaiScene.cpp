#include "NagaiScene.h"
#include "GameTime.h"
#include "LineManager.h"
#include "TextManager.h"
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
			lm->DrawLine({ std::cos(a0) * radius, 0.0f, std::sin(a0) * radius }, { std::cos(a1) * radius, 0.0f, std::sin(a1) * radius }, color);
		}
	}
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void NagaiScene::Initialize() {
	// ボスの後ろから自機を追うカメラ（追従先とロックオン先はボス・自機の生成後に登録する）
	camera_ = std::make_unique<TuboEngine::UniversalCamera>();
	SetupCamera();

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_->GetCamera());
	axis_->SetPosition({ 0.0f, 0.0f, 0.0f });

	// ボス（見た目のオブジェクトはボスクラスが持つ）
	boss_ = std::make_unique<Boss>();
	boss_->Initialize(camera_->GetCamera(), { 0.0f, 0.0f, 0.0f });
	boss_->SetStageRadius(stageRadius_);

	// プレイヤ―の生成、初期化処理
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_->GetCamera());
	// ボスを中心に旋回させる
	player_->SetOrbitCenter(boss_->GetCenter());
	player_->SetDistanceRange(minDistance_, stageRadius_);

	// 攻撃が当たったら自機にダメージ（無敵・ダッシュ中の判定は Player 側で行う）
	boss_->SetHitCallback([this](float damage) { player_->TakeDamage(damage); });

	// 追従先 = 自機、ロックオン先 = ボス。
	// ロックオン中のカメラは「自機の後ろから、ボスの方向」を向くので、ボス → 自機 → カメラ が一直線に並ぶ
	camera_->AddTarget("player", [this] { return player_->GetPosition(); });
	camera_->AddTarget("boss", [this] { return boss_->GetCenter(); });
	camera_->SetTarget("player", 0.0f);
	camera_->SetViewMode(TuboEngine::UniversalCamera::ViewMode::Behind, 0.0f);
	camera_->LockOn("boss", 0.0f);
	camera_->Update(0.0f);
}

// =============================================================================
//  カメラの設定（仕様書 v2「5. カメラ」に合わせた初期値）
// =============================================================================
void NagaiScene::SetupCamera() {
	// 仕様書の値：自機の後方 10m、注視点は自機から 45% ボス寄り・高さ 2.5m、追従の速さ 8、画角 60°
	constexpr float kBackDistance = 10.0f; // 自機からカメラまでの水平距離
	constexpr float kPitchDeg = 20.0f;     // 見下ろす角度
	constexpr float kLookRatio = 0.45f;    // 注視点を自機からボス寄りにする割合
	constexpr float kDegToRad = 3.14159265f / 180.0f;
	const float cosPitch = std::cos(kPitchDeg * kDegToRad);

	TuboEngine::UniversalCamera::Preset& preset = camera_->GetPreset(TuboEngine::UniversalCamera::ViewMode::Behind);
	preset.distance = kBackDistance / cosPitch; // 注視点からの距離（斜め）
	preset.pitchDeg = kPitchDeg;
	preset.yawOffsetDeg = 0.0f; // 真後ろからずらさない（ずらすと一直線にならない）
	preset.lookHeight = 2.5f;   // 注視点の高さ
	preset.shoulder = 0.0f;     // 肩越しの横ずれなし（ずらすと一直線にならない）
	preset.lookAhead = 0.0f;
	preset.followYaw = true;
	preset.followSpeed = 8.0f; // 追従の速さ
	preset.fovDeg = 60.0f;     // 画角

	// 注視点がボス寄りにずれた分だけカメラを引き、自機とカメラの水平距離を一定（kBackDistance）に保つ
	camera_->lockOnLookWeight = kLookRatio;
	camera_->lockOnExtraDistance = kLookRatio / cosPitch;
	camera_->lockOnMaxExtraDistance = 1000.0f;
}

// =============================================================================
//  Update
// =============================================================================
void NagaiScene::Update() {
	const float dt = TuboEngine::Time::DeltaTime();

	// カメラ（ボス → 自機 → カメラ の一直線）。
	// Object3d は Update 時のカメラで行列を作るので、オブジェクトより先に更新する（追従は前フレームの位置）
	camera_->Update(dt);

	// プレイヤ―更新処理（移動・ダッシュ・HP）。ボスを中心に旋回する
	player_->SetOrbitCenter(boss_->GetCenter());
	player_->SetDistanceRange(minDistance_, stageRadius_);
	player_->Update();

	// ボス（自機の最新位置を渡してから更新する）
	boss_->SetStageRadius(stageRadius_);
	boss_->SetTarget(player_->GetPosition(), player_->GetRadius());
	boss_->Update(dt); // 攻撃範囲のラインもここで描かれる

	// デバッグライン：ステージの縁
	DrawStageCircle(stageRadius_, { 0.6f, 0.6f, 0.6f, 1.0f });

	axis_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void NagaiScene::Object3DDraw() {
	axis_->Draw();
	boss_->Draw();

	// プレイヤ―描画処理
	player_->Draw();
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
	camera_->DrawImGui(); // カメラの距離・角度・追従の速さなどを調整できる

	if (ImGui::Begin("Player")) {
		ImGui::DragFloat("Stage radius", &stageRadius_, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Min distance", &minDistance_, 0.1f, 0.0f, stageRadius_);
		ImGui::Separator();
		player_->DrawImGui();
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
