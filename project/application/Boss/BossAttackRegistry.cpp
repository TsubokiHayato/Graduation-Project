#include "BossAttackRegistry.h"
#include "BossAttackBarrage.h"
#include "BossAttackLockOnBeam.h"
#include "BossAttackOmniBomb.h"
#include "BossAttackSweep.h"

std::vector<std::unique_ptr<BossAttackState>> BossAttackRegistry::CreateAll() {
	std::vector<std::unique_ptr<BossAttackState>> attacks;

	// ---- 攻撃の一覧（新しい攻撃はここに1行足す） ----
	attacks.push_back(std::make_unique<BossAttackOmniBomb>());   // "OmniBomb"   全方位爆撃
	attacks.push_back(std::make_unique<BossAttackLockOnBeam>()); // "LockOnBeam" ロックオンビーム
	attacks.push_back(std::make_unique<BossAttackBarrage>());    // "Barrage"    弾幕射撃
	attacks.push_back(std::make_unique<BossAttackSweep>());      // "Sweep"      近接薙ぎ払い

	return attacks;
}
