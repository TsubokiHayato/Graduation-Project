#include "BoosterParticleEmitter.h"
#include "Camera.h"
#include "Matrix.h"
#include <algorithm>
#include <cmath>

using namespace TuboEngine;
using TuboEngine::Math::Matrix4x4;
using TuboEngine::Math::Vector3;
using TuboEngine::Math::Vector4;

namespace {

Vector3 Cross(const Vector3& a, const Vector3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float Length(const Vector3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
Vector3 Scale(const Vector3& v, float s) { return {v.x * s, v.y * s, v.z * s}; }

Vector4 LerpColor(const Vector4& a, const Vector4& b, float t) {
    return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t)};
}

// 行ベクトル形式のワールド行列を軸ベクトルから直接作る
Matrix4x4 MakeWorld(const Vector3& right, const Vector3& up, const Vector3& forward, const Vector3& pos) {
    Matrix4x4 m{};
    m.m[0][0] = right.x;   m.m[0][1] = right.y;   m.m[0][2] = right.z;   m.m[0][3] = 0.0f;
    m.m[1][0] = up.x;      m.m[1][1] = up.y;      m.m[1][2] = up.z;      m.m[1][3] = 0.0f;
    m.m[2][0] = forward.x; m.m[2][1] = forward.y; m.m[2][2] = forward.z; m.m[2][3] = 0.0f;
    m.m[3][0] = pos.x;     m.m[3][1] = pos.y;     m.m[3][2] = pos.z;     m.m[3][3] = 1.0f;
    return m;
}

} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void BoosterParticleEmitter::Initialize(const std::string& name, const std::string& texture, uint32_t maxInstances) {
    ParticlePreset preset{};
    preset.name = name;
    preset.texture = texture;
    preset.maxInstances = maxInstances;
    preset.billboard = true;
    preset.autoEmit = false;
    IParticleEmitter::Initialize(preset);
    parts_.reserve(maxInstances);
}

void BoosterParticleEmitter::BuildGeometry(std::vector<VertexData>& out) {
    // 中心原点・一辺1の四角形（幅・長さは行列の軸の長さで決める）
    out.clear();
    VertexData v0{}, v1{}, v2{}, v3{};
    v0.position = {-0.5f,  0.5f, 0.0f, 1.0f}; v0.texcoord = {0.0f, 0.0f}; v0.normal = {0, 0, -1};
    v1.position = { 0.5f,  0.5f, 0.0f, 1.0f}; v1.texcoord = {1.0f, 0.0f}; v1.normal = {0, 0, -1};
    v2.position = {-0.5f, -0.5f, 0.0f, 1.0f}; v2.texcoord = {0.0f, 1.0f}; v2.normal = {0, 0, -1};
    v3.position = { 0.5f, -0.5f, 0.0f, 1.0f}; v3.texcoord = {1.0f, 1.0f}; v3.normal = {0, 0, -1};
    out = {v0, v1, v2, v2, v1, v3};
}

// =============================================================================
//  Spawn
// =============================================================================
void BoosterParticleEmitter::Spawn(const BoosterParticle& particle) {
    if (parts_.size() >= allocatedInstances_ || particle.lifeTime <= 0.0f) {
        return;
    }
    parts_.push_back(particle);
}

// =============================================================================
//  Update
// =============================================================================
void BoosterParticleEmitter::Update(float dt, const Camera* camera) {
    // 寿命切れを消しつつ移動させる
    if (dt > 0.0f) {
        for (auto& p : parts_) {
            p.currentTime += dt;
            p.velocity.y += p.gravity * dt;
            if (p.drag > 0.0f) {
                float d = std::clamp(1.0f - p.drag * dt, 0.0f, 1.0f);
                p.velocity = Scale(p.velocity, d);
            }
            p.position = p.position + Scale(p.velocity, dt);
            p.rotation += p.rotationSpeed * dt;

            // 地面に当たったら、下向きの勢いを横に逃がして地面に沿って広げる
            if (useGround_ && p.position.y < groundHeight_) {
                p.position.y = groundHeight_;
                if (p.velocity.y < 0.0f) {
                    const float hit = -p.velocity.y;
                    const float horizontal = std::sqrt(p.velocity.x * p.velocity.x + p.velocity.z * p.velocity.z);
                    if (horizontal > 0.0001f) {
                        const float spread = 1.0f + hit * 0.6f / horizontal;
                        p.velocity.x *= spread;
                        p.velocity.z *= spread;
                    }
                    p.velocity.y = hit * 0.1f;
                }
            }
        }
        parts_.erase(std::remove_if(parts_.begin(), parts_.end(),
            [](const BoosterParticle& p) { return p.currentTime >= p.lifeTime; }), parts_.end());
    }

    instanceCount_ = 0;
    if (!camera || !instancingPtr_) {
        return;
    }

    // カメラの軸（行ベクトル形式なので行がそのまま右・上・前）
    const Matrix4x4& camWorld = camera->GetWorldMatrix();
    const Vector3 camRight{camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2]};
    const Vector3 camUp{camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2]};
    const Vector3 camForward{camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2]};
    const Vector3 camPos{camWorld.m[3][0], camWorld.m[3][1], camWorld.m[3][2]};
    const Matrix4x4& viewProj = camera->GetViewProjectionMatrix();

    uint32_t i = 0;
    for (const auto& p : parts_) {
        if (i >= allocatedInstances_) {
            break;
        }
        const float t = std::clamp(p.currentTime / p.lifeTime, 0.0f, 1.0f);

        // 大きさは早めに変化させる（OutQuad）
        const float sizeT = 1.0f - (1.0f - t) * (1.0f - t);
        const float width = std::max(0.0f, std::lerp(p.sizeStart, p.sizeEnd, sizeT));

        // 色は 開始→中間→終了 の2区間で補間
        Vector4 color;
        if (t < p.midPoint) {
            color = LerpColor(p.colorStart, p.colorMid, t / std::max(p.midPoint, 0.0001f));
        } else {
            color = LerpColor(p.colorMid, p.colorEnd, (t - p.midPoint) / std::max(1.0f - p.midPoint, 0.0001f));
        }

        // 伸ばす方向と長さを決める
        Vector3 dir{};
        float length = 0.0f;
        if (p.axisLength > 0.0f) {
            float len = Length(p.axis);
            if (len > 0.0001f) { dir = Scale(p.axis, 1.0f / len); length = p.axisLength; }
        } else if (p.stretch > 0.0f) {
            float speed = Length(p.velocity);
            if (speed > 0.0001f) { dir = Scale(p.velocity, 1.0f / speed); length = width + speed * p.stretch; }
        }

        // 大きな板が地面に刺さると、そこで絵がスパッと切れる。板の下端が地面に届かない程度に持ち上げる
        Vector3 drawPos = p.position;
        if (useGround_ && length <= 0.0f) {
            drawPos.y = std::max(drawPos.y, groundHeight_ + width * 0.35f);
        }

        Matrix4x4 world;
        Vector3 toCam = camPos - drawPos;
        Vector3 right = Cross(dir, toCam);
        float rightLen = Length(right);
        if (length > 0.0f && rightLen > 0.0001f) {
            // 速度（軸）方向に伸びた板。カメラの方を向くよう軸回りに回す
            right = Scale(right, width / rightLen);
            Vector3 forward = Cross(right, dir);
            float fLen = Length(forward);
            forward = fLen > 0.0001f ? Scale(forward, 1.0f / fLen) : camForward;
            world = MakeWorld(right, Scale(dir, length), forward, drawPos);
        } else {
            // 通常のビルボード（Z回転あり）
            const float c = std::cos(p.rotation);
            const float s = std::sin(p.rotation);
            Vector3 r = camRight * c + camUp * s;
            Vector3 u = camUp * c - camRight * s;
            world = MakeWorld(Scale(r, width), Scale(u, width), camForward, drawPos);
        }

        instancingPtr_[i].World = world;
        instancingPtr_[i].WVP = Math::Multiply(world, viewProj);
        instancingPtr_[i].color = color;
        ++i;
    }
    instanceCount_ = i;
}
