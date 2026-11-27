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
    // シールドエフェクト（ハニカム球）の生成、初期化
    effect_.Initialize(camera);
    // シールドオブジェクトの表示状態   
    data_.isvisible = false;
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
	// プレイヤ―の中心（Player の座標はモデル中心）+ 微調整用オフセット
    // =========================================
    TuboEngine::Math::Vector3 position{
        playerPosition.x + data_.offset.x,
        playerPosition.y + data_.offset.y,
        playerPosition.z + data_.offset.z
    };

    // =========================================
    // 数値を見た目に反映
    // =========================================
    ShieldEffect::State state;
    state.position = position;
    // 残量率（基準最大値に対して。出力率で最大値が下がった分もセルの減りとして見せる）
    state.shieldRate = data_.maxshield > 0.0f ? data_.shield / data_.maxshield : 0.0f;
    // 出力率（基準最大値に対して）
    state.outputRate = data_.maxshield > 0.0f ? effectivemaxshield_ / data_.maxshield : 0.0f;
    // 表示はシールド値がある限り常に（isvisible には左右されない）
    effect_.Update(kDeltaTime, state);
}

// =========================================
// Draw
// =========================================
void Shield::Draw() {
	// シールド値がある間は常に描画（セル数・破壊演出は ShieldEffect 側で決まる）
    effect_.Draw();
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
    // 耐久値が残っていれば表示
    data_.isvisible = data_.shield > 0.0f;
    // 見た目：残っていればフラッシュ、0 になったら破壊演出
    if (data_.shield > 0.0f) {
        effect_.NotifyHit();
    } else {
        effect_.NotifyBreak();
    }
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
        data_.isvisible = false;
        return;
    }

    // ダメージを受けてからの経過時間
    rechargetimer_ += deltaTime;
    // リチャージ開始まで待機
    if (rechargetimer_ < data_.rechargetime) {
        return;
    }
    // リチャージ開始
    isrecharging_ = true;
    // リチャージ中は非表示
    data_.isvisible = false;
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
    auto oldOffset = data_.offset;
    bool oldIsVisible = data_.isvisible;
    bool editing = false;

    // 現在値（ドラッグ中はリチャージを止めるので、見た目の変化を確認しやすい）
    float oldShield = data_.shield;
    ImGui::SliderFloat("Current Shield", &data_.shield, 0.0f, effectivemaxshield_);
    editing = ImGui::IsItemActive();
    if (data_.shield != oldShield) {
        data_.isvisible = data_.shield > 0.0f;
    }

    // シールド耐久値設定
    ImGui::DragFloat("Max Shield", &data_.maxshield, 1.0f);
    editing |= ImGui::IsItemActive();
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
    // =========================================
    // 設定値が変更されたか判定
    // =========================================
    isediting_ =
        editing ||
        oldMaxShield != data_.maxshield ||
        oldRechargeTime != data_.rechargetime ||
        oldRechargeRate != data_.rechargerate ||
        oldIsVisible != data_.isvisible ||
        oldOffset.x != data_.offset.x ||
        oldOffset.y != data_.offset.y ||
        oldOffset.z != data_.offset.z;

    // 見た目の調整（ここは isediting_ に含めない＝リチャージを止めない）
    effect_.DrawImGui();
#endif
}