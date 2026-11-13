#pragma once
#include "Vector3.h"
#include "Vector4.h"

class Boss;

// =============================================================================
//  BossAttackState
//  ボスの攻撃ステートの基底クラス。
//  仕様書「4-3. 行動の基本サイクル」の 予兆 → 攻撃 → 硬直 の進行をここで共通化し、
//  攻撃ごとの動きは派生クラスで各段階の処理（UpdateTelegraph など）を実装する。
// =============================================================================
class BossAttackState {
public:
	// 攻撃中の段階
	enum class Step {
		Telegraph, // 予兆
		Attack,    // 攻撃
		Recovery,  // 硬直
		Finished,  // 終了（ボスが次の攻撃を選ぶ）
	};

	// すべての攻撃が持つ共通パラメータ（仕様書「調整パラメータ／攻撃ごと」）
	struct CommonParams {
		float telegraphTime = 1.0f; // 予兆時間（秒）
		float damage = 10.0f;       // ダメージ
		float recoveryTime = 1.0f;  // 硬直時間（秒）
	};

public:
	virtual ~BossAttackState() = default;

	// 攻撃の名前（ImGui 表示用）
	virtual const char* GetName() const = 0;

	// 攻撃開始時に呼ぶ（予兆から始まる）
	void Enter(Boss& boss);
	// 毎フレーム呼ぶ
	void Update(Boss& boss, float deltaTime);
	// 警告範囲などのライン描画（Update の後に呼ぶ）
	virtual void DrawLines(const Boss& boss) const { (void)boss; }
	// パラメータ調整用
	void DrawImGui();

	bool IsFinished() const { return step_ == Step::Finished; }
	Step GetStep() const { return step_; }
	float GetStepTimer() const { return stepTimer_; }
	CommonParams& GetCommonParams() { return common_; }

protected:
	// ---- 派生クラスで実装する各段階の処理 ----
	// t は各段階の進み具合（0 → 1）
	virtual void OnEnter(Boss& boss) { (void)boss; }
	virtual void OnStepEnter(Boss& boss, Step step) { (void)boss; (void)step; }
	virtual void UpdateTelegraph(Boss& boss, float t) = 0;
	virtual void UpdateAttack(Boss& boss, float t) = 0;
	virtual void UpdateRecovery(Boss& boss, float t) { (void)boss; (void)t; }
	virtual void OnExit(Boss& boss) { (void)boss; }

	// 予兆段階の長さ。既定は共通パラメータの予兆時間
	// （ロックオンビームのように「追尾時間＋回避猶予」で決まる攻撃は上書きする）
	virtual float GetTelegraphDuration() const { return common_.telegraphTime; }
	// 予兆時間を共通パラメータで調整するか（ImGui の表示切り替え用）
	virtual bool UsesCommonTelegraphTime() const { return true; }
	// 攻撃段階の長さ（ビームの照射時間など攻撃ごとに違う）
	virtual float GetAttackDuration() const = 0;
	// 派生クラス固有パラメータの ImGui
	virtual void DrawImGuiParams() {}

	// ---- 派生クラス向けの補助 ----
	// 現在の段階の進み具合（0 → 1）
	float StepProgress(float duration) const;
	// 今フレームの経過時間（速度倍率を反映済み）
	float GetDeltaTime() const { return deltaTime_; }
	// 地面に水平な円をラインで描く
	static void DrawGroundCircle(const TuboEngine::Math::Vector3& center, float radius, const TuboEngine::Math::Vector4& color, int divisions = 96);

	CommonParams common_;

private:
	void ChangeStep(Boss& boss, Step next);
	float GetStepDuration(Step step) const;

	Step step_ = Step::Finished;
	float stepTimer_ = 0.0f;
	float deltaTime_ = 0.0f;
};
