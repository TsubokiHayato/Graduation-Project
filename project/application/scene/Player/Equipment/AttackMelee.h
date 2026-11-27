#pragma once
#include "EquipmentData.h"
#include "Object3d.h"

// =========================================
//　近距離攻撃クラス
// =========================================
class AttackMelee {
public:
    // 初期化処理
    void Initialize(const MeleeAttackData& data, TuboEngine::Camera* camera);
    // 更新処理
    void Update(const TuboEngine::Math::Vector3& playerPosition);
    // 描画処理
    void Draw();
    // ImGui
    void DrawImGui();
    // 設定反映
    void ApplySettings(const MeleeAttackData& data);
    // エネルギー出力率設定
    void SetOutputRate(float outputRate);
private:
    // 近距離攻撃データ
    MeleeAttackData data_{};
    // オブジェクト本体
    std::unique_ptr<TuboEngine::Object3d> object_;
    // 出力率適用後の最大攻撃力
    float effectivemaxdamage_;
    // ImGuiで設定編集中かどうかの変数
    bool isediting_;
public:
    // データ取得
    MeleeAttackData GetMeleeAttackData() const { return data_; }
    // オブジェクト取得
    TuboEngine::Object3d* GetShieldObject() const { return object_.get(); }
    // 編集中かどうか
    bool IsEditing() const { return isediting_; }
};