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
    float maxspeed;  // 最大移動速度
    // ダッシュ設定
    float maxdashdistance = 10.0f;  // 最大ダッシュ距離
    float dashduration = 0.5f;      // ダッシュ時間
    float dashcooldown = 2.0f;      // クールタイム
};

// =========================================
// プレイヤ―データ
// =========================================
struct PlayerData {
    // Transform
    TuboEngine::Transform transform;
	// エネルギー配分データ
	PlayerEnergyData energy;
    // 移動速度
    MovementData movement;
};

//// =========================================
//// 移動設定
//// =========================================
//struct MovementData {
//    // =========================================
//    // 通常移動
//    // =========================================  
//    float speed = 0.1f; // 通常移動速度
//
//    // =========================================     
//    // ダッシュ
//    // =========================================
//     // ダッシュ時の最大移動距離
//    float maxDashDistance = 5.0f;     // ダッシュ時の最大移動距離
//    float dashSideMultiplier = 1.5f;  // 左右方向へのダッシュ距離倍率  
//    int dashDuration = 12;            // ダッシュの継続時間（フレーム）
//    int dashCooldown = 60;            // ダッシュ終了後の再使用待機時間（フレーム)     

//};
//
//// =========================================
//// HP設定
//// =========================================
//struct PlayerHPData {
//    // 最大HP
//    int maxhp = 100;
//    // 現在HP
//    int currenthp = 100;
//    // 無敵時間（フレーム）
//    int invincibleDuration = 60;
//};
//
////
////enum class PlayerState {
////    Alive,  // 生存中
////    Dead    // 死亡中
////};
