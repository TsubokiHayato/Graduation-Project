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
    Input* input = Input::GetInstance();
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
// ImGui
// =========================================
void Player::DrawImGui() {
#ifdef USE_IMGUI

    object_->DrawImGui("Player");


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