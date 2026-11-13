#pragma once
#include "BossAttackState.h"

// =============================================================================
//  BossAttackOmniBomb（全方位爆撃）
//  予兆 ：光りながら上昇。ステージ全域（爆撃の半径）を赤く警告表示
//  攻撃 ：爆発。入った瞬間に一度だけ「距離 ≤ 爆撃の半径」で判定し、衝撃波が広がる
//  硬直 ：出力が落ちて暗くなり、元の高さへ降りて止まる（長い）
// =============================================================================
class BossAttackOmniBomb : public BossAttackState {
public:
	// 全方位爆撃の固有パラメータ（すべて要調整）
	struct Params {
		float radius = 25.0f;    // 爆撃の半径（ステージ半径以上にすること）
		// --- 演出 ---
		float riseHeight = 4.0f; // 予兆で上昇する高さ
		float blastTime = 0.5f;  // 爆発（衝撃波が広がる）時間（秒）
		float landTime = 0.6f;   // 硬直に入ってから元の高さへ降りる時間（秒）
	};

public:
	BossAttackOmniBomb();

	const char* GetName() const override { return "全方位爆撃 (OmniBomb)"; }
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
	float GetAttackDuration() const override { return params_.blastTime; }
	void DrawImGuiParams() override;

private:
	// 当たり判定（4-7）：距離 ≤ 爆撃の半径
	bool IsHit(const Boss& boss) const;

	Params params_;
	bool lastHitResult_ = false;
};
