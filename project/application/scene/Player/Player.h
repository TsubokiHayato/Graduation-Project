#pragma once
#include "Object3d.h"
#include "Input.h"
#include <memory>
#include "PlayerDataJson.h"
#include "PlayerData.h"
#include "PlayerEnergyOutput.h"
#include "PlayerMovement.h"
#include "PlayerHP.h"

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
private:
    // プレイヤー本体
    std::unique_ptr<TuboEngine::Object3d> object_;
    // プレイヤーデータ
    PlayerData data_;
    // プレイヤーデータJSON読み込みクラス
    std::unique_ptr <PlayerDataJson> dataJson_;
    // エネルギークラス
    std::unique_ptr <PlayerEnergyOutput> energyoutput_;
    // 移動処理関連クラス
    std::unique_ptr <PlayerMovement> movement_;
    // HP関連クラス
    std::unique_ptr <PlayerHP> hp_;

public:
    // プレイヤー本体取得
    TuboEngine::Object3d* GetPlayerObject() const { return object_.get(); }
    // プレイヤーデータ取得
    const PlayerData& GetPlayerData() const { return data_; }
    // ステート取得
    PlayerState GetState() const { return data_.state; }
    // エネルギー管理クラス取得
    PlayerEnergyOutput* GetEnergyOutput() const { return energyoutput_.get(); }
    // 移動処理クラス取得
    PlayerMovement* GetMovement() const { return movement_.get(); }
    // HP管理クラス取得
    PlayerHP* GetHP() const { return hp_.get(); }
};