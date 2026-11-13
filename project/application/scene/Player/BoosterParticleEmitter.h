#pragma once
#include "IParticleEmitter.h"
#include <string>
#include <vector>

// ブースター噴射用のパーティクル1個分
// エンジン標準の ParticleInfo と違い、粒子ごとに大きさ・色のグラデーション・伸び方を持てる
struct BoosterParticle {
    TuboEngine::Math::Vector3 position{};
    TuboEngine::Math::Vector3 velocity{};

    float sizeStart = 0.2f;   // 生成時の大きさ（幅）
    float sizeEnd = 0.0f;     // 消える時の大きさ（幅）

    // 速度方向への伸び（0ならビルボード）。長さ = 幅 + 速さ * stretch
    float stretch = 0.0f;
    // 伸ばす方向を固定する場合に使う（axisLength > 0 で有効。速度は無視して axis 方向に伸ばす）
    TuboEngine::Math::Vector3 axis{};
    float axisLength = 0.0f;

    // 色のグラデーション（開始 → 中間 → 終了）
    TuboEngine::Math::Vector4 colorStart{1, 1, 1, 1};
    TuboEngine::Math::Vector4 colorMid{1, 1, 1, 1};
    TuboEngine::Math::Vector4 colorEnd{1, 1, 1, 0};
    float midPoint = 0.35f;   // 中間色になるタイミング(0～1)

    float gravity = 0.0f;     // Y方向の加速度
    float drag = 0.0f;        // 速度減衰率（/秒）
    float rotation = 0.0f;    // ビルボード時のZ回転
    float rotationSpeed = 0.0f;

    float lifeTime = 0.3f;
    float currentTime = 0.0f;
};

// ブースター噴射専用のエミッター
// ParticleManager には登録せず、PlayerBooster が所有して直接 Update / Draw する
class BoosterParticleEmitter : public IParticleEmitter {
public:
    // 初期化処理
    void Initialize(const std::string& name, const std::string& texture, uint32_t maxInstances);
    // 粒子を1つ追加する（上限に達していたら捨てる）
    void Spawn(const BoosterParticle& particle);
    // 更新処理（移動・色/大きさの変化・GPUバッファへの書き込み）
    void Update(float dt, const TuboEngine::Camera* camera) override;
    // 全粒子を消す
    void Clear() { parts_.clear(); instanceCount_ = 0; }
    // 地面との当たりを設定する
    // 地面より下に行った粒子は地面に沿って広がり、板の下半分が地面で切れないよう持ち上げて描く
    void SetGround(bool enable, float height) { useGround_ = enable; groundHeight_ = height; }
    // 現在の粒子数
    size_t GetCount() const { return parts_.size(); }

protected:
    // 生成は Spawn で行うので未使用
    ParticleInfo GenerateParticle() override { return {}; }
    // 1x1 の四角形
    void BuildGeometry(std::vector<TuboEngine::VertexData>& out) override;

private:
    std::vector<BoosterParticle> parts_;
    bool useGround_ = false;
    float groundHeight_ = 0.0f;
};
