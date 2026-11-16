#include "Shield.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include <algorithm>

// =========================================
// Initialize
// =========================================
void Shield::Initialize(const ShieldData& data) {
    data_ = data;
    // 100%時の基準最大シールド量を保存
    basemaxshield_ = data.maxshield;

    rechargetimer_ = 0.0f;
    isrecharging_ = false;
    // 現在値を最大値以内に収める
    data_.shield = std::clamp(data_.shield, 0.0f, data_.maxshield);
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
    if (data_.shield >= data_.maxshield) {
        data_.shield = data_.maxshield;
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
    data_.shield = std::min(data_.shield, data_.maxshield);
}

// =========================================
// シールド出力率設定
// =========================================
void Shield::SetOutputRate(float outputRate) {
    // 出力率を0.0～1.0に制限
    outputRate = std::clamp(outputRate, 0.0f, 1.0f);
    // 出力率に応じて最大シールド量を変更
    data_.maxshield = basemaxshield_ * outputRate;
    // 現在のシールド量が最大値を超えないようにする
    data_.shield = std::min(data_.shield, data_.maxshield);
}

// =========================================
// ImGui
// =========================================
void Shield::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::SeparatorText("Shield");

    ImGui::Text("Shield : %.1f / %.1f", data_.shield, data_.maxshield);

    float shieldRate = 0.0f;
    if (data_.maxshield > 0.0f) {
        shieldRate = data_.shield / data_.maxshield;
    }


    // シールドゲージ（青色）
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(60.0f / 255.0f, 140.0f / 255.0f, 255.0f / 255.0f, 1.0f));
    ImGui::ProgressBar(shieldRate, ImVec2(-1.0f, 0.0f));
    ImGui::PopStyleColor();

    ImGui::Text("Recharge Timer : %.2f", rechargetimer_);
    ImGui::Text("Recharging : %s", isrecharging_ ? "true" : "false");

    ImGui::Separator();

    // 設定変更前の値を保存
    float oldMaxShield = basemaxshield_;
    float oldRechargeTime = data_.rechargetime;
    float oldRechargeRate = data_.rechargerate;

    // 設定項目
    ImGui::DragFloat("Max Shield", &basemaxshield_, 1.0f);
    bool editing = ImGui::IsItemActive();
    ImGui::DragFloat("Recharge Delay", &data_.rechargetime, 0.1f);
    editing |= ImGui::IsItemActive();
    ImGui::DragFloat("Recharge Rate", &data_.rechargerate, 1.0f);
    editing |= ImGui::IsItemActive();
    // 編集中かどうか
    isediting_ = editing;
    // 設定値が変更されたか判定
    isediting_ = oldMaxShield != basemaxshield_ || oldRechargeTime != data_.rechargetime || oldRechargeRate != data_.rechargerate;
#endif
}