#pragma once
#include "Object3d.h"
#include "Input.h"
#include "PlayerData.h"

// プレイヤ―の移動処理関連クラス
class PlayerMovement {
public:
    // 初期化処理
    void Initialize(TuboEngine::Object3d* object, const MovementData& movedata);
    // 更新処理
    void Update(float booster);
    // ImGui
    void DrawImGui(float booster);
    // 移動処理
    void Move(float booster);
	// 移動速度の計算
    float CalculateMoveSpeed(float booster) const;
    // ダッシュ処理
    void Dash(float deltaTime);
    // ダッシュ開始
    void StartDash(float booster);
private:   
    TuboEngine::Object3d* object_ = nullptr;
	MovementData movedata_; // 移動データ
    // =========================================
    // ダッシュ管理    
    // =========================================
	bool isDashing_;                            // ダッシュ中かどうか
    TuboEngine::Math::Vector3 dashDirection_{}; // ダッシュ方向
    float dashTimer_;                           // ダッシュ経過時間
    float dashCooldownTimer_;                   // クールタイム経過時間
    float currentDashDistance_ ;                // ダッシュ距離
    float dashSpeed_;                           // ダッシュ速度
    bool isSpacePressed_;                       // 入力管理(SPACEで実行される)
public:
    // 移動データ取得
    const MovementData& GetMovementData() const { return movedata_; }
    // ダッシュ状態取得
    bool IsDashing() const { return isDashing_; }
};