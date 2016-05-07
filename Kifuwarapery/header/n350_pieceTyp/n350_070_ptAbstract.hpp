#pragma once

#include "../n080_common__/n080_100_common.hpp"
#include "../n105_color___/n105_100_color.hpp"
#include "../n110_square__/n110_100_square.hpp"
#include "../n112_pieceTyp/n112_050_pieceType.hpp"
#include "../n160_board___/n160_100_bitboard.hpp"
#include "../n160_board___/n160_230_setMaskBb.hpp"
#include "../n165_movStack/n165_300_moveType.hpp"
#include "../n165_movStack/n165_500_moveStack.hpp"
#include "../n165_movStack/n165_600_convMove.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "n350_040_ptEvent.hpp"
#include "n350_045_pieceTypeSeeEvent.hpp"


class PtAbstract {
public:


	virtual Move AsMove() const = 0;//�V�����i�O���O�j


	virtual Bitboard GetAttacks2From(const PieceTypeEvent& ptEvent) const = 0;

	// pin �͏Ȃ��Ȃ��B//FORCE_INLINE 
	virtual void Generate2RecaptureMoves_usWhite(
		MoveStack* moveStackList,
		const Position& pos,
		const Square from,
		const Square to
	) const = 0;
	virtual void Generate2RecaptureMoves_usBlack(
		MoveStack* moveStackList,
		const Position& pos,
		const Square from,
		const Square to
		) const = 0;


	virtual PieceType AppendToNextAttackerAndTryPromote(
		Bitboard& occupied,
		Bitboard& attackers,
		PieceType nextPT,
		const PieceTypeSeeEvent ptsEvent
		) const = 0;

};
