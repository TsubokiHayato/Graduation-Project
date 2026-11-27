#pragma once
#include "Transform.h"

// =========================================
// エネルギー配分データ
// =========================================
struct PlayerEnergyData {
    float maxenergy;   // 最大エネルギー
    float shield;      // シールド
    float fight;       // 近距離武器
    float beam;        // 遠距離武器
    float booster;     // ブースター
};

// =========================================
// 移動設定
// =========================================
struct MovementData {
    float maxspeed;          // 最大移動速度
    // ダッシュ設定
    float maxdashdistance;   // 最大ダッシュ距離
    float dashduration;      // ダッシュ時間
    float dashcooldown;      // クールタイム
};

// =========================================
// HP設定
// =========================================
struct PlayerHPData {
    float maxhp;                 // 最大HP
    float currenthp;             // 現在HP
    float invincibleduration;    // 無敵時間（秒）
};

// =========================================
// プレイヤ―の状態
// =========================================
enum class PlayerState {
    Alive,  // 生存中
    Dead    // 死亡中
};

// =========================================
// プレイヤ―データ
// =========================================
struct PlayerData {
    TuboEngine::Transform transform;    // Transform
    PlayerState state;                  // プレイヤーの状態
    PlayerEnergyData energy;	        // エネルギー配分データ 
    MovementData movement;              // 移動速度
    PlayerHPData hp;                    // HP設定
};