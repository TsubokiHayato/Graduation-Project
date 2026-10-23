#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h"
#include "Animator.h"
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
	TuboEngine::Camera* GetMainCamera() const override { return camera_.get(); }

private:
	/// <summary>
	/// 入力からキャラクターを移動・旋回させ、状態に合わせてアニメーションを切り替える
	/// </summary>
	void UpdateCharacter();

	/// <summary>
	/// カメラをキャラクターに追従させる
	/// </summary>
	void UpdateCamera();

	/// <summary>
	/// 歩きアニメーションから待機ポーズ（1フレームのアニメーション）を作る
	/// </summary>
	TuboEngine::Animation CreateIdlePose() const;

private:
	std::unique_ptr<TuboEngine::Camera> camera_;

	std::unique_ptr<TuboEngine::Object3d> axis_;
	std::unique_ptr<TuboEngine::Object3d> ground_;

	// 人型キャラクター（スキニング）
	std::unique_ptr<TuboEngine::Animator> human_;
	TuboEngine::Math::Vector3 position_ = {0.0f, 0.0f, 0.0f};
	float yaw_ = 0.0f;            // 向き（Y軸回転、ラジアン）
	float modelScale_ = 2.0f;     // 元モデルは身長約1m
	// 移動速度（m/秒）。足が滑らないよう各アニメーションの歩幅に合わせてある
	float walkSpeed_ = 1.7f;
	float sneakSpeed_ = 2.4f;
	float turnSpeed_ = 10.0f;     // 旋回の追従の速さ
	float blendTime_ = 0.2f;      // アニメーション切り替えのクロスフェード時間（秒）
	const char* state_ = "Idle";  // 現在の状態（表示用）

	// カメラ（キャラクターからの相対位置）
	TuboEngine::Math::Vector3 cameraOffset_ = {0.0f, 4.0f, -9.0f};
	float cameraPitch_ = 0.3f;
};
