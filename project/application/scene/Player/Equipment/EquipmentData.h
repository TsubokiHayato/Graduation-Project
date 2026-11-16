#pragma once

// =========================================
// シールド設定
// =========================================
struct ShieldData {
    float shield = 1000.0f;          // 現在シールド量
    float maxshield = 1000.0f;       // 最大シールド量
    float rechargetime = 3.0f;       // リチャージ開始までの時間
    float rechargerate = 100.0f;     // 1秒あたりの回復量
};

// =========================================
// 装備データ
// =========================================
struct EquipmentData {
    ShieldData shield;  // シールドの設定
};