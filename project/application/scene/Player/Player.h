#pragma once
#include "Object3d.h"
#include "Input.h"
#include <memory>
#include "PlayerDataJson.h"
#include "PlayerData.h"
#include "PlayerEnergyOutput.h"
#include "PlayerMovement.h"

enum class PlayerState {
    Alive,  // 生存中
    Dead    // 死亡中
};


// プレイヤ―要素をまとめたクラス
class Player {
public:
    // 初期化処理
    void Initialize(TuboEngine::Camera* camera);
    // 更新処理
    void Update();
    // 描画処理
    void Draw();
    // ImGui
    void DrawImGui();

    //// ダメージ処理    
    //void TakeDamage();
    //// 無敵時間更新
    //void UpdateInvincible();
    //// 死亡処理
    //void Die();
    //// HPリセット
    //void ResetHP();

    //// 強制的に生存状態にする
    //void SetAlive();
    //// 強制的に死亡状態にする
    //void SetDead();

private:
    // プレイヤー本体
    std::unique_ptr<TuboEngine::Object3d> object_;
    // プレイヤーデータ
    PlayerData data_;
	// プレイヤーデータJSON読み込みクラス
    std::unique_ptr <PlayerDataJson> dataJson_;
    // 現在の状態
    PlayerState state_ = PlayerState::Alive;
    // クラス
    std::unique_ptr <PlayerEnergyOutput> energyoutput_;    
    // 移動処理関連クラス
    std::unique_ptr <PlayerMovement> movement_;

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