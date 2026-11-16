#include "EquipmentManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// =========================================
// Initialize
// =========================================
void EquipmentManager::Initialize(EquipmentData& data, EquipmentDataJson* dataJson) {
    // 装備データとJSON読み込みを保持
    data_ = &data;
    dataJson_ = dataJson;

    // シールド初期化
    shield_.Initialize(data.shield);
}

// =========================================
// Update
// =========================================
void EquipmentManager::Update(float shieldOutputRate) {
    // シールド出力率を反映
    shield_.SetOutputRate(shieldOutputRate);

    // シールド更新
    shield_.Update();
}

// =========================================
// ダメージ処理
// =========================================
void EquipmentManager::TakeDamage(float damage, PlayerHP& hp, PlayerState& state) {
    // シールドでダメージを吸収
    float remainingDamage = shield_.TakeDamage(damage);
    // シールドで吸収しきれなかった場合、HPへダメージを渡す
    if (remainingDamage > 0.0f) {
        hp.TakeDamage(remainingDamage, state);
    }
}

// =========================================
// ImGui
// =========================================
void EquipmentManager::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Begin("Equipment");
    // シールド設定
    shield_.DrawImGui();

    ImGui::Separator();
    // シールド動作中はSave・Resetを禁止
    ImGui::BeginDisabled(shield_.IsBusy() || data_ == nullptr || dataJson_ == nullptr);

    // Save
    if (ImGui::Button("Save")) {
        // 現在のシールドデータを取得
        data_->shield = shield_.GetShieldData();
        // 基準最大シールド量を保存
        data_->shield.maxshield = shield_.GetMaxShield();
        // 保存時はシールドを満タン状態として保存
        data_->shield.shield = data_->shield.maxshield;
        dataJson_->Save(*data_);
    }

    ImGui::SameLine();
    // Reset
    if (ImGui::Button("Reset")) {
        if (dataJson_->Reset(*data_)) {
            // defaultの設定でシールドを再初期化
            shield_.Initialize(data_->shield);
        }
    }
    ImGui::EndDisabled();
    ImGui::End();
#endif
}