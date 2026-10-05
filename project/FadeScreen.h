#pragma once
#include "EngineCore/engine/graphic/2d/Sprite.h"

/// <summary>
/// シーン変更処理
/// </summary>
class FadeScreen {
public:

	FadeScreen();

	~FadeScreen();

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize();
	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();
	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// フェードアウト開始(ゲーム画面から黒に)
	/// </summary>
	void FadeOut();

	/// <summary>
	/// フェードアウト終了
	/// </summary>
	/// <returns>真っ黒になったら終了</returns>
	bool IsFadeOuting() {
		return fadeTimer_ >= 1.0f;
	}

private:

	/// <summary>
	/// フェードイン開始(黒からゲーム画面に)
	/// </summary>
	void FadeIn();

	//
	std::unique_ptr<TuboEngine::Sprite> fadeScreenSprite_;
	

	//時間
	float fadeTimer_ = 1.0f;
	const float kFadeMaxTime_ = 1.0f;

	bool isBrackOut_ = false;

	TuboEngine::Math::Vector2 startPoint_;//開始場所
	TuboEngine::Math::Vector2 goalPoint_;//終了場所

	const TuboEngine::Math::Vector2 kFadeIn_ = { 0,0 };

	const TuboEngine::Math::Vector2 kFadeOut_ = { 0,-720.0f };//下にはけるイメージ
};

