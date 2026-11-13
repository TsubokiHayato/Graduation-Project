#include "SatouScene.h"
#include "BossHitDebugDraw.h"
#include "Input.h"
#include "TextManager.h"
#include <cmath>
#include "LineManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
	constexpr float kDegToRad = 3.14159265f / 180.0f;
}

// =============================================================================
//  Initialize
// =============================================================================
void SatouScene::Initialize() {
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	// 真上寄りに引いて、XZ平面の判定が見やすい位置にする
	camera_->SetTranslate({ 0.0f, 18.0f, -12.0f });
	camera_->setRotation({ 1.0f, 0.0f, 0.0f });
	camera_->setScale({ 1.0f, 1.0f, 1.0f });
	camera_->Update();

	debugCamera_ = std::make_unique<DebugCamera>();

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_.get());
	axis_->SetPosition({ 0.0f, 0.0f, 0.0f });

	warningRenderer_.Initialize(camera_.get());
}

// =============================================================================
//  Update
// =============================================================================
void SatouScene::Update() {
	camera_->Update();
	debugCamera_->Update(camera_.get());
	axis_->Update();
	warning_.Update((1.0f / 60.0f) * warnTimeScale_);
	warningRenderer_.Update(warning_);

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void SatouScene::Object3DDraw() {
	axis_->Draw();

	using namespace BossHitDebug;
	using Vec3 = TuboEngine::Math::Vector3;

	const Vec3 boss = { 0.0f, 0.0f, 0.0f };
	const Vec3 player = { playerX_, 0.0f, playerZ_ };

	// 角度(度) → 向きベクトル（0度 = +Z、時計回りに増える）
	auto dirOf = [](float deg) { return Vec3{ std::sin(deg * kDegToRad), 0.0f, std::cos(deg * kDegToRad) }; };

	// --- 形状データを作り、判定する（本番のGP-16〜24でもこの形で使う想定） ---
	BossHit::Circle circle{ boss, circleRadius_ };
	BossHit::Beam beam{ boss, dirOf(beamAngle_), beamLength_, beamWidth_ };
	BossHit::Fan fan{ boss, dirOf(bossYaw_), fanRadius_,BossHit::WarningRenderer::FanPresetRad(warnFanIdx_) };
	BossHit::Bullet bullet{ {bulletX_, 0.0f, bulletZ_}, bulletRadius_ };

	hitCircle_ = BossHit::HitCircle(circle, player, playerRadius_);
	hitBeam_ = BossHit::HitBeam(beam, player, playerRadius_);
	hitFan_ = BossHit::HitFan(fan, player, playerRadius_);
	hitBullet_ = BossHit::HitBullet(bullet, player, playerRadius_);

	// --- 描画（当たっていれば赤、外れていれば白） ---
	if (showCircle_) DrawCircle(circle, Color(hitCircle_));
	if (showBeam_) {
		DrawBeam(beam, Color(hitBeam_));
		// 自機の大きさ分を足した「実際に効く幅」を薄く表示
		DrawBeam(beam, { 0.5f, 0.5f, 0.5f, 1.0f }, playerRadius_);
	}
	if (showFan_) DrawFan(fan, Color(hitFan_));
	if (showBullet_) DrawBullet(bullet, Color(hitBullet_));

	if (warnStart_) {
		warnStart_ = false;
		BossHit::Shape s = (warnType_ == 0) ? BossHit::Shape{ circle }
			: (warnType_ == 1) ? BossHit::Shape{ beam }
		: BossHit::Shape{ fan };
		warning_.Start(s, warnTelegraph_, warnHitSec_);
	}

	if (warning_.IsActive()) {
		bool hitPhase = warning_.IsHitActive();
		DrawShape(warning_.GetShape(), { 1.0f, 0.4f, 0.4f, 1.0f });                               // 外枠
		DrawShape(warning_.GetFillShape(), hitPhase ? Vec4{ 1, 1, 1, 1 } : Vec4{ 1, 0, 0, 1 });     // 広がる塗り
		if (hitPhase) warnHitResult_ = BossHit::Hit(warning_.GetShape(), player, playerRadius_);
	}

	warningRenderer_.Draw();

	// 自機（緑の球）とボス位置（黄）
	TuboEngine::LineManager::GetInstance()->DrawSphere(player, playerRadius_, { 0.0f, 1.0f, 0.0f, 1.0f }, 16);
	TuboEngine::LineManager::GetInstance()->DrawSphere(boss, 0.3f, { 1.0f, 1.0f, 0.0f, 1.0f }, 8);
}

void SatouScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void SatouScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void SatouScene::ImGuiDraw() {
#ifdef USE_IMGUI
	axis_->DrawImGui("3D Object : axis");

	ImGui::Begin("BossHit Test");

	debugCamera_->DrawImGui();

	if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("x", &playerX_, -15.0f, 15.0f);
		ImGui::SliderFloat("z", &playerZ_, -15.0f, 15.0f);
		ImGui::SliderFloat("radius", &playerRadius_, 0.1f, 2.0f);
	}

	auto hitText = [](const char* name, bool hit) {
		ImGui::TextColored(hit ? ImVec4(1, 0.2f, 0.2f, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1), "%s : %s", name, hit ? "HIT" : "-");
		};

	if (ImGui::CollapsingHeader("Circle", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("show##c", &showCircle_);
		ImGui::SliderFloat("radius##c", &circleRadius_, 0.5f, 15.0f);
		//hitText("circle", hitCircle_);
	}
	if (ImGui::CollapsingHeader("Beam", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("show##b", &showBeam_);
		ImGui::SliderFloat("angle(deg)##b", &beamAngle_, -180.0f, 180.0f);
		ImGui::SliderFloat("length##b", &beamLength_, 1.0f, 20.0f);
		ImGui::SliderFloat("width##b", &beamWidth_, 0.2f, 6.0f);
		//hitText("beam", hitBeam_);
	}
	if (ImGui::CollapsingHeader("Fan", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("show##f", &showFan_);
		ImGui::SliderFloat("boss yaw(deg)##f", &bossYaw_, -180.0f, 180.0f);
		ImGui::SliderFloat("radius##f", &fanRadius_, 0.5f, 15.0f);
		static const char* kFanLabels[] = { "30", "45", "60", "90", "120", "180", "270" };
		static_assert(IM_ARRAYSIZE(kFanLabels) == BossHit::WarningRenderer::kFanPresetCount, "プリセット数とラベル数を揃える");
		ImGui::Combo("angle(deg)##f", &warnFanIdx_, kFanLabels, IM_ARRAYSIZE(kFanLabels));

		//hitText("fan", hitFan_);
	}
	if (ImGui::CollapsingHeader("Bullet", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("show##u", &showBullet_);
		ImGui::SliderFloat("x##u", &bulletX_, -15.0f, 15.0f);
		ImGui::SliderFloat("z##u", &bulletZ_, -15.0f, 15.0f);
		ImGui::SliderFloat("radius##u", &bulletRadius_, 0.1f, 3.0f);
		hitText("bullet", hitBullet_);
	}
	if (ImGui::CollapsingHeader("Warning", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Combo("shape", &warnType_, "Circle\0Beam\0Fan\0");
		ImGui::SliderFloat("telegraph(s)", &warnTelegraph_, 0.2f, 5.0f);
		ImGui::SliderFloat("hit window(s)", &warnHitSec_, 0.02f, 0.5f);
		ImGui::SliderFloat("time scale", &warnTimeScale_, 0.1f, 2.0f);
		if (ImGui::Button("Start")) warnStart_ = true;
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) warning_.Cancel();
		ImGui::Text("progress: %.2f", warning_.GetProgress());
		ImGui::TextColored(warnHitResult_ ? ImVec4(1, 0.2f, 0.2f, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1),
			"last hit: %s", warnHitResult_ ? "HIT" : "MISS");
	}
	ImGui::End();
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void SatouScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}