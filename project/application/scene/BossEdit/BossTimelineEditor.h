#pragma once
#include "Boss/BossData.h"
#include "JsonManager.h"
#include "Vector3.h"
#include <memory>
#include <string>

class Boss;
class BossAttackState;

// =============================================================================
//  BossTimelineEditor
//  ボスの攻撃パターンをタイムラインで作る ImGui の編集画面（BossEdit シーンで使う）。
//
//  ・Boss Edit     ：パターンの一覧（追加・複製・削除・名前の変更）、硬直時間、使う形態、的の位置、保存
//  ・Boss Timeline ：攻撃（クリップ）を時刻に並べる。再生・一時停止、目盛りのドラッグで好きな時刻へ移動
//      - 空いている所を右クリック → 攻撃を追加
//      - クリップをドラッグ → 開始時刻（左右）と段（上下）を変える。Shift を押しながらで吸着しない
//      - クリップを右クリック → 複製・ボスを動かす・削除。Delete キーでも削除
//      - 硬直の帯の右端をドラッグ → 硬直時間を変える
//  ・Clip Inspector：選んだクリップの数値。変えた項目だけクリップに保存し、残りは攻撃の標準の数値を使う
//
//  データは Boss::GetData() を直接書き換え、Save で Resources/Boss.json に保存する。
// =============================================================================
class BossTimelineEditor {
public:
	BossTimelineEditor();
	~BossTimelineEditor();

	void Initialize(Boss* boss);

	// ボスの Update の前に毎フレーム呼ぶ（プレビューの再生・時刻の移動をボスに反映する）
	void Update();

	void DrawImGui();

	// プレビューで攻撃を当てる的（自機の代わり）の位置
	TuboEngine::Math::Vector3 GetTargetPosition() const;
	float GetTargetRadius() const { return targetRadius_; }

	// プレビューを今の時刻で計算し直す（ボスの形態や見た目を外で変えたときにも呼ぶ）
	void MarkDirty() { dirty_ = true; }

private:
	// ---- 各ウィンドウ ----
	void DrawPatternWindow();
	void DrawTimelineWindow();
	void DrawInspectorWindow();

	// タイムラインの描画と操作
	void DrawTimelineCanvas(BossPatternData& pattern);

	// ---- パターンの操作 ----
	BossPatternData* GetSelectedPattern();
	void SelectPattern(int index);
	std::string MakeUniqueName(const std::string& base) const;
	bool IsNameUsed(const std::string& name, int ignoreIndex) const;
	void RenamePattern(int index, const std::string& newName);
	void DeletePattern(int index);
	void AddClip(BossPatternData& pattern, const std::string& attackId, float start, int track);

	// クリップの開始時刻を吸着させる
	float Snap(float time) const;

	// ---- クリップの数値 ----
	// 標準の数値との差（変えた項目だけ）
	static TuboEngine::JsonManager::Json Diff(const TuboEngine::JsonManager::Json& base, const TuboEngine::JsonManager::Json& now);

private:
	Boss* boss_ = nullptr;

	// 選択
	int selectedPattern_ = -1;
	int selectedClip_ = -1;
	char nameBuffer_[64] = {};

	// プレビュー
	bool playing_ = false;
	float playhead_ = 0.0f;
	bool dirty_ = true;

	// タイムラインの表示
	float pixelsPerSecond_ = 90.0f;
	bool snap_ = true;
	float snapStep_ = 0.05f;

	// ドラッグ中のクリップ
	int dragClip_ = -1;
	float dragGrabOffset_ = 0.0f; // クリップの開始時刻から、つかんだ位置までの時間

	// 右クリックで攻撃を追加する位置
	float addTime_ = 0.0f;
	int addTrack_ = 0;

	// Clip Inspector で数値を編集するための攻撃（選んだクリップが変わったら作り直す）
	std::unique_ptr<BossAttackState> inspectAttack_;
	int inspectPattern_ = -1;
	int inspectClip_ = -1;
	std::string inspectAttackId_;

	// 的（自機の代わり）
	float targetDistance_ = 10.0f;
	float targetAngleDeg_ = -90.0f; // -90° でカメラの手前
	float targetRadius_ = 0.5f;

	std::string message_;
};
