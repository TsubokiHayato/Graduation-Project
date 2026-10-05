#pragma once
// シーン番号。ゲームを増やすときはここに追加し、SceneRegistration.cpp で生成関数を登録する。
// 起動シーンは Order.cpp の SceneManager::Initialize(...) で指定する（既定は TITLE）。
enum SCENE { SAMPLE, TITLE, OPTION };
