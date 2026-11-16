#pragma once
#include "EquipmentData.h"
#include "Shield.h"
#include "../PlayerHP.h"

class EquipmentManager {
public:
    // 初期化
    void Initialize(const EquipmentData& data);

    // 更新
    void Update(float shieldOutputRate);

    // ダメージ処理
    void TakeDamage(float damage, PlayerHP& hp, PlayerState& state);

    // ImGui
    void DrawImGui();

    // Getter
    Shield* GetShield() { return &shield_; }
    const Shield* GetShield() const { return &shield_; }

private:
    // シールド
    Shield shield_;
};