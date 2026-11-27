#pragma once
#include <memory>
#include <string>
#include <vector>

class BossAttackState;

// =============================================================================
//  BossAttackRegistry
//  ボスが使える攻撃の一覧。新しい攻撃を作ったら、BossAttackRegistry.cpp の一覧に1行足すだけでよい
//  （Boss 側の変更は不要。JSON・タイムラインからは攻撃クラスの GetId() の名前で指定する）。
// =============================================================================
namespace BossAttackRegistry {

// 登録されている攻撃の名前（GetId()）の一覧（登録順）
const std::vector<std::string>& GetIds();

// 名前から攻撃を1つ作る。知らない名前なら nullptr
std::unique_ptr<BossAttackState> Create(const std::string& id);

// 登録されている攻撃をすべて1つずつ作る（登録順）
std::vector<std::unique_ptr<BossAttackState>> CreateAll();

} // namespace BossAttackRegistry
