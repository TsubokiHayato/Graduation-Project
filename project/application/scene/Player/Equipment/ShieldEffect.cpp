#include "ShieldEffect.h"
#include "DirectXCommon.h"
#include "Matrix.h"
#include "TextureManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <tuple>

using TuboEngine::Math::Matrix4x4;
using TuboEngine::Math::Vector3;
using TuboEngine::Math::Vector4;

namespace {
constexpr const char* kTileModel = "shield/shield_hex_tile.obj";
constexpr const char* kTexture = "shield_hex.png";
constexpr int kFrequency = 5; // 球の分割数（5 → 252 セル）
constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.2831853f;

float Lerp(float a, float b, float t) { return a + (b - a) * t; }
Vector4 Lerp(const Vector4& a, const Vector4& b, float t) {
	return {Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t), Lerp(a.w, b.w, t)};
}
float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vector3 Cross(const Vector3& a, const Vector3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
Vector3 Normalize(const Vector3& v) {
	float l = std::sqrt(Dot(v, v));
	return l > 0.0f ? Vector3{v.x / l, v.y / l, v.z / l} : Vector3{0.0f, 0.0f, 1.0f};
}
} // namespace

// =========================================
// Initialize
// =========================================
void ShieldEffect::Initialize(TuboEngine::Camera* camera) {
	camera_ = camera;
	TuboEngine::TextureManager::GetInstance()->LoadTexture(kTexture);
	BuildCells(kFrequency);

	const uint32_t count = static_cast<uint32_t>(cells_.size());
	renderer_.Initialize(kTileModel, static_cast<int>(count), camera);
	instanceBuffer_ = TuboEngine::DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TuboEngine::InstanceData) * count);
	instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));

	order_.resize(count);
	depth_.resize(count);
	time_ = 0.0f;
	hitTimer_ = 0.0f;
	breakTimer_ = 0.0f;
	drawCount_ = 0;
}

// =========================================
// 球面をセルに分割（正二十面体を分割したジオデシック球の双対）
// =========================================
void ShieldEffect::BuildCells(int frequency) {
	const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
	const Vector3 baseV[12] = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
	                           {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
	const int baseF[20][3] = {{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4},
	                          {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8},
	                          {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};

	// --- ジオデシック球の頂点と三角形 ---
	std::vector<Vector3> verts;
	std::map<std::tuple<int, int, int>, int> index;
	auto vid = [&](const Vector3& p) {
		Vector3 n = Normalize(p);
		auto key = std::make_tuple(static_cast<int>(std::lround(n.x * 1e5f)), static_cast<int>(std::lround(n.y * 1e5f)), static_cast<int>(std::lround(n.z * 1e5f)));
		auto it = index.find(key);
		if (it != index.end()) {
			return it->second;
		}
		int id = static_cast<int>(verts.size());
		index.emplace(key, id);
		verts.push_back(n);
		return id;
	};

	std::vector<std::array<int, 3>> faces;
	const float f = static_cast<float>(frequency);
	for (const auto& bf : baseF) {
		const Vector3 A = Normalize(baseV[bf[0]]), B = Normalize(baseV[bf[1]]), C = Normalize(baseV[bf[2]]);
		std::map<std::pair<int, int>, int> grid;
		for (int i = 0; i <= frequency; ++i) {
			for (int j = 0; j <= frequency - i; ++j) {
				const float k = static_cast<float>(frequency - i - j);
				Vector3 p = A * (k / f) + B * (static_cast<float>(i) / f) + C * (static_cast<float>(j) / f);
				grid[{i, j}] = vid(p);
			}
		}
		for (int i = 0; i < frequency; ++i) {
			for (int j = 0; j < frequency - i; ++j) {
				faces.push_back({grid[{i, j}], grid[{i + 1, j}], grid[{i, j + 1}]});
				if (i + j < frequency - 1) {
					faces.push_back({grid[{i + 1, j}], grid[{i + 1, j + 1}], grid[{i, j + 1}]});
				}
			}
		}
	}

	// --- 各頂点 = 1 セル。角 = 周りの三角形の重心 ---
	std::vector<std::vector<int>> around(verts.size());
	std::vector<Vector3> centers(faces.size());
	for (size_t fi = 0; fi < faces.size(); ++fi) {
		const auto& fc = faces[fi];
		centers[fi] = Normalize(verts[fc[0]] + verts[fc[1]] + verts[fc[2]]);
		for (int v : fc) {
			around[v].push_back(static_cast<int>(fi));
		}
	}

	cells_.clear();
	cells_.reserve(verts.size());
	for (size_t vi = 0; vi < verts.size(); ++vi) {
		Cell c;
		c.normal = verts[vi];
		// セル半径 = 角までの平均距離、タイルの向き = 最初の角の方向
		float sum = 0.0f;
		for (int fi : around[vi]) {
			Vector3 d = centers[fi] - c.normal;
			sum += std::sqrt(Dot(d, d));
		}
		c.size = sum / static_cast<float>(around[vi].size());
		Vector3 toCorner = centers[around[vi][0]] - c.normal;
		c.tangent = Normalize(toCorner - c.normal * Dot(toCorner, c.normal));
		cells_.push_back(c);
	}

	// --- セルごとの個体差 ---
	auto rnd = [this]() {
		random_ = random_ * 1664525u + 1013904223u;
		return static_cast<float>((random_ >> 8) & 0xFFFFFF) / static_cast<float>(0x1000000);
	};
	// 消える順番：0～N-1 を均等に割り振ってシャッフル（残量率 × N 枚がちょうど残る）
	const uint32_t n = static_cast<uint32_t>(cells_.size());
	std::vector<uint32_t> rank(n);
	std::iota(rank.begin(), rank.end(), 0u);
	for (uint32_t i = n; i > 1; --i) {
		std::swap(rank[i - 1], rank[static_cast<uint32_t>(rnd() * static_cast<float>(i)) % i]);
	}
	for (uint32_t i = 0; i < n; ++i) {
		cells_[i].threshold = static_cast<float>(rank[i] + 1) / static_cast<float>(n);
	}
	for (auto& c : cells_) {
		c.phase = rnd() * kTwoPi;
		c.speed = Lerp(0.8f, 1.2f, rnd());
		c.flySpeed = Lerp(0.5f, 1.5f, rnd());
		c.presence = 0.0f;
		c.flyPresence = 0.0f;
	}
}

// =========================================
// Update
// =========================================
void ShieldEffect::Update(float deltaTime, const State& state) {
	time_ += deltaTime;
	hitTimer_ = std::max(0.0f, hitTimer_ - deltaTime);
	breakTimer_ = std::max(0.0f, breakTimer_ - deltaTime);
	renderer_.Update(camera_);

	const float rate = std::clamp(state.shieldRate, 0.0f, 1.0f);
	const float output = std::clamp(state.outputRate, 0.0f, 1.0f);
	const float radius = params_.radius * Lerp(params_.minRadiusRate, 1.0f, output);
	const Vector4 baseColor = RateToColor(rate);
	const Vector3 camPos = camera_ ? camera_->GetTranslate() : Vector3{0.0f, 0.0f, 0.0f};
	const Matrix4x4 viewProj = camera_ ? camera_->GetViewProjectionMatrix() : TuboEngine::Math::MakeIdentity4x4();

	// 走査線：上から下へ等速で通過し、輪郭はくっきり
	const float scanPos = params_.scanPeriod > 0.0f ? 1.2f - std::fmod(time_ / params_.scanPeriod, 1.0f) * 2.4f : 10.0f;
	const float hitProgress = params_.hitDuration > 0.0f && hitTimer_ > 0.0f ? 1.0f - hitTimer_ / params_.hitDuration : -1.0f;
	const float breakProgress = params_.breakDuration > 0.0f && breakTimer_ > 0.0f ? 1.0f - breakTimer_ / params_.breakDuration : -1.0f;
	const Vector4 white{1.0f, 1.0f, 1.0f, 1.0f};

	drawCount_ = 0;
	std::vector<TuboEngine::InstanceData> built; // 描画するセル（あとで奥→手前に並べ替え）
	built.reserve(cells_.size());

	for (uint32_t i = 0; i < static_cast<uint32_t>(cells_.size()); ++i) {
		Cell& c = cells_[i];
		// --- 出入り（残量率でセルの数が決まる。等速で変化） ---
		// 残量率 × N を切り捨てた枚数だけ残る（ごくわずかな値で 1 枚だけ残ることはない）
		const float target = rate >= c.threshold ? 1.0f : 0.0f;
		const float popStep = params_.popSpeed * c.speed * deltaTime;
		c.presence = c.presence < target ? std::min(target, c.presence + popStep) : std::max(target, c.presence - popStep);
		// シールド値 0 のときは残さず即座に消す（破壊の飛び散り演出だけは別）
		if (rate <= 0.0f) {
			c.presence = 0.0f;
		}

		const bool flying = breakProgress >= 0.0f && c.flyPresence > 0.0f;
		if (c.presence <= 0.001f && !flying) {
			continue;
		}

		// --- 被弾：くっきりした衝撃リングが hitDir_ から球面を等速で伝わる ---
		float ripple = 0.0f;
		if (hitProgress >= 0.0f) {
			const float angle = std::acos(std::clamp(Dot(c.normal, hitDir_), -1.0f, 1.0f)) / kPi; // 0～1
			ripple = std::abs(hitProgress * 1.2f - angle) < 0.07f ? 1.0f : 0.0f;
		}

		// --- 待機時：走査線 ---
		const bool scan = std::abs(c.normal.y - scanPos) < params_.scanWidth;

		float dist = radius * (1.0f + params_.hitPush * ripple);
		// 出入り中は大きさを少しだけ変える
		float scale = c.size * params_.tileFill * radius * Lerp(0.7f, 1.0f, c.presence);
		Vector4 color = baseColor;
		if (scan) {
			color = Lerp(color, white, 0.45f);
		}
		if (ripple > 0.0f) {
			color = Lerp(color, white, 0.85f);
		}
		float alpha = params_.alpha * c.presence;
		Vector3 tangent = c.tangent;

		// --- 破壊：等速で一直線に飛び、一定速度で回転・縮小して消える ---
		if (flying) {
			dist = radius * (1.0f + params_.breakDistance * c.flySpeed * breakProgress);
			scale = c.size * params_.tileFill * radius * (1.0f - 0.6f * breakProgress);
			color = Lerp(white, params_.lowColor, breakProgress);
			alpha = params_.alpha * c.flyPresence * (1.0f - breakProgress);
			const float spin = c.flySpeed * breakProgress * 6.0f * (c.phase > kPi ? 1.0f : -1.0f);
			const Vector3 bin = Cross(c.normal, c.tangent);
			tangent = c.tangent * std::cos(spin) + bin * std::sin(spin);
		}
		if (alpha <= 0.002f || scale <= 0.0001f) {
			continue;
		}
		color.w = std::clamp(alpha, 0.0f, 1.0f);

		// --- ワールド行列（行ベクトル：X=角の向き, Y=従法線, Z=法線） ---
		const Vector3 bin = Cross(c.normal, tangent);
		const Vector3 pos = state.position + c.normal * dist;
		Matrix4x4 world = {};
		world.m[0][0] = tangent.x * scale; world.m[0][1] = tangent.y * scale; world.m[0][2] = tangent.z * scale;
		world.m[1][0] = bin.x * scale;     world.m[1][1] = bin.y * scale;     world.m[1][2] = bin.z * scale;
		world.m[2][0] = c.normal.x;        world.m[2][1] = c.normal.y;        world.m[2][2] = c.normal.z;
		world.m[3][0] = pos.x;             world.m[3][1] = pos.y;             world.m[3][2] = pos.z;
		world.m[3][3] = 1.0f;

		TuboEngine::InstanceData inst;
		inst.World = world;
		inst.WVP = TuboEngine::Math::Multiply(world, viewProj);
		inst.Color = color;
		const Vector3 toCam = pos - camPos;
		depth_[built.size()] = Dot(toCam, toCam);
		built.push_back(inst);
	}

	// --- 奥 → 手前の順に並べる（半透明の重なりを正しくする） ---
	const uint32_t n = static_cast<uint32_t>(built.size());
	std::iota(order_.begin(), order_.begin() + n, 0u);
	std::sort(order_.begin(), order_.begin() + n, [this](uint32_t a, uint32_t b) { return depth_[a] > depth_[b]; });
	for (uint32_t i = 0; i < n; ++i) {
		instanceData_[i] = built[order_[i]];
	}
	drawCount_ = n;
}

// =========================================
// Draw
// =========================================
void ShieldEffect::Draw() {
	if (drawCount_ == 0) {
		return;
	}
	renderer_.Draw(instanceBuffer_->GetGPUVirtualAddress(), drawCount_);
}

// =========================================
// イベント
// =========================================
void ShieldEffect::NotifyHit() {
	hitTimer_ = params_.hitDuration;
	// 波紋の中心：カメラから見える側のどこか
	auto rnd = [this]() {
		random_ = random_ * 1664525u + 1013904223u;
		return static_cast<float>((random_ >> 8) & 0xFFFFFF) / static_cast<float>(0x1000000) * 2.0f - 1.0f;
	};
	Vector3 toCam{0.0f, 0.0f, -1.0f};
	if (camera_ && !cells_.empty()) {
		// カメラの前方（ワールド行列の Z 行）の逆向き = 球からカメラへの向き
		const Matrix4x4& w = camera_->GetWorldMatrix();
		toCam = Normalize({-w.m[2][0], -w.m[2][1], -w.m[2][2]});
	}
	hitDir_ = Normalize(toCam + Vector3{rnd(), rnd(), rnd()} * 0.8f);
}

void ShieldEffect::NotifyBreak() {
	breakTimer_ = params_.breakDuration;
	hitTimer_ = 0.0f;
	for (auto& c : cells_) {
		c.flyPresence = c.presence;
		c.presence = 0.0f;
	}
}

// =========================================
// 残量率 → 色（1:full / 0.5:mid / 0:low）
// =========================================
Vector4 ShieldEffect::RateToColor(float rate) const {
	if (rate >= 0.5f) {
		return Lerp(params_.midColor, params_.fullColor, (rate - 0.5f) * 2.0f);
	}
	return Lerp(params_.lowColor, params_.midColor, rate * 2.0f);
}

// =========================================
// ImGui
// =========================================
void ShieldEffect::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::SeparatorText("Shield Effect");
	ImGui::Text("Cells : %u / %u", drawCount_, static_cast<uint32_t>(cells_.size()));
	if (ImGui::Button("Test Hit")) {
		NotifyHit();
	}
	ImGui::SameLine();
	if (ImGui::Button("Test Break")) {
		NotifyBreak();
	}	ImGui::DragFloat("Radius", &params_.radius, 0.01f, 0.1f, 20.0f);
	ImGui::SliderFloat("Min Radius Rate", &params_.minRadiusRate, 0.1f, 1.0f);
	ImGui::SliderFloat("Tile Fill", &params_.tileFill, 0.3f, 1.2f);
	ImGui::SliderFloat("Alpha", &params_.alpha, 0.0f, 1.0f);
	ImGui::DragFloat("Pop Speed", &params_.popSpeed, 0.1f, 0.1f, 30.0f);
	ImGui::DragFloat("Scan Period", &params_.scanPeriod, 0.01f, 0.0f, 10.0f);
	ImGui::SliderFloat("Scan Width", &params_.scanWidth, 0.0f, 0.3f);
	ImGui::DragFloat("Hit Duration", &params_.hitDuration, 0.01f, 0.05f, 3.0f);
	ImGui::DragFloat("Hit Push", &params_.hitPush, 0.005f, 0.0f, 1.0f);
	ImGui::DragFloat("Break Duration", &params_.breakDuration, 0.01f, 0.0f, 3.0f);
	ImGui::DragFloat("Break Distance", &params_.breakDistance, 0.01f, 0.0f, 5.0f);
	ImGui::ColorEdit3("Full Color", &params_.fullColor.x);
	ImGui::ColorEdit3("Mid Color", &params_.midColor.x);
	ImGui::ColorEdit3("Low Color", &params_.lowColor.x);
#endif
}
