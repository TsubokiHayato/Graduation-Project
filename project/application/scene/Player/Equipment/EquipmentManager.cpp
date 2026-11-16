#include "EquipmentManager.h"

void EquipmentManager::Initialize(const EquipmentData& data) {
    // シールド初期化
    shield_.Initialize(data.shield);
}

// 更新
void EquipmentManager::Update() {
    shield_.Update();
}

// ダメージ処理
void EquipmentManager::TakeDamage(float damage, PlayerHP& hp, PlayerState& state) {
    // シールドでダメージを吸収
    float remainingDamage = shield_.TakeDamage(damage);
    // シールドで吸収しきれなかった場合、HPへダメージを渡す
    if (remainingDamage > 0.0f) {
        hp.TakeDamage(remainingDamage, state);
    }
}

// ImGui
void EquipmentManager::DrawImGui() {
    shield_.DrawImGui();
}