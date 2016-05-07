﻿#include "../../header/n160_board___/n160_120_bishopAttackBb.hpp"
#include "../../header/n160_board___/n160_150_rookAttackBb.hpp"
#include "../../header/n160_board___/n160_160_kingAttackBb.hpp"


KingAttackBb g_kingAttackBb;//本当はconst にしたいが、やり方がわからない☆ C2373エラーになるんだぜ☆


void KingAttackBb::Initialize()
{
	for (Square sq = I9; sq < SquareNum; ++sq)
	{
		this->m_controllBb_[sq] =
			g_rookAttackBb.GetControllBb(Bitboard::CreateAllOneBB(), sq) |
			g_bishopAttackBb.BishopAttack(Bitboard::CreateAllOneBB(), sq);
	}
}
