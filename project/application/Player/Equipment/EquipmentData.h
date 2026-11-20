#pragma once
#include "Transform.h"

// =========================================
// シールド設定
// =========================================
struct ShieldData {
    float shield;                // 現在シールド量
    float maxshield;             // 最大シールド量
    float rechargetime;          // リチャージ開始までの時間
    float rechargerate;          // 1秒あたりの回復量
    // オブジェクト設定
    bool isvisible;                   // シールドオブジェクトを表示するか
    TuboEngine::Math::Vector3 offset; // プレイヤー座標からの表示位置
};

// =========================================
// 装備データ
// =========================================
struct EquipmentData {
    ShieldData shield;  // シールドの設定
};