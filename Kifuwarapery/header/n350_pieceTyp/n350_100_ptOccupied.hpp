#pragma once

#include "../n105_color___/n105_100_color.hpp"
#include "../n110_square__/n110_100_square.hpp"
#include "../n112_pieceTyp/n112_050_pieceType.hpp"
#include "../n160_board___/n160_100_bitboard.hpp"
#include "../n160_board___/n160_230_setMaskBb.hpp"
#include "../n165_movStack/n165_300_moveType.hpp"
#include "../n165_movStack/n165_500_moveStack.hpp"
#include "../n165_movStack/n165_600_convMove.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "n350_030_makePromoteMove.hpp"
#include "n350_040_ptEvent.hpp"
#include "n350_045_pieceTypeSeeEvent.hpp"
#include "n350_070_ptAbstract.hpp"


const static Move g_PTOCCUPIED_AS_MOVE = ConvMove::FROM_PIECETYPE_ONBOARD10(PieceType::N00_Occupied);


// PieceType::N00_Occupied
class PtOccupied : public PtAbstract {
public:

	virtual Move AsMove() const override {
		return g_PTOCCUPIED_AS_MOVE;
	}

	Bitboard GetAttacks2From(const PieceTypeEvent& ptEvent) const override {
		return Bitboard::CreateAllZeroBB();
	}

	void Generate2RecaptureMoves_usWhite(//FORCE_INLINE
		MoveStack* moveStackList,
		const Position& pos,
		const Square from,
		const Square to
	) const override {
		// 最適化の為のダミー
		assert(false);
	}

	void Generate2RecaptureMoves_usBlack(//FORCE_INLINE
		MoveStack* moveStackList,
		const Position& pos,
		const Square from,
		const Square to
		) const override {
		// 最適化の為のダミー
		assert(false);
	}

	PieceType AppendToNextAttackerAndTryPromote(
		Bitboard& occupied,
		Bitboard& attackers,
		PieceType nextPT,
		const PieceTypeSeeEvent ptsEvent
		) const override {
		PieceType PT = PieceType::N00_Occupied;

		UNREACHABLE;

		// それ以外の駒種類は、そのまま返す☆
		return PT;
	}

};
