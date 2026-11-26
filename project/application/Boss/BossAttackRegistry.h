#pragma once
#include <memory>
#include <vector>

class BossAttackState;

// =============================================================================
//  BossAttackRegistry
//  ボスが使える攻撃の一覧。ボスは初期化時にここに並んだ攻撃をすべて1つずつ作る。
//  新しい攻撃を作ったら、BossAttackRegistry.cpp の一覧に1行足すだけでよい
//  （Boss 側の変更は不要。JSON からは攻撃クラスの GetId() の名前で指定する）。
// =============================================================================
namespace BossAttackRegistry {

// 登録されている攻撃をすべて作る
std::vector<std::unique_ptr<BossAttackState>> CreateAll();

} // namespace BossAttackRegistry
