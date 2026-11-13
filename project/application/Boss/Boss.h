#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "Vector4.h"
#include <functional>
#include <memory>
#include <vector>

namespace TuboEngine {
class Camera;
}
class BossAttackState;
class BossAttackOmniBomb;
class BossAttackLockOnBeam;
class BossAttackBarrage;
class BossAttackSweep;

// =============================================================================
//  Boss
//  仕様書「4. ボス」のボス本体。
//  見た目のオブジェクト（block）を持ち、攻撃の選択と攻撃ステートの切り替えを行う。
//  各攻撃の動き（予兆 → 攻撃 → 硬直）は BossAttackState の派生クラスに分けている。
//
//  実装済みの攻撃： 全方位爆撃（BossAttackOmniBomb）、ロックオンビーム（BossAttackLockOnBeam）、
//                   弾幕射撃（BossAttackBarrage）、近接薙ぎ払い（BossAttackSweep）
// =============================================================================
class Boss {

public:

	// 攻撃が自機に当たったときに呼ばれる（引数はダメージ）。シールド軽減は受け取る側で行う
	using HitCallback = std::function<void(float damage)>;

public:

	Boss();

	~Boss();

	//オブジェクト・ステートの生成、ステート関連変数の初期化を行う
	void Initialize(TuboEngine::Camera* camera, const TuboEngine::Math::Vector3& center);

	//オブジェクト・ステートの更新
	void Update(float deltaTime);

	//オブジェクトの描画
	void Draw();

	//行動サイクルの表示、各ステートのパラメータ調節
	void DrawImGui();

	// ---- シーンから渡す情報 ----

	// 自機の位置と大きさ（当たり判定用）。毎フレーム Update の前に渡す
	void SetTarget(const TuboEngine::Math::Vector3& position, float radius) {
		targetPosition_ = position;
		targetRadius_ = radius;
	}

	//接触時の処理を設定
	void SetHitCallback(HitCallback callback) { onHit_ = std::move(callback); }

	//ステージの広さを設定
	void SetStageRadius(float radius) { stageRadius_ = radius; }

	// チュートリアル時の速度倍率（1 未満でゆっくり）
	void SetSpeedScale(float scale) { speedScale_ = scale; }

	// ---- 攻撃ステートから使う操作 ----

	// 予備動作によるY軸の移動などで使用
	void SetHeight(float height) { height_ = height; } 
	float GetHeight() const { return height_; }

	//攻撃時の警告色への設定などで使用
	void SetColor(const TuboEngine::Math::Vector4& color) { color_ = color; }

	// ボスの向きを「ボスの周りの角度」で設定する（X軸 +方向が 0、Z軸 +方向が π/2）
	void SetFacingAngle(float angle);
	float GetFacingAngle() const;

	// 自機の方へ向きを変える。旋回速度の上限より速くは回らない
	void TurnTowardTarget(float deltaTime);

	void NotifyHitTarget(float damage) {
		if (onHit_) {
			onHit_(damage);
		}
	}

	// ---- 取得 ----

	const TuboEngine::Math::Vector3& GetCenter() const { return center_; }

	const TuboEngine::Math::Vector3& GetTargetPosition() const { return targetPosition_; }

	float GetTargetRadius() const { return targetRadius_; }

	float GetStageRadius() const { return stageRadius_; }

	float GetYaw() const { return yaw_; }

	TuboEngine::Object3d* GetObject3d() const { return object_.get(); }

private:

	// 次の攻撃ステートの選出。現状は初期化時の登録順を繰り返す。
	void SelectNextAttack();

	// 攻撃ステートの切り替え。
	void ChangeAttack(BossAttackState* next);

	//座標・回転・色の適用
	void ApplyObjectTransform();

private:

	// ボスのオブジェクト
	std::unique_ptr<TuboEngine::Object3d> object_;

	TuboEngine::Math::Vector3 center_ = {0.0f, 0.0f, 0.0f};      // ステージ中央（ボスの基準位置）

	float baseHeight_ = 1.0f;                                    // 通常時の高さ（ブロックが地面に乗る高さ）

	float height_ = 0.0f;                                        // 基準からの上昇量

	float yaw_ = 0.0f;                                           // 向き（Object3d の Y 回転）

	TuboEngine::Math::Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // 色

	// 攻撃の一覧（この順にくり返す）
	std::vector<BossAttackState*> attacks_;

	//現在の攻撃ステート
	BossAttackState* currentAttack_ = nullptr;

	// 攻撃ステート（各攻撃は使い回し、Enter で初期化する）
	std::unique_ptr<BossAttackOmniBomb> omniBomb_;

	std::unique_ptr<BossAttackLockOnBeam> lockOnBeam_;

	std::unique_ptr<BossAttackBarrage> barrage_;

	std::unique_ptr<BossAttackSweep> sweep_;

	//攻撃ステートの予約番号
	size_t nextAttackIndex_ = 0;

	// 自機の情報
	TuboEngine::Math::Vector3 targetPosition_ = {0.0f, 0.0f, 0.0f};

	float targetRadius_ = 0.5f;

	HitCallback onHit_;

	float stageRadius_ = 20.0f;

	float speedScale_ = 1.0f;

	float turnSpeed_ = 1.2f; // 旋回速度（自機の方へ向きを変える速さの上限、ラジアン/秒）

	// デバッグ
	int deForcedAttackIndex_ = -1; //-1 以外ならその攻撃だけを出す

	bool dePaused_ = false;

	bool deDrawLines_ = true;
};
