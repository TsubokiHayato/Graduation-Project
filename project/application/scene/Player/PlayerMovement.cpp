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
// ボスを中心に、左右で周りを回り込み、前後で近づく・離れる
// =============================================================================
void PlayerMovement::Move(float booster) {
    Input* input = Input::GetInstance();
    // ブースター出力による移動速度の計算
    float movespeed = CalculateMoveSpeed(booster);

    // 入力方向（x = 左右、z = 前後。前 = ボスへ近づく）
    float side = 0.0f;
    float forward = 0.0f;
    if (input->PushKey(DIK_D)) side += 1.0f;
    if (input->PushKey(DIK_A)) side -= 1.0f;
    if (input->PushKey(DIK_W)) forward += 1.0f;
    if (input->PushKey(DIK_S)) forward -= 1.0f;

    // 斜めは各成分を 1/√2 にする
    if (side != 0.0f && forward != 0.0f) {
        constexpr float kInvSqrt2 = 0.70710678f;
        side *= kInvSqrt2;
        forward *= kInvSqrt2;
    }

    // 前入力は距離を縮める
    MoveOnOrbit(side * movespeed, -forward * movespeed);
}

// =============================================================================
// 旋回の中心の設定
// =============================================================================
void PlayerMovement::SetOrbitCenter(const TuboEngine::Math::Vector3& center) {
    orbitCenter_ = center;
    // 初期位置（JSON）の高さに関係なく、すぐにボスと同じ地面に乗せる
    Math::Vector3 pos = object_->GetPosition();
    pos.y = GetGroundedY();
    object_->SetPosition(pos);
}

// =============================================================================
// 地面に乗る高さ
// =============================================================================
float PlayerMovement::GetGroundedY() const {
    // モデル（block.obj）は高さ 2（-1〜1）なので、半分の高さ × スケール だけ持ち上げると底が地面に付く
    // ボスも同じ block.obj を「中心の高さ + 1」に置いて地面に乗せている
    constexpr float kModelHalfHeight = 1.0f;
    return orbitCenter_.y + kModelHalfHeight * object_->GetScale().y;
}

// =============================================================================
// 旋回移動の反映
// =============================================================================
void PlayerMovement::MoveOnOrbit(float tangential, float radial) {
    Math::Vector3 pos = object_->GetPosition();

    // 中心から見た現在の距離と角度
    const float dx = pos.x - orbitCenter_.x;
    const float dz = pos.z - orbitCenter_.z;
    float distance = std::sqrt(dx * dx + dz * dz);
    float angle = std::atan2(dz, dx);

    // 中心に重なっているときは角度が決まらないので最小距離へ押し出す
    if (distance < 0.0001f) {
        distance = minDistance_;
        angle = -1.5707963f;
    }

    // 周方向の移動量を角度に変換する（近いほど角度は速く変わる）
    // 自機からボスを見て右に動くのは角度が増える向き
    angle += tangential / distance;

    // 距離を変えて、最小距離〜ステージ半径に収める
    distance = std::clamp(distance + radial, minDistance_, (std::max)(minDistance_, maxDistance_));

    pos.x = orbitCenter_.x + std::cos(angle) * distance;
    pos.z = orbitCenter_.z + std::sin(angle) * distance;
    pos.y = GetGroundedY(); // ボスと同じ地面に乗せる
    object_->SetPosition(pos);

    // 常にボスの方を向く（ボスと同じく yaw = π/2 - 向きの角度。向きの角度 = angle + π）
    constexpr float kHalfPi = 1.5707963f;
    object_->SetRotation({ 0.0f, -kHalfPi - angle, 0.0f });
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
    // WASDから方向を取得（x = 周方向、z = 距離方向。前 = ボスへ近づく）
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

    // ダッシュ中の移動（左右は周方向、前後は距離方向）
    const float step = dashSpeed_ * deltaTime;
    MoveOnOrbit(dashDirection_.x * step, -dashDirection_.z * step);
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