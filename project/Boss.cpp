#include "Boss.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
void Boss::Initialize(TuboEngine::Camera* camera)
{
	model_ = std::make_unique<TuboEngine::Object3d>();
	model_->Initialize("player/player.obj");
	camera_ = camera;
	model_->SetCamera(camera_);
	model_->SetPosition({ 0.0f, 0.0f, 0.0f });
}

void Boss::Update()
{
	model_->Update();
}

void Boss::Draw()
{
	model_->Draw();
}

void Boss::DrawImGui()
{
#ifdef USE_IMGUI
	model_->DrawImGui("3D Object : Boss");
#endif
}
