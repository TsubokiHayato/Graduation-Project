#include "PlayerMovement.h"
#include "Input.h"
#include <algorithm>
#include <cmath>
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

    // ロックオン中：相手を基準に動く（W/S = 近づく/離れる、A/D = 周りを回る）
    if (hasLockOn_) {
        Math::Vector3 in = ReadMoveInput();
        const float length = std::sqrt(in.x * in.x + in.z * in.z);
        if (length > 0.0001f) {
            // 斜め入力で速くならないように正規化
            MoveAroundTarget(in.x / length * movespeed, in.z / length * movespeed);
        }
        return;
    }

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

    // WASDから方向を取得（ロックオン中は「横・前後」の向きとして使う）
    Math::Vector3 direction = ReadMoveInput();

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
    // ロックオン中のダッシュは相手を中心に動く（横ダッシュで回り込み、後ろダッシュで距離を取る）
    isLockOnDash_ = hasLockOn_;
    dashInput_ = direction;
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
    const float step = dashSpeed_ * deltaTime;
    // ロックオン中に始めたダッシュは、相手がいる間は相手を中心に動く
    if (isLockOnDash_ && hasLockOn_) {
        MoveAroundTarget(dashInput_.x * step, dashInput_.z * step);
        return;
    }
    Math::Vector3 position = object_->GetPosition();
    position.x += dashDirection_.x * step;
    position.z += dashDirection_.z * step;
    object_->SetPosition(position);
}

// =============================================================================
// WASD 入力
// =============================================================================
Math::Vector3 PlayerMovement::ReadMoveInput() const {
    Input* input = Input::GetInstance();
    Math::Vector3 in{};
    if (input->PushKey(DIK_D)) in.x += 1.0f;
    if (input->PushKey(DIK_A)) in.x -= 1.0f;
    if (input->PushKey(DIK_W)) in.z += 1.0f;
    if (input->PushKey(DIK_S)) in.z -= 1.0f;
    return in;
}

// =============================================================================
// ロックオン相手を中心にした移動
//  相手から見た「距離」と「角度」で位置を持ち、
//  前後入力で距離、左右入力で角度を変える（まっすぐ横に動くと外へ膨らむため円周に沿わせる）
// =============================================================================
void PlayerMovement::MoveAroundTarget(float strafe, float forward) {
    Math::Vector3 pos = object_->GetPosition();
    float dx = pos.x - lockOnTarget_.x;
    float dz = pos.z - lockOnTarget_.z;
    float distance = std::sqrt(dx * dx + dz * dz);
    if (distance < 0.0001f) {
        // 相手と重なっているときは手前（-Z 側）にいることにする
        dx = 0.0f;
        dz = -1.0f;
        distance = 0.0001f;
    }

    // 相手を中心とした角度（+Z が 0）
    float angle = std::atan2(dx, dz);

    // 前後：W で近づく。最小距離より内側には入らない（もともと内側にいる場合は押し出さない）
    const float minDistance = (std::min)(minLockOnDistance_, distance);
    const float newDistance = (std::max)(distance - forward, minDistance);

    // 左右：円周に沿って回る。相手の方を向いたとき右(D)に動くと角度は減る
    angle -= strafe / (std::max)(newDistance, 0.0001f);

    pos.x = lockOnTarget_.x + std::sin(angle) * newDistance;
    pos.z = lockOnTarget_.z + std::cos(angle) * newDistance;
    object_->SetPosition(pos);
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
        // ダッシュ状態（色分け）
        if (isDashing_) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "DASHING");
        } else if (dashCooldownTimer_ <= 0.0f) {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "DASH READY");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "COOLDOWN");
        }
        // ダッシュ中は編集不可
        if (isDashing_) {
            ImGui::BeginDisabled();
        }
        // ダッシュ距離
        ImGui::InputFloat("Current Dash Distance", &movedata_.maxdashdistance, 0.1f, 1.0f, "%.2f");
        // ダッシュ設定時間
        ImGui::InputFloat("Dash Duration", &movedata_.dashduration, 0.1f, 1.0f, "%.2f");
        // ダッシュ経過時間（表示のみ）
        ImGui::Text("Dash Timer: %.2f sec", dashTimer_);
        // ダッシュクールタイム設定
        ImGui::InputFloat("Dash Cooldown", &movedata_.dashcooldown, 0.1f, 1.0f, "%.2f");

        // ダッシュ中の編集制限解除
        if (isDashing_) {
            ImGui::EndDisabled();
        }

        // マイナス値を許可しない
        movedata_.maxdashdistance = std::max(0.0f, movedata_.maxdashdistance);
        movedata_.dashduration = std::max(0.0f, movedata_.dashduration);
        movedata_.dashcooldown = std::max(0.0f, movedata_.dashcooldown);

        // =========================================
        // クールタイム表示
        // =========================================
        if (movedata_.dashcooldown > 0.0f) {
            float cooldownProgress = 1.0f - (dashCooldownTimer_ / movedata_.dashcooldown);
            cooldownProgress = std::clamp(cooldownProgress, 0.0f, 1.0f);

            ImGui::Text("Cooldown Remaining: %.2f sec", dashCooldownTimer_);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
            ImGui::ProgressBar(cooldownProgress, ImVec2(-1.0f, 0.0f), "Cooldown");

            ImGui::PopStyleColor();
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