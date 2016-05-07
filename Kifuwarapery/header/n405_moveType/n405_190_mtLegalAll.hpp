﻿#pragma once


#include "../n165_movStack/n165_300_moveType.hpp"
#include "../n165_movStack/n165_500_moveStack.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "../n374_genMove_/n374_350_PieceMovesGenerator.hpp"
#include "n405_070_mtAbstract.hpp"
#include "n405_160_mtEvasion.hpp"
#include "n405_170_mtNonEvasion.hpp"


class MoveTypeLegalAll : public MoveTypeAbstract {
public:
	// 部分特殊化
	// Evasion のときに歩、飛、角と、香の2段目の不成も生成する。
	MoveStack* GenerateMove(MoveStack* moveStackList, const Position& pos, bool all = false
		) const override {
		//Color us = pos.GetTurn();
		MoveStack* curr = moveStackList;
		const Bitboard pinned = pos.GetPinnedBB();

		moveStackList = pos.InCheck() ?
			MoveTypeEvasion().GenerateMove( moveStackList, pos, true) :
			MoveTypeNonEvasion().GenerateMove( moveStackList, pos);

		// 玉の移動による自殺手と、pinされている駒の移動による自殺手を削除
		while (curr != moveStackList) {
			if (!
				(
					pos.GetTurn()==Color::Black
					?
					pos.IsPseudoLegalMoveIsLegal<false, false, Color::Black, Color::White>(curr->m_move, pinned)
					:
					pos.IsPseudoLegalMoveIsLegal<false, false, Color::White, Color::Black>(curr->m_move, pinned)
				)
			) {
				curr->m_move = (--moveStackList)->m_move;
			}
			else {
				++curr;
			}
		}

		return moveStackList;
	}

};


