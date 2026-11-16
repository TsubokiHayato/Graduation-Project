#pragma once

// =========================================
// シールド設定
// =========================================
struct ShieldData {
    float shield;          // 現在シールド量
    float maxshield;       // 最大シールド量
    float rechargetime;    // リチャージ開始までの時間
    float rechargerate;    // 1秒あたりの回復量
};

// =========================================
// 装備データ
// =========================================
struct EquipmentData {
    ShieldData shield;  // シールドの設定
};