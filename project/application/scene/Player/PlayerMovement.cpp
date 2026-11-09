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
}

// =============================================================================
//  Update
// =============================================================================
void PlayerMovement::Update(float booster) {
    // 移動処理
    Move(booster);
}

// =============================================================================
// 通常移動
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
    }
#endif
}

float PlayerMovement::CalculateMoveSpeed(float booster) const {
    return movedata_.maxspeed * booster;
}