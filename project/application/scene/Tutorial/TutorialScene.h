#pragma once
#include "Boss/Boss.h"
#include "Camera.h"
#include "IScene.h"
#include "Player/Player.h"
#include "UniversalCamera.h"
#include "Vector4.h"
#include <array>
#include <cstdint>
#include <memory>

namespace TuboEngine {
class TextObject;
}

// =============================================================================
//  TutorialScene
//  チュートリアル。次の順に1つずつ進める。
//    1. 移動 → 2. 近距離攻撃 → 3. 遠距離攻撃 → 4. レバーで出力の調整
//    → 5. ボスの全方位爆撃をシールドで防御 → 6. ボスの硬直中に弱点（背中のコア）を攻撃 → 完了
//
//  ・ステップの達成の判定は IsStepCompleted に1か所にまとめている。
//    各要素がまだできていないので、今はすべて「Enter キーが押されたか」で仮に判定する。
//    要素ができたら、ステップごとの case を本物の判定に差し替える（コメントに判定の案を書いてある）。
//  ・表示するテキストはすべて Resources/Text/Tutorial.json（TextManager のレイアウト）にある。
//    ステップのテキストの name は「step_<id>_title」「step_<id>_body」（id は kStepIds）。
//    位置・色・文言は TextManager の ImGui パネルで調整して Save できる。
// =============================================================================
class TutorialScene : public IScene {
public:
	void Initialize() override;
	void Update() override;
	void Finalize() override;
	void Object3DDraw() override;
	void SpriteDraw() override;
	void ImGuiDraw() override;
	void ParticleDraw() override;
	TuboEngine::Camera* GetMainCamera() const override { return camera_->GetCamera(); }

private:
	// チュートリアルのステップ（この順に進む）
	enum Step {
		kStepMove = 0,     // 移動
		kStepMelee,        // 近距離攻撃
		kStepRanged,       // 遠距離攻撃
		kStepPowerLever,   // レバーで出力の調整
		kStepShieldGuard,  // 全方位爆撃をシールドで防御
		kStepCoreAttack,   // 硬直中に弱点を攻撃
		kStepCount,
		kStepComplete = kStepCount, // すべて終わった（完了の表示）
	};

	// ---- 進行 ----
	// ステップを始める（テキストの切り替え・ボスへの指示）
	void EnterStep(int step);
	// ステップを達成したか（今はすべて Enter キーで仮に判定する）
	bool IsStepCompleted(int step) const;
	// ステップ中のボスの動き（防御・弱点のステップでは全方位爆撃をくり返す）
	void UpdateBossForStep();

	// ---- テキスト ----
	// TextManager から名前でテキストを取り直す（レイアウトを読み込み直したときも呼ぶ）
	void RefreshTextRefs();
	// 今のステップのテキストだけを表示する（切り替わった直後はフェードイン）
	void UpdateTextVisual(float deltaTime);

	// ---- カメラ ----
	void SetupCamera();

private:
	std::unique_ptr<TuboEngine::UniversalCamera> camera_;
	std::unique_ptr<Boss> boss_;
	std::unique_ptr<Player> player_;
	float stageRadius_ = 20.0f;
	float minDistance_ = 2.0f;

	int step_ = kStepMove;
	float stepTimer_ = 0.0f; // ステップが始まってからの時間（テキストのフェードイン用）
	bool bossRequestPending_ = false; // ボスに攻撃を予約して、まだ始まっていない

	// ---- テキスト（TextManager 所有。ここは参照用の生ポインタ） ----
	struct TextRef {
		TuboEngine::TextObject* text = nullptr;
		TuboEngine::Math::Vector4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f}; // JSON の色（アルファの基準）
	};
	std::array<TextRef, kStepCount> stepTitles_{};
	std::array<TextRef, kStepCount> stepBodies_{};
	TextRef label_;         // 「TUTORIAL」
	TextRef hint_;          // 「Enter で次へ」
	TextRef completeTitle_; // 完了
	TextRef completeBody_;
	uint32_t textRevision_ = 0;
};
