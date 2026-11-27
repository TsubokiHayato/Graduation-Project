#include "Reticle.h"
#include "WinApp.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using TuboEngine::Math::Vector2;
using TuboEngine::Math::Vector4;

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kDegToRad = kPi / 180.0f;
constexpr float kStartAngle = -kPi * 0.5f; // 真上から時計回りに並べる
constexpr float kMinVisiblePercent = 0.5f; // これ未満の部位は描かない
const char* const kTexture = "white.png";
const char* const kPartNames[] = {"近接 (黄)", "遠距離 (緑)", "シールド (青)", "ブースター (赤)"};
} // namespace

// =============================================================================
//  初期化・出力の設定
// =============================================================================
void Reticle::Initialize() {
	pool_.clear();
	used_ = 0;
	displayed_ = targets_;
	drawCenter_ = center_;
	lockOnBlend_ = 0.0f;
}

void Reticle::SetOutputs(const std::array<float, kPartCount>& outputs) {
	for (int i = 0; i < kPartCount; ++i) {
		targets_[i] = (std::max)(outputs[i], 0.0f);
	}
}

void Reticle::SetOutput(Part part, float percent) {
	targets_[static_cast<int>(part)] = (std::max)(percent, 0.0f);
}

// =============================================================================
//  Update：毎フレーム、スプライトを先頭から使い直して配置する
// =============================================================================
void Reticle::Update(float deltaTime) {
	// 表示中の配分を目標へ近づける（切り替え時に円弧が伸び縮みして見える）
	float t = (std::min)(followSpeed_ * deltaTime, 1.0f);
	for (int i = 0; i < kPartCount; ++i) {
		displayed_[i] += (targets_[i] - displayed_[i]) * t;
	}

	// ロックオン中は敵の位置へ、解除したら中央へなめらかに動かす
	Vector2 goal = isLockedOn_ ? lockOnScreen_ : center_;
	float move = (std::min)(moveSpeed_ * deltaTime, 1.0f);
	drawCenter_.x += (goal.x - drawCenter_.x) * move;
	drawCenter_.y += (goal.y - drawCenter_.y) * move;
	float lockStep = lockOnSpeed_ * deltaTime;
	lockOnBlend_ = isLockedOn_ ? (std::min)(lockOnBlend_ + lockStep, 1.0f) : (std::max)(lockOnBlend_ - lockStep, 0.0f);

	bars_.clear();

	// 内側の円
	PlaceArc(innerRadius_, 0.0f, 2.0f * kPi, innerThickness_, baseColor_);

	// 中心の「＋」
	PlaceBar(drawCenter_, 0.0f, crossSize_, crossThickness_, baseColor_);
	PlaceBar(drawCenter_, kPi * 0.5f, crossSize_, crossThickness_, baseColor_);

	// 100% 分の土台の輪（割り振っていないエネルギーはこの色のまま見える）
	PlaceArc(gaugeRadius_, 0.0f, 2.0f * kPi, gaugeThickness_, trackColor_);

	// 出力ゲージ。一周 = 100%（合計が100%を超えたときは合計で割って一周に収める）
	float total = 0.0f;
	int visibleCount = 0;
	for (float v : displayed_) {
		total += v;
		if (v >= kMinVisiblePercent) { ++visibleCount; }
	}
	if (total > 0.0f && visibleCount > 0) {
		float gap = gapDegrees_ * kDegToRad;
		float available = (std::max)(2.0f * kPi - gap * visibleCount, 0.0f);
		float angle = kStartAngle + gap * 0.5f;
		for (int i = 0; i < kPartCount; ++i) {
			if (displayed_[i] < kMinVisiblePercent) { continue; }
			float sweep = available * (displayed_[i] / (std::max)(total, 100.0f));
			PlaceArc(gaugeRadius_, angle, sweep, gaugeThickness_, colors_[i]);
			PlaceArrowHead(gaugeRadius_, angle + sweep, colors_[i]);
			angle += sweep + gap;
		}
	}

	// ロックオンの角かっこ
	if (lockOnBlend_ > 0.0f) {
		PlaceLockOnBrackets();
	}

	// 縁取りを先に全部置き、その上に本体を置く（縁取りが隣の本体に重ならない）
	used_ = 0;
	if (outlineWidth_ > 0.0f) {
		for (const Bar& bar : bars_) {
			Vector4 color = outlineColor_;
			color.w *= bar.color.w;
			AcquireSprite(bar.mid, bar.angle, {bar.length + outlineWidth_ * 2.0f, bar.thickness + outlineWidth_ * 2.0f}, color);
		}
	}
	for (const Bar& bar : bars_) {
		AcquireSprite(bar.mid, bar.angle, {bar.length, bar.thickness}, bar.color);
	}

	for (size_t i = 0; i < used_; ++i) {
		pool_[i]->Update();
	}
}

void Reticle::Draw() {
	for (size_t i = 0; i < used_; ++i) {
		pool_[i]->Draw();
	}
}

// =============================================================================
//  配置の下請け
// =============================================================================
Vector2 Reticle::PointOnCircle(float radius, float angle) const {
	return {drawCenter_.x + std::cos(angle) * radius, drawCenter_.y + std::sin(angle) * radius};
}

void Reticle::PlaceBar(Vector2 mid, float angle, float length, float thickness, const Vector4& color) {
	bars_.push_back({mid, angle, length, thickness, color});
}

void Reticle::AcquireSprite(Vector2 mid, float angle, Vector2 size, const Vector4& color) {
	if (used_ >= pool_.size()) {
		auto sprite = std::make_unique<TuboEngine::Sprite>();
		sprite->Initialize(kTexture);
		sprite->SetGetIsAdjustTextureSize(false);
		sprite->SetAnchorPoint({0.5f, 0.5f});
		pool_.push_back(std::move(sprite));
	}
	TuboEngine::Sprite* sprite = pool_[used_++].get();
	sprite->SetPosition(mid);
	sprite->SetRotation(angle);
	sprite->SetSize(size);
	sprite->SetColor(color);
}

void Reticle::PlaceArc(float radius, float startAngle, float sweep, float thickness, const Vector4& color) {
	if (sweep <= 0.0f) { return; }
	int count = (std::max)(1, static_cast<int>(std::ceil(sweep / (segmentDegrees_ * kDegToRad))));
	float step = sweep / count;
	// 隣のバーとの間にすき間が見えないよう少しだけ長くする
	float length = 2.0f * radius * std::sin(step * 0.5f) + 1.0f;
	for (int i = 0; i < count; ++i) {
		float a = startAngle + step * (i + 0.5f);
		// 円周の接線方向に向ける
		PlaceBar(PointOnCircle(radius, a), a + kPi * 0.5f, length, thickness, color);
	}
}

void Reticle::PlaceArrowHead(float radius, float endAngle, const Vector4& color) {
	Vector2 tip = PointOnCircle(radius, endAngle);
	float forward = endAngle + kPi * 0.5f; // 時計回りの進行方向
	constexpr float kSpread = 40.0f * kDegToRad;
	for (float side : {-1.0f, 1.0f}) {
		float back = forward + kPi + side * kSpread;
		Vector2 mid = {tip.x + std::cos(back) * arrowSize_ * 0.5f, tip.y + std::sin(back) * arrowSize_ * 0.5f};
		PlaceBar(mid, back, arrowSize_, gaugeThickness_, color);
	}
}

void Reticle::PlaceLockOnBrackets() {
	// 外側から閉じてくる演出（OutCubic）。解除時は逆に開きながら消える
	float ease = 1.0f - std::pow(1.0f - lockOnBlend_, 3.0f);
	float half = bracketRadius_ * (2.0f - ease) / std::numbers::sqrt2_v<float>;
	Vector4 color = lockOnColor_;
	color.w *= lockOnBlend_;
	for (float sx : {-1.0f, 1.0f}) {
		for (float sy : {-1.0f, 1.0f}) {
			// 角の位置から、横と縦に内向きの辺を伸ばして L 字にする
			Vector2 corner = {drawCenter_.x + sx * half, drawCenter_.y + sy * half};
			float overlap = bracketThickness_ * 0.5f; // 角が欠けないよう少しはみ出させる
			PlaceBar({corner.x - sx * (bracketLength_ * 0.5f - overlap), corner.y}, 0.0f, bracketLength_, bracketThickness_, color);
			PlaceBar({corner.x, corner.y - sy * (bracketLength_ * 0.5f - overlap)}, kPi * 0.5f, bracketLength_, bracketThickness_, color);
		}
	}
}

// =============================================================================
//  ロックオン
// =============================================================================
bool Reticle::WorldToScreen(const TuboEngine::Math::Vector3& world, const TuboEngine::Camera& camera, Vector2& out) {
	const TuboEngine::Math::Matrix4x4& m = camera.GetViewProjectionMatrix();
	float x = world.x * m.m[0][0] + world.y * m.m[1][0] + world.z * m.m[2][0] + m.m[3][0];
	float y = world.x * m.m[0][1] + world.y * m.m[1][1] + world.z * m.m[2][1] + m.m[3][1];
	float w = world.x * m.m[0][3] + world.y * m.m[1][3] + world.z * m.m[2][3] + m.m[3][3];
	bool inFront = w > 1e-4f;
	float invW = 1.0f / (std::max)(std::abs(w), 1e-4f);
	float ndcX = x * invW;
	float ndcY = y * invW;
	if (!inFront) { // カメラの後ろは投影が反転するので向きを戻す
		ndcX = -ndcX;
		ndcY = -ndcY;
	}
	TuboEngine::WinApp* winApp = TuboEngine::WinApp::GetInstance();
	out.x = (ndcX * 0.5f + 0.5f) * winApp->GetClientWidth();
	out.y = (0.5f - ndcY * 0.5f) * winApp->GetClientHeight();
	return inFront;
}

void Reticle::SetLockOn(const TuboEngine::Math::Vector3& targetWorld, const TuboEngine::Camera& camera) {
	isLockedOn_ = true;
	Vector2 screen;
	bool inFront = WorldToScreen(targetWorld, camera, screen);

	TuboEngine::WinApp* winApp = TuboEngine::WinApp::GetInstance();
	float halfW = (std::max)(winApp->GetClientWidth() * 0.5f - screenMargin_, 1.0f);
	float halfH = (std::max)(winApp->GetClientHeight() * 0.5f - screenMargin_, 1.0f);
	float dx = screen.x - center_.x;
	float dy = screen.y - center_.y;
	lockOnOffScreen_ = !inFront || std::abs(dx) > halfW || std::abs(dy) > halfH;
	if (lockOnOffScreen_) {
		// 中央から敵の方向へ伸ばし、画面の端（余白つき）で止める
		if (std::abs(dx) < 1e-3f && std::abs(dy) < 1e-3f) { dy = 1.0f; }
		float scale = (std::min)(std::abs(dx) > 1e-3f ? halfW / std::abs(dx) : 1e9f, std::abs(dy) > 1e-3f ? halfH / std::abs(dy) : 1e9f);
		screen = {center_.x + dx * scale, center_.y + dy * scale};
	}
	lockOnScreen_ = screen;
}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void Reticle::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Reticle");
	ImGui::Text("出力配分（合計 %.0f%%）", targets_[0] + targets_[1] + targets_[2] + targets_[3]);
	for (int i = 0; i < kPartCount; ++i) {
		ImGui::SliderFloat(kPartNames[i], &targets_[i], 0.0f, 100.0f, "%.0f%%");
	}
	if (ImGui::Button("均等 (25%)")) { targets_ = {25.0f, 25.0f, 25.0f, 25.0f}; }
	ImGui::Separator();
	ImGui::DragFloat2("Center", &center_.x, 1.0f);
	ImGui::DragFloat("Inner Radius", &innerRadius_, 0.5f, 1.0f, 300.0f);
	ImGui::DragFloat("Inner Thickness", &innerThickness_, 0.1f, 0.5f, 20.0f);
	ImGui::DragFloat("Cross Size", &crossSize_, 0.5f, 0.0f, 100.0f);
	ImGui::DragFloat("Cross Thickness", &crossThickness_, 0.1f, 0.5f, 20.0f);
	ImGui::DragFloat("Gauge Radius", &gaugeRadius_, 0.5f, 1.0f, 300.0f);
	ImGui::DragFloat("Gauge Thickness", &gaugeThickness_, 0.1f, 0.5f, 30.0f);
	ImGui::DragFloat("Gap (deg)", &gapDegrees_, 0.1f, 0.0f, 45.0f);
	ImGui::DragFloat("Arrow Size", &arrowSize_, 0.1f, 0.0f, 50.0f);
	ImGui::DragFloat("Segment (deg)", &segmentDegrees_, 0.1f, 0.5f, 30.0f);
	ImGui::DragFloat("Follow Speed", &followSpeed_, 0.1f, 0.0f, 60.0f);
	ImGui::ColorEdit4("Base Color", &baseColor_.x);
	ImGui::ColorEdit4("Track Color", &trackColor_.x);
	ImGui::DragFloat("Outline Width", &outlineWidth_, 0.05f, 0.0f, 10.0f);
	ImGui::ColorEdit4("Outline Color", &outlineColor_.x);
	for (int i = 0; i < kPartCount; ++i) {
		ImGui::PushID(i);
		ImGui::ColorEdit4(kPartNames[i], &colors_[i].x);
		ImGui::PopID();
	}
	ImGui::SeparatorText("Lock On");
	ImGui::Text("状態: %s%s", isLockedOn_ ? "ロックオン中" : "解除", lockOnOffScreen_ && isLockedOn_ ? "（画面外）" : "");
	ImGui::DragFloat("Move Speed", &moveSpeed_, 0.1f, 0.0f, 60.0f);
	ImGui::DragFloat("Lock Speed", &lockOnSpeed_, 0.1f, 0.1f, 60.0f);
	ImGui::DragFloat("Bracket Radius", &bracketRadius_, 0.5f, 1.0f, 300.0f);
	ImGui::DragFloat("Bracket Length", &bracketLength_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Bracket Thickness", &bracketThickness_, 0.1f, 0.5f, 20.0f);
	ImGui::DragFloat("Screen Margin", &screenMargin_, 0.5f, 0.0f, 300.0f);
	ImGui::ColorEdit4("Lock On Color", &lockOnColor_.x);
	ImGui::Text("Sprites: %zu / pool %zu", used_, pool_.size());
	ImGui::End();
#endif
}
