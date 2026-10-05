#include "FadeScreen.h"

FadeScreen::FadeScreen() {}
FadeScreen::~FadeScreen() {}

void FadeScreen::Initialize() {
	fadeScreenSprite_ = std::make_unique<TuboEngine::Sprite>();
	fadeScreenSprite_->Initialize("sceneScreen.png");
	fadeScreenSprite_->SetSize({1280.0f,720.0f});

	//フェードイン開始
	FadeIn();
}

void FadeScreen::Update() {
	if (isBrackOut_) {
		fadeTimer_ += 1.0f / 60.0f;
	}
	else {
		fadeTimer_ -= 1.0f / 60.0f;
	}
	fadeTimer_ = std::clamp(fadeTimer_, 0.0f, kFadeMaxTime_);

	fadeScreenSprite_->SetColor({ 1,1,1,fadeTimer_ });
	fadeScreenSprite_->Update();
}

void FadeScreen::Draw() {
	if (fadeTimer_ <= 0.0f) return;
	fadeScreenSprite_->Draw();
}

void FadeScreen::FadeIn() {
	fadeTimer_ = kFadeMaxTime_;
	isBrackOut_ = false;
}

void FadeScreen::FadeOut() {
	if (!isBrackOut_) {
		fadeTimer_ = 0.0f;
	}
	isBrackOut_ = true;
}
