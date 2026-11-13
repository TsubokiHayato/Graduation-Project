#pragma once
#include "BossAttackState.h"
#include <vector>

// =============================================================================
//  BossAttackBarrage（弾幕射撃）
//  予兆 ：その場で砲門を展開する。最初の一斉射の弾道を赤く警告表示
//  攻撃 ：中央から放射状に複数の弾を撃つ。これを「発射回数」回、「発射間隔」ごとにくり返す
//         一斉射ごとに角度をずらすので、弾の隙間へ回り込んで避ける
//  硬直 ：短い
//
//  当たり判定（4-7）：弾ごとに、自機との距離 ≤ 弾の大きさ＋自機の大きさ
//  ダメージは弾1発ごと。当たった弾は消える
// =============================================================================
class BossAttackBarrage : public BossAttackState {
public:
	// 弾幕射撃の固有パラメータ（すべて要調整）
	struct Params {
		int bulletCount = 16;        // 弾数（1回の発射で放射状に出す数）
		float bulletSpeed = 8.0f;    // 弾速
		float bulletSize = 0.5f;     // 弾の大きさ（当たり判定の半径）
		int volleyCount = 3;         // 発射回数
		float volleyInterval = 0.6f; // 発射間隔（秒）
		// --- 演出／配置 ---
		float volleyAngleShift = 0.5f; // 一斉射ごとの角度のずらし（弾と弾の間隔に対する割合。0.5 で隙間の真ん中）
		float portLength = 1.5f;       // 予兆で展開する砲門の長さ
	};

public:
	BossAttackBarrage();

	const char* GetName() const override { return "弾幕射撃 (Barrage)"; }
	void DrawLines(const Boss& boss) const override;

	Params& GetParams() { return params_; }
	int GetHitCount() const { return hitCount_; }

protected:
	void OnEnter(Boss& boss) override;
	void UpdateTelegraph(Boss& boss, float t) override;
	void UpdateAttack(Boss& boss, float t) override;
	void UpdateRecovery(Boss& boss, float t) override;
	void OnExit(Boss& boss) override;
	// 攻撃段階は「最後の発射」＋「最後の弾がステージ端に届くまで」
	float GetAttackDuration() const override;
	void DrawImGuiParams() override;

private:
	struct Bullet {
		TuboEngine::Math::Vector3 position;
		TuboEngine::Math::Vector3 velocity;
	};

	// i 回目の一斉射の基準角度
	float VolleyBaseAngle(int volley) const;
	void FireVolley(const Boss& boss, int volley);
	void UpdateBullets(Boss& boss);

	Params params_;
	std::vector<Bullet> bullets_;
	int firedVolleys_ = 0;      // 発射済みの回数
	float baseAngle_ = 0.0f;    // 攻撃開始時のボスの向き（弾の並びの基準）
	float stageRadius_ = 20.0f; // 攻撃開始時に Boss から受け取る
	int hitCount_ = 0;          // 今回の攻撃で当たった弾の数（デバッグ表示）
};
