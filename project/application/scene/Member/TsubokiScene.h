#pragma once
#include "IScene.h"
#include "Camera.h"
#include "UniversalCamera.h"
#include "Object3d.h"
#include "Animator.h"
#include "AnimatorController.h"
#include "Player/Player.h"
#include "Player/PlayerBooster.h"
#include <memory>

class TsubokiScene : public IScene {
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
	/// <summary>
	/// 人形（アニメーション確認用）のアニメーションを切り替える。移動はしない
	/// </summary>
	void UpdateHumanPreview();

	/// <summary>
	/// 歩きアニメーションから待機ポーズ（1フレームのアニメーション）を作る
	/// </summary>
	TuboEngine::Animation CreateIdlePose() const;

private:
	// 追従カメラ（真後ろ / 後ろ斜め / 見下ろし、ロックオン、シェイク、FOV、キーフレーム）
	std::unique_ptr<TuboEngine::UniversalCamera> camera_;

	std::unique_ptr<TuboEngine::Object3d> axis_;
	std::unique_ptr<TuboEngine::Object3d> ground_;

	// プレイヤー（WASD で移動。カメラはこちらを追従する）
	std::unique_ptr<Player> player_;

	// 人形（スキニング）。アニメーション確認用でその場に置いておく
	std::unique_ptr<TuboEngine::Animator> human_;
	float modelScale_ = 2.0f;     // 元モデルは身長約1m
	float blendTime_ = 0.2f;      // アニメーション切り替えのクロスフェード時間（秒）
	// アニメーション確認用の入力（ImGui で切り替える）
	bool previewMoving_ = false;
	bool previewSneak_ = false;
	// アニメーションの状態遷移（Idle / Walk / SneakWalk）。条件は Initialize で登録
	TuboEngine::AnimatorController animController_;
	bool showHandMarker_ = false; // 右手の位置に球を表示する（ボーン位置取得の確認用）

	// プレイヤー背中のバーニアノズル（移動方向に合わせて回転する）
	std::unique_ptr<PlayerBooster> booster_;
};
