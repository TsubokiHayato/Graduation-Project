#pragma once
#include "PlayerHP.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =============================================================================
//  Initialize
// ============================================================================= 
void PlayerHP::Initialize(const PlayerHPData& hpdata) {
    // HPデータを受け取る
    hpdata_ = hpdata;

    // HPの範囲を調整
    hpdata_.maxhp = std::max(0.0f, hpdata_.maxhp);
    hpdata_.currenthp = std::clamp(hpdata_.currenthp, 0.0f, hpdata_.maxhp);

    // 無敵状態の初期化
    isInvincible_ = false;
    invincibleTimer_ = 0.0f;
}

// =============================================================================
// Update
// =============================================================================
void PlayerHP::Update() {
    // deltaTime（60FPS固定）
    constexpr float kDeltaTime = 1.0f / 60.0f;

    // 無敵時間の更新
    UpdateInvincible(kDeltaTime);
}

// =============================================================================
// ダメージ処理
// =============================================================================
void PlayerHP::TakeDamage(float damage) {
    //// HPが0、または無敵状態ならダメージを受けない
    //if (IsDead() || isInvincible_) {
    //    return;
    //}

    // 余計なダメージを防止
    if (damage <= 0.0f) {
        return;
    }
    // HPを減少
    hpdata_.currenthp -= damage;
    // HPが0以下にならないようにする
    hpdata_.currenthp = std::max(0.0f, hpdata_.currenthp);

    // 無敵時間を開始
    if (hpdata_.invincibleduration > 0.0f) {
        isInvincible_ = true;
        invincibleTimer_ = 0.0f;
    }
}

// =============================================================================
// 無敵時間更新
// =============================================================================
void PlayerHP::UpdateInvincible(float deltaTime) {
    if (!isInvincible_) {
        return;
    }
    // 無敵時間を進める
    invincibleTimer_ += deltaTime;
    // 無敵時間終了判定
    if (invincibleTimer_ >= hpdata_.invincibleduration) {
        isInvincible_ = false;
        invincibleTimer_ = 0.0f;
    }
}

// =============================================================================
// HPリセット
// =============================================================================
void PlayerHP::ResetHP() {
    // HPを最大値に戻す
    hpdata_.currenthp = hpdata_.maxhp;
    // 無敵状態を解除
    isInvincible_ = false;
    invincibleTimer_ = 0.0f;
}

// =============================================================================
// ImGui
// =============================================================================
void PlayerHP::DrawImGui() {
#ifdef USE_IMGUI

    if (ImGui::CollapsingHeader("HP", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Separator();
        // HP設定
        ImGui::InputFloat("Max HP", &hpdata_.maxhp, 1.0f, 10.0f, "%.1f");
        // 最大HPの補正
        hpdata_.maxhp = std::max(0.0f, hpdata_.maxhp);
        // 現在HP
        ImGui::InputFloat("Current HP", &hpdata_.currenthp, 1.0f, 10.0f, "%.1f");
        // 現在HPを最大HPの範囲内にする
        hpdata_.currenthp = std::clamp(hpdata_.currenthp, 0.0f, hpdata_.maxhp);

        // 無敵時間設定
        ImGui::InputFloat("Invincible Duration", &hpdata_.invincibleduration, 0.1f, 1.0f, "%.2f");
        hpdata_.invincibleduration = std::max(0.0f, hpdata_.invincibleduration);
        ImGui::Separator();

        // 無敵状態
        ImGui::Text("Invincible: %s", isInvincible_ ? "YES" : "NO");
        // 無敵時間経過
        ImGui::Text("Invincible Timer: %.2f sec", invincibleTimer_);
        //// HP状態
        //if (IsDead()) {
        //    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "DEAD");
        //} else {
        //    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "ALIVE");
        //}

        // HPリセット
        if (ImGui::Button("Reset HP")) {
            ResetHP();
        }
    }

#endif
}