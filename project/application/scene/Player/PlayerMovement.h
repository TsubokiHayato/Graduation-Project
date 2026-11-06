#pragma once
#include "Object3d.h"
#include "Input.h"

// プレイヤ―の移動処理関連クラス
class PlayerMovement {
public:

    void Initialize(TuboEngine::Object3d* object);

    void Update();

    // 通常移動
    void Move(float speed);

    // ダッシュ
   // void StartDash();
   // void UpdateDash();

    // ブースター
   // void UpdateBooster();
   // float GetBoosterOutput() const;

private:
    // 
    TuboEngine::Object3d* object_ = nullptr;

    // =========================================
    // ダッシュ設定・状態
    // =========================================
    float maxDashDistance_ = 5.0f;
    float dashSideMultiplier_ = 1.5f;

    int dashDuration_ = 12;
    int dashCooldown_ = 60;

    bool isDashing_ = false;
    bool spaceWasPressed_ = false;

    int dashTimer_ = 0;
    int dashCooldownTimer_ = 0;

    TuboEngine::Math::Vector3 dashStartPos_ = {};
    TuboEngine::Math::Vector3 dashDirection_ = {};

    // =========================================
    // ブースター設定・状態
    // =========================================
    float maxBooster_ = 100.0f;
    float currentBooster_ = 100.0f;

    float boosterConsumeRate_ = 30.0f;
    float boosterRecoverRate_ = 20.0f;

    bool isBoosting_ = false;
};