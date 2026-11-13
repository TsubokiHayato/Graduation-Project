#include "BossWarning.h"
#include <algorithm>

namespace BossHit {

void Warning::Start(const Shape& shape, float telegraphSec, float hitSec) {
	shape_ = shape;
	telegraphSec_ = std::max(telegraphSec, 0.001f);
	hitSec_ = hitSec;
	elapsed_ = 0.0f;
	justHit_ = false;
	phase_ = Phase::Telegraph;
}

void Warning::Update(float dt) {
	justHit_ = false;
	switch (phase_) {
	case Phase::Telegraph:
		elapsed_ += dt;
		if (elapsed_ >= telegraphSec_) {
			phase_ = Phase::Hit;
			elapsed_ = 0.0f;
			justHit_ = true;
		}
		break;
	case Phase::Hit:
		elapsed_ += dt;
		if (elapsed_ >= hitSec_) phase_ = Phase::Done;
		break;
	default: break;
	}
}

float Warning::GetProgress() const {
	if (phase_ == Phase::Telegraph) return std::clamp(elapsed_ / telegraphSec_, 0.0f, 1.0f);
	return (phase_ == Phase::Hit) ? 1.0f : 0.0f;
}

float Warning::GetHitProgress() const {
	if (phase_ != Phase::Hit) return 0.0f;
	return std::clamp(elapsed_ / std::max(hitSec_, 0.001f), 0.0f, 1.0f);
}

Shape Warning::GetFillShape() const { return ScaleShape(shape_, GetProgress()); }

} // namespace BossHit
