#pragma once
#include "EquipmentData.h"

// =========================================
// シールドクラス
// =========================================
class Shield {
public:
    // 初期化
    void Initialize(const ShieldData& data);
    // 更新
    void Update();
    // ダメージ処理
    float TakeDamage(float damage);
    // リチャージ処理
    void UpdateRecharge(float deltaTime);
    // ImGui
    void DrawImGui();
    // シールド出力率設定
    void SetOutputRate(float outputRate);

private:
    ShieldData data_{};
    float basemaxshield_;     // エネルギー出力率100%時の最大シールド量
    // リチャージ関連
    float rechargetimer_;     // ダメージを受けてからの経過時間
    bool isrecharging_;       // リチャージ中かどうか
    // ImGuiで設定編集中かどうかの変数
    bool isediting_ = false;
public:
    // シールドが動作中かどうか
    bool IsBusy() const { return data_.shield < data_.maxshield || isrecharging_ || rechargetimer_ > 0.0f; }
    // ImGuiで設定編集中かどうか
    bool IsEditing() const { return isediting_; }
    // 現在のシールド耐久値を取得
    float GetCurrentShield() const { return data_.shield; }
    // 現在の最大シールド耐久値を取得
    float GetMaxShield() const { return data_.maxshield; }
    // シールドが破壊されているかどうか
    bool IsBroken() const { return data_.shield <= 0.0f; }
    // 現在のシールド設定データを取得
    const ShieldData& GetShieldData() const { return data_; }
};