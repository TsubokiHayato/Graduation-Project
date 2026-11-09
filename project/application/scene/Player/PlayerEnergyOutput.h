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
    // エネルギー配分の正規化
    void NormalizeEnergy();
private:
    // プレイヤーのエネルギーデータ
    PlayerEnergyData energyData_{};
public:
    // エネルギーデータ取得
    const PlayerEnergyData& GetEnergyData() const { return energyData_; }
	// ブースター出力率取得（0.0～1.0）
    float GetBoosterOutputRate() const { return energyData_.booster / 100.0f; }
};