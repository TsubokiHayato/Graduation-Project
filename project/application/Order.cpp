#include "Order.h"

#include "SceneManager.h"
#include "SceneRegistration.h"
#include "GameScenes.h"
#include "settings/Settings.h"

void TuboEngine::Order::Initialize() {
	// エンジン基盤の初期化（シーンには触れない）
	TuboEngine::Framework::Initialize();

	// 保存済みの設定を読み込む（未保存なら既定値のまま）
	Settings::GetInstance()->Load();

	// ゲームのシーンを登録してから開始シーンを指定（登録は Initialize より前）
	RegisterGameScenes();
	SceneManager::GetInstance()->Initialize(TITLE);
}

void TuboEngine::Order::Update() {
	TuboEngine::Framework::Update();
}

void TuboEngine::Order::Finalize() {
	TuboEngine::Framework::Finalize();
}

void TuboEngine::Order::Draw() {
	TuboEngine::Framework::FrameWorkRenderTargetPreDraw();
	TuboEngine::Framework::Object3dCommonDraw();
	TuboEngine::Framework::SpriteCommonDraw();
	LineManager::GetInstance()->Draw();
	TuboEngine::Framework::ParticleCommonDraw();
	TuboEngine::Framework::FrameworkSwapChainPreDraw();
	TuboEngine::Framework::OffScreenRenderingDraw();
#ifdef USE_IMGUI
	Framework::ImguiPreDraw();
	Framework::ImguiPostDraw();
#endif
	Framework::FrameworkSwapChainPostDraw();
}
