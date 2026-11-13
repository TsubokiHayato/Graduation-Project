#pragma once
#include "Object3d.h"
#include "BoosterParticleEmitter.h"
#include "BoosterParticlePSO.h"
#include <array>
#include <memory>
#include <random>

// プレイヤーの背中に付くバーニアノズル（スラスター）クラス
// ・プレイヤー本体の位置の変化から移動方向を求め、ノズルを回転させる
// ・ブースター出力と移動状態（停止 / 移動 / ダッシュ）に応じて噴射の強さを変える
// ※ Player 本体のクラスには処理を書かず、ここで完結させる
class PlayerBooster {
public:
    // 初期化処理（追従するプレイヤー本体とカメラを渡す）
    void Initialize(const TuboEngine::Object3d* owner, TuboEngine::Camera* camera);
    // 更新処理
    // booster   : ブースター出力率 0.0～1.0（エネルギー配分）
    // isDashing : ダッシュ中か（ダッシュ開始の瞬間にバーストを出す）
    void Update(float booster, bool isDashing = false);
    // ノズル（モデル）の描画。Object3DDraw で呼ぶ
    void Draw();
    // 噴射パーティクルの描画。ParticleDraw で呼ぶ
    void DrawParticles();
    // ImGui
    void DrawImGui();

private:
    // 角度 from から to へ最短経路で t だけ補間する
    static float LerpAngle(float from, float to, float t);
    // ノズル1本分の Transform を計算して反映する
    void UpdateNozzle(size_t index);
    // ノズル1本分の噴射パーティクルを出す
    void EmitExhaust(size_t index, float dt);
    // ダッシュ開始時のバースト
    void EmitBurst(size_t index);
    // 一様乱数
    float Rand(float min, float max);
    // 円盤内のランダムな点（dir に垂直な面）
    TuboEngine::Math::Vector3 RandomInDisk(size_t index, float radius);

private:
    // ノズルの数（左右2基）
    static constexpr size_t kNozzleCount = 2;
    // cone.obj の底面（原点）から先端までの長さ
    static constexpr float kModelTipLength = 1.93f;

    // 追従するプレイヤー本体（所有はしない）
    const TuboEngine::Object3d* owner_ = nullptr;
    TuboEngine::Camera* camera_ = nullptr;
    // ノズルのモデル
    std::array<std::unique_ptr<TuboEngine::Object3d>, kNozzleCount> nozzles_;

    // =========================================
    // 噴射パーティクル（描画順 = 配列順。奥にあるものから描く）
    // =========================================
    std::unique_ptr<BoosterParticleEmitter> smoke_;   // 排煙
    std::unique_ptr<BoosterParticleEmitter> plume_;   // 炎の広がり
    std::unique_ptr<BoosterParticleEmitter> sparks_;  // 火花
    std::unique_ptr<BoosterParticleEmitter> core_;    // 高温の芯（速度方向に伸びる）
    std::unique_ptr<BoosterParticleEmitter> glow_;    // 噴射口の発光・ジェット・ショックダイヤモンド
    // 描画用PSO（煙・炎・火花はアルファ合成、芯・発光は加算合成。どちらも描画先のαを変えない）
    std::unique_ptr<BoosterParticlePSO> pso_;

    // 前フレームのプレイヤー位置（移動方向の算出用）
    TuboEngine::Math::Vector3 prevOwnerPos_{};
    // プレイヤーの速度（噴射に慣性を乗せる）
    TuboEngine::Math::Vector3 ownerVelocity_{};

    // ノズルごとの噴射口の位置と噴射方向
    std::array<TuboEngine::Math::Vector3, kNozzleCount> mouthPos_{};
    std::array<TuboEngine::Math::Vector3, kNozzleCount> exhaustDir_{};
    // ノズルごとの粒子の放出の端数
    std::array<std::array<float, 4>, kNozzleCount> emitAccum_{};

    std::mt19937 rng_{std::random_device{}()};

    // =========================================
    // 現在の状態
    // =========================================
    float yaw_ = 0.0f;        // ユニット全体の向き（移動方向, Y軸回転）
    float tilt_ = 0.0f;       // ノズルの傾き（X軸回転）
    bool isMoving_ = false;   // 移動中か
    bool isDashing_ = false;  // ダッシュ中か
    float output_ = 0.0f;     // ブースター出力率（0～1）
    float throttle_ = 0.0f;   // スロットル（状態から決まる。なめらかに変化）
    float power_ = 0.0f;      // 実際の噴射の強さ = スロットル × 出力
    float heat_ = 0.0f;       // ノズルの加熱具合（0～1）
    float flicker_ = 1.0f;    // 炎のゆらぎ
    float time_ = 0.0f;       // 経過時間

    // =========================================
    // 調整用パラメータ（ImGuiで編集可能）
    // =========================================
    // ノズル先端を付ける位置（+Zが前方。背中側＝-Z）
    std::array<TuboEngine::Math::Vector3, kNozzleCount> mountOffsets_ = {
        TuboEngine::Math::Vector3{ 0.6f, 0.4f, -0.8f },  // 右
        TuboEngine::Math::Vector3{-0.6f, 0.4f, -0.8f },  // 左
    };
    TuboEngine::Math::Vector3 nozzleScale_{ 0.3f, 0.45f, 0.3f };
    float idleTilt_ = 0.35f;        // 停止中の傾き(rad)：噴射口がやや下向き
    float moveTilt_ = 1.25f;        // 移動中の傾き(rad)：噴射口が後方を向く
    float boostTiltBonus_ = 0.25f;  // ブースター出力最大時に追加で倒す量(rad)
    float yawFollow_ = 0.2f;        // 移動方向への追従速度(0～1)
    float tiltFollow_ = 0.15f;      // 傾きの追従速度(0～1)
    float moveThreshold_ = 0.0001f; // これ以上動いたら「移動中」とみなす距離

    // スロットル（状態ごとの目標値）
    float idleThrottle_ = 0.18f;    // 停止中（アイドリング）
    float moveThrottle_ = 0.6f;     // 移動中
    float dashThrottle_ = 1.3f;     // ダッシュ中（オーバーブースト）
    float minOutputGain_ = 0.15f;   // 出力0%でも残る噴射の割合
    float outputOverride_ = -1.0f;  // 0以上なら出力をこの値で上書き（ImGuiでの確認用）

    // 噴射の見た目
    float jetSpeedMin_ = 3.0f;      // 噴射速度（出力最小）
    float jetSpeedMax_ = 22.0f;     // 噴射速度（出力最大）
    float inheritVelocity_ = 0.6f;  // プレイヤーの速度を粒子に乗せる割合
    float coreRate_ = 260.0f;       // 芯の放出数（/秒・ノズル1基・出力最大時）
    float plumeRate_ = 200.0f;      // 炎の放出数
    float sparkRate_ = 90.0f;       // 火花の放出数
    float smokeRate_ = 30.0f;       // 煙の放出数
    float effectScale_ = 1.0f;      // パーティクル全体の大きさ倍率
    bool enableSmoke_ = true;
    bool enableSparks_ = true;
    bool enableDiamonds_ = true;    // ショックダイヤモンド

    // 色
    TuboEngine::Math::Vector4 coreColor_{0.5f, 0.88f, 1.0f, 1.0f};   // 芯（高温）
    TuboEngine::Math::Vector4 flameColor_{0.15f, 0.4f, 1.0f, 0.7f};  // 炎
    TuboEngine::Math::Vector4 flameEndColor_{0.45f, 0.15f, 0.9f, 0.0f};// 炎の消え際
    TuboEngine::Math::Vector4 sparkColor_{1.0f, 0.6f, 0.2f, 1.0f};   // 火花
    TuboEngine::Math::Vector4 heatColor_{1.0f, 0.45f, 0.25f, 1.0f};  // 加熱したノズルの色

public:
    // 取り付け位置の設定（index 0:右 1:左、+Zが前方）
    void SetMountOffset(size_t index, const TuboEngine::Math::Vector3& offset) {
        if (index < kNozzleCount) { mountOffsets_[index] = offset; }
    }
    // ノズルの大きさの設定
    void SetNozzleScale(const TuboEngine::Math::Vector3& scale) { nozzleScale_ = scale; }
    // 地面の高さの設定（噴射が地面に当たると地面に沿って広がる）
    void SetGroundHeight(float height) {
        for (auto* e : {smoke_.get(), plume_.get(), sparks_.get(), core_.get(), glow_.get()}) {
            if (e) { e->SetGround(true, height); }
        }
    }
    // 現在の向き(Y軸回転)取得
    float GetYaw() const { return yaw_; }
    // 移動中か取得
    bool IsMoving() const { return isMoving_; }
    // 現在の噴射の強さ取得（0～1.3程度）
    float GetPower() const { return power_; }
};
