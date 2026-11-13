#pragma once
#include "PlayerData.h"

// プレイヤ―のHP管理クラス
class PlayerHP {
public:
    // 初期化処理    
    void Initialize(const PlayerHPData& hpdata);
    // 更新処理
    void Update();
    // ImGui
    void DrawImGui();
    // ダメージ処理
    void TakeDamage(float damage);
    // 無敵時間更新
    void UpdateInvincible(float deltaTime);
    // HPリセット
    void ResetHP();

private:
    // プレイヤーのHPデータ
    PlayerHPData hpdata_;
    // 無敵状態
    bool isInvincible_ = false;
    // 無敵時間の経過時間
    float invincibleTimer_ = 0.0f;
public:
    // HPデータ取得
    const PlayerHPData& GetPlayerHPData() const { return hpdata_; }
    // HP取得
    float GetCurrentHP() const { return hpdata_.currenthp; }
    // 最大HP取得
    float GetMaxHP() const { return hpdata_.maxhp; }
    // 無敵状態取得
    bool IsInvincible() const { return isInvincible_; }
};