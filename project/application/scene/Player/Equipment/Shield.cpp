#include "Shield.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include <algorithm>

using namespace TuboEngine;

// =========================================
// Initialize
// =========================================
void Shield::Initialize(const ShieldData& data, TuboEngine::Camera* camera) {
    data_ = data;
    // 初期状態では基準最大値をそのまま使用
    effectivemaxshield_ = data.maxshield;
    rechargetimer_ = 0.0f;
    isrecharging_ = false;
    isediting_ = false;
    // 現在値を最大値以内に収める
    data_.shield = std::clamp(data_.shield, 0.0f, effectivemaxshield_);


    // シールドオブジェクトの生成、初期化
    object_ = std::make_unique<Object3d>();
    object_->Initialize("block/block.obj");
    object_->SetCamera(camera);       
    // 初期位置をオフセットに設定
    object_->SetPosition(data_.offset);
}

// =========================================
// Update
// =========================================
void Shield::Update(const TuboEngine::Math::Vector3& playerPosition) {
    constexpr float kDeltaTime = 1.0f / 60.0f;

    // ImGuiで設定編集中でなければリチャージ処理
    if (!isediting_) {
        UpdateRecharge(kDeltaTime);
    }

    // =========================================
    // プレイヤーに追従
    // =========================================
    TuboEngine::Math::Vector3 position{
        playerPosition.x + data_.offset.x,
        playerPosition.y + data_.offset.y,
        playerPosition.z + data_.offset.z
    };
    object_->SetPosition(position);
    object_->Update();
}

// =========================================
// Draw
// =========================================
void Shield::Draw() {
	// シールドオブジェクトが表示中であれば描画
    if (data_.isvisible) {
        object_->Draw();
    }
}

// =========================================
// ダメージ処理
// =========================================
float Shield::TakeDamage(float damage) {
    if (damage <= 0.0f) {
        return 0.0f;
    }

    // シールドがない場合は全ダメージを返す
    if (data_.shield <= 0.0f) {
        return damage;
    }

    // リチャージを停止
    rechargetimer_ = 0.0f;
    isrecharging_ = false;
    // シールドで吸収できるダメージ
    float absorbedDamage = std::min(data_.shield, damage);
    data_.shield -= absorbedDamage;
    // 吸収しきれなかったダメージを返す
    return damage - absorbedDamage;
}

// =========================================
// リチャージ処理
// =========================================
void Shield::UpdateRecharge(float deltaTime) {
    // ブースター適用後の最大値に到達しているか判定
    if (data_.shield >= effectivemaxshield_) {
        data_.shield = effectivemaxshield_;
        // リチャージ完了
        rechargetimer_ = 0.0f;
        isrecharging_ = false;
        return;
    }

    // ダメージを受けてからの経過時間
    rechargetimer_ += deltaTime;
    // リチャージ開始まで待機
    if (rechargetimer_ < data_.rechargetime) {
        return;
    }

    isrecharging_ = true;
    // シールドを回復
    data_.shield += data_.rechargerate * deltaTime;
    // 最大値を超えないようにする
    data_.shield = std::min(data_.shield, effectivemaxshield_);
}

// =========================================
// シールド出力率設定
// =========================================
void Shield::SetOutputRate(float outputRate) {
    // 出力率を0.0～1.0に制限
    outputRate = std::clamp(outputRate, 0.0f, 1.0f);
    // 基準最大シールド値から実際の最大値を計算
    effectivemaxshield_ = data_.maxshield * outputRate;
    // 現在値が最大値を超えないようにする
    data_.shield = std::min(data_.shield, effectivemaxshield_);
}

// =========================================
// 設定データを反映
// =========================================
void Shield::ApplySettings(const ShieldData& data) {
    // 設定値のみ反映
    data_.maxshield = data.maxshield;
    data_.rechargetime = data.rechargetime;
    data_.rechargerate = data.rechargerate;

    data_.isvisible = data.isvisible;
    data_.fadetime = data.fadetime;
    data_.offset = data.offset;

    // 現在のシールド値は最大値を超えないようにする
    data_.shield = std::clamp(data_.shield, 0.0f, effectivemaxshield_);
}

// =========================================
// ImGui
// =========================================
void Shield::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::SeparatorText("Shield");
    ImGui::Text("Shield : %.1f / %.1f", data_.shield, effectivemaxshield_);
    float shieldRate = 0.0f;
    if (effectivemaxshield_ > 0.0f) {
        shieldRate = data_.shield / effectivemaxshield_;
    }
    // シールドゲージ（青色）
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(60.0f / 255.0f, 140.0f / 255.0f, 255.0f / 255.0f, 1.0f));
    ImGui::ProgressBar(shieldRate, ImVec2(-1.0f, 0.0f));
    ImGui::PopStyleColor();

    ImGui::Text("Recharge Timer : %.2f", rechargetimer_);
    ImGui::Text("Recharging : %s", isrecharging_ ? "true" : "false");

    ImGui::Separator();

    // 設定変更前の値を保存
    float oldMaxShield = data_.maxshield;
    float oldRechargeTime = data_.rechargetime;
    float oldRechargeRate = data_.rechargerate;
    float oldFadeTime = data_.fadetime;
    auto oldOffset = data_.offset;
    bool oldIsVisible = data_.isvisible;
    bool editing = false;

    // シールド耐久値設定    
    ImGui::DragFloat("Max Shield", &data_.maxshield, 1.0f);
    editing = ImGui::IsItemActive();
    ImGui::DragFloat("Recharge Delay", &data_.rechargetime, 0.1f);
    editing |= ImGui::IsItemActive();
    ImGui::DragFloat("Recharge Rate", &data_.rechargerate, 1.0f);
    editing |= ImGui::IsItemActive();

    ImGui::SeparatorText("Shield Object");
    // オブジェクト設定
    ImGui::Checkbox("Display", &data_.isvisible);
    editing |= ImGui::IsItemActive();
    ImGui::DragFloat3("Offset", &data_.offset.x, 0.01f);
    editing |= ImGui::IsItemActive();
    ImGui::DragFloat("Fade Time", &data_.fadetime, 0.01f, 0.0f, 10.0f);
    editing |= ImGui::IsItemActive();
    ImGui::Text(
        "Offset: X=%.2f Y=%.2f Z=%.2f",
        data_.offset.x,
        data_.offset.y,
        data_.offset.z
    );
    // =========================================
    // 設定値が変更されたか判定
    // =========================================
    isediting_ =
        editing ||
        oldMaxShield != data_.maxshield ||
        oldRechargeTime != data_.rechargetime ||
        oldRechargeRate != data_.rechargerate ||
        oldIsVisible != data_.isvisible ||
        oldFadeTime != data_.fadetime ||
        oldOffset.x != data_.offset.x ||
        oldOffset.y != data_.offset.y ||
        oldOffset.z != data_.offset.z;

#endif
}