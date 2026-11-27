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

    // =========================================
    // ロックオン（ゼルダの注目移動）
    //  ロックオン中は W/S = 相手に近づく/離れる、A/D = 相手の周りを回る
    //  毎フレーム Update の前に呼ぶ（解除中は ClearLockOnTarget）
    // =========================================
    void SetLockOnTarget(const TuboEngine::Math::Vector3& target) { hasLockOn_ = true; lockOnTarget_ = target; }
    void ClearLockOnTarget() { hasLockOn_ = false; }
    bool IsLockOnMove() const { return hasLockOn_; }
private:
    // WASD 入力（x = 右(D)+ / 左(A)-、z = 前(W)+ / 後(S)-）
    TuboEngine::Math::Vector3 ReadMoveInput() const;
    // ロックオン相手を中心に移動する（strafe = 右へ回る量、forward = 相手へ近づく量）
    void MoveAroundTarget(float strafe, float forward);

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
    // =========================================
    // ロックオン
    // =========================================
    bool hasLockOn_ = false;                    // ロックオン移動中か
    TuboEngine::Math::Vector3 lockOnTarget_{};  // ロックオン相手の位置
    TuboEngine::Math::Vector3 dashInput_{};     // ロックオン中のダッシュ方向（入力の向き。x=横, z=前後）
    bool isLockOnDash_ = false;                 // ロックオン中に始めたダッシュか
    float minLockOnDistance_ = 3.0f;            // 相手にこれ以上は近づかない距離
public:
    // 移動データ取得
    const MovementData& GetMovementData() const { return movedata_; }
    // ダッシュ状態取得
    bool IsDashing() const { return isDashing_; }
};