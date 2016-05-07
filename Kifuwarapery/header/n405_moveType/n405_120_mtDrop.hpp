﻿#pragma once


#include "../n165_movStack/n165_300_moveType.hpp"
#include "../n165_movStack/n165_500_moveStack.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "../n374_genMove_/n374_350_PieceMovesGenerator.hpp"
#include "../n374_genMove_/n374_750_dropMoveGenerator.hpp"
#include "n405_070_mtAbstract.hpp"


class MoveTypeDrop : public MoveTypeAbstract {
public:
	// 部分特殊化
	// 駒打ち生成
	MoveStack* GenerateMove(MoveStack* moveStackList, const Position& pos, bool all = false
		) const override {

		if (pos.GetTurn() == Color::Black) {
			return g_dropMoveGenerator.GenerateDropMoves<Color::Black,Color::White>(
				moveStackList,
				pos,
				pos.GetEmptyBB()//target
				);
		}
		else
		{
			return g_dropMoveGenerator.GenerateDropMoves<Color::White, Color::Black>(
				moveStackList,
				pos,
				pos.GetEmptyBB()//target
				);
		}
	}

};


