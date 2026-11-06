#pragma once
#include "BossAttackState.h"

// =============================================================================
//  BossAttackSweep（近接薙ぎ払い）
//  予兆 ：腕を大きく振りかぶる。ボスは旋回速度の上限つきで自機の方へ向きを変え続ける
//         ボス前方の扇形（＝実際の判定範囲）を赤く警告表示
//  攻撃 ：向きを固定して腕を振り抜く。振っている間に扇形の中にいたら1回だけダメージ
//  硬直 ：中程度
//
//  当たり判定（4-7）：距離 ≤ 扇の半径、かつボスの向きとの角度差 ≤ 扇の角度の半分
//  回避：離れる（扇の半径の外）、背後へ回る（旋回が追いつかない速さで回り込む）、シールドで受ける
// =============================================================================
class BossAttackSweep : public BossAttackState {
public:
	// 近接薙ぎ払いの固有パラメータ（すべて要調整）
	struct Params {
		float radius = 7.0f;     // 扇の半径（最小距離より大きく、ステージ半径より小さく）
		float angleDeg = 120.0f; // 扇の角度（度）
		// --- 演出 ---
		float swingTime = 0.3f;   // 腕を振り抜く時間（秒）。この間が判定の出ている時間
		float windUpDeg = 40.0f;  // 振りかぶりで扇の端からさらに後ろへ引く角度（度）
	};

public:
	BossAttackSweep();

	const char* GetName() const override { return "近接薙ぎ払い (Sweep)"; }
	void DrawLines(const Boss& boss) const override;

	Params& GetParams() { return params_; }
	bool GetLastHitResult() const { return lastHitResult_; }

protected:
	void OnEnter(Boss& boss) override;
	void OnStepEnter(Boss& boss, Step step) override;
	void UpdateTelegraph(Boss& boss, float t) override;
	void UpdateAttack(Boss& boss, float t) override;
	void UpdateRecovery(Boss& boss, float t) override;
	void OnExit(Boss& boss) override;
	float GetAttackDuration() const override { return params_.swingTime; }
	void DrawImGuiParams() override;

private:
	// 当たり判定（4-7）
	bool IsHit(const Boss& boss) const;
	// 腕の角度（ボスの向きからのずれ、ラジアン）
	float ArmOffset() const;
	// 扇形の輪郭を描く
	void DrawFan(const Boss& boss, float facing, float radius, const TuboEngine::Math::Vector4& color) const;

	Params params_;
	bool hasHit_ = false;
	bool lastHitResult_ = false;
};
