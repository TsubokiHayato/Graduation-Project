#include "SceneRegistration.h"

#include "SceneManager.h"
#include "GameScenes.h"
#include "Sample/SampleScene.h"
#include "Title/TitleScene.h"
#include "Option/OptionScene.h"

#include <memory>

void RegisterGameScenes() {
	SceneManager* sm = SceneManager::GetInstance();
	// シーン番号 → 生成関数 を登録。シーンを増やすときはここに足す。
	sm->RegisterScene(SAMPLE, [] { return std::make_unique<SampleScene>(); }, "Sample");
	sm->RegisterScene(TITLE, [] { return std::make_unique<TitleScene>(); }, "Title");
	sm->RegisterScene(OPTION, [] { return std::make_unique<OptionScene>(); }, "Option");
}
