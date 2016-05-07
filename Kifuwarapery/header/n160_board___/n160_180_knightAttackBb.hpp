﻿#pragma once

#include "n160_100_bitboard.hpp"

//────────────────────────────────────────────────────────────────────────────────
// 桂
//────────────────────────────────────────────────────────────────────────────────
class KnightAttackBb {
private:

	Bitboard m_controllBb_[g_COLOR_NUM][SquareNum];

	Bitboard m_knightCheckTable_[g_COLOR_NUM][SquareNum];

public:

	void Initialize();

	void InitCheckTableKnight();

	inline Bitboard GetControllBb(const Color c, const Square sq) const {
		return this->m_controllBb_[c][sq];
	}

	inline Bitboard KnightCheckTable(const Color c, const Square sq) const {
		return this->m_knightCheckTable_[c][sq];
	}
};


// クラス定義のあとに書くとビルドできるぜ☆（＾ｑ＾）
extern KnightAttackBb g_knightAttackBb;
