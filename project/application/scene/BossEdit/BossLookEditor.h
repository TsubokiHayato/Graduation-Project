#pragma once
#include <deque>
#include <string>
#include <vector>

class Boss;

// =============================================================================
//  BossLookEditor
//  ボスの見た目・演出を形態ごとに調整する ImGui の画面（BossEdit シーンの「Boss Look」ウィンドウ）。
//
//  ・形態を選ぶと、ボスがすぐその形態の見た目になる（タイムラインのプレビューもその形態で再生される）
//  ・モデル・テクスチャは Resources/Models・Resources/Textures のファイル一覧から選ぶ
//  ・モデルの補正（位置のずれ・向き）、材質（光沢・映り込み）、攻撃の線の色（effectTint）、コアの見た目
//  ・移行演出の中身と「移行演出を再生」ボタン、演出の合図の名前
//  ・アニメーション名の対応（仕組みだけ。いま指示されているアニメーション名を表示する）
//  ・ボスから届いた演出の合図の記録
//
//  データは Boss::GetData() を直接書き換える。保存は Boss Edit ウィンドウの Save。
// =============================================================================
class BossLookEditor {
public:
	// boss の演出の合図・アニメーションの指示を受け取って記録する（コールバックを設定する）
	void Initialize(Boss* boss);

	void DrawImGui();

	// 移行演出を再生中か（その間はタイムラインのプレビューを止めておく）
	bool IsPlayingTransition() const;
	// 移行演出の再生が終わった瞬間に true を返す（タイムラインのプレビューを計算し直すため）
	bool ConsumeTransitionFinished();
	// 形態を切り替えたら true を返す（タイムラインのプレビューを計算し直すため）
	bool ConsumePhaseChanged();

private:
	// ファイルの一覧を作り直す
	void RefreshFileLists();
	// 一覧から選ぶコンボボックス。emptyLabel があれば先頭に「空」の選択肢を出す
	bool FileCombo(const char* label, std::string& value, const std::vector<std::string>& files, const char* emptyLabel = nullptr);
	// std::string を編集する入力欄
	static bool InputString(const char* label, std::string& value);

	void AddLog(const std::string& text);

private:
	Boss* boss_ = nullptr;
	int phase_ = 0;

	std::vector<std::string> models_;   // Resources/Models からの相対パス（.obj / .gltf）
	std::vector<std::string> textures_; // Resources/Textures からの相対パス（.png など）

	bool previewingTransition_ = false;
	bool transitionFinished_ = false;
	bool phaseChanged_ = false;

	std::deque<std::string> log_; // 届いた演出の合図・アニメーションの指示（新しいものが上）
};
