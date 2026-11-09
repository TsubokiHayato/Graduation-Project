#include "PlayerMovement.h"
#include "Input.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =============================================================================
//  Initialize
// =============================================================================
void PlayerMovement::Initialize(TuboEngine::Object3d* object, const MovementData& movedata) {
    object_ = object;
    movedata_ = movedata;
    // 変数の初期化
    isDashing_ = false;            // ダッシュ中かどうか
    dashTimer_ = 0.0f;             // ダッシュ経過時間
    dashCooldownTimer_ = 0.0f;     // クールタイム経過時間
    currentDashDistance_ = 0.0f;   // ダッシュ距離
    dashSpeed_ = 0.0f;             // ダッシュ速度
    isSpacePressed_ = false;       // 入力管理(SPACEで実行される)

}

// =============================================================================
//  Update
// =============================================================================
void PlayerMovement::Update(float booster) {
    constexpr float kDeltaTime = 1.0f / 60.0f;
    Input* input = Input::GetInstance();

    // クールタイム更新
    if (dashCooldownTimer_ > 0.0f) {
        dashCooldownTimer_ -= kDeltaTime;
        if (dashCooldownTimer_ < 0.0f) {
            dashCooldownTimer_ = 0.0f;
        }
    }

    // ダッシュ中
    if (isDashing_) {
        Dash(kDeltaTime);
        return;
    }

    // 移動入力確認
    bool isMoving = input->PushKey(DIK_W) || input->PushKey(DIK_A) || input->PushKey(DIK_S) || input->PushKey(DIK_D);
    // SPACEを押した瞬間にダッシュ
    if (isMoving && input->PushKey(DIK_SPACE) && dashCooldownTimer_ <= 0.0f) {
        StartDash(booster);
        return;
    }

    // 移動処理
    Move(booster);
}

// =============================================================================
// 移動処理
// =============================================================================
void PlayerMovement::Move(float booster) {
    Input* input = Input::GetInstance();
    // ブースター出力による移動速度の計算
    float movespeed = CalculateMoveSpeed(booster);

    Math::Vector3 pos = object_->GetPosition();
    if (input->PushKey(DIK_D)) pos.x += movespeed;
    if (input->PushKey(DIK_A)) pos.x -= movespeed;
    if (input->PushKey(DIK_S)) pos.z -= movespeed;
    if (input->PushKey(DIK_W)) pos.z += movespeed;
    object_->SetPosition(pos);
}

// =============================================================================
// ダッシュ開始
// =============================================================================
void PlayerMovement::StartDash(float booster) {
    if (movedata_.dashduration <= 0.0f) {
        return;
    }

    // ブースター出力によるダッシュ距離
    currentDashDistance_ = movedata_.maxdashdistance * booster;
    // ダッシュ距離が0なら開始しない
    if (currentDashDistance_ <= 0.0f) {
        return;
    }

    Input* input = Input::GetInstance();
    Math::Vector3 direction{};
    // WASDから方向を取得
    if (input->PushKey(DIK_D)) direction.x += 1.0f;
    if (input->PushKey(DIK_A)) direction.x -= 1.0f;
    if (input->PushKey(DIK_W)) direction.z += 1.0f;
    if (input->PushKey(DIK_S)) direction.z -= 1.0f;

    // 方向ベクトルの長さ
    float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);

    // 移動入力がない場合はダッシュしない
    if (length <= 0.0001f) {
        return;
    }

    // 斜め方向の正規化
    direction.x /= length;
    direction.z /= length;
 
    dashDirection_ = direction;
    // ダッシュ速度を計算
    dashSpeed_ = currentDashDistance_ / movedata_.dashduration;
    dashTimer_ = 0.0f;
    isDashing_ = true;
}

// =============================================================================
// ダッシュ処理
// =============================================================================
void PlayerMovement::Dash(float deltaTime) {
    if (!isDashing_) {
        return;
    }
    // ダッシュ時間を進める
    dashTimer_ += deltaTime;
    // ダッシュ終了判定
    if (dashTimer_ >= movedata_.dashduration) {
        isDashing_ = false;
        // ダッシュ終了時にクールタイム開始
        dashCooldownTimer_ = movedata_.dashcooldown;
        return;
    }

    // ダッシュ中の移動
    Math::Vector3 position = object_->GetPosition();
    position.x += dashDirection_.x * dashSpeed_ * deltaTime;
    position.z += dashDirection_.z * dashSpeed_ * deltaTime;
    object_->SetPosition(position);
}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void PlayerMovement::DrawImGui(float booster) {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {

        // 現在のブースター出力率
        float boosterPercent = booster * 100.0f;

        ImGui::Text("Booster Output: %.1f%%", boosterPercent);
        // ゲージの色を変更
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(80.0f / 255.0f, 210.0f / 255.0f, 120.0f / 255.0f, 1.0f));
        ImGui::ProgressBar(booster, ImVec2(-1.0f, 0.0f));

        ImGui::PopStyleColor();

        // 最大移動速度（編集可能）
        ImGui::InputFloat("Max Speed", &movedata_.maxspeed, 0.01f, 0.1f, "%.3f");
        // マイナス値を許可しない
        if (movedata_.maxspeed < 0.0f) {
            movedata_.maxspeed = 0.0f;
        }
        // 現在の移動速度（表示のみ）
        float currentSpeed = CalculateMoveSpeed(booster);
        ImGui::Text("Current Speed: %.3f", currentSpeed);

        // =========================================
        // ダッシュ状態表示
        // =========================================
        ImGui::Separator();
        ImGui::Text("Dash Status");

        // 現在のダッシュ距離
        ImGui::Text("Current Dash Distance: %.2f", movedata_.maxdashdistance * booster);

        // ダッシュ中のゲージ
        if (isDashing_) {
            float dashProgress = dashTimer_ / movedata_.dashduration;
            dashProgress = std::clamp(dashProgress, 0.0f, 1.0f);

            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "DASHING!");
            ImGui::Text("Dash Time: %.2f / %.2f sec", dashTimer_, movedata_.dashduration);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
            ImGui::ProgressBar(dashProgress, ImVec2(-1.0f, 0.0f), "Dash");
            ImGui::PopStyleColor();
        }

        // クールタイムゲージ
        if (movedata_.dashcooldown > 0.0f) {
            float cooldownProgress = 1.0f - (dashCooldownTimer_ / movedata_.dashcooldown);
            cooldownProgress = std::clamp(cooldownProgress, 0.0f, 1.0f);

            ImGui::Text("Dash Cooldown");
            ImGui::Text("Remaining: %.2f sec", dashCooldownTimer_);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
            ImGui::ProgressBar(cooldownProgress, ImVec2(-1.0f, 0.0f), "Cooldown");
            ImGui::PopStyleColor();
        }

        // ダッシュ可能状態
        if (!isDashing_ && dashCooldownTimer_ <= 0.0f) {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "DASH READY");
        }
    }

#endif
}

// =============================================================================
// 移動速度の計算
// =============================================================================
float PlayerMovement::CalculateMoveSpeed(float booster) const { 
	// ブースター出力率に応じて移動速度を計算
    return movedata_.maxspeed * booster;
}