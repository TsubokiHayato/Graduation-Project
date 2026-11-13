#include "BossWarningRenderer.h"
#include <cmath>
#include <type_traits>

namespace BossHit {

namespace {
constexpr const char* kTexZone = "warning_zone.png"; // 赤・低アルファ
constexpr const char* kTexFill = "warning_fill.png"; // 白・不透明（色は Model の色で付ける）
// 塗りの色（予兆中）と判定時のアルファ
constexpr float kTintR = 0.90f, kTintG = 0.12f, kTintB = 0.12f, kTintA = 0.59f;
constexpr float kHitA = 0.90f;
// 地面（y=0）とのZファイトを避けるための浮かせ量
constexpr float kZoneY = 0.02f;
constexpr float kFillY = 0.04f;
constexpr float kPi = 3.14159265f;

float EaseOutCubic(float t) {
	float u = 1.0f - t;
	return 1.0f - u * u * u;
}

// "warning/xxx.obj" → "warning/xxx_fill.obj"
std::string FillPath(const std::string& path) { return path.substr(0, path.rfind(".obj")) + "_fill.obj"; }
} // namespace

float WarningRenderer::FanPresetRad(int index) {
	index = index < 0 ? 0 : (index >= kFanPresetCount ? kFanPresetCount - 1 : index);
	return static_cast<float>(kFanPresets[index]) * kPi / 180.0f;
}

void WarningRenderer::Initialize(TuboEngine::Camera* camera) {
	MakeLayer(circle_, "warning/warning_circle.obj", camera);
	MakeLayer(beam_, "warning/warning_beam.obj", camera);
	for (int deg : kFanPresets) {
		fans_.emplace_back(deg, Layer{});
		MakeLayer(fans_.back().second, "warning/warning_fan_" + std::to_string(deg) + ".obj", camera);
	}
}

void WarningRenderer::Update(const Warning& w) {
	active_ = nullptr;
	drawFill_ = false;
	missingFanPreset_ = false;
	if (!w.IsActive()) return;

	const float p = w.GetProgress();
	drawFill_ = p > 0.001f; // 縮尺 0 は描かない

	// 塗りの色：予兆中は赤(半透明)。判定フェーズでは白(ほぼ不透明)へイージング
	const float e = w.IsHitActive() ? EaseOutCubic(w.GetHitProgress()) : 0.0f;
	const TuboEngine::Math::Vector4 fillColor = {
	    kTintR + (1.0f - kTintR) * e,
	    kTintG + (1.0f - kTintG) * e,
	    kTintB + (1.0f - kTintB) * e,
	    kTintA + (kHitA - kTintA) * e,
	};

	std::visit(
	    [&](const auto& s) {
		    using T = std::decay_t<decltype(s)>;
		    if constexpr (std::is_same_v<T, Circle>) {
			    active_ = &circle_;
			    Place(*active_, s.center, 0.0f, s.radius, s.radius, s.radius * p, s.radius * p, fillColor);
		    } else if constexpr (std::is_same_v<T, Beam>) {
			    active_ = &beam_;
			    float yaw = std::atan2(s.dir.x, s.dir.z);
			    // 幅は固定、長さだけ根元から伸びる
			    Place(*active_, s.origin, yaw, s.width, s.length, s.width, s.length * p, fillColor);
		    } else {
			    int deg = static_cast<int>(std::lround(s.angle * 180.0f / kPi));
			    active_ = FindFan(deg);
			    if (!active_) {
				    missingFanPreset_ = true; // プリセットに無い角度
				    return;
			    }
			    float yaw = std::atan2(s.forward.x, s.forward.z);
			    Place(*active_, s.origin, yaw, s.radius, s.radius, s.radius * p, s.radius * p, fillColor);
		    }
	    },
	    w.GetShape());
}

void WarningRenderer::Draw() {
	if (!active_) return;
	active_->zone->Draw();
	if (drawFill_) active_->fill->Draw();
}

void WarningRenderer::MakeLayer(Layer& l, const std::string& path, TuboEngine::Camera* camera) {
	auto make = [&](const std::string& modelPath, const char* tex) {
		auto o = std::make_unique<TuboEngine::Object3d>();
		o->Initialize(modelPath);
		o->SetCamera(camera);
		o->SetTexture(tex);
		return o;
	};
	l.zone = make(path, kTexZone);
	l.fill = make(FillPath(path), kTexFill);
}

WarningRenderer::Layer* WarningRenderer::FindFan(int deg) {
	for (auto& f : fans_) {
		if (f.first == deg) return &f.second;
	}
	return nullptr;
}

void WarningRenderer::Place(Layer& l, const Vec3& pos, float yaw, float zoneX, float zoneZ, float fillX, float fillZ, const TuboEngine::Math::Vector4& fillColor) {
	l.zone->SetPosition({pos.x, kZoneY, pos.z});
	l.zone->SetRotation({0.0f, yaw, 0.0f});
	l.zone->SetScale({zoneX, 1.0f, zoneZ});
	l.zone->Update();

	l.fill->SetPosition({pos.x, kFillY, pos.z});
	l.fill->SetRotation({0.0f, yaw, 0.0f});
	l.fill->SetScale({fillX, 1.0f, fillZ});
	l.fill->SetModelColor(fillColor);
	l.fill->Update();
}

} // namespace BossHit
