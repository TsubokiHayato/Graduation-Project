#include "EquipmentManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// シングルトン用インスタンス
EquipmentManager* EquipmentManager::instance_ = nullptr;

// =========================================
// シングルトンインスタンスの取得
// =========================================
EquipmentManager* EquipmentManager::GetInstance() {
    // まだインスタンスが生成されていなければ作成
    if (instance_ == nullptr) {
        instance_ = new EquipmentManager;
    }
    return instance_;
}

// =========================================
// 終了処理
// =========================================
void EquipmentManager::Finalize() {
    // インスタンスを削除してnullptrに戻す
    delete instance_;
    instance_ = nullptr;
}

// =========================================
// Initialize
// =========================================
void EquipmentManager::Initialize(TuboEngine::Camera* camera) { 
    // カメラを保持
    camera_ = camera;
    // 装備データを読み込む
    dataJson_ = std::make_unique<EquipmentDataJson>();
    dataJson_->Load(data_);
    // シールド初期化
    shield_.Initialize(data_.shield, camera);
	// 近距離攻撃初期化
	attackmelee_.Initialize(data_.melee, camera);
}

// =========================================
// Update
// =========================================
void EquipmentManager::Update(float shieldOutputRate, const TuboEngine::Math::Vector3& playerPosition) {
    // シールド出力率を反映
    shield_.SetOutputRate(shieldOutputRate);
    // 近距離攻撃出力率を反映
    attackmelee_.SetOutputRate(shieldOutputRate);

    // シールド更新
    shield_.Update(playerPosition);
	// 近距離攻撃更新
    attackmelee_.Update(playerPosition);
}

// =========================================
// Draw
// =========================================
void EquipmentManager::Draw() {
    // シールド 描画
    shield_.Draw();
	// 近距離攻撃 描画
    attackmelee_.Draw();
}

// =========================================
// ダメージ処理
// =========================================
void EquipmentManager::TakeDamage(float damage, PlayerHP& hp, PlayerState& state) {
    // シールドでダメージを吸収
    float remainingDamage = shield_.TakeDamage(damage);
    // シールドで吸収しきれなかった場合、HPへダメージを渡す
    if (remainingDamage > 0.0f) {
        hp.TakeDamage(remainingDamage, state);
    }
}

// =========================================
// JSONにデータを保存
// =========================================
void EquipmentManager::DataSave(const std::string& type) {
    // シールドのみ保存
    if (type == "Shield") {
        // 現在のシールドデータを取得
        data_.shield = shield_.GetShieldData();
        // 保存時は現在のシールドを満タン状態にする
        data_.shield.shield = data_.shield.maxshield;
    }    
    // 近距離攻撃のみ保存
    else if (type == "melee") {
        // 現在の近距離攻撃データを取得
        data_.melee = attackmelee_.GetMeleeAttackData();
        // 保存時は現在の近距離攻撃を満タン状態にする
        data_.melee.attack.damage = data_.melee.attack.maxdamage;        
    }  
    // JSON保存
    dataJson_->Save(data_);
}

// =========================================
// 初期値に戻す
// =========================================
void EquipmentManager::DataReset(const std::string& type) {
    if (dataJson_->Reset(data_)) {
        // シールドのみリセット
        if (type == "Shield") {
            // 読み込んだ設定値をシールドに反映し、再初期化        
            shield_.ApplySettings(data_.shield);
        }
        // 近距離攻撃のみリセット
        else if (type == "melee") {
            // 読み込んだ設定値を近距離攻撃に反映し、再初期化
            attackmelee_.ApplySettings(data_.melee);
        }
    }
}

// =========================================
// ImGui
// =========================================
void EquipmentManager::DrawImGui(PlayerHP& hp, PlayerState& state) {
#ifdef USE_IMGUI
    // 各装備の設定
    ImGui::Begin("Equipment");
    // 設定編集中、またはデータ未初期化の場合はSave・Resetを禁止
    bool disabled = shield_.IsEditing() || dataJson_ == nullptr;

    // シールド設定
    shield_.DrawImGui();    
    // シールドのダメージ確認用
    if (ImGui::Button("Test Damage (50)")) {
        TakeDamage(50.0f, hp, state);
    } 
    ImGui::BeginDisabled(disabled);
    // セーブ関数
    if (ImGui::Button("Shield Save")) {
        DataSave("Shield");
    }
    // リセット関数
    if (ImGui::Button("Shield Reset")) {
        DataReset("Shield");
    }      
    ImGui::EndDisabled();

    ImGui::Separator();
    // 近距離攻撃設定
    attackmelee_.DrawImGui();

    ImGui::BeginDisabled(disabled);
    // セーブ関数
    if (ImGui::Button("Melee Save")) {
        DataSave("melee");
    }
    // リセット関数
    if (ImGui::Button("Melee Reset")) {
        DataReset("melee");
    }
    ImGui::EndDisabled();

    ImGui::Separator();        
    ImGui::End();
#endif
}