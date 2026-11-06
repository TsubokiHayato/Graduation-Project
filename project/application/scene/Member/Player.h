#pragma once
#include "Object3d.h"
#include "Input.h"
#include <memory>
#include "PlayerDataJson.h"
#include "PlayerData.h"

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

    // 通常移動処理
    void Move();
    // ダッシュ開始処理
    void StartDash();
    // ダッシュ更新処理
    void UpdateDash();
private:
    // プレイヤーデータ
    PlayerData data_;
	// プレイヤーデータJSON読み込みクラス
    std::unique_ptr <PlayerDataJson> dataJson_;

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
};