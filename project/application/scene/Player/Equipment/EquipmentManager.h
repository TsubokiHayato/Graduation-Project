#pragma once
#include "EquipmentData.h"
#include "EquipmentDataJson.h"
#include "Shield.h"
#include "../PlayerHP.h"

// =========================================
// プレイヤ―の装備管理クラス
// =========================================
class EquipmentManager {
public:
    // 初期化
    void Initialize(EquipmentData& data, EquipmentDataJson* dataJson);
    // 更新
    void Update(float shieldOutputRate);
    // ダメージ処理
    void TakeDamage(float damage, PlayerHP& hp, PlayerState& state);
    // ImGui
    void DrawImGui();

private:
    // 装備データ
    EquipmentData* data_ = nullptr;
    // JSON読み込みクラス
    EquipmentDataJson* dataJson_ = nullptr;
    // シールド
    Shield shield_;
public:
    // Getter
    Shield* GetShield() { return &shield_; }
    const Shield* GetShield() const { return &shield_; }
};