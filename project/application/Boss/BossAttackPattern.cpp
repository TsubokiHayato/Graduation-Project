#include "BossAttackPattern.h"
#include "Boss.h"
#include "BossAttackRegistry.h"
#include "BossAttackState.h"
#include <algorithm>

namespace {
// 攻撃に「標準の数値 → クリップで変えた数値」の順で読み込む
void ApplyClipParams(const Boss& boss, BossAttackState& attack, const BossTimelineClip& clip) {
	attack.LoadParams(boss.GetAttackBaseParams(attack.GetId()));
	attack.LoadParams(clip.overrides);
	attack.SetupContext(boss);
}
} // namespace

BossAttackPattern::BossAttackPattern(const BossPatternData& data, bool useAttackRecovery)
    : data_(data), useAttackRecovery_(useAttackRecovery) {
	for (const BossTimelineClip& clip : data_.clips) {
		Clip runtime;
		runtime.attack = BossAttackRegistry::Create(clip.attack);
		if (!runtime.attack && error_.empty()) {
			error_ = "知らない攻撃名です: " + clip.attack;
		}
		clips_.push_back(std::move(runtime));
	}
	if (clips_.empty()) {
		error_ = "攻撃が1つも置かれていません";
	}
}

void BossAttackPattern::SetError(std::string error) {
	if (error_.empty()) {
		error_ = std::move(error);
	}
}

BossAttackPattern::~BossAttackPattern() = default;

// =============================================================================
//  長さの計算
// =============================================================================
BossAttackPattern::ClipTiming BossAttackPattern::ComputeClipTiming(const Boss& boss, const BossTimelineClip& clip) {
	ClipTiming timing;
	timing.start = clip.start;
	std::unique_ptr<BossAttackState> attack = BossAttackRegistry::Create(clip.attack);
	if (attack) {
		ApplyClipParams(boss, *attack, clip);
		timing.telegraph = attack->GetTelegraphLength();
		timing.attack = attack->GetAttackLength();
	}
	return timing;
}

float BossAttackPattern::ComputeTotalDuration(const Boss& boss, const BossPatternData& data) {
	float end = 0.0f;
	for (const BossTimelineClip& clip : data.clips) {
		end = (std::max)(end, ComputeClipTiming(boss, clip).End());
	}
	return end + data.recoveryTime;
}

// =============================================================================
//  進行
// =============================================================================
void BossAttackPattern::Enter(Boss& boss) {
	time_ = 0.0f;
	finished_ = !IsValid();
	if (finished_) {
		return;
	}

	// 数値を作り直し、いちばん最後に終わるクリップを探す（同じなら「ボスを動かす」クリップ、さらに同じなら後ろのもの）
	size_t last = 0;
	float lastEnd = -1.0f;
	for (size_t i = 0; i < clips_.size(); ++i) {
		BossAttackState& attack = *clips_[i].attack;
		const BossTimelineClip& clip = data_.clips[i];
		ApplyClipParams(boss, attack, clip);
		const float end = clip.start + attack.GetTelegraphLength() + attack.GetAttackLength();
		const bool later = end > lastEnd + 1e-4f;
		const bool tie = std::abs(end - lastEnd) <= 1e-4f;
		if (later || (tie && (clip.controlsBody || !data_.clips[last].controlsBody))) {
			last = i;
			lastEnd = (std::max)(end, lastEnd);
		}
		clips_[i].entered = false;
	}

	// 最後に終わるクリップだけが硬直する（硬直時間はパターンのもの。単発は攻撃の標準のまま）
	for (size_t i = 0; i < clips_.size(); ++i) {
		BossAttackState& attack = *clips_[i].attack;
		attack.SetControlsBody(data_.clips[i].controlsBody);
		attack.SetControlsBodyInRecovery(i == last); // 最後の硬直はボスの見た目にも出す
		attack.SetSkipRecovery(i != last);
		if (i == last && !useAttackRecovery_) {
			attack.GetCommonParams().recoveryTime = data_.recoveryTime;
		}
	}

	StartDueClips(boss);
}

void BossAttackPattern::StartDueClips(Boss& boss) {
	for (size_t i = 0; i < clips_.size(); ++i) {
		if (!clips_[i].entered && data_.clips[i].start <= time_ + 1e-4f) {
			clips_[i].entered = true;
			clips_[i].attack->Enter(boss);
		}
	}
}

void BossAttackPattern::Update(Boss& boss, float deltaTime) {
	if (finished_) {
		return;
	}
	time_ += deltaTime;

	// 始まっているクリップを更新（ボスを動かすクリップを先にして、向きを決めてから他を動かす）
	for (int pass = 0; pass < 2; ++pass) {
		for (size_t i = 0; i < clips_.size(); ++i) {
			Clip& clip = clips_[i];
			if (clip.entered && !clip.attack->IsFinished() && data_.clips[i].controlsBody == (pass == 0)) {
				clip.attack->Update(boss, deltaTime);
			}
		}
	}

	// 開始時刻になったクリップを始める
	StartDueClips(boss);

	// どのクリップもボスを動かしていない間（攻撃と攻撃の間など）は、自機の方へ向き直る
	bool bodyActive = false;
	for (size_t i = 0; i < clips_.size(); ++i) {
		bodyActive = bodyActive || (clips_[i].entered && !clips_[i].attack->IsFinished() && data_.clips[i].controlsBody);
	}
	if (!bodyActive) {
		boss.TurnTowardTarget(deltaTime);
	}

	finished_ = std::all_of(clips_.begin(), clips_.end(), [](const Clip& c) { return c.entered && c.attack->IsFinished(); });
}

void BossAttackPattern::Cancel(Boss& boss) {
	for (Clip& clip : clips_) {
		if (clip.attack) {
			clip.attack->Cancel(boss);
		}
	}
	finished_ = true;
}

void BossAttackPattern::DrawLines(const Boss& boss) const {
	if (finished_) {
		return;
	}
	for (const Clip& clip : clips_) {
		if (clip.entered && !clip.attack->IsFinished()) {
			clip.attack->DrawLines(boss);
		}
	}
}

BossAttackState* BossAttackPattern::GetCurrentAttack() const {
	BossAttackState* any = nullptr;
	for (size_t i = 0; i < clips_.size(); ++i) {
		const Clip& clip = clips_[i];
		if (!clip.attack || !clip.entered || clip.attack->IsFinished()) {
			continue;
		}
		if (data_.clips[i].controlsBody) {
			return clip.attack.get();
		}
		if (!any) {
			any = clip.attack.get();
		}
	}
	return any;
}

const BossAttackState* BossAttackPattern::GetClipAttack(size_t index) const {
	return index < clips_.size() ? clips_[index].attack.get() : nullptr;
}
