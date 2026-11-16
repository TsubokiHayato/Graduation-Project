#pragma once
#include "EquipmentData.h"

class Shield {
public:
    // 初期化
    void Initialize(const ShieldData& data);
    // 更新
    void Update();

    // ダメージ処理
    // シールドで吸収しきれなかったダメージを返す
    float TakeDamage(float damage);
    // リチャージ処理
    void UpdateRecharge(float deltaTime);
    // ImGui
    void DrawImGui();
    // シールド出力率設定
    void SetOutputRate(float outputRate);

private:
    ShieldData data_{};
    float baseMaxShield_;
    // リチャージ関連
    float rechargeTimer_;
    bool isRecharging_;
public:
    float GetCurrentShield() const { return data_.shield; }
    float GetMaxShield() const { return data_.maxshield; }
    bool IsBroken() const { return data_.shield <= 0.0f; }
    const ShieldData& GetShieldData() const { return data_; }
};