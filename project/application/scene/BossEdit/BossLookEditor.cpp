#include "BossLookEditor.h"
#include "Boss/Boss.h"
#include "Boss/BossAttackRegistry.h"
#include "JsonManager.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr size_t kMaxLog = 14;

// root の下にある、拡張子が exts のどれかのファイルを、root からの相対パス（/ 区切り）で集める
std::vector<std::string> CollectFiles(const std::string& root, const std::vector<std::string>& exts) {
	std::vector<std::string> files;
	namespace fs = std::filesystem;
	std::error_code ec;
	const fs::path base = JsonManager::ResolvePath(root);
	if (!fs::is_directory(base, ec)) {
		return files;
	}
	for (fs::recursive_directory_iterator it(base, ec), end; it != end && !ec; it.increment(ec)) {
		if (!it->is_regular_file(ec)) {
			continue;
		}
		std::string ext = it->path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (std::find(exts.begin(), exts.end(), ext) == exts.end()) {
			continue;
		}
		files.push_back(fs::relative(it->path(), base, ec).generic_string());
	}
	std::sort(files.begin(), files.end());
	return files;
}
} // namespace

void BossLookEditor::Initialize(Boss* boss) {
	boss_ = boss;
	phase_ = boss_->GetPhaseIndex();
	RefreshFileLists();

	// 演出の合図・アニメーションの指示を記録する（BossEdit では受け取る側の代わりに表示するだけ）
	boss_->SetCueCallback([this](const Boss::Cue& cue) {
		static const char* kTypes[] = {"移行の始まり", "移行の終わり", "撃破"};
		char text[160];
		snprintf(text, sizeof(text), "[合図] %s : \"%s\" (形態 %d)", kTypes[static_cast<int>(cue.type)], cue.name.c_str(), cue.phaseIndex + 1);
		AddLog(text);
	});
	boss_->SetAnimationCallback([this](const std::string& name, float duration) {
		char text[160];
		snprintf(text, sizeof(text), "[アニメ] %s (%.2f s)", name.c_str(), duration);
		AddLog(text);
	});
}

void BossLookEditor::RefreshFileLists() {
	models_ = CollectFiles("Resources/Models", {".obj", ".gltf", ".glb"});
	textures_ = CollectFiles("Resources/Textures", {".png", ".jpg", ".jpeg", ".dds"});
}

bool BossLookEditor::IsPlayingTransition() const {
	return previewingTransition_ && boss_->GetState() == Boss::State::Transition;
}

bool BossLookEditor::ConsumeTransitionFinished() {
	if (previewingTransition_ && boss_->GetState() != Boss::State::Transition) {
		previewingTransition_ = false;
		transitionFinished_ = true;
	}
	const bool finished = transitionFinished_;
	transitionFinished_ = false;
	return finished;
}

bool BossLookEditor::ConsumePhaseChanged() {
	const bool changed = phaseChanged_;
	phaseChanged_ = false;
	return changed;
}

void BossLookEditor::AddLog(const std::string& text) {
	log_.push_front(text);
	while (log_.size() > kMaxLog) {
		log_.pop_back();
	}
}

// =============================================================================
//  ImGui
// =============================================================================
bool BossLookEditor::InputString(const char* label, std::string& value) {
#ifdef USE_IMGUI
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%s", value.c_str());
	if (ImGui::InputText(label, buffer, sizeof(buffer))) {
		value = buffer;
		return true;
	}
#else
	(void)label;
	(void)value;
#endif
	return false;
}

bool BossLookEditor::FileCombo(const char* label, std::string& value, const std::vector<std::string>& files, const char* emptyLabel) {
	bool changed = false;
#ifdef USE_IMGUI
	const char* preview = value.empty() && emptyLabel ? emptyLabel : value.c_str();
	if (ImGui::BeginCombo(label, preview, ImGuiComboFlags_HeightLarge)) {
		if (emptyLabel && ImGui::Selectable(emptyLabel, value.empty())) {
			value.clear();
			changed = true;
		}
		for (const std::string& file : files) {
			if (ImGui::Selectable(file.c_str(), file == value)) {
				value = file;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
#else
	(void)label;
	(void)value;
	(void)files;
	(void)emptyLabel;
#endif
	return changed;
}

void BossLookEditor::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::SetNextWindowSize(ImVec2(380.0f, 640.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Boss Look")) {
		ImGui::End();
		return;
	}

	BossData& data = boss_->GetData();
	phase_ = std::clamp(phase_, 0, static_cast<int>(data.phases.size()) - 1);

	// ---- 形態の選択 ----
	if (ImGui::BeginCombo("Phase", data.phases[phase_].name.c_str())) {
		for (int i = 0; i < static_cast<int>(data.phases.size()); ++i) {
			if (ImGui::Selectable(data.phases[i].name.c_str(), i == phase_)) {
				phase_ = i;
				boss_->ForcePhase(phase_);
				phaseChanged_ = true;
			}
		}
		ImGui::EndCombo();
	}
	if (boss_->GetPhaseIndex() != phase_ && !IsPlayingTransition()) {
		if (ImGui::Button("ボスをこの形態にする")) {
			boss_->ForcePhase(phase_);
			phaseChanged_ = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(いまのボス: %s)", boss_->GetPhase().name.c_str());
	}
	ImGui::TextDisabled("タイムラインのプレビューも、いまのボスの形態の見た目で再生します");
	if (ImGui::Button("ファイル一覧を更新")) {
		RefreshFileLists();
	}

	BossPhaseData& phase = data.phases[phase_];
	BossLookData& look = phase.look;

	// ---- 見た目 ----
	if (ImGui::CollapsingHeader("Look (見た目)", ImGuiTreeNodeFlags_DefaultOpen)) {
		FileCombo("Model", look.model, models_);
		ImGui::TextDisabled("自機などと同じモデルを使うと色・材質が共有されます。ボス専用のモデルを使ってください");
		FileCombo("Texture", look.texture, textures_, "(モデルのテクスチャ)");
		ImGui::ColorEdit4("Color (体の色)", &look.color.x);
		ImGui::DragFloat3("Scale", &look.scale.x, 0.01f, 0.05f, 20.0f);
		ImGui::SeparatorText("モデルの補正");
		ImGui::DragFloat3("Offset", &look.offset.x, 0.01f, -20.0f, 20.0f);
		ImGui::TextDisabled("ボスの向きで回り、大きさに合わせて伸びる。block は y=1 で地面に乗る");
		ImGui::DragFloat3("Rotation (deg)", &look.rotationDeg.x, 1.0f, -180.0f, 180.0f);
		ImGui::SeparatorText("材質");
		ImGui::DragFloat("Shininess (光沢)", &look.shininess, 1.0f, 0.0f, 1000.0f);
		ImGui::SliderFloat("Environment (映り込み)", &look.environmentCoefficient, 0.0f, 1.0f);
		ImGui::SeparatorText("攻撃の演出");
		ImGui::ColorEdit4("Effect tint (線の色)", &look.effectTint.x);
		ImGui::TextDisabled("攻撃範囲・弾・ビームの線の色に掛け合わせる。攻撃ごとの色は Boss ウィンドウの Attacks");
	}

	// ---- コア ----
	if (ImGui::CollapsingHeader("Core (背中のコア)")) {
		BossCoreLook& core = look.core;
		ImGui::Checkbox("Visible", &core.visible);
		ImGui::SameLine();
		ImGui::Checkbox("狙える範囲の線", &core.showArcLines);
		FileCombo("Core model", core.model, models_);
		ImGui::DragFloat3("Core offset", &core.offset.x, 0.01f, -20.0f, 20.0f);
		ImGui::TextDisabled("x = 右、y = 上、z = 前（背中は -z）");
		ImGui::DragFloat("Core scale", &core.scale, 0.01f, 0.01f, 10.0f);
		ImGui::ColorEdit4("Core color", &core.color.x);
		ImGui::ColorEdit4("Recovery color (硬直中)", &core.recoveryColor.x);
		ImGui::DragFloat("Blink (Hz)", &core.blinkHz, 0.1f, 0.0f, 30.0f);
	}

	// ---- 移行演出 ----
	if (ImGui::CollapsingHeader("Transition (この形態に入る移行演出)")) {
		BossTransitionData& tr = phase.transition;
		ImGui::DragFloat("Time", &phase.transitionTime, 0.05f, 0.0f, 10.0f, "%.2f s");
		ImGui::DragFloat("Rise height", &tr.riseHeight, 0.05f, -5.0f, 20.0f);
		ImGui::ColorEdit4("Glow color", &tr.glowColor.x);
		ImGui::DragFloat("Blink start (Hz)", &tr.blinkStartHz, 0.1f, 0.0f, 30.0f);
		ImGui::DragFloat("Blink end (Hz)", &tr.blinkEndHz, 0.1f, 0.0f, 30.0f);
		ImGui::SliderFloat("Look switch", &tr.lookSwitchRatio, 0.0f, 1.0f, "%.2f");
		ImGui::TextDisabled("見た目を新しい形態に切り替える時点（演出の長さに対する割合）");
		ImGui::SeparatorText("演出の合図（ライティングなどの制御側へ）");
		InputString("Start cue", tr.startCue);
		InputString("End cue", tr.endCue);
		ImGui::BeginDisabled(IsPlayingTransition());
		if (ImGui::Button("移行演出を再生")) {
			boss_->PlayTransitionPreview(phase_);
			previewingTransition_ = true;
			phaseChanged_ = true;
		}
		ImGui::EndDisabled();
		if (phase_ == 0) {
			ImGui::SameLine();
			ImGui::TextDisabled("最初の形態は移行しないので確認用");
		}
	}

	// ---- アニメーション（仕組みだけ） ----
	if (ImGui::CollapsingHeader("Animation (アニメーション名の対応)")) {
		BossAnimationMap& anim = phase.animations;
		ImGui::TextDisabled("空なら指示を出さない。再生する側はまだ無いので、名前の対応だけ決めておく");
		InputString("Idle", anim.idle);
		InputString("Transition", anim.transition);
		InputString("Defeated", anim.defeated);
		for (const std::string& id : BossAttackRegistry::GetIds()) {
			ImGui::PushID(id.c_str());
			if (ImGui::TreeNode(id.c_str())) {
				BossAttackAnimation& a = anim.attacks[id];
				InputString("Telegraph", a.telegraph);
				InputString("Attack", a.attack);
				InputString("Recovery", a.recovery);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		// 中身が全部空の攻撃は保存しない
		for (auto it = anim.attacks.begin(); it != anim.attacks.end();) {
			if (it->second.telegraph.empty() && it->second.attack.empty() && it->second.recovery.empty()) {
				it = anim.attacks.erase(it);
			} else {
				++it;
			}
		}
		ImGui::Text("いまの指示 : %s", boss_->GetCurrentAnimation().empty() ? "-" : boss_->GetCurrentAnimation().c_str());
	}

	// ---- ボス共通 ----
	if (ImGui::CollapsingHeader("Common (ボス共通)")) {
		InputString("Defeat cue (撃破の合図)", data.common.defeatCue);
	}

	// ---- 記録 ----
	ImGui::SeparatorText("届いた合図・アニメーションの指示");
	if (ImGui::Button("記録を消す")) {
		log_.clear();
	}
	for (const std::string& line : log_) {
		ImGui::TextUnformatted(line.c_str());
	}

	ImGui::End();
#endif
}
