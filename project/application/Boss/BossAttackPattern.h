#pragma once
#include "BossData.h"
#include <memory>
#include <string>
#include <vector>

class Boss;
class BossAttackState;

// =============================================================================
//  BossAttackPattern
//  ボスが「1回の行動」として出す攻撃パターン（タイムライン）。抽選（重み付きランダム）の単位になる。
//
//  ・タイムラインに置いた攻撃（クリップ）は、それぞれの開始時刻になったら始まる
//    → 開始時刻をそろえれば同時攻撃、前の攻撃の終わりに合わせれば連続技、少しずらすこともできる
//  ・クリップごとに攻撃のインスタンスを持つので、同じ攻撃を何回置いてもよく、数値もクリップごとに変えられる
//    （変えた項目だけ overrides に持ち、残りは攻撃の標準の数値を使う）
//  ・硬直はパターンの最後に1回だけ：いちばん最後に終わる攻撃だけが、パターンの硬直時間だけ硬直する
//  ・ボスの向き・高さ・色を動かすのは「ボスを動かす」にしたクリップだけ。どれも動かしていない間は自機の方へ向き直る
//
//  攻撃名をそのまま使う単発のパターンは、攻撃1つを 0 秒に置き、硬直はその攻撃の標準の硬直時間を使う。
// =============================================================================
class BossAttackPattern {
public:
	// クリップ1つ分の長さ（タイムラインの表示用）
	struct ClipTiming {
		float start = 0.0f;
		float telegraph = 0.0f; // 予兆
		float attack = 0.0f;    // 攻撃
		float End() const { return start + telegraph + attack; }
	};

public:
	// data からパターンを作る。useAttackRecovery が true なら、硬直は最後の攻撃の標準の硬直時間を使う（単発用）
	BossAttackPattern(const BossPatternData& data, bool useAttackRecovery = false);
	~BossAttackPattern();

	// 行動を始める（クリップの数値をボスの標準の数値＋overrides で作り直す）
	void Enter(Boss& boss);
	// 毎フレーム呼ぶ
	void Update(Boss& boss, float deltaTime);
	// 途中で打ち切る（形態移行・撃破時）。弾なども消す
	void Cancel(Boss& boss);
	// 警告範囲などのライン描画
	void DrawLines(const Boss& boss) const;

	bool IsFinished() const { return finished_; }
	// 使える状態か（知らない攻撃名がある・攻撃が1つも無い、などは使えない）
	bool IsValid() const { return error_.empty(); }
	const std::string& GetError() const { return error_; }
	// 外で見つけた問題（名前の重複など）を設定する。すでに問題があればそちらを残す
	void SetError(std::string error);

	const std::string& GetName() const { return data_.name; }
	const BossPatternData& GetData() const { return data_; }
	// パターンの始まりからの経過時間
	float GetTime() const { return time_; }
	// いまボスを動かしている攻撃（無ければ動いている攻撃のどれか。無ければ nullptr）
	BossAttackState* GetCurrentAttack() const;
	// クリップの攻撃（ImGui 表示用）
	size_t GetClipCount() const { return clips_.size(); }
	const BossAttackState* GetClipAttack(size_t index) const;

	// クリップの長さを、ボスの標準の数値＋overrides で計算する（エディタの表示用）
	static ClipTiming ComputeClipTiming(const Boss& boss, const BossTimelineClip& clip);
	// パターン全体の長さ（攻撃の終わり＋硬直）
	static float ComputeTotalDuration(const Boss& boss, const BossPatternData& data);

private:
	struct Clip {
		std::unique_ptr<BossAttackState> attack;
		bool entered = false;
	};

	// 開始時刻になったクリップを始める
	void StartDueClips(Boss& boss);

	BossPatternData data_;
	bool useAttackRecovery_ = false;
	std::vector<Clip> clips_; // data_.clips と同じ順
	std::string error_;

	float time_ = 0.0f;
	bool finished_ = true;
};
