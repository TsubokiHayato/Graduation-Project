#pragma once
#include "BossAttackState.h"

// =============================================================================
//  BossAttackLockOnBeam（ロックオンビーム）
//  仕様書「4-5. ロックオンビームの流れ」
//  予兆（前半）追尾     ：照準が自機の角度を追いかける
//  予兆（後半）ロック確定：照準が止まり、ビームの範囲を赤く警告表示する（回避猶予）
//  攻撃        発射     ：ロックした方向に、中央からステージ端までビームを照射する
//  硬直                 ：中程度
//
//  当たり判定（4-7）：ロックした方向からの横のずれ ≤ ビーム幅の半分＋自機の大きさ
//  （ボスより後ろ側には出ないので、方向の前側にいるときだけ当たる）
// =============================================================================
class BossAttackLockOnBeam : public BossAttackState {
public:
	// ロックオンビームの固有パラメータ（すべて要調整）
	struct Params {
		float trackingTime = 1.5f; // 追尾時間（照準が自機を追う長さ。予兆の前半）
		float evadeTime = 0.8f;    // 回避猶予（ロック確定から発射まで。予兆の後半）
		float beamWidth = 2.0f;    // ビーム幅
		float fireTime = 1.0f;     // 照射時間（判定が続く長さ）
		// --- 演出 ---
		float trackingSpeed = 3.0f; // 照準が自機の角度を追う速さ（ラジアン/秒）
	};

public:
	BossAttackLockOnBeam();

	const char* GetName() const override { return "ロックオンビーム (LockOnBeam)"; }
	void DrawLines(const Boss& boss) const override;

	Params& GetParams() { return params_; }
	bool GetLastHitResult() const { return lastHitResult_; }
	bool IsLocked() const;
	float GetAimAngle() const { return aimAngle_; }

protected:
	void OnEnter(Boss& boss) override;
	void OnStepEnter(Boss& boss, Step step) override;
	void UpdateTelegraph(Boss& boss, float t) override;
	void UpdateAttack(Boss& boss, float t) override;
	void UpdateRecovery(Boss& boss, float t) override;
	void OnExit(Boss& boss) override;
	float GetTelegraphDuration() const override { return params_.trackingTime + params_.evadeTime; }
	bool UsesCommonTelegraphTime() const override { return false; }
	float GetAttackDuration() const override { return params_.fireTime; }
	void DrawImGuiParams() override;

private:
	// 自機がボスの周りのどの角度にいるか
	static float TargetAngle(const Boss& boss);
	// 当たり判定（4-7）
	bool IsHit(const Boss& boss) const;
	// ビームの帯（長方形）の輪郭を描く
	void DrawBand(const Boss& boss, float width, float y, const TuboEngine::Math::Vector4& color) const;

	Params params_;
	float aimAngle_ = 0.0f; // 照準の角度（ラジアン。X軸 +方向が 0、Z軸 +方向が π/2）
	bool hasHit_ = false;   // 今回の照射で既に当たったか（1回の照射で1回だけダメージ）
	bool lastHitResult_ = false;
};
