#pragma once
#include "Transform.h"

// =========================================
// プレイヤ―データ
// =========================================
struct PlayerData {
    // Transform
    TuboEngine::Transform transform;
    // 移動速度
    float speed;
};
//
//enum class PlayerState {
//    Alive,  // 生存中
//    Dead    // 死亡中
//};


// =========================================
// 移動設定
// =========================================
struct PlayerMovementData {
    // 通常移動速度
    float speed = 0.1f;

    // ダッシュ
    float maxdashdistance = 5.0f;
    float dashSidemultiplier = 1.5f;
    int dashduration = 12;
    int dashcooldown = 60;

    // ブースター
    float maxBooster = 100.0f;
    float boosterconsumerate = 30.0f;
    float boosterrecoverrate = 20.0f;
};

// =========================================
// HP設定
// =========================================
struct PlayerHPData {
    // 最大HP
    int maxhp = 100;
    // 現在HP
    int currenthp = 100;
    // 無敵時間（フレーム）
    int invincibleDuration = 60;
};