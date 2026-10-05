#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "Audio.h"

// =============================================================================
//  AudioManager ── ゲーム全体の音を扱う簡易マネージャ（シングルトン）
//
//  ・BGM はシーンをまたいで鳴らし続ける（同じ曲なら再スタートしないので途切れない）
//  ・SE はワンショット再生（重ね掛け可）
//  ・音量は Settings（bgmVolume / seVolume）を毎回参照する
//
//  ★別の曲/SEを鳴らしたいときは、Resources/Audio/ に wav を置いて
//    PlayBgm("xxx.wav") / PlaySe("xxx.wav") を呼ぶだけ。
// =============================================================================
class AudioManager {
public:
	static AudioManager* GetInstance();

	// BGM を切り替えてループ再生する。既に同じ曲が鳴っていれば音量だけ合わせて何もしない。
	void PlayBgm(const std::string& fileName);
	// BGM を無音から seconds 秒かけてフェードインしつつループ再生する。
	// 既に同じ曲が鳴っていれば PlayBgm と同じ（フェードし直さない）。
	// ※フェードを進めるには、毎フレーム UpdateFade(dt) を呼ぶこと。
	void PlayBgmFadeIn(const std::string& fileName, float seconds);
	// フェード進行を1フレーム分進める（シーンの Update から dt を渡して呼ぶ）。
	void UpdateFade(float dt);
	void StopBgm();
	// 再生中の BGM 音量を Settings の bgmVolume に即時反映する（スライダーのライブ反映用）。
	void ApplyBgmVolume();

	// SE をワンショット再生する（Settings の seVolume で）。
	void PlaySe(const std::string& fileName);

private:
	AudioManager() = default;
	static AudioManager* instance_;

	std::unique_ptr<TuboEngine::Audio> bgm_;
	std::string bgmFile_;
	bool bgmPlaying_ = false;

	// BGM フェードイン用の状態（UpdateFade で進める）。
	bool fading_ = false;
	float fadeElapsed_ = 0.0f;
	float fadeDuration_ = 0.0f;

	// SE は使い回すため読み込み済みのものをキャッシュする。
	std::unordered_map<std::string, std::unique_ptr<TuboEngine::Audio>> seCache_;
};
