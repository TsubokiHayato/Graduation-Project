#pragma once
#include "IScene.h"
#include "Camera.h"
#include "UniversalCamera.h"
#include "Object3d.h"
#include "Player/Player.h"
#include "Player/PlayerBooster.h"
#include "../../Boss/Boss.h"
#include "BossWarning.h"
#include "BossWarningRenderer.h"
#include "ui/Reticle.h"
#include "Player/Equipment/EquipmentDataJson.h"
#include <memory>

// =============================================================================
//  GameScene（統合シーン）
//  メンバー個人シーンの成果を 1 つにまとめたもの。
//   - Tsuboki  : UniversalCamera（追従・ロックオン） / PlayerBooster（背中のノズルと噴射）
//   - Onodera  : Player（エネルギー配分・移動・ダッシュ・HP）
//   - Nagai    : Boss（4 種の攻撃。当たったらシールド配分で軽減してダメージ）
//   - Miyamoto : 地形（terrain）とステージの円形の移動範囲
//   - Satou    : 攻撃予兆（Warning / WarningRenderer）。現状はボス攻撃と未接続のため ImGui から発動
// =============================================================================
class GameScene : public IScene {
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
	// プレイヤーをステージの円の内側に収める
	void ClampPlayerToStage();
	// ボスの攻撃が当たったとき（シールド配分で軽減してから HP を減らす）
	void OnBossHit(float damage);
	// 予兆のテスト（Satou）。ボス中心から自機へ向けた形状で発動し、判定が出た瞬間に当たっていればダメージ
	void UpdateWarning(float dt);

private:
	// ---- カメラ（Tsuboki） ----
	std::unique_ptr<TuboEngine::UniversalCamera> camera_;

	// ---- 地形（Miyamoto） ----
	std::unique_ptr<TuboEngine::Object3d> terrain_;

	// ---- プレイヤー（Onodera） + ブースター（Tsuboki） ----
	std::unique_ptr<Player> player_;
	std::unique_ptr<PlayerBooster> booster_;

	// ---- ボス（Nagai） ----
	std::unique_ptr<Boss> boss_;

	// ---- 攻撃予兆（Satou） ----
	BossHit::Warning warning_;
	BossHit::WarningRenderer warningRenderer_;
	int   warnType_ = 0;            // 0:円 1:帯 2:扇
	int   warnFanIdx_ = 3;          // 扇の角度プリセット（初期は 90°）
	float warnTelegraph_ = 2.0f;    // 広がりきるまでの秒数
	float warnHitSec_ = 0.15f;      // 判定が出ている秒数
	float warnDamage_ = 20.0f;      // 当たったときのダメージ
	bool  warnLastHit_ = false;

	// ---- レティクル（Tsuboki）。出力配分ゲージ付き、ロックオン中はボスへ寄る ----
	Reticle reticle_;
	float lockOnAimHeight_ = 0.0f; // ロックオン時に狙う高さ（ボスの中心からの距離）

	// ---- ステージ・調整値 ----
	float stageRadius_ = 20.0f;   // ボスの攻撃範囲 / プレイヤーの移動範囲（半径）
	float playerRadius_ = 1.0f;   // プレイヤーの当たり判定の半径（block は 2x2x2）
	float lastDamage_ = 0.0f;     // 直近に減った HP（シールドで吸収した分を除く）

	// ---- 装備（Onodera のシールド）。EquipmentManager はシングルトンなので Finalize で片付ける ----
	EquipmentData equipmentData_{};
	std::unique_ptr<EquipmentDataJson> equipmentDataJson_;
};
