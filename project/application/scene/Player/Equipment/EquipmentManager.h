#pragma once
#include "EquipmentData.h"
#include "EquipmentDataJson.h"
#include "Shield.h"
#include "../PlayerHP.h"
#include "Camera.h"

// =========================================
// プレイヤ―の装備管理クラス
// =========================================
class EquipmentManager {
private:    
    // シングルトンパターン
    // シングルトンインスタンス
    static EquipmentManager* instance_;
    // コピー・代入を禁止
    EquipmentManager(const EquipmentManager&) = delete;
    EquipmentManager& operator=(const EquipmentManager&) = delete;
public:
    // コンストラクタ
    EquipmentManager() = default;
    // デストラクタ
    ~EquipmentManager() = default;
    // シングルトンインスタンスの取得
    static EquipmentManager* GetInstance();
    // シングルトンの終了処理
    static void Finalize();
public:
    // 初期化処理
    void Initialize(EquipmentData& data, EquipmentDataJson* dataJson, TuboEngine::Camera* camera);
    // 更新処理
    void Update(float shieldOutputRate);
    // 描画処理
    void Draw();
    // ImGui
    void DrawImGui();
    // ダメージ処理
    void TakeDamage(float damage, PlayerHP& hp, PlayerState& state);
private:
    // 装備データ
    EquipmentData* data_ = nullptr;
    // JSON読み込みクラス
    EquipmentDataJson* dataJson_ = nullptr;
    // シールド
    Shield shield_;
    // カメラ
    TuboEngine::Camera* camera_ = nullptr;

public:
    // Getter
    Shield* GetShield() { return &shield_; }
    const Shield* GetShield() const { return &shield_; }
};