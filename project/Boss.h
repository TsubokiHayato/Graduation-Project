#pragma once
#include "Object3d.h" 
#include <memory>
#include "Camera.h"       // 3D カメラ
#include <string>
#include <engine\Collider\Collider.h>
// =============================================================================
//  Boss ── ボスキャラクターのクラス
// =============================================================================

class Boss 
{
public:
	// 初期化
	void Initialize(TuboEngine::Camera* camera);

	// 更新
	void Update();

	// 描画
	void Draw();

	// ImGui デバッグ用の描画
	void DrawImGui();
public:

	TuboEngine::Math::Vector3 GetPosition() const { return model_->GetPosition(); }

	void SetPosition(const TuboEngine::Math::Vector3& position) { model_->SetPosition(position); }

	auto* GetModel() { return model_.get(); }
	
	void SetCamera(TuboEngine::Camera* camera) { camera_ = camera; }

private:
	// 3D モデル
	std::unique_ptr<TuboEngine::Object3d> model_;

	// 体力
	int hp_ = 100;

	// 3D カメラ
	TuboEngine::Camera* camera_ = nullptr;
};

