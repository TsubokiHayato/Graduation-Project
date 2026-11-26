#pragma once
#include <string>
#include <vector>

class Boss;
class BossAttackState;

// =============================================================================
//  BossAttackPattern
//  ボスが「1回の行動」として出す攻撃のまとまり。抽選（重み付きランダム）の単位になる。
//
//    Single   ：攻撃1つ
//    Sequence ：連続技。並べた順に出す。途中の攻撃は硬直を飛ばし、最後の攻撃だけ硬直する
//    Parallel ：同時攻撃。並べた攻撃を同時に始める
//               先頭の攻撃が「主」で、ボスの向き・高さ・色を動かすのは主だけ（残りは当たり判定と警告表示のみ）
//               全部の攻撃段階が終わってから、主の硬直に入る
//
//  攻撃のインスタンスはボスが攻撃の種類ごとに1つ持ち、パターン同士で使い回す。
//  そのため同時攻撃に同じ攻撃を2つ入れることはできない（連続技ならよい）。
// =============================================================================
class BossAttackPattern {
public:
	enum class Type {
		Single,
		Sequence,
		Parallel,
	};

	// 文字列（JSON の "type"）との変換。知らない文字列は Single
	static Type TypeFromString(const std::string& text);
	static const char* TypeToString(Type type);

public:
	BossAttackPattern(std::string name, Type type, std::vector<BossAttackState*> attacks, std::string error = {});

	// 行動を始める
	void Enter(Boss& boss);
	// 毎フレーム呼ぶ
	void Update(Boss& boss, float deltaTime);
	// 途中で打ち切る（形態移行・撃破時）。弾なども消す
	void Cancel(Boss& boss);
	// 警告範囲などのライン描画
	void DrawLines(const Boss& boss) const;

	bool IsFinished() const { return finished_; }
	// 使える状態か（攻撃が見つからない・同時攻撃に同じ攻撃が2つ、などは使えない）
	bool IsValid() const { return error_.empty() && !attacks_.empty(); }
	const std::string& GetError() const { return error_; }

	const std::string& GetName() const { return name_; }
	Type GetType() const { return type_; }
	const std::vector<BossAttackState*>& GetAttacks() const { return attacks_; }
	// いまボスの動きを決めている攻撃（連続技は出している攻撃、同時攻撃は主）
	BossAttackState* GetCurrentAttack() const;

private:
	void EnterSequenceAttack(Boss& boss, size_t index);

	std::string name_;
	Type type_ = Type::Single;
	std::vector<BossAttackState*> attacks_;
	std::string error_;

	size_t index_ = 0; // 連続技で出している攻撃の番号
	bool finished_ = true;
};
