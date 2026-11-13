#include "MiyamotoScene.h"
#include "Input.h"
#include "TextManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =============================================================================
//  Initialize
// =============================================================================
void MiyamotoScene::Initialize()
{
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({0.0f, 2.0f, -15.0f});
	camera_->setRotation({0.1f, 0.0f, 0.0f});
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_.get());
	axis_->SetPosition({0.0f, 0.0f, 0.0f});

	terrain_ = std::make_unique<Object3d>();
	terrain_->Initialize("terrain/terrain.obj");
	terrain_->SetCamera(camera_.get());
	terrain_->SetPosition({ 0.0f, 0.0f, 0.0f });
	terrain_->SetScale(stageSize_);
}

// =============================================================================
//  Update
// =============================================================================
void MiyamotoScene::Update()
{

	Input* input = Input::GetInstance();
	if (input->PushKey(DIK_A))
	{
		angle_ += 0.1f;
	}
	else if (input->PushKey(DIK_D))
	{
		angle_ -= 0.1f;
	}
	if(input->PushKey(DIK_S))
	{
		radius_ -= 0.1f;
	}
	else if(input->PushKey(DIK_W))
	{
		radius_ += 0.1f;
	}

	radius_ = std::clamp(radius_, minRadius_, maxRadius_);

	position_.x = radius_ * sinf(angle_);
	position_.z = radius_ * cosf(angle_);
	axis_->SetPosition(position_);

	TuboEngine::Math::Vector3 direction = position_;
	direction.Normalize();

	axis_->SetRotation({ 0.0f, angle_, 0.0f });

	camera_->Update();
	axis_->Update();
	terrain_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void MiyamotoScene::Object3DDraw() {
	axis_->Draw();
	terrain_->Draw();
}

void MiyamotoScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void MiyamotoScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void MiyamotoScene::ImGuiDraw() {
#ifdef USE_IMGUI
	axis_->DrawImGui("3D Object : axis");
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void MiyamotoScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
