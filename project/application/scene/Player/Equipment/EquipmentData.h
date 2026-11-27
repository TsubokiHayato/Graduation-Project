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
// 攻撃の状態
// =========================================
enum class AttackState {
    None,       // 攻撃していない
    Startup,    // 攻撃発生前
    Active,     // 攻撃判定中
    Recovery    // 攻撃後の硬直
};

// =========================================
// 攻撃共通設定
// =========================================
struct AttackData {
	AttackState state;     // 攻撃の状態 
    float damage;          // 現在攻撃力
    float maxdamage;       // 最大攻撃力
    float attackinterval;  // 攻撃間隔
    int startupframes;     // 攻撃発生までのフレーム数
    int activeframes;      // 攻撃判定の持続フレーム数
    int recoveryframes;    // 攻撃後の硬直フレーム数
};

// =========================================
// 近距離攻撃設定
// =========================================
struct MeleeAttackData {
    AttackData attack; //攻撃共通設定
    TuboEngine::Math::Vector3 offset; // プレイヤー座標からの表示位置
};

// =========================================
// 遠距離攻撃設定
// =========================================
struct RangedAttackData {
    AttackData attack; //攻撃共通設定
    TuboEngine::Math::Vector3 offset; // プレイヤー座標からの表示位置
};

// =========================================
// 装備データ
// =========================================
struct EquipmentData {
    ShieldData shield;          // シールドの設定
    MeleeAttackData melee;      // 近距離攻撃の設定
  //  RangedAttackData ranged;    // 遠距離攻撃の設定
};