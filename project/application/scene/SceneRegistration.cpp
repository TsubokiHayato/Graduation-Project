#include "SceneRegistration.h"

#include "SceneManager.h"
#include "GameScenes.h"
#include "Sample/SampleScene.h"
#include "Title/TitleScene.h"
#include "Option/OptionScene.h"
#include "Member/OnoderaScene.h"
#include "Member/NagaiScene.h"
#include "Member/SatouScene.h"
#include "Member/TsubokiScene.h"
#include "Member/MiyamotoScene.h"

#include <memory>

void RegisterGameScenes() {
	SceneManager* sm = SceneManager::GetInstance();
	// シーン番号 → 生成関数 を登録。シーンを増やすときはここに足す。
	sm->RegisterScene(SAMPLE, [] { return std::make_unique<SampleScene>(); }, "Sample");
	sm->RegisterScene(TITLE, [] { return std::make_unique<TitleScene>(); }, "Title");
	sm->RegisterScene(OPTION, [] { return std::make_unique<OptionScene>(); }, "Option");

	// メンバー個人用シーン
	sm->RegisterScene(ONODERA, [] { return std::make_unique<OnoderaScene>(); }, "Onodera");
	sm->RegisterScene(NAGAI, [] { return std::make_unique<NagaiScene>(); }, "Nagai");
	sm->RegisterScene(SATOU, [] { return std::make_unique<SatouScene>(); }, "Satou");
	sm->RegisterScene(TSUBOKI, [] { return std::make_unique<TsubokiScene>(); }, "Tsuboki");
	sm->RegisterScene(MIYAMOTO, [] { return std::make_unique<MiyamotoScene>(); }, "Miyamoto");
}
