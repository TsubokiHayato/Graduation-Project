#include "settings/Settings.h"

#include <externals/nlohmann/json.hpp>
#include <fstream>

using nlohmann::json;

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
	json j;
	// ★項目を増やしたらここに1行足す
	j["bgmVolume"] = bgmVolume;
	j["seVolume"]  = seVolume;

	std::ofstream ofs(kFilePath);
	if (!ofs) {
		return false;
	}
	ofs << j.dump(2);
	return true;
}

bool Settings::Load() {
	std::ifstream ifs(kFilePath);
	if (!ifs) {
		return false; // 未保存なら既定値のまま
	}

	json j;
	try {
		ifs >> j;
	} catch (...) {
		return false;
	}

	// ★項目を増やしたらここに1行足す（第2引数=既定値なのでキーが無くても安全）
	bgmVolume = j.value("bgmVolume", bgmVolume);
	seVolume  = j.value("seVolume", seVolume);
	return true;
}
