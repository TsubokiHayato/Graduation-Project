#include "OnoderaScene.h"
#include "Input.h"
#include "TextManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include "JsonManager.h"

using namespace TuboEngine;

// =============================================================================
//  Initialize
// =============================================================================
void OnoderaScene::Initialize() {
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->setRotation({ 0.1f, 0.0f, 0.0f });
	camera_->setScale({ 1.0f, 1.0f, 1.0f });
	camera_->Update();

	player_ = std::make_unique<Object3d>();
	player_->Initialize("block/block.obj");
	player_->SetCamera(camera_.get());
	player_->SetPosition({ 0.0f, 0.0f, 0.0f });

	// PlayerDataを生成
	playerdata_ = std::make_unique<PlayerData>();

	if (!playerdata_->Load(player_.get())) {
		// 読み込み失敗時の初期位置
		player_->SetPosition({ 0.0f, 0.0f, 0.0f });
	}


	// JSONを読み込む
	//if (TuboEngine::JsonManager::LoadObject("Resources/Player.json", *playerdata_)) {
	//	// JSONのtranslateをaxisの位置に設定
	//	player_->SetPosition(playerdata_->transform.translate);
	//} else {
	//	// 読み込み失敗時の初期位置
	//	player_->SetPosition({ 0.0f, 0.0f, 10.0f });
	//}
}

// =============================================================================
//  Update
// =============================================================================
void OnoderaScene::Update() {
	camera_->Update();
	player_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void OnoderaScene::Object3DDraw() {
	player_->Draw();
}

void OnoderaScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void OnoderaScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void OnoderaScene::ImGuiDraw() {
#ifdef USE_IMGUI
	player_->DrawImGui("3D Object : axis");

	//playerdata_->DrawImGui(player_.get());

#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void OnoderaScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
