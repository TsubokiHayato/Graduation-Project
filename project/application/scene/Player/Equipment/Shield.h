#pragma once
#include "EquipmentData.h"
#include "Object3d.h"
#include "ShieldEffect.h"

// =========================================
// シールドクラス
// =========================================
class Shield {
public:
    // 初期化処理
    void Initialize(const ShieldData& data, TuboEngine::Camera* camera);
    // 更新処理
    void Update(const TuboEngine::Math::Vector3& playerPosition);
    // 描画処理
    void Draw();
    // ImGui
    void DrawImGui();
    // ダメージ処理
    float TakeDamage(float damage);
    // リチャージ処理
    void UpdateRecharge(float deltaTime);
    // シールド出力率設定
    void SetOutputRate(float outputRate);
    // 設定データを反映（オブジェクトは再生成しない）
    void ApplySettings(const ShieldData& data);

    // スケール演出開始
    void StartScaleAnimation(float startScale, float endScale, float duration);
    // スケール演出更新
    void UpdateScaleAnimation(float deltaTime);
private:
    // シールドデータ
    ShieldData data_{};
    // シールドの見た目（ハニカム球）
    ShieldEffect effect_;
    float effectivemaxshield_;      // ブースター適用後の最大シールド値
    // リチャージ関連
    float rechargetimer_;      // ダメージを受けてからの経過時間
    bool isrecharging_;        // リチャージ中かどうか
    // ImGuiで設定編集中かどうかの変数
    bool isediting_;
public:
	// シールドデータ取得
    ShieldData GetShieldData() const { return data_; }
    // シールドエフェクト取得
    ShieldEffect* GetEffect() { return &effect_; }
    // シールドが動作中かどうか
    bool IsBusy() const { return data_.shield < data_.maxshield || isrecharging_ || rechargetimer_ > 0.0f; }
    // ImGuiで設定編集中かどうか
    bool IsEditing() const { return isediting_; }
    // 現在のシールド耐久値を取得
    float GetCurrentShield() const { return data_.shield; }
    // ブースター適用前の基準最大シールド耐久値を取得
    float GetBaseMaxShield() const { return data_.maxshield; }
    // ブースター適用後の最大シールド耐久値を取得
    float GetMaxShield() const { return effectivemaxshield_; }
    // シールドが破壊されているかどうか
    bool IsBroken() const { return data_.shield <= 0.0f; }
};