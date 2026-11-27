#pragma once
#include "EquipmentData.h"
#include "EquipmentDataJson.h"
#include "Shield.h"
#include "../PlayerHP.h"
#include "Camera.h"
#include "AttackMelee.h"

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
    void Initialize(TuboEngine::Camera* camera);
    // 更新処理
    void Update(float shieldOutputRate, const TuboEngine::Math::Vector3& playerPosition);
    // 描画処理
    void Draw();
    // ImGui
    void DrawImGui(PlayerHP& hp, PlayerState& state);
    // ダメージ処理
    void TakeDamage(float damage, PlayerHP& hp, PlayerState& state);
    // JSONにデータを保存
    void DataSave(const std::string& type);
    // 初期値に戻す
    void DataReset(const std::string& type);
private:
    // 装備データ
    EquipmentData data_;
    // JSON読み込みクラス
    std::unique_ptr<EquipmentDataJson> dataJson_ = nullptr;
    // シールド
    Shield shield_;
    // 近距離攻撃
    AttackMelee attackmelee_;
    // カメラ
    TuboEngine::Camera* camera_ = nullptr;

public:    
	// シールド取得
    Shield* GetShield() { return &shield_; }
    const Shield* GetShield() const { return &shield_; }
	// 近距離攻撃取得
    AttackMelee* GetAttackMelee() { return &attackmelee_; }
    const AttackMelee* GetAttackMelee() const { return &attackmelee_; }
};