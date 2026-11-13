#pragma once
#include "BossHitShapes.h"

// =============================================================================
//  攻撃予兆（警告範囲）の進行管理
//  - 警告表示と当たり判定は同じ Shape から作る（GP-12 の完了条件）
//  - 描画は含まない。GetShape()/GetFillShape()/GetProgress() を読んで描く
// =============================================================================
namespace BossHit {

class Warning {
public:
	enum class Phase { Idle, Telegraph, Hit, Done };

	// telegraphSec: 塗りが外枠に届くまで / hitSec: 攻撃判定が出ている時間
	void Start(const Shape& shape, float telegraphSec, float hitSec = 0.1f);
	void Cancel() { phase_ = Phase::Idle; }

	// dt には速度倍率（GP-15 のチュートリアル用）を掛けた値を渡す
	void Update(float dt);

	Phase GetPhase() const { return phase_; }
	bool IsActive() const { return phase_ == Phase::Telegraph || phase_ == Phase::Hit; }
	bool IsHitActive() const { return phase_ == Phase::Hit; } // 判定が出ている間
	bool JustHit() const { return justHit_; }                 // 判定が出た最初の1フレームだけ true（多段ヒット防止に）
	bool IsFinished() const { return phase_ == Phase::Done; }

	// 0〜1。判定フェーズでは 1
	float GetProgress() const;
	// 判定フェーズ中の進行度 0〜1（判定が出ている時間に対する経過割合）。それ以外は 0
	float GetHitProgress() const;

	const Shape& GetShape() const { return shape_; } // 外枠 = 実際の当たり判定
	Shape GetFillShape() const;                      // 広がっていく塗り

private:
	Shape shape_ = Circle{};
	Phase phase_ = Phase::Idle;
	float telegraphSec_ = 1.0f;
	float hitSec_ = 0.1f;
	float elapsed_ = 0.0f;
	bool justHit_ = false;
};

} // namespace BossHit
