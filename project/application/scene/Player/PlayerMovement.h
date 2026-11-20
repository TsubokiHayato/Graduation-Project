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

    // 旋回の中心（ボスの位置）を設定。自機の高さもボスと同じ地面に合わせる
    void SetOrbitCenter(const TuboEngine::Math::Vector3& center);
    // 中心からの距離の範囲（最小距離〜ステージ半径）を設定
    void SetDistanceRange(float minDistance, float maxDistance) {
        minDistance_ = minDistance;
        maxDistance_ = maxDistance;
    }
private:
    // 「中心からの距離」と「中心の周りの角度」の変化量を位置に反映する
    // tangential : 周方向の移動量（右入力が +）、radial : 距離方向の移動量（離れる方向が +）
    void MoveOnOrbit(float tangential, float radial);
    // ボスと同じ地面（旋回の中心の高さ）に乗る高さ
    float GetGroundedY() const;
private:
    TuboEngine::Object3d* object_ = nullptr;
    MovementData movedata_; // 移動データ
    // =========================================
    // ダッシュ管理    
    // =========================================
    bool isDashing_;                            // ダッシュ中かどうか
    TuboEngine::Math::Vector3 dashDirection_{}; // ダッシュ方向（x = 周方向、z = 距離方向。前が近づく向き）
    float dashTimer_;                           // ダッシュ経過時間
    float dashCooldownTimer_;                   // クールタイム経過時間
    float currentDashDistance_;                // ダッシュ距離
    float dashSpeed_;                           // ダッシュ速度
    bool isSpacePressed_;                       // 入力管理(SPACEで実行される)
    // =========================================
    // 旋回管理
    // =========================================
    TuboEngine::Math::Vector3 orbitCenter_{}; // 旋回の中心（ボスの位置）
    float minDistance_ = 2.0f;                // 中心からの最小距離
    float maxDistance_ = 20.0f;               // 中心からの最大距離（ステージ半径）
public:
    // 移動データ取得
    const MovementData& GetMovementData() const { return movedata_; }
    // ダッシュ状態取得
    bool IsDashing() const { return isDashing_; }
};