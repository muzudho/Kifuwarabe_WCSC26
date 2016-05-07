﻿#pragma once


#include "../n165_movStack/n165_300_moveType.hpp"
#include "../n165_movStack/n165_500_moveStack.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "../n374_genMove_/n374_350_PieceMovesGenerator.hpp"
#include "../n374_genMove_/n374_750_dropMoveGenerator.hpp"
#include "n405_070_mtAbstract.hpp"


class MoveTypeEvasion : public MoveTypeAbstract {
public:
	// 部分特殊化
	// 王手回避生成
	// 王手をしている駒による王手は避けるが、
	// 玉の移動先に敵の利きがある場合と、pinされている味方の駒を動かした場合、非合法手を生成する。
	// そのため、pseudo legal である。
	MoveStack* GenerateMove(MoveStack* moveStackList, const Position& pos, bool all = false
		) const override {

		if (pos.GetTurn() == Color::Black)
		{
			moveStackList = MoveTypeEvasion::GENERATE_MOVE_<Color::Black, Color::White>(moveStackList, pos, all);
		}
		else
		{
			moveStackList = MoveTypeEvasion::GENERATE_MOVE_<Color::White, Color::Black>(moveStackList, pos, all);
		}

		return moveStackList;
	}

private:

	template<Color US, Color THEM>
	static inline MoveStack* GENERATE_MOVE_(MoveStack* moveStackList, const Position& pos, bool all = false
		) {

		assert(pos.IsOK());
		assert(pos.InCheck());

		const Square ksq = pos.GetKingSquare(US);

		const Bitboard checkers = pos.GetCheckersBB();
		Bitboard bb = checkers;
		Bitboard bannedKingToBB = Bitboard::CreateAllZeroBB();
		int checkersNum = 0;
		Square checkSq;

		// 玉が逃げられない位置の bitboard を生成する。
		// 絶対に王手が掛かっているので、while ではなく、do while
		do {

			// 最初にビットが立っていたマス☆
			checkSq = bb.PopFirstOneFromI9();

			assert(ConvPiece::TO_COLOR10(pos.GetPiece(checkSq)) == THEM);

			// カウントアップ☆
			++checkersNum;


			// checkSq にある駒で王手されたとき、玉はその駒の利きの位置には移動できないので、
			// 移動できない位置を bannnedKingToBB に格納する。
			// 両王手のときには二度連続で呼ばれるため、= ではなく |= を使用している。
			// 最初に呼ばれたときは、bannedKingToBB == allZeroBB() である。
			// todo: FOECE_INLINE と template 省いてNPS比較
			g_pieceArray.m_pieceAbstractArray[pos.GetPiece(checkSq)]->MakeBanned2KingTo(bannedKingToBB, pos, checkSq, ksq);

		} while (bb.Exists1Bit());


		// 玉が移動出来る移動先を格納。
		bb = bannedKingToBB.NotThisAnd(pos.GetBbOf10(US).NotThisAnd(g_kingAttackBb.GetControllBb(ksq)));
		while (bb.Exists1Bit()) {
			const Square to = bb.PopFirstOneFromI9();
			// 移動先に相手駒の利きがあるか調べずに指し手を生成する。
			// attackersTo() が重いので、ＭｏｖｅＰｉｃｋｅｒ か search で合法手か調べる。
			moveStackList->m_move = g_makePromoteMove.GetSelectedMakeMove_ExceptPromote_CaptureCategory(g_PTKING_ONBOARD_AS_MOVE, ksq, to, pos);
			moveStackList++;
		}

		// 両王手なら、玉を移動するしか回避方法は無い。
		// 玉の移動は生成したので、ここで終了
		if (1 < checkersNum) {
			return moveStackList;
		}

		// 王手している駒を玉以外で取る手の生成。
		// pin されているかどうかは ＭｏｖｅＰｉｃｋｅｒ か search で調べる。
		const Bitboard target1 = g_betweenBb.GetBetweenBB(checkSq, ksq);
		const Bitboard target2 = target1 | checkers;

		const PieceMoveEvent pmEvent(MoveType::N06_Evasion, all, pos, ksq);

		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N01_Pawn<US>(moveStackList, pmEvent, target2);
		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N02_Lance<US>(moveStackList, pmEvent, target2);
		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N03_Knight<US>(moveStackList, pmEvent, target2);
		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N04_Silver<US>(moveStackList, pmEvent, target2);
		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N05_Bishop<US>(moveStackList, pmEvent, target2);
		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N06_Rook<US>(moveStackList, pmEvent, target2);
		moveStackList = PieceMovesGenerator::GeneratePieceMoves_N16_GoldHorseDragon<US>(moveStackList, pmEvent, target2);

		if (target1.Exists1Bit()) {
			moveStackList = g_dropMoveGenerator.GenerateDropMoves<US,THEM>(moveStackList, pos, target1);
		}

		return moveStackList;
	}

};


