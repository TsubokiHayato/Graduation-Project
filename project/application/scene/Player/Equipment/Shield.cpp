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
    baseMaxShield_ = data.maxshield;

    rechargeTimer_ = 0.0f;
    isRecharging_ = false;
    // 現在値を最大値以内に収める
    data_.shield = std::clamp(data_.shield, 0.0f, data_.maxshield);
}

// =========================================
// Update
// =========================================
void Shield::Update() {
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
    rechargeTimer_ = 0.0f;
    isRecharging_ = false;
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
        return;
    }
    // ダメージを受けてからの経過時間
    rechargeTimer_ += deltaTime;
    // リチャージ開始まで待機
    if (rechargeTimer_ < data_.rechargetime) {
        return;
    }

    isRecharging_ = true;
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
    data_.maxshield = baseMaxShield_ * outputRate;
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

    ImGui::Text("Recharge Timer : %.2f", rechargeTimer_);
    ImGui::Text("Recharging : %s", isRecharging_ ? "true" : "false");

    ImGui::Separator();

    ImGui::DragFloat("Max Shield", &data_.maxshield, 1.0f);
    ImGui::DragFloat("Recharge Delay", &data_.rechargetime, 0.1f);
    ImGui::DragFloat("Recharge Rate", &data_.rechargerate, 1.0f);
#endif
}