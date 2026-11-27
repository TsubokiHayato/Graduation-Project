#pragma once
#include "BossData.h"
#include "JsonManager.h"
#include "Object3d.h"
#include "Vector3.h"
#include "Vector4.h"
#include <deque>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace TuboEngine {
class Camera;
}
class BossAttackState;
class BossAttackPattern;

// JSONファイルのパス
inline constexpr char kBossJsonPath[] = "Resources/Boss.json";

// =============================================================================
//  Boss
//  仕様書「4. ボス」のボス本体。
//  見た目のオブジェクトを持ち、行動サイクル・攻撃の抽選・形態（フェーズ）・HPとコア判定を行う。
//
//  行動サイクル：待機（自機の方を向く）→ 攻撃（予兆 → 攻撃 → 硬直）→ 待機 → …
//    ・攻撃は BossAttackState の派生クラス。BossAttackRegistry に並んだものをすべて持つ
//    ・1回の行動は BossAttackPattern（攻撃を時刻に並べたタイムライン）。形態ごとの重み付きランダムで選ぶ
//      タイムラインは BossEdit シーンで作る
//
//  形態：Resources/Boss.json の "phases" を上から順に進む。
//    ・次の形態のきっかけ（いまは HP の割合）を満たしたら、攻撃をすぐに打ち切り、弾も消して移行演出へ
//    ・移行演出の間は無敵。演出の途中で見た目（モデル・色・大きさ）が切り替わる
//    ・HP は1本。次の形態の閾値を超えた分のダメージは持ち越さない
//
//  HP とコア：TakeDamage に攻撃してきた位置を渡す。背中のコアの向きからのときだけ HP が減る
//
//  数値はすべて Resources/Boss.json（ImGui で調整して Save）。攻撃パターンは BossEdit シーン、形態は JSON で組み立てる
// =============================================================================
class Boss {

public:

	// 攻撃が自機に当たったときに呼ばれる（引数はダメージ）。シールド軽減は受け取る側で行う
	using HitCallback = std::function<void(float damage)>;
	// 形態が変わったとき（移行演出の始まり）に呼ばれる（引数は形態の番号と名前）
	using PhaseChangedCallback = std::function<void(int phaseIndex, const std::string& phaseName)>;
	// 撃破されたときに呼ばれる
	using DefeatedCallback = std::function<void()>;
	// 攻撃（パターン）を始めたときに呼ばれる（引数はパターン名。チュートリアルの指示表示などに使う）
	using AttackStartedCallback = std::function<void(const std::string& patternName)>;

	// 演出の合図（ライティング・音・カメラなどの制御側が受け取って演出を始める。ボス自身は光などを持たない）
	struct Cue {
		enum class Type {
			TransitionStart, // 形態の移行演出が始まった
			TransitionEnd,   // 形態の移行演出が終わった
			Defeated,        // 撃破された
		};
		Type type;
		std::string name;  // 形態・ボス共通のデータで決めた合図の名前（空のこともある）
		int phaseIndex;    // 関係する形態（移行先の形態。撃破ならそのときの形態）
	};
	using CueCallback = std::function<void(const Cue& cue)>;

	// アニメーションの指示（再生する側が受け取る。duration はその状態の長さで、再生速度を合わせるのに使える。0 なら長さが決まっていない）
	using AnimationCallback = std::function<void(const std::string& animationName, float duration)>;

	// ボスの状態
	enum class State {
		Idle,       // 待機（次の攻撃の前に自機の方を向く）
		Acting,     // 攻撃中（予兆・攻撃・硬直）
		Transition, // 形態の移行演出中（無敵）
		Defeated,   // 撃破
	};

	// TakeDamage の結果
	enum class DamageResult {
		Hit,     // コアに当たった（HP が減った）
		Blocked, // コア以外の向きから当たったので弾いた
		Ignored, // 無敵中・撃破後などで受け付けなかった
	};

public:

	Boss();

	~Boss();

	// オブジェクト・攻撃の生成、Resources/Boss.json の読み込み、戦闘の初期化を行う
	void Initialize(TuboEngine::Camera* camera, const TuboEngine::Math::Vector3& center);

	// 行動サイクル・形態の更新
	void Update(float deltaTime);

	// オブジェクトの描画
	void Draw();

	// 状態の表示、各種パラメータの調整、JSON の保存
	void DrawImGui();

	// 戦闘をはじめからやり直す（HP 満タン・最初の形態）
	void Restart();

	// ---- シーンから渡す情報 ----

	// 自機の位置と大きさ（当たり判定用）。毎フレーム Update の前に渡す
	void SetTarget(const TuboEngine::Math::Vector3& position, float radius) {
		targetPosition_ = position;
		targetRadius_ = radius;
	}

	// 攻撃が自機に当たったときの処理を設定
	void SetHitCallback(HitCallback callback) { onHit_ = std::move(callback); }
	// 形態が変わったときの処理を設定
	void SetPhaseChangedCallback(PhaseChangedCallback callback) { onPhaseChanged_ = std::move(callback); }
	// 撃破されたときの処理を設定
	void SetDefeatedCallback(DefeatedCallback callback) { onDefeated_ = std::move(callback); }
	// 攻撃を始めたときの処理を設定
	void SetAttackStartedCallback(AttackStartedCallback callback) { onAttackStarted_ = std::move(callback); }
	// 演出の合図を受け取る処理を設定
	void SetCueCallback(CueCallback callback) { onCue_ = std::move(callback); }
	// アニメーションの指示を受け取る処理を設定（状態が変わるたびに呼ばれる）
	void SetAnimationCallback(AnimationCallback callback) { onAnimation_ = std::move(callback); }

	// ステージの広さを設定
	void SetStageRadius(float radius) { stageRadius_ = radius; }

	// チュートリアル時の速度倍率（1 未満でゆっくり）
	void SetSpeedScale(float scale) { speedScale_ = scale; }

	// ---- 外からの指示（チュートリアルなど） ----

	// 次に出す攻撃（パターン名）を予約する。予約は抽選より優先し、予約順に出す
	void RequestPattern(const std::string& patternName) { requestedPatterns_.push_back(patternName); }
	// false にすると自分では攻撃を選ばず、予約されたときだけ攻撃する
	void SetAutoAttack(bool enabled) { autoAttack_ = enabled; }

	// ---- ダメージ ----

	// 自機の攻撃が当たったときに呼ぶ。attackerPosition は攻撃してきた位置（コアの向きの判定に使う）
	DamageResult TakeDamage(float damage, const TuboEngine::Math::Vector3& attackerPosition);

	// その位置からボスを攻撃したらコアに当たるか（背中の方向から左右にコアの角度幅の半分ずつ）
	bool IsInCoreArc(const TuboEngine::Math::Vector3& position) const;

	// ---- 攻撃ステートから使う操作 ----
	// ※ 同時攻撃の従が呼んだときは何もしない（ボスを動かすのは主だけ。SetBodyControlEnabled で切り替わる）

	// 予備動作によるY軸の移動などで使用
	void SetHeight(float height) {
		if (bodyControlEnabled_) {
			height_ = height;
		}
	}
	float GetHeight() const { return height_; }

	// 攻撃時の警告色への設定などで使用（形態の色に掛け合わせて表示する）
	void SetColor(const TuboEngine::Math::Vector4& color) {
		if (bodyControlEnabled_) {
			color_ = color;
		}
	}

	// ボスの向きを「ボスの周りの角度」で設定する（X軸 +方向が 0、Z軸 +方向が π/2）
	void SetFacingAngle(float angle);
	float GetFacingAngle() const;

	// 自機の方へ向きを変える。旋回速度の上限（形態ごと）より速くは回らない
	void TurnTowardTarget(float deltaTime);

	void NotifyHitTarget(float damage) {
		if (onHit_) {
			onHit_(damage);
		}
	}

	// 攻撃範囲・弾・ビームの線の色に、形態の effectTint を掛け合わせる（攻撃の DrawLines から使う）
	TuboEngine::Math::Vector4 TintEffect(const TuboEngine::Math::Vector4& color) const {
		return {color.x * look_.effectTint.x, color.y * look_.effectTint.y, color.z * look_.effectTint.z, color.w * look_.effectTint.w};
	}

	// 攻撃がボスを動かしてよいか（BossAttackState が攻撃の処理の間だけ切り替える）
	void SetBodyControlEnabled(bool enabled) { bodyControlEnabled_ = enabled; }
	bool IsBodyControlEnabled() const { return bodyControlEnabled_; }

	// ---- 取得 ----

	const TuboEngine::Math::Vector3& GetCenter() const { return center_; }

	const TuboEngine::Math::Vector3& GetTargetPosition() const { return targetPosition_; }

	float GetTargetRadius() const { return targetRadius_; }

	float GetStageRadius() const { return stageRadius_; }

	float GetYaw() const { return yaw_; }

	TuboEngine::Object3d* GetObject3d() const { return object_.get(); }

	State GetState() const { return state_; }

	float GetHp() const { return hp_; }

	float GetMaxHp() const { return data_.common.maxHp; }

	float GetHpRatio() const { return data_.common.maxHp > 0.0f ? hp_ / data_.common.maxHp : 0.0f; }

	bool IsInvincible() const { return state_ == State::Transition || state_ == State::Defeated; }

	bool IsDefeated() const { return state_ == State::Defeated; }

	int GetPhaseIndex() const { return phaseIndex_; }

	const BossPhaseData& GetPhase() const { return data_.phases[phaseIndex_]; }

	// ---- 攻撃の標準の数値 ----

	// 攻撃の標準の数値（"attacks" の中身と同じ形）。タイムラインのクリップはこれに overrides を重ねて使う
	TuboEngine::JsonManager::Json GetAttackBaseParams(const std::string& attackId) const;
	// 攻撃の種類ごとのインスタンス（標準の数値の編集用）
	const std::vector<std::unique_ptr<BossAttackState>>& GetAttacks() const { return attacks_; }

	// ---- データの編集（BossEdit シーン） ----

	// データを直接書き換える。書き換えたら ApplyDataChanges を呼ぶ
	BossData& GetData() { return data_; }
	// 書き換えたデータから抽選用のパターンを作り直す（プレビュー中のパターンはそのまま）
	void ApplyDataChanges();

	// ---- プレビュー（BossEdit シーン） ----

	// パターンを最初から再生する（ボスは自機の方を向いた状態から始める。自分では次の攻撃を選ばなくなる）
	void StartPreview(const BossPatternData& pattern);
	// パターンを最初から time 秒まで進めた状態にする（1/60 秒ずつ計算し直す）
	void SeekPreview(const BossPatternData& pattern, float time);
	// プレビューをやめて待機に戻る
	void StopPreview();
	bool IsPreviewing() const;
	// プレビュー中のパターンの経過時間
	float GetPreviewTime() const;

	// 見た目の確認用：その形態にすぐ切り替える（移行演出なし。攻撃は打ち切る）
	void ForcePhase(int phaseIndex);
	// 見た目の確認用：ひとつ前の形態から、その形態への移行演出を再生する（合図も出す）
	void PlayTransitionPreview(int phaseIndex);
	// いま出しているアニメーションの指示（確認用）
	const std::string& GetCurrentAnimation() const { return currentAnimation_; }
	float GetCurrentAnimationDuration() const { return currentAnimationDuration_; }

	// 一時停止（止まっている間も警告範囲は描く）
	void SetPaused(bool paused) { dePaused_ = paused; }
	bool IsPaused() const { return dePaused_; }

	// ---- データ（Resources/Boss.json） ----

	// 読み込んで戦闘をはじめからやり直す。ファイルが無ければコードの初期値を使う
	bool LoadData();
	// いまの数値を "current" として保存する（"default" は残す）
	bool SaveData();
	// "default" の数値に戻して保存する
	bool ResetDataToDefault();

private:

	// ---- 行動サイクル ----
	// 状態ごとの更新（描画・オブジェクトの更新は含まない。プレビューの早送りでも使う）
	void Simulate(float deltaTime);
	void UpdateIdle(float deltaTime);
	void UpdateActing(float deltaTime);
	void UpdateTransition(float deltaTime);
	void UpdateDefeated(float deltaTime);

	// 次の攻撃を選ぶ（予約 → デバッグ固定 → 形態の重み付きランダム）。出せるものが無ければ nullptr
	BossAttackPattern* SelectNextPattern();
	// 形態の候補から重み付きランダムで選ぶ（再使用までの回数も考える）
	BossAttackPattern* SelectFromPhase();
	void StartPattern(BossAttackPattern* pattern);
	void CancelPattern();
	void EnterIdle(float waitTime);

	// ---- 形態 ----
	void CheckPhaseTriggers();
	bool IsTriggerSatisfied(const BossPhaseTrigger& trigger) const;
	void StartTransition(int nextPhase);
	void Defeat();
	// 次の形態に入る HP（これより下には一度に削れない）。HP がきっかけでなければ 0
	float GetNextPhaseHpFloor() const;
	// 形態の数値（旋回の速さ・抽選の再使用回数）を反映
	void ResetPhaseRuntime();
	void ApplyLook(const BossLookData& look);

	// ---- データ ----
	BossAttackState* FindAttack(const std::string& id) const;
	BossAttackPattern* FindPattern(const std::string& name) const;
	// 攻撃を作り直し、JSON の数値を反映する
	void ApplyJson(const TuboEngine::JsonManager::Json& json);
	TuboEngine::JsonManager::Json BuildJson() const;
	// JSON の patterns と攻撃の一覧から、抽選に使うパターンを作る
	void RebuildPatterns();
	// 古い形式（連続技・同時攻撃）のパターンをタイムラインに変換する
	void ConvertLegacyPatterns();
	// 形態が1つも無いときは、すべての攻撃を使う形態を1つ作る
	void EnsurePhases();

	// 座標・回転・色の適用
	void ApplyObjectTransform();
	// 演出の合図を出す
	void EmitCue(Cue::Type type, const std::string& name, int phaseIndex);
	// 状態に合ったアニメーションの指示を出す（変わったときだけ）
	void UpdateAnimationRequest();
	// ボスの向きで回したオフセット（大きさに合わせて伸ばす）
	TuboEngine::Math::Vector3 LocalToWorldOffset(const TuboEngine::Math::Vector3& offset) const;
	// コアの向き（狙える範囲）をラインで描く
	void DrawCoreLines() const;
	// ImGui の各部分
	void DrawImGuiStatus();
	void DrawImGuiData();

private:

	// ボスのオブジェクト
	std::unique_ptr<TuboEngine::Object3d> object_;

	// 背中のコアのオブジェクト
	std::unique_ptr<TuboEngine::Object3d> core_;

	std::string appliedCoreModel_;                               // いまコアに使っているモデル

	std::string appliedTexture_;                                 // いまボスに使っているテクスチャ（空ならモデルのもの）

	TuboEngine::Math::Vector3 center_ = {0.0f, 0.0f, 0.0f};      // ステージ中央（ボスの基準位置）

	float height_ = 0.0f;                                        // 基準からの上昇量

	float yaw_ = 0.0f;                                           // 向き（Object3d の Y 回転）

	TuboEngine::Math::Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // 攻撃の演出の色

	BossLookData look_;                                          // いま表示している形態の見た目

	std::string appliedModel_;                                   // いま使っているモデル

	bool bodyControlEnabled_ = true;

	// ---- データ ----
	BossData data_;

	TuboEngine::JsonManager::Json defaultJson_; // "default" の中身

	// ---- 攻撃 ----
	std::vector<std::unique_ptr<BossAttackState>> attacks_;      // 攻撃の種類ごとに1つ

	std::vector<std::unique_ptr<BossAttackPattern>> patterns_;   // 抽選に使うパターン（単発＋タイムライン）

	std::unique_ptr<BossAttackPattern> previewPattern_;          // プレビュー中のパターン

	BossAttackPattern* currentPattern_ = nullptr;

	std::vector<int> cooldowns_;                                 // 形態の候補ごとの、再使用までの残り回数

	std::deque<std::string> requestedPatterns_;                  // 外から予約された攻撃

	bool autoAttack_ = true;

	std::mt19937 random_{std::random_device{}()};

	// ---- 状態 ----
	State state_ = State::Idle;

	float stateTimer_ = 0.0f;

	float idleWait_ = 0.0f;                                      // 待機の最低時間

	int phaseIndex_ = 0;

	bool transitionLookApplied_ = false;                         // 移行演出で見た目を切り替えたか

	float hp_ = 0.0f;

	// ---- 自機の情報 ----
	TuboEngine::Math::Vector3 targetPosition_ = {0.0f, 0.0f, 0.0f};

	float targetRadius_ = 0.5f;

	// ---- コールバック ----
	HitCallback onHit_;

	PhaseChangedCallback onPhaseChanged_;

	DefeatedCallback onDefeated_;

	AttackStartedCallback onAttackStarted_;

	CueCallback onCue_;

	AnimationCallback onAnimation_;

	// アニメーションの指示（状態・攻撃・段階が変わったら出し直す）
	const void* animationKeyAttack_ = nullptr;
	int animationKeyState_ = -1;
	int animationKeyStep_ = -1;
	std::string currentAnimation_;
	float currentAnimationDuration_ = 0.0f;

	float stageRadius_ = 20.0f;

	float speedScale_ = 1.0f;

	// ---- デバッグ ----
	std::string deForcedPattern_;   // 空でなければその攻撃だけを出す

	bool dePaused_ = false;

	bool deDrawLines_ = true;

	float deTestDamage_ = 200.0f;   // ImGui のダメージテストの量

	std::string deLastDamage_;      // 直近のダメージテストの結果

	std::string deDataMessage_;     // 直近の保存・読み込みの結果
};
