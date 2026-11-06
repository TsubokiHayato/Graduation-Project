#pragma once
#include "Object3d.h"
#include "Input.h"
#include <memory>
#include "PlayerDataJson.h"
#include "PlayerData.h"
#include "PlayerMovement.h"

enum class PlayerState {
    Alive,  // 生存中
    Dead    // 死亡中
};


// プレイヤ―要素をまとめたクラス
class Player {
public:
    // 初期化
    void Initialize(TuboEngine::Camera* camera);
    // 更新
    void Update();
    // 描画
    void Draw();
    // ImGui
    void DrawImGui();

    // ダッシュ開始処理
    void StartDash();
    // ダッシュ更新処理
    void UpdateDash();

    // ダメージ処理    
    void TakeDamage();
    // 無敵時間更新
    void UpdateInvincible();
    // 死亡処理
    void Die();

    // HPリセット
    void ResetHP();

    // 強制的に生存状態にする
    void SetAlive();

    // 強制的に死亡状態にする
    void SetDead();

    // ブースター更新
    void UpdateBooster();
    // ブースター出力を取得（0.0～1.0）
    float GetBoosterOutput() const;
private:
    // プレイヤーデータ
    PlayerData data_;
	// プレイヤーデータJSON読み込みクラス
    std::unique_ptr <PlayerDataJson> dataJson_;
    // 現在の状態
    PlayerState state_ = PlayerState::Alive;
    // 移動処理関連クラス
    std::unique_ptr <PlayerMovement> movement_;

    // プレイヤー本体
    std::unique_ptr<TuboEngine::Object3d> object_;
    // ダッシュ設定
    float maxDashDistance_ = 5.0f;
    float dashSideMultiplier_ = 1.5f;

    int dashDuration_ = 12;
    int dashCooldown_ = 60;

    // ダッシュ状態
    bool isDashing_ = false;
    bool spaceWasPressed_ = false;

    int dashTimer_ = 0;
    int dashCooldownTimer_ = 0;

    // ダッシュ位置・方向
    TuboEngine::Math::Vector3 dashStartPos_ = {};
    TuboEngine::Math::Vector3 dashDirection_ = {};
    // =========================================
    // ブースター設定
    // =========================================
    // ブースター最大値
    float maxBooster_ = 100.0f;
    // 現在のブースター残量
    float currentBooster_ = 100.0f;
    // ブースター消費速度（1秒あたり）
    float boosterConsumeRate_ = 30.0f;
    // ブースター回復速度（1秒あたり）
    float boosterRecoverRate_ = 20.0f;
    // 現在ブーストを使用しているか
    bool isBoosting_ = false;


    // 最大HP
    int maxHP_ = 100;
    // 現在HP
    int currentHP_ = 100;
    // ダメージ量
    int damage_ = 10;
    // 無敵時間（フレーム）
    int invincibleDuration_ = 60;
    // 無敵タイマー
    int invincibleTimer_ = 0;

    // Qキーの入力状態(HPの動作確認用変数)
    bool qWasPressed_ = false;
public:
    // 無敵状態かどうか
    bool IsInvincible() const { return invincibleTimer_ > 0; }
};