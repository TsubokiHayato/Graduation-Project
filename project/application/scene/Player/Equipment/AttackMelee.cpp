#include "AttackMelee.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =============================================================================
// 初期化
// =============================================================================
void AttackMelee::Initialize(const MeleeAttackData& data, TuboEngine::Camera* camera) {
    data_ = data;
    // オブジェクトの生成、初期化
    object_ = std::make_unique<Object3d>();
    object_->Initialize("block/block.obj");
    object_->SetCamera(camera);

    // 攻撃状態を初期化
    data_.attack.state = AttackState::None;
    effectivemaxdamage_ = 0.0f;
    isediting_ = false;
    data_.offset.x = 2.0f;
  //  data_.offset.y = 0.0f;
  //  data_.offset.z = 0.0f;
}

// =============================================================================
// 更新処理
// =============================================================================
void AttackMelee::Update(const TuboEngine::Math::Vector3& playerPosition) {
    // プレイヤーの位置 + オフセット
    TuboEngine::Math::Vector3 position{
        playerPosition.x + data_.offset.x,
        playerPosition.y + data_.offset.y,
        playerPosition.z + data_.offset.z
    };

    object_->SetPosition(position);
    object_->Update();
}

// =============================================================================
// 描画処理
// =============================================================================
void AttackMelee::Draw() {
    object_->Draw();
}

// =============================================================================
// 設定反映
// =============================================================================
void AttackMelee::ApplySettings(const MeleeAttackData& data) {
    data_ = data;
    // 攻撃状態を初期化
    data_.attack.state = AttackState::None;
}

// =============================================================================
// エネルギー出力率設定
// =============================================================================
void AttackMelee::SetOutputRate(float outputRate) {
    // 出力率を0.0～1.0に制限
    outputRate = std::clamp(outputRate, 0.0f, 1.0f);    
    // 基準最大攻撃力から実際の最大攻撃力を計算
    effectivemaxdamage_ = data_.attack.maxdamage * outputRate;
    // 現在の攻撃力を最大値以内に収める
    data_.attack.damage = std::min(data_.attack.damage, effectivemaxdamage_);
}

// =============================================================================
// ImGui
// =============================================================================
void AttackMelee::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("Melee Attack", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 攻撃力ゲージ
        ImGui::Text("Damage : %.1f / %.1f", data_.attack.damage, data_.attack.maxdamage);
        float damageRate = 0.0f;
        // 最大ダメージが0より大きい場合のみ割合を計算
        if (data_.attack.maxdamage > 0.0f) {
            damageRate = data_.attack.damage / data_.attack.maxdamage;
        }
        // 攻撃ゲージ（赤色）
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
        ImGui::ProgressBar(damageRate, ImVec2(-1.0f, 0.0f));
        ImGui::PopStyleColor();

        // 攻撃状態を表示
        const char* stateName = "None";
        switch (data_.attack.state) {
        case AttackState::None:
            stateName = "None";
            break;
        case AttackState::Startup:
            stateName = "Startup";
            break;
        case AttackState::Active:
            stateName = "Active";
            break;
        case AttackState::Recovery:
            stateName = "Recovery";
            break;
        }

        ImGui::Text("Attack State : %s", stateName);

        ImGui::Separator();

        // 設定変更前の値を保存
        float oldMaxDamage = data_.attack.maxdamage;
        float oldAttackInterval = data_.attack.attackinterval;
        int oldStartupFrames = data_.attack.startupframes;
        int oldActiveFrames = data_.attack.activeframes;
        int oldRecoveryFrames = data_.attack.recoveryframes;
        auto oldOffset = data_.offset;

        // 攻撃中は設定変更不可
        bool isAttacking = data_.attack.state != AttackState::None;
        ImGui::BeginDisabled(isAttacking);

        // 攻撃設定
        ImGui::DragFloat("Max Damage", &data_.attack.maxdamage, 1.0f, 0.0f);
        ImGui::DragFloat("Attack Interval", &data_.attack.attackinterval, 0.1f, 0.0f);

        ImGui::DragInt("Startup Frames", &data_.attack.startupframes, 1.0f, 0);
        ImGui::DragInt("Active Frames", &data_.attack.activeframes, 1.0f, 0);
        ImGui::DragInt("Recovery Frames", &data_.attack.recoveryframes, 1.0f, 0);

        ImGui::SeparatorText("Melee Attack Object");

        // オブジェクト設定
        ImGui::DragFloat3("Melee Offset", &data_.offset.x, 0.01f);

        ImGui::EndDisabled();

        // 設定値が変更されたか判定
        isediting_ =
            oldMaxDamage != data_.attack.maxdamage ||
            oldAttackInterval != data_.attack.attackinterval ||
            oldStartupFrames != data_.attack.startupframes ||
            oldActiveFrames != data_.attack.activeframes ||
            oldRecoveryFrames != data_.attack.recoveryframes ||
            oldOffset.x != data_.offset.x ||
            oldOffset.y != data_.offset.y ||
            oldOffset.z != data_.offset.z;

        // 現在の攻撃力を更新
        data_.attack.damage = effectivemaxdamage_;
    }

#endif
}