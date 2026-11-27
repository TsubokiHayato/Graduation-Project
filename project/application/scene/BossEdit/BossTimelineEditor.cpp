#include "BossTimelineEditor.h"
#include "Boss/Boss.h"
#include "Boss/BossAttackPattern.h"
#include "Boss/BossAttackRegistry.h"
#include "Boss/BossAttackState.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kDegToRad = kPi / 180.0f;

// タイムラインの見た目
constexpr float kLabelWidth = 90.0f;  // 左の段の名前の幅
constexpr float kRulerHeight = 24.0f; // 目盛りの高さ
constexpr float kRowHeight = 34.0f;   // 1段の高さ
constexpr float kMinClipWidth = 8.0f; // クリップの最小の幅（ピクセル）

// 小数点以下4桁に丸める（保存したときに 0.800000011920929 のようにならないように）
double Round4(double value) { return std::round(value * 10000.0) / 10000.0; }

// "params.trackingTime = 2.5" のように、変えた項目を1行ずつにする
void FlattenOverrides(const JsonManager::Json& json, const std::string& prefix, std::vector<std::string>& out) {
	for (auto it = json.begin(); it != json.end(); ++it) {
		const std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
		if (it->is_object()) {
			FlattenOverrides(*it, key, out);
		} else {
			out.push_back(key + " = " + it->dump());
		}
	}
}
} // namespace

BossTimelineEditor::BossTimelineEditor() = default;
BossTimelineEditor::~BossTimelineEditor() = default;

void BossTimelineEditor::Initialize(Boss* boss) {
	boss_ = boss;
	SelectPattern(boss_->GetData().patterns.empty() ? -1 : 0);
}

Math::Vector3 BossTimelineEditor::GetTargetPosition() const {
	const Math::Vector3& c = boss_->GetCenter();
	const float a = targetAngleDeg_ * kDegToRad;
	return {c.x + std::cos(a) * targetDistance_, c.y, c.z + std::sin(a) * targetDistance_};
}

// =============================================================================
//  プレビュー
// =============================================================================
void BossTimelineEditor::Update() {
	BossPatternData* pattern = GetSelectedPattern();
	if (!pattern) {
		playing_ = false;
		boss_->StopPreview();
		boss_->SetPaused(true);
		return;
	}

	// 数値や配置を変えたら、今の時刻の状態を計算し直す（再生中ならそこから再生を続ける）
	if (dirty_) {
		dirty_ = false;
		boss_->SeekPreview(*pattern, playhead_);
	}

	if (playing_) {
		if (boss_->IsPreviewing()) {
			playhead_ = boss_->GetPreviewTime();
		} else {
			// 最後まで再生した
			playing_ = false;
			playhead_ = BossAttackPattern::ComputeTotalDuration(*boss_, *pattern);
		}
	}
	boss_->SetPaused(!playing_);
}

// =============================================================================
//  ImGui
// =============================================================================
void BossTimelineEditor::DrawImGui() {
#ifdef USE_IMGUI
	DrawPatternWindow();
	DrawTimelineWindow();
	DrawInspectorWindow();
#endif
}

// -----------------------------------------------------------------------------
//  Boss Edit：パターンの一覧・保存・的の位置
// -----------------------------------------------------------------------------
void BossTimelineEditor::DrawPatternWindow() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowSize(ImVec2(320.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Boss Edit")) {
		ImGui::End();
		return;
	}

	// ---- 保存・読み込み ----
	if (ImGui::Button("Save")) {
		boss_->ApplyDataChanges();
		message_ = boss_->SaveData() ? "保存しました" : "保存に失敗しました";
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload")) {
		boss_->LoadData();
		boss_->SetAutoAttack(false);
		const int count = static_cast<int>(boss_->GetData().patterns.size());
		SelectPattern(selectedPattern_ < count ? selectedPattern_ : count - 1);
		message_ = "読み込み直しました（保存していない変更は消えます）";
	}
	ImGui::TextDisabled("%s", message_.empty() ? "Save で Resources/Boss.json に保存" : message_.c_str());
	ImGui::Separator();

	// ---- パターンの一覧 ----
	std::vector<BossPatternData>& patterns = boss_->GetData().patterns;
	ImGui::Text("Patterns (攻撃パターン)");
	if (ImGui::BeginListBox("##patterns", ImVec2(-1.0f, 140.0f))) {
		for (int i = 0; i < static_cast<int>(patterns.size()); ++i) {
			ImGui::PushID(i);
			if (ImGui::Selectable(patterns[i].name.c_str(), i == selectedPattern_)) {
				SelectPattern(i);
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}
	if (ImGui::Button("New")) {
		BossPatternData pattern;
		pattern.name = MakeUniqueName("Pattern");
		patterns.push_back(pattern);
		SelectPattern(static_cast<int>(patterns.size()) - 1);
	}
	ImGui::SameLine();
	BossPatternData* selected = GetSelectedPattern();
	ImGui::BeginDisabled(!selected);
	if (ImGui::Button("Duplicate") && selected) {
		BossPatternData copy = *selected;
		copy.name = MakeUniqueName(selected->name + "_copy");
		patterns.push_back(copy);
		SelectPattern(static_cast<int>(patterns.size()) - 1);
		selected = GetSelectedPattern();
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete")) {
		ImGui::OpenPopup("delete_pattern");
	}
	ImGui::EndDisabled();
	if (ImGui::BeginPopupModal("delete_pattern", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("「%s」を削除しますか？", selected ? selected->name.c_str() : "");
		ImGui::TextDisabled("形態の攻撃一覧からも外します");
		if (ImGui::Button("削除する")) {
			DeletePattern(selectedPattern_);
			selected = GetSelectedPattern();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("やめる")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// ---- 選んだパターン ----
	if (selected) {
		ImGui::Separator();
		ImGui::InputText("Name", nameBuffer_, sizeof(nameBuffer_));
		ImGui::SameLine();
		if (ImGui::Button("Rename")) {
			const std::string newName = nameBuffer_;
			if (newName.empty()) {
				message_ = "名前が空です";
			} else if (IsNameUsed(newName, selectedPattern_)) {
				message_ = "その名前はすでに使われています（攻撃名も使えません）";
			} else {
				RenamePattern(selectedPattern_, newName);
				message_ = "名前を変えました（形態の攻撃一覧も書き換えました）";
			}
		}
		if (ImGui::DragFloat("Recovery (硬直)", &selected->recoveryTime, 0.05f, 0.0f, 10.0f, "%.2f s")) {
			MarkDirty();
		}
		ImGui::TextDisabled("硬直はパターンの最後に1回だけ（最後に終わる攻撃が硬直する）");

		// 使う形態（チェックで形態の攻撃一覧に追加・削除）
		ImGui::Text("使う形態");
		for (BossPhaseData& phase : boss_->GetData().phases) {
			auto it = std::find_if(phase.patterns.begin(), phase.patterns.end(), [&](const BossPatternEntry& e) { return e.pattern == selected->name; });
			bool used = it != phase.patterns.end();
			ImGui::PushID(&phase);
			if (ImGui::Checkbox(phase.name.c_str(), &used)) {
				if (used) {
					phase.patterns.push_back({selected->name, 1.0f, 1});
				} else {
					phase.patterns.erase(it);
				}
			}
			ImGui::PopID();
		}
		ImGui::TextDisabled("重み・再使用までの回数は Boss ウィンドウの Phases で調整");
	}

	// ---- 的（自機の代わり） ----
	ImGui::Separator();
	ImGui::Text("Target (的の位置)");
	bool moved = false;
	moved |= ImGui::SliderFloat("Distance", &targetDistance_, 1.0f, boss_->GetStageRadius());
	moved |= ImGui::SliderFloat("Angle (deg)", &targetAngleDeg_, -180.0f, 180.0f);
	if (moved) {
		MarkDirty(); // ボスの向き・照準は的の位置で変わるので計算し直す
	}

	ImGui::End();
#endif
}

// -----------------------------------------------------------------------------
//  Boss Timeline：再生の操作とタイムライン
// -----------------------------------------------------------------------------
void BossTimelineEditor::DrawTimelineWindow() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowSize(ImVec2(900.0f, 300.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Boss Timeline")) {
		ImGui::End();
		return;
	}

	BossPatternData* pattern = GetSelectedPattern();
	if (!pattern) {
		ImGui::Text("Boss Edit ウィンドウでパターンを選ぶか、New で作ってください");
		ImGui::End();
		return;
	}

	const float total = BossAttackPattern::ComputeTotalDuration(*boss_, *pattern);

	// ---- 再生の操作 ----
	const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	const bool togglePlay = focused && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Space, false);
	if (ImGui::Button(playing_ ? "Pause" : "Play") || togglePlay) {
		if (playing_) {
			playing_ = false;
		} else {
			if (playhead_ >= total - 1e-3f) {
				playhead_ = 0.0f; // 最後まで再生していたら最初から
			}
			playing_ = true;
			MarkDirty();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("|< 最初へ")) {
		playing_ = false;
		playhead_ = 0.0f;
		MarkDirty();
	}
	ImGui::SameLine();
	ImGui::Text("%.2f / %.2f s", playhead_, total);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(140.0f);
	ImGui::SliderFloat("Zoom", &pixelsPerSecond_, 30.0f, 300.0f, "%.0f px/s");
	ImGui::SameLine();
	ImGui::Checkbox("Snap", &snap_);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.0f);
	ImGui::DragFloat("##snapstep", &snapStep_, 0.01f, 0.01f, 1.0f, "%.2f s");

	// 再生位置に攻撃を追加
	static int addAttack = 0;
	const std::vector<std::string>& ids = BossAttackRegistry::GetIds();
	ImGui::SetNextItemWidth(140.0f);
	if (ImGui::BeginCombo("##addattack", ids.empty() ? "" : ids[std::clamp(addAttack, 0, static_cast<int>(ids.size()) - 1)].c_str())) {
		for (int i = 0; i < static_cast<int>(ids.size()); ++i) {
			if (ImGui::Selectable(ids[i].c_str(), i == addAttack)) {
				addAttack = i;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("再生位置に追加") && !ids.empty()) {
		// 再生位置で空いている段に置く
		int track = 0;
		for (bool found = false; !found; ++track) {
			found = true;
			for (const BossTimelineClip& clip : pattern->clips) {
				const BossAttackPattern::ClipTiming t = BossAttackPattern::ComputeClipTiming(*boss_, clip);
				if (clip.track == track && playhead_ >= t.start - 1e-3f && playhead_ < t.End()) {
					found = false;
					break;
				}
			}
			if (found) {
				break;
			}
		}
		AddClip(*pattern, ids[std::clamp(addAttack, 0, static_cast<int>(ids.size()) - 1)], playhead_, track);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("空いている所を右クリックでも追加できます");

	DrawTimelineCanvas(*pattern);

	ImGui::End();
#endif
}

void BossTimelineEditor::DrawTimelineCanvas(BossPatternData& pattern) {
#ifdef USE_IMGUI
	// ---- 長さと段の数 ----
	std::vector<BossAttackPattern::ClipTiming> timings;
	float activeEnd = 0.0f;
	int maxTrack = 0;
	for (const BossTimelineClip& clip : pattern.clips) {
		timings.push_back(BossAttackPattern::ComputeClipTiming(*boss_, clip));
		activeEnd = (std::max)(activeEnd, timings.back().End());
		maxTrack = (std::max)(maxTrack, clip.track);
	}
	const float total = activeEnd + pattern.recoveryTime;
	const int trackCount = maxTrack + 2; // 下に1段空けておき、ドラッグで新しい段へ移せるようにする
	const float viewDuration = (std::max)(total + 2.0f, 8.0f);
	const float canvasWidth = kLabelWidth + viewDuration * pixelsPerSecond_;
	const float canvasHeight = kRulerHeight + kRowHeight * static_cast<float>(trackCount + 1); // +1 は硬直の段

	ImGui::BeginChild("timeline_canvas", ImVec2(0.0f, canvasHeight + 24.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float scrollX = ImGui::GetScrollX();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImGuiIO& io = ImGui::GetIO();

	auto timeToX = [&](float t) { return origin.x + kLabelWidth + t * pixelsPerSecond_; };
	auto xToTime = [&](float x) { return (x - origin.x - kLabelWidth) / pixelsPerSecond_; };
	auto rowTop = [&](int row) { return origin.y + kRulerHeight + kRowHeight * static_cast<float>(row); };
	const int recoveryRow = trackCount;

	// ---- 背景（段） ----
	for (int row = 0; row <= recoveryRow; ++row) {
		const ImU32 col = row == recoveryRow ? IM_COL32(45, 40, 55, 255) : (row % 2 == 0 ? IM_COL32(38, 38, 42, 255) : IM_COL32(32, 32, 36, 255));
		dl->AddRectFilled(ImVec2(origin.x + kLabelWidth, rowTop(row)), ImVec2(origin.x + canvasWidth, rowTop(row) + kRowHeight), col);
	}
	// 攻撃の終わり（ここから硬直）
	dl->AddLine(ImVec2(timeToX(activeEnd), rowTop(0)), ImVec2(timeToX(activeEnd), rowTop(recoveryRow + 1)), IM_COL32(200, 200, 255, 60));

	// ---- 目盛り（クリック・ドラッグで再生位置を移動） ----
	dl->AddRectFilled(ImVec2(origin.x + kLabelWidth, origin.y), ImVec2(origin.x + canvasWidth, origin.y + kRulerHeight), IM_COL32(55, 55, 60, 255));
	for (int i = 0; i <= static_cast<int>(viewDuration * 10.0f); ++i) {
		const float t = static_cast<float>(i) * 0.1f;
		const float x = timeToX(t);
		const bool major = i % 10 == 0;
		const bool half = i % 5 == 0;
		dl->AddLine(ImVec2(x, origin.y + (major ? 4.0f : (half ? 12.0f : 17.0f))), ImVec2(x, origin.y + kRulerHeight), IM_COL32(170, 170, 170, 255));
		if (major) {
			char text[16];
			snprintf(text, sizeof(text), "%ds", i / 10);
			dl->AddText(ImVec2(x + 3.0f, origin.y + 2.0f), IM_COL32(220, 220, 220, 255), text);
			dl->AddLine(ImVec2(x, rowTop(0)), ImVec2(x, rowTop(recoveryRow + 1)), IM_COL32(255, 255, 255, 18));
		}
	}
	ImGui::SetCursorScreenPos(ImVec2(origin.x + kLabelWidth, origin.y));
	ImGui::InvisibleButton("ruler", ImVec2(canvasWidth - kLabelWidth, kRulerHeight));
	if (ImGui::IsItemActive()) {
		playing_ = false;
		playhead_ = std::clamp(xToTime(io.MousePos.x), 0.0f, total);
		MarkDirty();
	}

	// ---- クリップ ----
	bool clipHovered = false;
	int deleteClip = -1;
	// 複製でクリップが増えても、このフレームは長さを計算済みのクリップだけを扱う
	const int clipCount = static_cast<int>(timings.size());
	for (int i = 0; i < clipCount; ++i) {
		BossTimelineClip& clip = pattern.clips[i];
		const BossAttackPattern::ClipTiming t = timings[i];
		const float x0 = timeToX(t.start);
		const float xMid = timeToX(t.start + t.telegraph);
		const float x1 = (std::max)(timeToX(t.End()), x0 + kMinClipWidth);
		const float y0 = rowTop(clip.track) + 3.0f;
		const float y1 = rowTop(clip.track) + kRowHeight - 3.0f;
		const bool known = BossAttackRegistry::Create(clip.attack) != nullptr;

		// 予兆（橙）と攻撃（赤）
		dl->AddRectFilled(ImVec2(x0, y0), ImVec2((std::min)(xMid, x1), y1), known ? IM_COL32(205, 130, 40, 230) : IM_COL32(110, 110, 110, 230), 3.0f);
		if (xMid < x1) {
			dl->AddRectFilled(ImVec2(xMid, y0), ImVec2(x1, y1), IM_COL32(210, 55, 55, 230), 3.0f);
		}
		// 枠：選択中は白、ボスを動かすクリップは水色
		const ImU32 border = i == selectedClip_ ? IM_COL32(255, 255, 255, 255) : (clip.controlsBody ? IM_COL32(90, 220, 255, 255) : IM_COL32(20, 20, 20, 255));
		dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), border, 3.0f, 0, i == selectedClip_ ? 2.5f : 1.5f);
		// 名前（変えた数値があれば *、ボスを動かすなら [B]）
		char label[96];
		snprintf(label, sizeof(label), "%s%s%s", clip.controlsBody ? "[B] " : "", clip.attack.c_str(), clip.overrides.empty() ? "" : " *");
		dl->PushClipRect(ImVec2(x0, y0), ImVec2(x1, y1), true);
		dl->AddText(ImVec2(x0 + 4.0f, y0 + 6.0f), IM_COL32(255, 255, 255, 255), label);
		dl->PopClipRect();

		// 操作
		ImGui::PushID(i);
		ImGui::SetCursorScreenPos(ImVec2(x0, y0));
		ImGui::InvisibleButton("clip", ImVec2(x1 - x0, y1 - y0), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		if (ImGui::IsItemHovered()) {
			clipHovered = true;
			ImGui::SetTooltip("%s\n開始 %.2f s\n予兆 %.2f s / 攻撃 %.2f s\n終わり %.2f s%s", clip.attack.c_str(), t.start, t.telegraph, t.attack, t.End(),
			                  clip.overrides.empty() ? "" : "\n数値を変えています (*)");
		}
		if (ImGui::IsItemActivated() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			selectedClip_ = i;
			dragClip_ = i;
			dragGrabOffset_ = xToTime(io.MousePos.x) - clip.start;
		}
		if (ImGui::IsItemActive() && dragClip_ == i && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
			// 左右で開始時刻、上下で段
			float start = xToTime(io.MousePos.x) - dragGrabOffset_;
			start = io.KeyShift ? start : Snap(start);
			const int track = std::clamp(static_cast<int>(std::floor((io.MousePos.y - rowTop(0)) / kRowHeight)), 0, trackCount - 1);
			start = (std::max)(start, 0.0f);
			if (std::abs(start - clip.start) > 1e-4f || track != clip.track) {
				clip.start = start;
				clip.track = track;
				MarkDirty();
			}
		}
		if (ImGui::IsItemDeactivated()) {
			dragClip_ = -1;
		}
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			selectedClip_ = i;
			ImGui::OpenPopup("clip_menu");
		}
		if (ImGui::BeginPopup("clip_menu")) {
			ImGui::Text("%s", clip.attack.c_str());
			ImGui::Separator();
			if (ImGui::MenuItem("ボスを動かす", nullptr, clip.controlsBody)) {
				clip.controlsBody = !clip.controlsBody;
				MarkDirty();
			}
			if (ImGui::MenuItem("複製（すぐ後ろに置く）")) {
				BossTimelineClip copy = clip;
				copy.start = Snap(t.End());
				pattern.clips.push_back(copy);
				selectedClip_ = static_cast<int>(pattern.clips.size()) - 1;
				MarkDirty();
			}
			if (ImGui::MenuItem("削除", "Delete")) {
				deleteClip = i;
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	// ---- 硬直の帯（右端をドラッグで硬直時間を変える） ----
	{
		const float x0 = timeToX(activeEnd);
		const float x1 = (std::max)(timeToX(total), x0 + 2.0f);
		const float y0 = rowTop(recoveryRow) + 3.0f;
		const float y1 = rowTop(recoveryRow) + kRowHeight - 3.0f;
		dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(110, 110, 140, 230), 3.0f);
		char label[48];
		snprintf(label, sizeof(label), "硬直 %.2f s", pattern.recoveryTime);
		dl->PushClipRect(ImVec2(x0, y0), ImVec2(x1 + 200.0f, y1), true);
		dl->AddText(ImVec2(x0 + 4.0f, y0 + 6.0f), IM_COL32(255, 255, 255, 255), label);
		dl->PopClipRect();
		// 右端のつまみ
		dl->AddRectFilled(ImVec2(x1 - 3.0f, y0), ImVec2(x1 + 3.0f, y1), IM_COL32(230, 230, 255, 255));
		ImGui::SetCursorScreenPos(ImVec2(x1 - 5.0f, y0));
		ImGui::InvisibleButton("recovery_handle", ImVec2(10.0f, y1 - y0));
		if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			clipHovered = true;
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)) {
			float recovery = xToTime(io.MousePos.x) - activeEnd;
			recovery = io.KeyShift ? recovery : Snap(recovery);
			pattern.recoveryTime = (std::max)(recovery, 0.0f);
			MarkDirty();
		}
	}

	// ---- 段の名前（横にスクロールしても左に残す） ----
	const float labelX = origin.x + scrollX;
	dl->AddRectFilled(ImVec2(labelX, origin.y), ImVec2(labelX + kLabelWidth, rowTop(recoveryRow + 1)), IM_COL32(28, 28, 30, 255));
	for (int row = 0; row <= recoveryRow; ++row) {
		char text[32];
		if (row == recoveryRow) {
			snprintf(text, sizeof(text), "硬直");
		} else {
			snprintf(text, sizeof(text), "Track %d", row + 1);
		}
		dl->AddText(ImVec2(labelX + 6.0f, rowTop(row) + 9.0f), IM_COL32(200, 200, 200, 255), text);
	}

	// ---- 再生位置 ----
	{
		const float x = timeToX(playhead_);
		dl->AddLine(ImVec2(x, origin.y), ImVec2(x, rowTop(recoveryRow + 1)), IM_COL32(255, 220, 60, 255), 2.0f);
		dl->AddTriangleFilled(ImVec2(x - 6.0f, origin.y), ImVec2(x + 6.0f, origin.y), ImVec2(x, origin.y + 9.0f), IM_COL32(255, 220, 60, 255));
	}

	// ---- 空いている所を右クリック → 攻撃を追加 ----
	const bool inTracks = io.MousePos.y >= rowTop(0) && io.MousePos.y < rowTop(recoveryRow) && io.MousePos.x > labelX + kLabelWidth;
	if (ImGui::IsWindowHovered() && !clipHovered && inTracks) {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			addTime_ = (std::max)(Snap(xToTime(io.MousePos.x)), 0.0f);
			addTrack_ = std::clamp(static_cast<int>(std::floor((io.MousePos.y - rowTop(0)) / kRowHeight)), 0, trackCount - 1);
			ImGui::OpenPopup("add_menu");
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			selectedClip_ = -1; // 空いている所をクリックで選択を外す
		}
	}
	if (ImGui::BeginPopup("add_menu")) {
		ImGui::Text("%.2f s / Track %d に追加", addTime_, addTrack_ + 1);
		ImGui::Separator();
		for (const std::string& id : BossAttackRegistry::GetIds()) {
			if (ImGui::MenuItem(id.c_str())) {
				AddClip(pattern, id, addTime_, addTrack_);
			}
		}
		ImGui::EndPopup();
	}

	// ---- Delete キーで削除 ----
	const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (focused && selectedClip_ >= 0 && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
		deleteClip = selectedClip_;
	}
	if (deleteClip >= 0 && deleteClip < static_cast<int>(pattern.clips.size())) {
		pattern.clips.erase(pattern.clips.begin() + deleteClip);
		// このあとの重なりの確認で、消したクリップを参照しないように長さの一覧からも消す
		if (deleteClip < static_cast<int>(timings.size())) {
			timings.erase(timings.begin() + deleteClip);
		}
		selectedClip_ = -1;
		dragClip_ = -1;
		MarkDirty();
	}

	// キャンバスの大きさ（スクロールの範囲）
	ImGui::SetCursorScreenPos(origin);
	ImGui::Dummy(ImVec2(canvasWidth, canvasHeight));
	ImGui::EndChild();

	// ---- 注意 ----
	// ボスを動かすクリップが時間で重なっていると、後から更新した方の動きが残る
	// クリップと長さの一覧の両方にある分だけ調べる（このフレームで複製・削除したときも範囲を超えない）
	const size_t checkCount = (std::min)(timings.size(), pattern.clips.size());
	for (size_t i = 0; i < checkCount; ++i) {
		for (size_t j = i + 1; j < checkCount; ++j) {
			if (pattern.clips[i].controlsBody && pattern.clips[j].controlsBody && timings[i].start < timings[j].End() && timings[j].start < timings[i].End()) {
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "注意: ボスを動かす [B] クリップが重なっています（%s と %s）。重なる所ではどちらか1つにしてください",
				                   pattern.clips[i].attack.c_str(), pattern.clips[j].attack.c_str());
				i = checkCount; // 1つ出せば十分
				break;
			}
		}
	}
	if (pattern.clips.empty()) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "攻撃が置かれていないので、このパターンは使えません");
	}
#else
	(void)pattern;
#endif
}

// -----------------------------------------------------------------------------
//  Clip Inspector：選んだクリップの数値
// -----------------------------------------------------------------------------
void BossTimelineEditor::DrawInspectorWindow() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowSize(ImVec2(340.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Clip Inspector")) {
		ImGui::End();
		return;
	}

	BossPatternData* pattern = GetSelectedPattern();
	if (!pattern || selectedClip_ < 0 || selectedClip_ >= static_cast<int>(pattern->clips.size())) {
		ImGui::Text("タイムラインのクリップを選んでください");
		ImGui::End();
		return;
	}
	BossTimelineClip& clip = pattern->clips[selectedClip_];

	// ---- 配置 ----
	const std::vector<std::string>& ids = BossAttackRegistry::GetIds();
	if (ImGui::BeginCombo("Attack", clip.attack.c_str())) {
		for (const std::string& id : ids) {
			if (ImGui::Selectable(id.c_str(), id == clip.attack) && id != clip.attack) {
				clip.attack = id;
				clip.overrides = JsonManager::Json::object(); // 攻撃が変わったら変えた数値は使えない
				MarkDirty();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::DragFloat("Start (開始)", &clip.start, 0.01f, 0.0f, 60.0f, "%.2f s")) {
		clip.start = (std::max)(clip.start, 0.0f);
		MarkDirty();
	}
	if (ImGui::DragInt("Track (段)", &clip.track, 0.1f, 0, 10)) {
		clip.track = (std::max)(clip.track, 0);
	}
	if (ImGui::Checkbox("ボスを動かす [B]", &clip.controlsBody)) {
		MarkDirty();
	}
	ImGui::TextDisabled("ボスの向き・高さ・色を動かすか。重なる攻撃のうち1つだけにする");

	const BossAttackPattern::ClipTiming t = BossAttackPattern::ComputeClipTiming(*boss_, clip);
	ImGui::Text("予兆 %.2f s  攻撃 %.2f s  終わり %.2f s", t.telegraph, t.attack, t.End());

	// ---- 数値（変えた項目だけクリップに保存する） ----
	ImGui::Separator();
	ImGui::Text("数値（このクリップだけ変える）");

	// 選んだクリップが変わったら、編集用の攻撃を作り直す（ImGui の ID が変わらないように使い回す）
	if (!inspectAttack_ || inspectPattern_ != selectedPattern_ || inspectClip_ != selectedClip_ || inspectAttackId_ != clip.attack) {
		inspectAttack_ = BossAttackRegistry::Create(clip.attack);
		inspectPattern_ = selectedPattern_;
		inspectClip_ = selectedClip_;
		inspectAttackId_ = clip.attack;
	}
	if (!inspectAttack_) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "知らない攻撃名です: %s", clip.attack.c_str());
		ImGui::End();
		return;
	}

	// 毎フレーム「標準の数値 → 変えた数値」の順で読み込み直す（標準の数値を変えたらすぐ反映される）
	const JsonManager::Json base = boss_->GetAttackBaseParams(clip.attack);
	inspectAttack_->LoadParams(base);
	inspectAttack_->LoadParams(clip.overrides);
	inspectAttack_->DrawImGui();

	// 標準の数値との差を、このクリップの overrides にする
	JsonManager::Json now;
	inspectAttack_->SaveParams(now);
	JsonManager::Json overrides = Diff(base, now);
	// 硬直はパターンの設定を使うので、クリップでは持たない
	if (overrides.contains("common")) {
		overrides["common"].erase("recoveryTime");
		if (overrides["common"].empty()) {
			overrides.erase("common");
		}
	}
	if (overrides != clip.overrides) {
		clip.overrides = overrides;
		MarkDirty();
	}
	ImGui::TextDisabled("Recovery time はパターンの硬直を使います（ここでは変わりません）");

	// 変えた項目の一覧
	ImGui::Separator();
	if (clip.overrides.empty()) {
		ImGui::Text("標準の数値のまま");
	} else {
		ImGui::Text("標準から変えた項目 (*)");
		std::vector<std::string> lines;
		FlattenOverrides(clip.overrides, "", lines);
		for (const std::string& line : lines) {
			ImGui::BulletText("%s", line.c_str());
		}
		if (ImGui::Button("標準の数値に戻す")) {
			clip.overrides = JsonManager::Json::object();
			MarkDirty();
		}
	}
	ImGui::TextDisabled("攻撃の標準の数値は Boss ウィンドウの Attacks で変える");

	ImGui::End();
#endif
}

// =============================================================================
//  パターンの操作
// =============================================================================
BossPatternData* BossTimelineEditor::GetSelectedPattern() {
	std::vector<BossPatternData>& patterns = boss_->GetData().patterns;
	if (selectedPattern_ < 0 || selectedPattern_ >= static_cast<int>(patterns.size())) {
		return nullptr;
	}
	return &patterns[selectedPattern_];
}

void BossTimelineEditor::SelectPattern(int index) {
	selectedPattern_ = index;
	selectedClip_ = -1;
	dragClip_ = -1;
	playing_ = false;
	playhead_ = 0.0f;
	inspectAttack_.reset();
	if (BossPatternData* pattern = GetSelectedPattern()) {
		snprintf(nameBuffer_, sizeof(nameBuffer_), "%s", pattern->name.c_str());
	} else {
		nameBuffer_[0] = '\0';
	}
	MarkDirty();
}

bool BossTimelineEditor::IsNameUsed(const std::string& name, int ignoreIndex) const {
	// 攻撃名は単発のパターン名として使われているので使えない
	for (const std::string& id : BossAttackRegistry::GetIds()) {
		if (id == name) {
			return true;
		}
	}
	const std::vector<BossPatternData>& patterns = boss_->GetData().patterns;
	for (int i = 0; i < static_cast<int>(patterns.size()); ++i) {
		if (i != ignoreIndex && patterns[i].name == name) {
			return true;
		}
	}
	return false;
}

std::string BossTimelineEditor::MakeUniqueName(const std::string& base) const {
	if (!IsNameUsed(base, -1)) {
		return base;
	}
	for (int n = 2;; ++n) {
		const std::string name = base + std::to_string(n);
		if (!IsNameUsed(name, -1)) {
			return name;
		}
	}
}

void BossTimelineEditor::RenamePattern(int index, const std::string& newName) {
	BossPatternData& pattern = boss_->GetData().patterns[index];
	// 形態の攻撃一覧の名前も書き換える
	for (BossPhaseData& phase : boss_->GetData().phases) {
		for (BossPatternEntry& entry : phase.patterns) {
			if (entry.pattern == pattern.name) {
				entry.pattern = newName;
			}
		}
	}
	pattern.name = newName;
	snprintf(nameBuffer_, sizeof(nameBuffer_), "%s", newName.c_str());
}

void BossTimelineEditor::DeletePattern(int index) {
	std::vector<BossPatternData>& patterns = boss_->GetData().patterns;
	if (index < 0 || index >= static_cast<int>(patterns.size())) {
		return;
	}
	// 形態の攻撃一覧からも外す
	const std::string name = patterns[index].name;
	for (BossPhaseData& phase : boss_->GetData().phases) {
		phase.patterns.erase(std::remove_if(phase.patterns.begin(), phase.patterns.end(), [&](const BossPatternEntry& e) { return e.pattern == name; }), phase.patterns.end());
	}
	patterns.erase(patterns.begin() + index);
	SelectPattern((std::min)(index, static_cast<int>(patterns.size()) - 1));
}

void BossTimelineEditor::AddClip(BossPatternData& pattern, const std::string& attackId, float start, int track) {
	BossTimelineClip clip;
	clip.attack = attackId;
	clip.start = (std::max)(start, 0.0f);
	clip.track = (std::max)(track, 0);
	// 同じ時間にボスを動かすクリップがすでにあれば、新しいクリップは動かさない
	const BossAttackPattern::ClipTiming t = BossAttackPattern::ComputeClipTiming(*boss_, clip);
	for (const BossTimelineClip& other : pattern.clips) {
		const BossAttackPattern::ClipTiming o = BossAttackPattern::ComputeClipTiming(*boss_, other);
		if (other.controlsBody && o.start < t.End() && t.start < o.End()) {
			clip.controlsBody = false;
			break;
		}
	}
	pattern.clips.push_back(clip);
	selectedClip_ = static_cast<int>(pattern.clips.size()) - 1;
	MarkDirty();
}

float BossTimelineEditor::Snap(float time) const {
	if (!snap_ || snapStep_ <= 0.0f) {
		return time;
	}
	return std::round(time / snapStep_) * snapStep_;
}

// =============================================================================
//  クリップの数値
// =============================================================================
JsonManager::Json BossTimelineEditor::Diff(const JsonManager::Json& base, const JsonManager::Json& now) {
	JsonManager::Json out = JsonManager::Json::object();
	if (!now.is_object()) {
		return out;
	}
	for (auto it = now.begin(); it != now.end(); ++it) {
		const std::string& key = it.key();
		if (!base.is_object() || !base.contains(key)) {
			out[key] = *it;
			continue;
		}
		const JsonManager::Json& b = base[key];
		if (it->is_object() && b.is_object()) {
			JsonManager::Json child = Diff(b, *it);
			if (!child.empty()) {
				out[key] = child;
			}
		} else if (it->is_number() && b.is_number()) {
			// float の誤差は同じ値とみなす
			if (std::abs(it->get<double>() - b.get<double>()) > 1e-4) {
				out[key] = it->is_number_float() ? JsonManager::Json(Round4(it->get<double>())) : *it;
			}
		} else if (*it != b) {
			out[key] = *it;
		}
	}
	return out;
}
