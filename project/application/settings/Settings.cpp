#include "settings/Settings.h"

#include "JsonManager.h"

// Settings ⇔ JSON の変換（項目は Settings.h の SETTINGS_FIELDS）
TUBO_JSON_DEFINE(Settings, SETTINGS_FIELDS)

Settings* Settings::instance_ = nullptr;

Settings* Settings::GetInstance() {
	if (!instance_) {
		instance_ = new Settings();
	}
	return instance_;
}

void Settings::ResetToDefault() {
	// 既定値で構築した一時オブジェクトをコピーするだけ。
	// フィールドを増やしてもここは触らなくてよい（初期値はメンバ初期化子が持つ）。
	*this = Settings();
}

bool Settings::Save() const {
	return TuboEngine::JsonManager::SaveObject(kFilePath, *this);
}

bool Settings::Load() {
	// 未保存（初回起動）なら既定値のまま。ファイルが無いのは正常なのでログは出さない
	return TuboEngine::JsonManager::LoadObject(kFilePath, *this, /*logIfMissing*/ false);
}
