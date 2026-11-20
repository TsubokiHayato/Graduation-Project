#pragma once
#include "PlayerData.h"

// プレイヤ―のエネルギー管理クラス
class PlayerEnergyOutput {
public:
    // 初期化処理    
    void Initialize(const PlayerEnergyData& energyData);
    // 更新処理
    void Update();
    // ImGui
    void DrawImGui();
private:
    // スライダーの値からエネルギー配分を決める（合計が maxenergy を超えたら比率を保って縮める）
    void ApplySliderInput();

    // プレイヤーのエネルギーデータ
    PlayerEnergyData energyData_{};
    // マイコンの4スライダーで配分を操作するか（未接続の時は JSON / ImGui の値のまま）
    bool useSlider_ = true;
public:
    // スライダー操作の ON/OFF
    void SetUseSlider(bool use) { useSlider_ = use; }
    // 今スライダーで配分を操作しているか（ON かつ接続中）
    bool IsSliderControlled() const;
    // エネルギーデータ取得
    const PlayerEnergyData& GetEnergyData() const { return energyData_; }
	// ブースター出力率取得（0.0～1.0）
    float GetBoosterOutputRate() const { return energyData_.booster / 100.0f; }
};