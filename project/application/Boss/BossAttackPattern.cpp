#include "BossAttackPattern.h"
#include "Boss.h"
#include "BossAttackState.h"
#include <algorithm>

// =============================================================================
//  種類と文字列の変換
// =============================================================================
BossAttackPattern::Type BossAttackPattern::TypeFromString(const std::string& text) {
	if (text == "Sequence") {
		return Type::Sequence;
	}
	if (text == "Parallel") {
		return Type::Parallel;
	}
	return Type::Single;
}

const char* BossAttackPattern::TypeToString(Type type) {
	switch (type) {
	case Type::Sequence: return "Sequence";
	case Type::Parallel: return "Parallel";
	default:             return "Single";
	}
}

BossAttackPattern::BossAttackPattern(std::string name, Type type, std::vector<BossAttackState*> attacks, std::string error)
    : name_(std::move(name)), type_(type), attacks_(std::move(attacks)), error_(std::move(error)) {
	if (!error_.empty()) {
		return;
	}
	if (attacks_.empty()) {
		error_ = "攻撃が1つもありません";
		return;
	}
	// 単発は先頭の1つだけ使う
	if (type_ == Type::Single && attacks_.size() > 1) {
		attacks_.resize(1);
	}
	// 同時攻撃は同じ攻撃を2つ入れられない（攻撃のインスタンスを使い回しているため）
	if (type_ == Type::Parallel) {
		for (size_t i = 0; i < attacks_.size(); ++i) {
			for (size_t j = i + 1; j < attacks_.size(); ++j) {
				if (attacks_[i] == attacks_[j]) {
					error_ = std::string("同時攻撃に同じ攻撃が2つあります: ") + attacks_[i]->GetId();
					return;
				}
			}
		}
	}
}

// =============================================================================
//  進行
// =============================================================================
void BossAttackPattern::Enter(Boss& boss) {
	finished_ = !IsValid();
	index_ = 0;
	if (finished_) {
		return;
	}

	if (type_ == Type::Parallel) {
		// 主（先頭）：ボスを動かし、ほかが終わるまで待ってから硬直する
		BossAttackState* main = attacks_.front();
		main->SetControlsBody(true);
		main->SetSkipRecovery(false);
		main->SetHoldBeforeRecovery(attacks_.size() > 1);
		main->Enter(boss);
		// 従：ボスは動かさず、硬直もしない
		for (size_t i = 1; i < attacks_.size(); ++i) {
			attacks_[i]->SetControlsBody(false);
			attacks_[i]->SetSkipRecovery(true);
			attacks_[i]->SetHoldBeforeRecovery(false);
			attacks_[i]->Enter(boss);
		}
		return;
	}

	// 単発・連続技
	EnterSequenceAttack(boss, 0);
}

void BossAttackPattern::EnterSequenceAttack(Boss& boss, size_t index) {
	index_ = index;
	BossAttackState* attack = attacks_[index_];
	// 同じ攻撃が連続技に2回入っていてもよいように、出す直前に設定する
	attack->SetControlsBody(true);
	attack->SetSkipRecovery(index_ + 1 < attacks_.size()); // 最後以外は硬直しない
	attack->SetHoldBeforeRecovery(false);
	attack->Enter(boss);
}

void BossAttackPattern::Update(Boss& boss, float deltaTime) {
	if (finished_) {
		return;
	}

	if (type_ == Type::Parallel) {
		// 主を先に更新して、ボスの向きを決めてから従を更新する
		for (BossAttackState* attack : attacks_) {
			attack->Update(boss, deltaTime);
		}
		// 従がすべて終わったら、主は硬直に入ってよい
		const bool othersFinished = std::all_of(attacks_.begin() + 1, attacks_.end(), [](const BossAttackState* a) { return a->IsFinished(); });
		if (othersFinished) {
			attacks_.front()->ReleaseHold();
		}
		finished_ = attacks_.front()->IsFinished() && othersFinished;
		return;
	}

	// 単発・連続技：今の攻撃が終わったら次へ
	BossAttackState* attack = attacks_[index_];
	attack->Update(boss, deltaTime);
	if (attack->IsFinished()) {
		if (index_ + 1 < attacks_.size()) {
			EnterSequenceAttack(boss, index_ + 1);
		} else {
			finished_ = true;
		}
	}
}

void BossAttackPattern::Cancel(Boss& boss) {
	for (BossAttackState* attack : attacks_) {
		attack->Cancel(boss);
	}
	finished_ = true;
}

void BossAttackPattern::DrawLines(const Boss& boss) const {
	if (finished_) {
		return;
	}
	if (type_ == Type::Parallel) {
		for (const BossAttackState* attack : attacks_) {
			if (!attack->IsFinished()) {
				attack->DrawLines(boss);
			}
		}
		return;
	}
	attacks_[index_]->DrawLines(boss);
}

BossAttackState* BossAttackPattern::GetCurrentAttack() const {
	if (attacks_.empty()) {
		return nullptr;
	}
	if (type_ == Type::Parallel) {
		return attacks_.front();
	}
	return attacks_[(std::min)(index_, attacks_.size() - 1)];
}
