#include "BossAttackRegistry.h"
#include "BossAttackBarrage.h"
#include "BossAttackLockOnBeam.h"
#include "BossAttackOmniBomb.h"
#include "BossAttackSweep.h"
#include <functional>
#include <utility>

namespace {

using Factory = std::function<std::unique_ptr<BossAttackState>()>;

// ---- 攻撃の一覧（新しい攻撃はここに1行足す。名前は攻撃クラスの GetId() と同じにする） ----
const std::vector<std::pair<std::string, Factory>>& Entries() {
	static const std::vector<std::pair<std::string, Factory>> entries = {
	    {"OmniBomb",   [] { return std::make_unique<BossAttackOmniBomb>(); }},   // 全方位爆撃
	    {"LockOnBeam", [] { return std::make_unique<BossAttackLockOnBeam>(); }}, // ロックオンビーム
	    {"Barrage",    [] { return std::make_unique<BossAttackBarrage>(); }},    // 弾幕射撃
	    {"Sweep",      [] { return std::make_unique<BossAttackSweep>(); }},      // 近接薙ぎ払い
	};
	return entries;
}

} // namespace

const std::vector<std::string>& BossAttackRegistry::GetIds() {
	static const std::vector<std::string> ids = [] {
		std::vector<std::string> result;
		for (const auto& [id, factory] : Entries()) {
			result.push_back(id);
		}
		return result;
	}();
	return ids;
}

std::unique_ptr<BossAttackState> BossAttackRegistry::Create(const std::string& id) {
	for (const auto& [entryId, factory] : Entries()) {
		if (entryId == id) {
			return factory();
		}
	}
	return nullptr;
}

std::vector<std::unique_ptr<BossAttackState>> BossAttackRegistry::CreateAll() {
	std::vector<std::unique_ptr<BossAttackState>> attacks;
	for (const auto& [id, factory] : Entries()) {
		attacks.push_back(factory());
	}
	return attacks;
}
