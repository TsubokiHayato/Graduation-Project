#include "Player.h"
#include <cmath>
#include "JsonManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =========================================
// Initialize
// =========================================
void Player::Initialize(TuboEngine::Camera* camera) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize("block/block.obj");
    object_->SetCamera(camera);
  
    dataJson_ = std::make_unique<PlayerDataJson>();

    // JSON読み込み
    if (dataJson_->Load(data_)) {
        // Transform反映
        object_->SetPosition(data_.transform.translate);
        object_->SetRotation(data_.transform.rotate);
        object_->SetScale(data_.transform.scale);
    }
}

// =========================================
// Update
// =========================================
void Player::Update() {
    // 死亡中なら更新しない
    if (state_ == PlayerState::Dead) {
        return;
    }

    Input* input = Input::GetInstance();
    // 無敵時間更新
    UpdateInvincible();
    // =========================================
    // Qキーでダメージを受ける（確認用）
    // =========================================
    bool qPressed = input->PushKey(DIK_Q);
    // 押した瞬間だけ判定
    bool qTrigger = qPressed && !qWasPressed_;
    qWasPressed_ = qPressed;
    if (qTrigger) {
        TakeDamage();
    }

    // =========================================
    // ダッシュ処理
    // =========================================
    // クールタイム管理
    if (dashCooldownTimer_ > 0) {
        dashCooldownTimer_--;
    }
    // Spaceキーの入力判定
    bool spacePressed = input->PushKey(DIK_SPACE);
    // 押した瞬間だけ判定
    bool spaceTrigger = spacePressed && !spaceWasPressed_;
    spaceWasPressed_ = spacePressed;
    // ダッシュ開始
    if (spaceTrigger && !isDashing_ && dashCooldownTimer_ <= 0) {
        StartDash();
    }
    // ダッシュ中
    if (isDashing_) {
        UpdateDash();
    } else {
        // 通常移動
        Move();
    }
    // モデル更新
    object_->Update();
}

// =========================================
// 通常移動
// =========================================
void Player::Move() {
    Input* input = Input::GetInstance();
    Math::Vector3 pos = object_->GetPosition();
    if (input->PushKey(DIK_D)) pos.x += data_.speed;
    if (input->PushKey(DIK_A)) pos.x -= data_.speed;
    if (input->PushKey(DIK_S)) pos.z -= data_.speed;
    if (input->PushKey(DIK_W)) pos.z += data_.speed;
    object_->SetPosition(pos);
}

// =========================================
// ダッシュ開始
// =========================================
void Player::StartDash() {
    Input* input = Input::GetInstance();
    Math::Vector3 direction = {};
    // 入力方向を取得
    if (input->PushKey(DIK_D)) direction.x += 1.0f;
    if (input->PushKey(DIK_A)) direction.x -= 1.0f;
    if (input->PushKey(DIK_W)) direction.z += 1.0f;
    if (input->PushKey(DIK_S)) direction.z -= 1.0f;
    // 入力がない場合は前方
    if (direction.x == 0.0f && direction.z == 0.0f) {
        direction.z = 1.0f;
    }
    // 方向を正規化
    float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    direction.x /= length;
    direction.z /= length;
    // ダッシュ開始位置を保存
    dashStartPos_ = object_->GetPosition();
    // ダッシュ方向を保存
    dashDirection_ = direction;
    // ダッシュ開始
    isDashing_ = true;
    dashTimer_ = 0;
    // クールタイム開始
    dashCooldownTimer_ = dashCooldown_;
}

// =========================================
// ダッシュ更新
// =========================================
void Player::UpdateDash() {
    // ダッシュ時間が0以下にならないようにする
    if (dashDuration_ <= 0) {
        isDashing_ = false;
        return;
    }

    dashTimer_++;
    // 進行度
    float t = static_cast<float>(dashTimer_) / static_cast<float>(dashDuration_);

    if (t > 1.0f) {
        t = 1.0f;
    }

    // ダッシュ距離
    float dashDistance = maxDashDistance_;

    // 左右方向の場合は距離を増やす
    if (dashDirection_.x != 0.0f && dashDirection_.z == 0.0f) {
        dashDistance *= dashSideMultiplier_;
    }

    // 加速・減速の補間
    float ease = t * t * (3.0f - 2.0f * t);

    // 移動位置を計算
    Math::Vector3 pos;

    pos.x = dashStartPos_.x + dashDirection_.x * dashDistance * ease;
    pos.y = dashStartPos_.y;
    pos.z = dashStartPos_.z + dashDirection_.z * dashDistance * ease;

    object_->SetPosition(pos);

    // ダッシュ終了
    if (dashTimer_ >= dashDuration_) {
        isDashing_ = false;
        dashTimer_ = 0;
    }
}

// =========================================
// Draw
// =========================================
void Player::Draw() {
    object_->Draw();
}

// =========================================
// ダメージ処理
// =========================================
void Player::TakeDamage() {
    // 無敵時間中ならダメージを受けない
    if (IsInvincible()) {
        return;
    }
    // HPを減らす
    currentHP_ -= damage_;
    // HPが0以下にならないようにする
    if (currentHP_ < 0) {
        currentHP_ = 0;
    }
    // HPが0になったら死亡
    if (currentHP_ <= 0) {
        Die();
        return;
    }
    // 無敵時間を開始
    invincibleTimer_ = invincibleDuration_;
}

// =========================================
// 無敵時間更新
// =========================================
void Player::UpdateInvincible() {

    if (invincibleTimer_ > 0) {
        invincibleTimer_--;
    }
}

// =========================================
// HPリセット
// =========================================
void Player::ResetHP() {
    // HPを最大値に戻す
    currentHP_ = maxHP_;
}

// =========================================
// 生存状態にする
// =========================================
void Player::SetAlive() {
    // HPを最大値に戻す
    currentHP_ = maxHP_;
    // 生存状態に変更
    state_ = PlayerState::Alive;
}

// =========================================
// 死亡状態にする
// =========================================
void Player::SetDead() {
    // HPを0にする
    currentHP_ = 0;
    // 死亡処理
    Die();
}

// =========================================
// ImGui
// =========================================
void Player::DrawImGui() {
#ifdef USE_IMGUI

    object_->DrawImGui("Player");

    // プレイヤー状態
    ImGui::Text("Player State:");

    if (state_ == PlayerState::Alive) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Alive");
    } else {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Dead");
    }

    ImGui::Separator();
	// 移動速度の調整
    if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Speed変更
        ImGui::InputFloat(
            "Speed",
            &data_.speed,
            0.01f,  // 上下ボタンの変化量
            0.1f,   // 上下ボタンの大きな変化量
            "%.3f"  // 小数点以下3桁表示
        );
    }

    ImGui::Separator();

    // =========================================
    // Dash 状態表示
    // =========================================
    if (ImGui::CollapsingHeader("Dash", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ダッシュ状態
        ImGui::Text("State:");

        if (isDashing_) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Dashing");
        } else if (dashCooldownTimer_ > 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Cooldown");
        } else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Ready");
        }

        ImGui::Spacing();

        // クールタイムゲージ
        ImGui::Text("Cooldown");
        float cooldownRate = 0.0f;
        if (dashCooldown_ > 0) {
            cooldownRate = static_cast<float>(dashCooldownTimer_) / static_cast<float>(dashCooldown_);
        }

        char cooldownText[64];
        std::snprintf(cooldownText, sizeof(cooldownText), "%d / %d Frames", dashCooldownTimer_, dashCooldown_);
        ImGui::ProgressBar(cooldownRate, ImVec2(-1.0f, 0.0f), cooldownText);
        ImGui::Spacing();

        // ダッシュ中の進行度
        ImGui::Text("Dash Progress");
        float dashRate = 0.0f;
        if (isDashing_ && dashDuration_ > 0) {
            dashRate = static_cast<float>(dashTimer_) / static_cast<float>(dashDuration_);
        }
        char dashText[64];
        std::snprintf(dashText, sizeof(dashText), "%d / %d Frames", dashTimer_, dashDuration_);
        ImGui::ProgressBar(dashRate, ImVec2(-1.0f, 0.0f), dashText);
    }

    ImGui::Separator();

    // =========================================
    // HP設定・表示
    // =========================================
    if (ImGui::CollapsingHeader("HP", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 最大HP
        ImGui::InputInt("Max HP", &maxHP_);
        // 最大HPは1以上
        if (maxHP_ < 1) {
            maxHP_ = 1;
        }

        // 現在HPは最大HP以下にする
        if (currentHP_ > maxHP_) {
            currentHP_ = maxHP_;
        }

        // 現在HP
        ImGui::InputInt("Current HP", &currentHP_);

        // 現在HPの範囲を制限
        if (currentHP_ < 0) {
            currentHP_ = 0;
        }

        if (currentHP_ > maxHP_) {
            currentHP_ = maxHP_;
        }

        // ダメージ量
        ImGui::InputInt("Damage", &damage_);

        if (damage_ < 0) {
            damage_ = 0;
        }

        ImGui::Spacing();

        // HPゲージ
        ImGui::Text("HP");

        float hpRate = static_cast<float>(currentHP_) / static_cast<float>(maxHP_);

        char hpText[64];

        std::snprintf(hpText, sizeof(hpText), "%d / %d", currentHP_, maxHP_);
        ImGui::ProgressBar(hpRate, ImVec2(-1.0f, 0.0f), hpText);

        ImGui::Spacing();
        ImGui::Separator();

        // 無敵時間の設定
        ImGui::Text("Invincible Settings");

        ImGui::InputInt("Invincible Duration", &invincibleDuration_);

        if (invincibleDuration_ < 0) {
            invincibleDuration_ = 0;
        }

        // 無敵時間ゲージ
        ImGui::Text("Invincible Time");

        float invincibleRate = 0.0f;

        if (invincibleDuration_ > 0) {
            invincibleRate = static_cast<float>(invincibleTimer_) / static_cast<float>(invincibleDuration_);
        }
        char invincibleText[64];
        std::snprintf(invincibleText, sizeof(invincibleText), "%d / %d Frames", invincibleTimer_, invincibleDuration_);
        ImGui::ProgressBar(invincibleRate, ImVec2(-1.0f, 0.0f), invincibleText);

        // 無敵状態
        if (IsInvincible()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "INVINCIBLE");
        } else {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Normal");
        }
    }

    ImGui::Separator();

    ImGui::Text("Player Control");

    // HPリセット
    if (ImGui::Button("Reset HP", ImVec2(120.0f, 30.0f))) {
        ResetHP();
    }

    ImGui::SameLine();

    // 強制的に生存
    if (ImGui::Button("Alive", ImVec2(120.0f, 30.0f))) {
        SetAlive();
    }

    ImGui::SameLine();

    // 強制的に死亡
    if (ImGui::Button("Dead", ImVec2(120.0f, 30.0f))) {
        SetDead();
    }

    ImGui::Separator();

    // 保存ボタン
    if (ImGui::Button("Save")) {
        if (dataJson_->Save(data_)) {
            ImGui::OpenPopup("Save Success");
        } else {
            ImGui::OpenPopup("Save Error");
        }
    }

    ImGui::Separator();

    // 初期値に戻すボタン
    ImGui::BeginDisabled(!dataJson_->HasCurrent());

    if (ImGui::Button("Reset to Default")) {
        if (dataJson_->Reset(data_)) {
            // Object3dにも反映する
            object_->SetScale(data_.transform.scale);
            object_->SetRotation(data_.transform.rotate);
            object_->SetPosition(data_.transform.translate);
        }
    }

    ImGui::EndDisabled();
#endif
}

// =========================================
// 死亡処理
// =========================================
void Player::Die() {
    // すでに死亡している場合は何もしない
    if (state_ == PlayerState::Dead) {
        return;
    }
    // 死亡状態に変更
    state_ = PlayerState::Dead;
    // ダッシュを停止
    isDashing_ = false;
    // 無敵時間を停止
    invincibleTimer_ = 0;
    // ダッシュ関連のタイマーを停止
    dashTimer_ = 0;
    dashCooldownTimer_ = 0;
}