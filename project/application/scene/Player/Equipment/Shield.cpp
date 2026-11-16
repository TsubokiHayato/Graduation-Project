#include "Shield.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include <algorithm>

using namespace TuboEngine;

// =========================================
// Initialize
// =========================================
void Shield::Initialize(const ShieldData& data, TuboEngine::Camera* camera) {
    data_ = data;
    // 初期状態では基準最大値をそのまま使用
    effectivemaxshield_ = data.maxshield;
    rechargetimer_ = 0.0f;
    isrecharging_ = false;
    isediting_ = false;
    // 現在値を最大値以内に収める
    data_.shield = std::clamp(data_.shield, 0.0f, effectivemaxshield_);

    // シールドオブジェクトの生成、初期化
    object_ = std::make_unique<Object3d>();
    object_->Initialize("block/block.obj");
    object_->SetCamera(camera);
}

// =========================================
// Update
// =========================================
void Shield::Update() {
    // ImGuiで設定編集中なら処理を停止
    if (isediting_) {
        return;
    }
    constexpr float kDeltaTime = 1.0f / 60.0f;
    UpdateRecharge(kDeltaTime);


    object_->Update();
}

// =========================================
// Draw
// =========================================
void Shield::Draw() {
    object_->Draw();
}

// =========================================
// ダメージ処理
// =========================================
float Shield::TakeDamage(float damage) {
    if (damage <= 0.0f) {
        return 0.0f;
    }

    // シールドがない場合は全ダメージを返す
    if (data_.shield <= 0.0f) {
        return damage;
    }

    // リチャージを停止
    rechargetimer_ = 0.0f;
    isrecharging_ = false;
    // シールドで吸収できるダメージ
    float absorbedDamage = std::min(data_.shield, damage);
    data_.shield -= absorbedDamage;
    // 吸収しきれなかったダメージを返す
    return damage - absorbedDamage;
}

// =========================================
// リチャージ処理
// =========================================
void Shield::UpdateRecharge(float deltaTime) {
    // ブースター適用後の最大値に到達しているか判定
    if (data_.shield >= effectivemaxshield_) {
        data_.shield = effectivemaxshield_;
        // リチャージ完了
        rechargetimer_ = 0.0f;
        isrecharging_ = false;
        return;
    }

    // ダメージを受けてからの経過時間
    rechargetimer_ += deltaTime;
    // リチャージ開始まで待機
    if (rechargetimer_ < data_.rechargetime) {
        return;
    }

    isrecharging_ = true;
    // シールドを回復
    data_.shield += data_.rechargerate * deltaTime;
    // 最大値を超えないようにする
    data_.shield = std::min(data_.shield, effectivemaxshield_);
}

// =========================================
// シールド出力率設定
// =========================================
void Shield::SetOutputRate(float outputRate) {
    // 出力率を0.0～1.0に制限
    outputRate = std::clamp(outputRate, 0.0f, 1.0f);
    // 基準最大シールド値から実際の最大値を計算
    effectivemaxshield_ = data_.maxshield * outputRate;
    // 現在値が最大値を超えないようにする
    data_.shield = std::min(data_.shield, effectivemaxshield_);
}

// =========================================
// ImGui
// =========================================
void Shield::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::SeparatorText("Shield");
    ImGui::Text("Shield : %.1f / %.1f", data_.shield, effectivemaxshield_);
    float shieldRate = 0.0f;
    if (effectivemaxshield_ > 0.0f) {
        shieldRate = data_.shield / effectivemaxshield_;
    }
    // シールドゲージ（青色）
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(60.0f / 255.0f, 140.0f / 255.0f, 255.0f / 255.0f, 1.0f));
    ImGui::ProgressBar(shieldRate, ImVec2(-1.0f, 0.0f));
    ImGui::PopStyleColor();

    ImGui::Text("Recharge Timer : %.2f", rechargetimer_);
    ImGui::Text("Recharging : %s", isrecharging_ ? "true" : "false");

    ImGui::Separator();

    // 設定変更前の値を保存
    float oldMaxShield = data_.maxshield;
    float oldRechargeTime = data_.rechargetime;
    float oldRechargeRate = data_.rechargerate;

    // 設定項目
    ImGui::DragFloat("Max Shield", &data_.maxshield, 1.0f);
    bool editing = ImGui::IsItemActive();
    ImGui::DragFloat("Recharge Delay", &data_.rechargetime, 0.1f);
    editing |= ImGui::IsItemActive();
    ImGui::DragFloat("Recharge Rate", &data_.rechargerate, 1.0f);
    editing |= ImGui::IsItemActive();
    // 設定値が変更されたか判定
    isediting_ = editing || oldMaxShield != data_.maxshield || oldRechargeTime != data_.rechargetime || oldRechargeRate != data_.rechargerate;
#endif
}