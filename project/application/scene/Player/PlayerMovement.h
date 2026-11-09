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
    // 通常移動
    void Move(float booster);

    float CalculateMoveSpeed(float booster) const;

private:   
    TuboEngine::Object3d* object_ = nullptr;
    MovementData movedata_;
public:
    // 移動データ取得
    const MovementData& GetMovementData() const { return movedata_; }
};