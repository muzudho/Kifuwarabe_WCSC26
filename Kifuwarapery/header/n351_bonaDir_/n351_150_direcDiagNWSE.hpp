﻿#pragma once


#include <cstdlib>	// abs()
#include "../n080_common__/n080_100_common.hpp"
#include "../n105_color___/n105_100_color.hpp"
#include "../n110_square__/n110_100_square.hpp"
#include "../n110_square__/n110_250_squareDelta.hpp"
#include "../n161_sqDistan/n161_500_squareDistance.hpp"
#include "../n162_bonaDir_/n162_070_bonaDirAbstract.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "../n350_pieceTyp/n350_040_ptEvent.hpp"
#include "../n350_pieceTyp/n350_500_ptPrograms.hpp"


class DirecDiagNWSE : public BonaDirAbstract {
public:

	void InitializeSquareDistance(SquareDistance& squareDistance, Square sq0, Square sq1) const {
		squareDistance.SetValue( sq0, sq1,
			abs(static_cast<int>(sq0 - sq1) / static_cast<int>(SquareDelta::DeltaNW))
		);
	}

	void Do2Move(Position& position, Square from, const Square ksq, const Color us) const {
		const PieceTypeEvent ptEvent1(position.GetOccupiedBB(), Color::Null, ksq);
		position.GetStateInfo()->m_checkersBB |=
			PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent1) &
			(
				us==Color::Black
				?
				position.GetBbOf30<Color::Black>(PieceType::N05_Bishop, PieceType::N13_Horse)
				:
				position.GetBbOf30<Color::White>(PieceType::N05_Bishop, PieceType::N13_Horse)
				)
			;
	}

};


extern DirecDiagNWSE g_direcDiagNWSE;

