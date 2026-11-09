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
  
    // プレイヤ―のJsonデータ読み込みクラスの生成
    dataJson_ = std::make_unique<PlayerDataJson>();

    // JSON読み込み
    if (dataJson_->Load(data_)) {
        // Transform反映
        object_->SetPosition(data_.transform.translate);
        object_->SetRotation(data_.transform.rotate);
        object_->SetScale(data_.transform.scale);
    }
    // エネルギー管理クラスの生成、初期化
    energyoutput_ = std::make_unique<PlayerEnergyOutput>();
    energyoutput_->Initialize(data_.energy);
    // 移動処理クラスの生成、初期化
    movement_ = std::make_unique<PlayerMovement>();
    movement_->Initialize(object_.get(), data_.movement);
}

// =========================================
// Update
// =========================================
void Player::Update() {

    // 移動処理関連の更新処理
    movement_->Update(energyoutput_->GetBoosterOutputRate());

    // モデル更新
    object_->Update();
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

    // エネルギー管理クラスのimgui
    energyoutput_->DrawImGui();
    ImGui::Separator();
    // 移動処理関連のimgui
	movement_->DrawImGui(energyoutput_->GetBoosterOutputRate());

    ImGui::Separator();

    // 保存ボタン
    if (ImGui::Button("Save")) {
        // エネルギーデータをPlayer側に反映
        data_.energy = energyoutput_->GetEnergyData();
        // 移動データをPlayer側に反映
        data_.movement = movement_->GetMovementData();
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
            // Object3dに反映する
            object_->SetScale(data_.transform.scale);
            object_->SetRotation(data_.transform.rotate);
            object_->SetPosition(data_.transform.translate);
        }
        // エネルギー設定を初期化
        energyoutput_->Initialize(data_.energy);
        // 移動設定を初期化
        movement_->Initialize(object_.get(), data_.movement);
    }

    ImGui::EndDisabled();
#endif
}