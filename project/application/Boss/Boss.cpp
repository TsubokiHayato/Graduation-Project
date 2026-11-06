#include "Boss.h"
#include "BossAttackBarrage.h"
#include "BossAttackLockOnBeam.h"
#include "BossAttackOmniBomb.h"
#include "BossAttackState.h"
#include "BossAttackSweep.h"
#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

Boss::Boss() = default;
Boss::~Boss() = default;

// =============================================================================
//  Initialize
// =============================================================================
void Boss::Initialize(TuboEngine::Camera* camera, const Math::Vector3& center) {
	// 見た目（仮モデル）
	object_ = std::make_unique<Object3d>();
	object_->Initialize("block/block.obj");
	object_->SetCamera(camera);
	object_->SetScale({1.0f, 1.0f, 1.0f});

	center_ = center;
	height_ = 0.0f;
	yaw_ = 0.0f;
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// 攻撃ステートの生成
	omniBomb_ = std::make_unique<BossAttackOmniBomb>();
	lockOnBeam_ = std::make_unique<BossAttackLockOnBeam>();
	barrage_ = std::make_unique<BossAttackBarrage>();
	sweep_ = std::make_unique<BossAttackSweep>();
	attacks_ = {omniBomb_.get(), lockOnBeam_.get(), barrage_.get(), sweep_.get()};

	currentAttack_ = nullptr;
	nextAttackIndex_ = 0;
	SelectNextAttack();
	ApplyObjectTransform();
}

// =============================================================================
//  Update
// =============================================================================
void Boss::Update(float deltaTime) {
	const float dt = dePaused_ ? 0.0f : deltaTime * speedScale_;

	// 行動サイクル：攻撃が終わったら次の攻撃を選ぶ
	if (!currentAttack_ || currentAttack_->IsFinished()) {

		SelectNextAttack();
	}

	currentAttack_->Update(*this, dt);

	if (deDrawLines_) {

		currentAttack_->DrawLines(*this);
	}

	ApplyObjectTransform();

	object_->Update();
}

void Boss::Draw() {

	object_->Draw();
}

// =============================================================================
//  攻撃の切り替え
// =============================================================================
void Boss::SelectNextAttack() {

	// TODO: 行動順の制御（4-6）。全方位爆撃は決まったタイミング、残り3種はランダム
	//       現状は登録順（全方位爆撃 → ロックオンビーム → 弾幕射撃 → 近接薙ぎ払い → …）にくり返す

	//攻撃番号が指定されていたら特定の攻撃の実を繰り返す
	if (deForcedAttackIndex_ >= 0 && deForcedAttackIndex_ < static_cast<int>(attacks_.size())) {

		ChangeAttack(attacks_[deForcedAttackIndex_]);

		return;
	}

	//次の番号の攻撃に変更
	ChangeAttack(attacks_[nextAttackIndex_]);

	nextAttackIndex_ = (nextAttackIndex_ + 1) % attacks_.size();
}

void Boss::ChangeAttack(BossAttackState* next) {
	currentAttack_ = next;
	currentAttack_->Enter(*this);
}

// =============================================================================
//  向き
//  角度 θ の方向は (cosθ, 0, sinθ)。Object3d の Y 回転 yaw で前方(+Z)は (sin yaw, 0, cos yaw) を向くので
//  yaw = π/2 − θ で対応させる
// =============================================================================
void Boss::SetFacingAngle(float angle) {
	yaw_ = std::numbers::pi_v<float> * 0.5f - angle;
}

float Boss::GetFacingAngle() const {
	return std::numbers::pi_v<float> * 0.5f - yaw_;
}

void Boss::TurnTowardTarget(float deltaTime) {
	constexpr float kPi = std::numbers::pi_v<float>;
	const float targetAngle = std::atan2(targetPosition_.z - center_.z, targetPosition_.x - center_.x);
	float diff = targetAngle - GetFacingAngle();
	// -π〜π に収めて近い方へ回る
	diff = std::remainder(diff, 2.0f * kPi);
	const float maxStep = turnSpeed_ * deltaTime;
	SetFacingAngle(GetFacingAngle() + std::clamp(diff, -maxStep, maxStep));
	// 角度が大きくなり続けないように yaw を -π〜π に戻す
	yaw_ = std::remainder(yaw_, 2.0f * kPi);
}

// =============================================================================
//  見た目の反映
// =============================================================================
void Boss::ApplyObjectTransform() {
	object_->SetPosition({center_.x, center_.y + baseHeight_ + height_, center_.z});
	object_->SetRotation({0.0f, yaw_, 0.0f});
	object_->SetModelColor(color_);
}

// =============================================================================
//  ImGui
// =============================================================================
void Boss::DrawImGui() {
#ifdef USE_IMGUI
	object_->DrawImGui("3D Object : Boss");

	if (!ImGui::Begin("Boss")) {
		ImGui::End();
		return;
	}

	static const char* kStepNames[] = {"Telegraph (予兆)", "Attack (攻撃)", "Recovery (硬直)", "Finished"};
	if (currentAttack_) {
		ImGui::Text("Attack : %s", currentAttack_->GetName());
		ImGui::Text("Step   : %s", kStepNames[static_cast<int>(currentAttack_->GetStep())]);
		ImGui::Text("Timer  : %.2f", currentAttack_->GetStepTimer());
	}
	ImGui::Checkbox("Pause", &dePaused_);
	ImGui::SameLine();
	ImGui::Checkbox("Draw lines", &deDrawLines_);
	if (ImGui::Button("Restart attack") && currentAttack_) {
		currentAttack_->Enter(*this);
	}
	ImGui::DragFloat("Speed scale", &speedScale_, 0.01f, 0.1f, 3.0f);
	ImGui::DragFloat("Turn speed (rad/s)", &turnSpeed_, 0.01f, 0.0f, 10.0f);

	// デバッグ：出す攻撃を固定する
	std::vector<const char*> items = {"Loop (順番)"};
	for (BossAttackState* attack : attacks_) {
		items.push_back(attack->GetName());
	}
	int selected = deForcedAttackIndex_ + 1;
	if (ImGui::Combo("Attack select", &selected, items.data(), static_cast<int>(items.size()))) {
		deForcedAttackIndex_ = selected - 1;
	}

	ImGui::Separator();
	for (BossAttackState* attack : attacks_) {
		attack->DrawImGui();
	}
	// 仕様：爆撃の半径 ≥ ステージ半径（避けられない）
	if (omniBomb_->GetParams().radius < stageRadius_) {
		ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "全方位爆撃の半径がステージ半径(%.1f)未満です", stageRadius_);
	}
	// 仕様：扇の半径 ＜ ステージ半径（離れれば避けられる）
	if (sweep_->GetParams().radius >= stageRadius_) {
		ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "薙ぎ払いの半径がステージ半径(%.1f)以上です", stageRadius_);
	}

	ImGui::End();
#endif
}
