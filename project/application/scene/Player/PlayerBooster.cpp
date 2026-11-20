#include "PlayerBooster.h"
#include "DirectXCommon.h"
#include "GameTime.h"
#include "ParticleCommon.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;
using TuboEngine::Math::Vector3;
using TuboEngine::Math::Vector4;

namespace {
constexpr float kPi = std::numbers::pi_v<float>;

Vector3 Normalize(const Vector3& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return len > 0.0001f ? v / len : Vector3{0.0f, 0.0f, 1.0f};
}
Vector3 Cross(const Vector3& a, const Vector3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
// dir に垂直な2軸を作る
void MakeBasis(const Vector3& dir, Vector3& u, Vector3& w) {
    Vector3 up = std::abs(dir.y) < 0.9f ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
    u = Normalize(Cross(up, dir));
    w = Cross(dir, u);
}
Vector4 WithAlpha(const Vector4& c, float a) { return {c.x, c.y, c.z, a}; }
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void PlayerBooster::Initialize(const TuboEngine::Object3d* owner, TuboEngine::Camera* camera) {
    owner_ = owner;
    camera_ = camera;

    for (auto& nozzle : nozzles_) {
        nozzle = std::make_unique<Object3d>();
        // cone.obj は +Y 方向が先端（細い側）、原点が底面（太い側＝噴射口）
        nozzle->Initialize("cone/cone.obj");
        nozzle->SetCamera(camera);
        nozzle->SetScale(nozzleScale_);
    }

    // 噴射パーティクル
    // テクスチャは色が白一色でαだけがぼける画像（ストレートα）。
    // 色にαが掛かった画像だと、アルファ合成で縁が暗くにごるため専用に用意している
    const std::string texture = "boosterGlow.png";
    smoke_ = std::make_unique<BoosterParticleEmitter>();
    smoke_->Initialize("BoosterSmoke", "boosterSmoke.png", 400);
    plume_ = std::make_unique<BoosterParticleEmitter>();
    plume_->Initialize("BoosterPlume", texture, 1200);
    sparks_ = std::make_unique<BoosterParticleEmitter>();
    sparks_->Initialize("BoosterSparks", texture, 600);
    core_ = std::make_unique<BoosterParticleEmitter>();
    core_->Initialize("BoosterCore", texture, 1200);
    glow_ = std::make_unique<BoosterParticleEmitter>();
    glow_->Initialize("BoosterGlow", texture, 128);
    pso_ = std::make_unique<BoosterParticlePSO>();
    pso_->Initialize();

    prevOwnerPos_ = owner_ ? owner_->GetPosition() : Vector3{};
    ownerVelocity_ = {};
    yaw_ = 0.0f;
    tilt_ = idleTilt_;
    isMoving_ = false;
    isDashing_ = false;
    throttle_ = idleThrottle_;
    power_ = 0.0f;
    heat_ = 0.0f;
    time_ = 0.0f;
    emitAccum_ = {};

    for (size_t i = 0; i < kNozzleCount; ++i) {
        UpdateNozzle(i);
    }
}

// =============================================================================
//  Update
// =============================================================================
void PlayerBooster::Update(float booster, bool isDashing) {
    if (!owner_) {
        return;
    }
    const float dt = TuboEngine::Time::DeltaTime();
    time_ += dt;

    // ---------------------------------------------
    // プレイヤーの位置の変化から移動方向と速度を求める
    // （PlayerMovement に依存しないので、ダッシュ中も自動で追従する）
    // ---------------------------------------------
    const Vector3 ownerPos = owner_->GetPosition();
    const Vector3 delta = ownerPos - prevOwnerPos_;
    prevOwnerPos_ = ownerPos;

    isMoving_ = (delta.x * delta.x + delta.z * delta.z) > moveThreshold_ * moveThreshold_;
    if (dt > 0.0f) {
        ownerVelocity_ = delta / dt;
    }

    if (isMoving_) {
        // +Z を 0 とした移動方向の角度
        const float targetYaw = std::atan2(delta.x, delta.z);
        yaw_ = LerpAngle(yaw_, targetYaw, yawFollow_);
    }

    // ---------------------------------------------
    // 噴射の強さ = スロットル（状態）× ブースター出力
    // ---------------------------------------------
    output_ = outputOverride_ >= 0.0f ? outputOverride_ : std::clamp(booster, 0.0f, 1.0f);
    const bool dashStarted = isDashing && !isDashing_;
    isDashing_ = isDashing;

    const float targetThrottle = isDashing_ ? dashThrottle_ : (isMoving_ ? moveThrottle_ : idleThrottle_);
    // 吹かす時は素早く、絞る時はゆっくり
    const float throttleSpeed = targetThrottle > throttle_ ? 14.0f : 4.0f;
    throttle_ += (targetThrottle - throttle_) * std::min(1.0f, throttleSpeed * dt);
    if (dashStarted) {
        throttle_ = dashThrottle_;
    }
    power_ = throttle_ * (minOutputGain_ + (1.0f - minOutputGain_) * output_);

    // ノズルの加熱（熱くなるのは速く、冷めるのはゆっくり）
    const float targetHeat = std::clamp(power_, 0.0f, 1.0f);
    const float heatSpeed = targetHeat > heat_ ? 1.2f : 0.35f;
    heat_ += (targetHeat - heat_) * std::min(1.0f, heatSpeed * dt);

    // 炎のゆらぎ
    flicker_ = 0.88f + 0.08f * std::sin(time_ * 53.0f) * std::sin(time_ * 31.0f) + Rand(-0.05f, 0.05f);

    // ---------------------------------------------
    // 傾き：止まっている時は下向き、動いている時は後方へ倒す
    // ---------------------------------------------
    const float targetTilt = isMoving_ ? moveTilt_ + boostTiltBonus_ * output_ : idleTilt_;
    tilt_ += (targetTilt - tilt_) * tiltFollow_;

    for (size_t i = 0; i < kNozzleCount; ++i) {
        UpdateNozzle(i);
        nozzles_[i]->Update();
    }

    // ---------------------------------------------
    // 噴射パーティクル
    // ---------------------------------------------
    if (dt > 0.0f) {
        for (size_t i = 0; i < kNozzleCount; ++i) {
            if (dashStarted) {
                EmitBurst(i);
            }
            EmitExhaust(i, dt);
        }
    }
    smoke_->Update(dt, camera_);
    plume_->Update(dt, camera_);
    sparks_->Update(dt, camera_);
    core_->Update(dt, camera_);
    glow_->Update(dt, camera_);
}

// =============================================================================
//  ノズル1本分の Transform を計算
// =============================================================================
void PlayerBooster::UpdateNozzle(size_t index) {
    if (!owner_ || !nozzles_[index]) {
        return;
    }

    // 取り付け点（＝ノズル先端の位置）：オフセットを向き(yaw)で回転させ、常に背中側に付ける
    const Vector3& offset = mountOffsets_[index];
    const float s = std::sin(yaw_);
    const float c = std::cos(yaw_);
    Vector3 mount = owner_->GetPosition();
    mount.x += offset.x * c + offset.z * s;
    mount.y += offset.y;
    mount.z += -offset.x * s + offset.z * c;

    // 回転 (tilt, yaw, 0) をかけた時の、モデルの +Y（底面→先端）方向
    const float st = std::sin(tilt_);
    const float ct = std::cos(tilt_);
    const Vector3 tipDir{ st * s, ct, st * c };

    // モデルの原点は底面なので、先端が取り付け点に来るよう先端方向と逆にずらす
    // → 先端を支点にしてノズルが首を振る
    const float tipLength = kModelTipLength * nozzleScale_.y;
    Vector3 pos = mount - tipDir * tipLength;

    // 噴射口の位置と噴射方向（底面の中心から、先端と逆向き）
    mouthPos_[index] = pos;
    exhaustDir_[index] = -tipDir;

    // 高出力時は細かく振動させる
    const float shake = 0.012f * std::max(0.0f, power_ - 0.5f);
    pos += Vector3{Rand(-shake, shake), Rand(-shake, shake), Rand(-shake, shake)};

    // 回転：X軸で先端を前方へ倒し、Y軸で向きに合わせる
    // → 先端が背中に付き、噴射口が後方（停止中は下）を向く
    nozzles_[index]->SetPosition(pos);
    nozzles_[index]->SetRotation({ tilt_, yaw_, 0.0f });
    nozzles_[index]->SetScale(nozzleScale_);

    // 加熱に応じてノズルを赤熱させる
    const float h = heat_ * 0.85f;
    nozzles_[index]->SetModelColor({
        std::lerp(1.0f, heatColor_.x, h),
        std::lerp(1.0f, heatColor_.y, h),
        std::lerp(1.0f, heatColor_.z, h),
        1.0f });
}

// =============================================================================
//  噴射パーティクル（毎フレーム）
// =============================================================================
void PlayerBooster::EmitExhaust(size_t index, float dt) {
    const float p = std::clamp(power_, 0.0f, 1.5f);
    if (p <= 0.001f) {
        return;
    }
    const float pc = std::min(p, 1.0f);
    const Vector3 mouth = mouthPos_[index];
    const Vector3 dir = exhaustDir_[index];
    const float radius = nozzleScale_.x;   // cone.obj の底面の半径は 1
    const float s = effectScale_;
    const Vector3 baseVel = ownerVelocity_ * inheritVelocity_;
    const float jet = jetSpeedMin_ + (jetSpeedMax_ - jetSpeedMin_) * p;

    // rate(個/秒) から今フレームに出す数を求める（端数は持ち越す）
    auto count = [&](size_t slot, float rate) {
        float& acc = emitAccum_[index][slot];
        acc += rate * dt;
        int n = static_cast<int>(acc);
        acc -= static_cast<float>(n);
        return n;
    };

    // ---------------------------------------------
    // 芯：白→水色の高温部分。速度方向に伸ばして勢いを出す
    // ---------------------------------------------
    for (int n = count(0, coreRate_ * p); n > 0; --n) {
        BoosterParticle q;
        // フレーム内でばらけるよう、少し進んだ位置から出す
        q.position = mouth + RandomInDisk(index, radius * 0.35f) + dir * Rand(0.0f, jet * dt);
        q.velocity = dir * (jet * Rand(0.85f, 1.1f)) + RandomInDisk(index, jet * 0.04f) + baseVel;
        q.sizeStart = (0.16f + 0.12f * pc) * s;
        q.sizeEnd = 0.04f * s;
        q.stretch = 0.03f;
        q.colorStart = {1.0f, 1.0f, 1.0f, 1.0f};
        q.colorMid = coreColor_;
        q.colorEnd = WithAlpha(flameColor_, 0.0f);
        q.midPoint = 0.3f;
        q.drag = 2.0f;
        q.lifeTime = Rand(0.07f, 0.12f) * (0.7f + 0.5f * pc);
        core_->Spawn(q);
    }

    // ---------------------------------------------
    // 炎：青→紫に変わりながら広がって消える
    // ---------------------------------------------
    for (int n = count(1, plumeRate_ * p); n > 0; --n) {
        BoosterParticle q;
        q.position = mouth + RandomInDisk(index, radius * 0.6f) + dir * Rand(0.0f, jet * dt);
        q.velocity = dir * (jet * Rand(0.45f, 0.7f)) + RandomInDisk(index, jet * 0.12f) + baseVel;
        q.sizeStart = (0.3f + 0.2f * pc) * s;
        q.sizeEnd = (0.7f + 0.6f * pc) * s;
        q.colorStart = WithAlpha(coreColor_, 0.85f);
        q.colorMid = flameColor_;
        q.colorEnd = flameEndColor_;
        q.midPoint = 0.3f;
        q.drag = 3.5f;
        q.rotation = Rand(0.0f, kPi * 2.0f);
        q.rotationSpeed = Rand(-6.0f, 6.0f);
        q.lifeTime = Rand(0.16f, 0.28f) * (0.7f + 0.6f * pc);
        plume_->Spawn(q);
    }

    // ---------------------------------------------
    // 火花：出力が高い時だけ。重力で落ちるオレンジの筋
    // ---------------------------------------------
    if (enableSparks_) {
        const float sparkAmount = std::max(0.0f, (p - 0.45f) / 0.55f);
        for (int n = count(2, sparkRate_ * sparkAmount); n > 0; --n) {
            BoosterParticle q;
            q.position = mouth + RandomInDisk(index, radius * 0.3f);
            q.velocity = dir * (jet * Rand(0.4f, 1.0f)) + RandomInDisk(index, jet * 0.35f)
                + Vector3{0.0f, Rand(0.0f, 1.5f), 0.0f} + baseVel;
            q.gravity = -12.0f;
            q.drag = 1.0f;
            q.sizeStart = 0.06f * s;
            q.sizeEnd = 0.02f * s;
            q.stretch = 0.025f;
            q.colorStart = {1.0f, 1.0f, 0.85f, 1.0f};
            q.colorMid = sparkColor_;
            q.colorEnd = {sparkColor_.x, sparkColor_.y * 0.35f, 0.05f, 0.0f};
            q.midPoint = 0.25f;
            q.lifeTime = Rand(0.25f, 0.55f);
            sparks_->Spawn(q);
        }
    }

    // ---------------------------------------------
    // 煙：薄い灰色。ゆっくり広がって上へ昇る
    // ---------------------------------------------
    if (enableSmoke_) {
        for (int n = count(3, smokeRate_ * (0.25f + p)); n > 0; --n) {
            BoosterParticle q;
            q.position = mouth + dir * (Rand(0.2f, 0.5f) * (0.5f + pc)) + RandomInDisk(index, radius * 0.8f);
            q.velocity = dir * (jet * Rand(0.15f, 0.3f)) + RandomInDisk(index, 0.6f) + baseVel * 0.3f;
            q.gravity = 1.2f;
            q.drag = 2.5f;
            q.sizeStart = 0.3f * s;
            q.sizeEnd = (1.3f + 0.8f * pc) * s;
            q.colorStart = {0.62f, 0.64f, 0.7f, 0.0f};
            q.colorMid = {0.55f, 0.57f, 0.63f, 0.22f + 0.1f * pc};
            q.colorEnd = {0.5f, 0.51f, 0.55f, 0.0f};
            q.midPoint = 0.25f;
            q.rotation = Rand(0.0f, kPi * 2.0f);
            q.rotationSpeed = Rand(-1.5f, 1.5f);
            q.lifeTime = Rand(0.6f, 1.1f);
            smoke_->Spawn(q);
        }
    }

    // ---------------------------------------------
    // 1フレームだけ表示する発光（ジェット・噴射口の光・ショックダイヤモンド）
    // ---------------------------------------------
    const float oneFrame = dt * 1.5f;
    const float jetLength = (0.5f + 2.2f * pc) * std::max(1.0f, p) * flicker_ * s;
    const float jetWidth = radius * 1.6f * (0.7f + 0.4f * pc) * s;

    // 外側のジェット（青く大きい）
    {
        BoosterParticle q;
        const float len = jetLength * 1.6f;
        q.position = mouth + dir * (len * 0.5f);
        q.axis = dir;
        q.axisLength = len;
        q.sizeStart = q.sizeEnd = jetWidth * 1.9f;
        q.colorStart = q.colorMid = q.colorEnd = WithAlpha(flameColor_, 0.5f * pc + 0.25f);
        q.lifeTime = oneFrame;
        plume_->Spawn(q); // 色で見せる部分なのでアルファ合成側に入れる
    }
    // 内側のジェット（白に近い芯）
    {
        BoosterParticle q;
        q.position = mouth + dir * (jetLength * 0.5f);
        q.axis = dir;
        q.axisLength = jetLength;
        q.sizeStart = q.sizeEnd = jetWidth;
        q.colorStart = q.colorMid = q.colorEnd = {0.85f, 0.95f, 1.0f, 0.75f * pc + 0.1f};
        q.lifeTime = oneFrame;
        glow_->Spawn(q);
    }
    // 噴射口の光
    {
        BoosterParticle q;
        q.position = mouth + dir * 0.05f;
        q.sizeStart = q.sizeEnd = (0.55f + 0.45f * pc) * radius * 2.2f * flicker_ * s;
        q.colorStart = q.colorMid = q.colorEnd = WithAlpha(coreColor_, 0.55f * pc + 0.15f);
        q.lifeTime = oneFrame;
        glow_->Spawn(q);
    }
    // ショックダイヤモンド（高出力時にジェットの中に並ぶ光の節）
    if (enableDiamonds_ && p > 0.55f) {
        const float a = std::clamp((p - 0.55f) / 0.45f, 0.0f, 1.0f);
        for (int k = 0; k < 5; ++k) {
            BoosterParticle q;
            q.position = mouth + dir * (jetLength * (0.22f + 0.2f * static_cast<float>(k)));
            q.sizeStart = q.sizeEnd = radius * 1.1f * (1.0f - 0.15f * k) * (0.8f + 0.2f * flicker_) * s;
            q.colorStart = q.colorMid = q.colorEnd = {0.9f, 0.97f, 1.0f, a * 0.85f * (1.0f - 0.18f * k)};
            q.lifeTime = oneFrame;
            glow_->Spawn(q);
        }
    }
}

// =============================================================================
//  ダッシュ開始時のバースト
// =============================================================================
void PlayerBooster::EmitBurst(size_t index) {
    const Vector3 mouth = mouthPos_[index];
    const Vector3 dir = exhaustDir_[index];
    const float radius = nozzleScale_.x;
    const float s = effectScale_;
    const Vector3 baseVel = ownerVelocity_ * inheritVelocity_;
    const float jet = jetSpeedMax_;
    Vector3 u, w;
    MakeBasis(dir, u, w);

    // 閃光
    {
        BoosterParticle q;
        q.position = mouth;
        q.velocity = baseVel;
        q.sizeStart = radius * 7.0f * s;
        q.sizeEnd = radius * 1.0f * s;
        q.colorStart = {1.0f, 1.0f, 1.0f, 0.95f};
        q.colorMid = WithAlpha(coreColor_, 0.6f);
        q.colorEnd = WithAlpha(flameColor_, 0.0f);
        q.midPoint = 0.3f;
        q.lifeTime = 0.14f;
        glow_->Spawn(q);
    }
    // 衝撃波のリング（噴射方向に垂直に広がる）
    constexpr int kRing = 36;
    for (int k = 0; k < kRing; ++k) {
        const float angle = kPi * 2.0f * static_cast<float>(k) / kRing;
        BoosterParticle q;
        q.position = mouth;
        q.velocity = (u * std::cos(angle) + w * std::sin(angle)) * Rand(6.0f, 9.0f) + dir * (jet * 0.3f) + baseVel;
        q.sizeStart = 0.25f * s;
        q.sizeEnd = 0.7f * s;
        q.colorStart = WithAlpha(coreColor_, 0.9f);
        q.colorMid = flameColor_;
        q.colorEnd = flameEndColor_;
        q.drag = 5.0f;
        q.rotation = Rand(0.0f, kPi * 2.0f);
        q.lifeTime = Rand(0.25f, 0.35f);
        plume_->Spawn(q);
    }
    // 火花を一気に撒く
    for (int k = 0; k < 40; ++k) {
        BoosterParticle q;
        q.position = mouth + RandomInDisk(index, radius * 0.4f);
        q.velocity = dir * (jet * Rand(0.3f, 1.0f)) + RandomInDisk(index, jet * 0.6f) + baseVel;
        q.gravity = -12.0f;
        q.drag = 1.0f;
        q.sizeStart = 0.07f * s;
        q.sizeEnd = 0.02f * s;
        q.stretch = 0.03f;
        q.colorStart = {1.0f, 1.0f, 0.9f, 1.0f};
        q.colorMid = sparkColor_;
        q.colorEnd = {sparkColor_.x, sparkColor_.y * 0.35f, 0.05f, 0.0f};
        q.midPoint = 0.25f;
        q.lifeTime = Rand(0.3f, 0.7f);
        sparks_->Spawn(q);
    }
    // 煙の塊
    if (enableSmoke_) {
        for (int k = 0; k < 12; ++k) {
            BoosterParticle q;
            q.position = mouth + dir * Rand(0.2f, 0.8f) + RandomInDisk(index, radius);
            q.velocity = dir * Rand(2.0f, 5.0f) + RandomInDisk(index, 2.5f) + baseVel * 0.3f;
            q.gravity = 1.0f;
            q.drag = 3.0f;
            q.sizeStart = 0.5f * s;
            q.sizeEnd = 2.2f * s;
            q.colorStart = {0.65f, 0.67f, 0.72f, 0.0f};
            q.colorMid = {0.58f, 0.6f, 0.66f, 0.35f};
            q.colorEnd = {0.5f, 0.51f, 0.55f, 0.0f};
            q.midPoint = 0.2f;
            q.rotation = Rand(0.0f, kPi * 2.0f);
            q.rotationSpeed = Rand(-2.0f, 2.0f);
            q.lifeTime = Rand(0.8f, 1.3f);
            smoke_->Spawn(q);
        }
    }
}

// =============================================================================
//  Draw
// =============================================================================
void PlayerBooster::Draw() {
    for (auto& nozzle : nozzles_) {
        if (nozzle) {
            nozzle->Draw();
        }
    }
}

void PlayerBooster::DrawParticles() {
    auto* cmd = TuboEngine::DirectXCommon::GetInstance()->GetCommandList().Get();
    // 色で見せる層（煙・炎・火花）は通常のアルファ合成
    // ※ 加算合成は「背景 + 光」なので、白い床の上では足しても白のままで見えなくなる
    pso_->SetAlphaBlend(cmd);
    smoke_->Draw(cmd);
    plume_->Draw(cmd);
    sparks_->Draw(cmd);

    // 光らせる層（白い芯・噴射口の光・ショックダイヤモンド）は加算合成。暗い背景で強く光る
    pso_->SetAdditive(cmd);
    core_->Draw(cmd);
    glow_->Draw(cmd);

    // 後に描かれる他のパーティクルのため、通常のパーティクル描画設定に戻す
    TuboEngine::ParticleCommon::GetInstance()->DrawSettingsCommon();
}

// =============================================================================
//  補助関数
// =============================================================================
float PlayerBooster::LerpAngle(float from, float to, float t) {
    constexpr float kTwoPi = kPi * 2.0f;
    float diff = std::fmod(to - from, kTwoPi);
    if (diff > kPi) {
        diff -= kTwoPi;
    } else if (diff < -kPi) {
        diff += kTwoPi;
    }
    float result = from + diff * t;
    // 値が際限なく大きくならないよう -π～π に収める
    result = std::remainder(result, kTwoPi);
    return result;
}

float PlayerBooster::Rand(float min, float max) {
    if (min >= max) {
        return min;
    }
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng_);
}

Vector3 PlayerBooster::RandomInDisk(size_t index, float radius) {
    Vector3 u, w;
    MakeBasis(exhaustDir_[index], u, w);
    const float angle = Rand(0.0f, kPi * 2.0f);
    const float r = radius * std::sqrt(Rand(0.0f, 1.0f));
    return u * (std::cos(angle) * r) + w * (std::sin(angle) * r);
}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void PlayerBooster::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("Booster Nozzle", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* state = isDashing_ ? "DASH" : (isMoving_ ? "MOVING" : "IDLE");
        ImGui::Text("State: %s", state);
        ImGui::Text("Output  : %.0f%%", output_ * 100.0f);
        ImGui::Text("Throttle: %.2f", throttle_);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.35f, 0.7f, 1.0f, 1.0f));
        ImGui::ProgressBar(std::clamp(power_ / dashThrottle_, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f), "Power");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.45f, 0.25f, 1.0f));
        ImGui::ProgressBar(heat_, ImVec2(-1.0f, 0.0f), "Heat");
        ImGui::PopStyleColor();
        ImGui::Text("Particles: smoke %zu / plume %zu / sparks %zu / core %zu / glow %zu",
            smoke_->GetCount(), plume_->GetCount(), sparks_->GetCount(), core_->GetCount(), glow_->GetCount());

        // 出力の上書き（エネルギー配分を変えずに見た目を確認する用）
        bool overrideOutput = outputOverride_ >= 0.0f;
        if (ImGui::Checkbox("Override Output", &overrideOutput)) {
            outputOverride_ = overrideOutput ? output_ : -1.0f;
        }
        if (overrideOutput) {
            ImGui::SliderFloat("Output", &outputOverride_, 0.0f, 1.0f);
        }

        if (ImGui::TreeNode("Throttle")) {
            ImGui::SliderFloat("Idle", &idleThrottle_, 0.0f, 1.0f);
            ImGui::SliderFloat("Move", &moveThrottle_, 0.0f, 1.5f);
            ImGui::SliderFloat("Dash", &dashThrottle_, 0.0f, 1.5f);
            ImGui::SliderFloat("Min Output Gain", &minOutputGain_, 0.0f, 1.0f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Exhaust")) {
            ImGui::DragFloat("Jet Speed Min", &jetSpeedMin_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Jet Speed Max", &jetSpeedMax_, 0.1f, 0.0f, 80.0f);
            ImGui::SliderFloat("Inherit Velocity", &inheritVelocity_, 0.0f, 1.0f);
            ImGui::SliderFloat("Effect Scale", &effectScale_, 0.1f, 3.0f);
            ImGui::DragFloat("Core Rate", &coreRate_, 1.0f, 0.0f, 1000.0f);
            ImGui::DragFloat("Plume Rate", &plumeRate_, 1.0f, 0.0f, 1000.0f);
            ImGui::DragFloat("Spark Rate", &sparkRate_, 1.0f, 0.0f, 500.0f);
            ImGui::DragFloat("Smoke Rate", &smokeRate_, 1.0f, 0.0f, 200.0f);
            ImGui::Checkbox("Smoke", &enableSmoke_);
            ImGui::SameLine();
            ImGui::Checkbox("Sparks", &enableSparks_);
            ImGui::SameLine();
            ImGui::Checkbox("Shock Diamonds", &enableDiamonds_);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Colors")) {
            ImGui::ColorEdit4("Core", &coreColor_.x);
            ImGui::ColorEdit4("Flame", &flameColor_.x);
            ImGui::ColorEdit4("Flame End", &flameEndColor_.x);
            ImGui::ColorEdit4("Spark", &sparkColor_.x);
            ImGui::ColorEdit4("Heat", &heatColor_.x);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Nozzle")) {
            ImGui::Text("Yaw : %.1f deg", yaw_ * 180.0f / kPi);
            ImGui::Text("Tilt: %.1f deg", tilt_ * 180.0f / kPi);
            ImGui::DragFloat3("Right Offset", &mountOffsets_[0].x, 0.01f);
            ImGui::DragFloat3("Left Offset", &mountOffsets_[1].x, 0.01f);
            ImGui::DragFloat3("Nozzle Scale", &nozzleScale_.x, 0.01f, 0.01f, 5.0f);
            ImGui::SliderAngle("Idle Tilt", &idleTilt_, -90.0f, 180.0f);
            ImGui::SliderAngle("Move Tilt", &moveTilt_, -90.0f, 180.0f);
            ImGui::SliderAngle("Boost Tilt Bonus", &boostTiltBonus_, 0.0f, 90.0f);
            ImGui::SliderFloat("Yaw Follow", &yawFollow_, 0.01f, 1.0f);
            ImGui::SliderFloat("Tilt Follow", &tiltFollow_, 0.01f, 1.0f);
            ImGui::TreePop();
        }
    }
#endif
}
