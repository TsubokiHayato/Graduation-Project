#pragma once

// =============================================================================
//  Settings ── ゲーム全体の設定値（シングルトン）
//
//  ★設定項目を増やすときの手順（3か所だけ）:
//    1. このクラスに public フィールドを1つ足す（初期値付きで書く）
//    2. Settings.cpp の Save() / Load() に1行ずつ足す
//    3. OptionScene::BuildItems() に項目を1つ push_back する
//
//  ResetToDefault() はフィールドを全部まとめて初期化するので、増やしても触らなくてよい。
// =============================================================================
class Settings {
public:
	static Settings* GetInstance();

	// ---- 設定値（増やすならここに足す）------------------------------------
	float bgmVolume = 0.8f; // BGM 音量 0.0〜1.0
	float seVolume  = 0.8f; // SE  音量 0.0〜1.0
	// ----------------------------------------------------------------------

	// 全設定を既定値へ戻す。
	void ResetToDefault();

	// Resources/Settings.json との読み書き。
	bool Load(); // ファイルが無ければ既定値のまま false を返す
	bool Save() const;

	static constexpr const char* kFilePath = "Resources/Settings.json";

private:
	Settings() = default; // 既定値はメンバ初期化子で持つ
	static Settings* instance_;
};
