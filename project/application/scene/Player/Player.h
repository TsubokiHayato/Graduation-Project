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
    // 被弾処理（ボスの攻撃が当たったときに呼ぶ）。ダッシュ中・無敵中・死亡中は受けない
    void TakeDamage(float damage);
    // 旋回の中心（ボスの位置）を設定
    void SetOrbitCenter(const TuboEngine::Math::Vector3& center) { movement_->SetOrbitCenter(center); }
    // ボスからの距離の範囲（最小距離〜ステージ半径）を設定
    void SetDistanceRange(float minDistance, float maxDistance) { movement_->SetDistanceRange(minDistance, maxDistance); }
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
    // 現在位置取得
    TuboEngine::Math::Vector3 GetPosition() const { return object_->GetPosition(); }
    // 当たり判定の半径取得（Transform の scale.x を半径として扱う）
    float GetRadius() const { return data_.transform.scale.x; }
};