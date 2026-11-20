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
void EquipmentManager::Initialize(EquipmentData& data, EquipmentDataJson* dataJson, TuboEngine::Camera* camera) {
    // 装備データとJSON読み込みを保持
    data_ = &data;
    dataJson_ = dataJson;
	// カメラを保持
    camera_ = camera;
    // シールド初期化
    shield_.Initialize(data.shield, camera);
}

// =========================================
// Update
// =========================================
void EquipmentManager::Update(float shieldOutputRate, const TuboEngine::Math::Vector3& playerPosition) {
    // シールド出力率を反映
    shield_.SetOutputRate(shieldOutputRate);

    // シールド更新
    shield_.Update(playerPosition);
}

// =========================================
// Draw
// =========================================
void EquipmentManager::Draw() {
    // シールド 描画
    shield_.Draw();
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
// ImGui
// =========================================
void EquipmentManager::DrawImGui() {
#ifdef USE_IMGUI
	// 各装備の設定
    ImGui::Begin("Equipment");
    // シールド設定
    shield_.DrawImGui();

    ImGui::Separator();    
    // 設定編集中、またはデータ未初期化の場合はSave・Resetを禁止
    bool disabled = shield_.IsEditing() || data_ == nullptr || dataJson_ == nullptr;
    ImGui::BeginDisabled(disabled);

    // Save
    if (ImGui::Button("Save")) {
        // 現在のシールドデータを取得
        data_->shield = shield_.GetShieldData();
        // 保存時は現在のシールドを満タン状態にする
        data_->shield.shield = data_->shield.maxshield;      
        // JSON保存
        dataJson_->Save(*data_);
    }

    ImGui::SameLine();
    // Reset
    if (ImGui::Button("Reset")) {
        if (dataJson_->Reset(*data_)) {
            // 読み込んだ設定値をシールドに反映し、再初期化
            shield_.ApplySettings(data_->shield);
        }
    }
    ImGui::EndDisabled();
    ImGui::End();
#endif
}