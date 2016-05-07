﻿#include <iostream>		// std::cout
#include <algorithm>	// std::min,std::max
#include <sstream>      // std::istringstream
#include "../../header/n105_color___/n105_100_color.hpp"
#include "../../header/n110_square__/n110_100_square.hpp"
#include "../../header/n110_square__/n110_400_squareRelation.hpp"
#include "../../header/n112_pieceTyp/n112_050_pieceType.hpp"
#include "../../header/n113_piece___/n113_150_piece.hpp"
#include "../../header/n113_piece___/n113_155_convPiece.hpp"
#include "../../header/n119_score___/n119_090_scoreIndex.hpp"
#include "../../header/n119_score___/n119_200_pieceScore.hpp"
#include "../../header/n160_board___/n160_100_bitboard.hpp"
#include "../../header/n160_board___/n160_170_goldAndSilverAttackBb.hpp"
#include "../../header/n160_board___/n160_230_setMaskBb.hpp"
#include "../../header/n160_board___/n160_240_betweenBb.hpp"
#include "../../header/n165_movStack/n165_300_moveType.hpp"
#include "../../header/n220_position/n220_100_repetitionType.hpp"
#include "../../header/n220_position/n220_640_utilAttack.hpp"
#include "../../header/n220_position/n220_650_position.hpp"

#include "../../header/n350_pieceTyp/n350_040_ptEvent.hpp"
#include "../../header/n350_pieceTyp/n350_110_ptPawn.hpp"
#include "../../header/n350_pieceTyp/n350_120_ptLance.hpp"
#include "../../header/n350_pieceTyp/n350_130_ptKnight.hpp"
#include "../../header/n350_pieceTyp/n350_140_ptSilver.hpp"
#include "../../header/n350_pieceTyp/n350_150_ptBishop.hpp"
#include "../../header/n350_pieceTyp/n350_160_ptRook.hpp"
#include "../../header/n350_pieceTyp/n350_170_ptGold.hpp"
#include "../../header/n350_pieceTyp/n350_230_ptHorse.hpp"
#include "../../header/n350_pieceTyp/n350_240_ptDragon.hpp"
#include "../../header/n350_pieceTyp/n350_500_ptPrograms.hpp"
#include "../../header/n351_bonaDir_/n351_500_bonaDirArray.hpp"
#include "../../header/n407_moveGen_/n407_900_moveList.hpp"

#include "../../header/n520_evaluate/n520_500_kkKkpKppStorage1.hpp"
#include "../../header/n600_book____/n600_100_mt64bit.hpp"
#include "../../header/n885_searcher/n885_040_rucksack.hpp"



Key Position::m_ZOBRIST_[g_PIECETYPE_NUM][SquareNum][g_COLOR_NUM];
Key Position::m_ZOB_HAND_[HandPieceNum][g_COLOR_NUM];
Key Position::m_ZOB_EXCLUSION_;



Color Position::GetTurn() const
{
	return this->m_turn_;
}




// 実際に指し手が合法手かどうか判定
// 連続王手の千日手は排除しない。
// 確実に駒打ちではないときは、MUSTNOTDROP == true とする。
// 確実に玉の移動で無いときは、FROMMUSTNOTKING == true とする。英語として正しい？
// 遠隔駒で王手されているとき、その駒の利きがある場所に逃げる手を検出出来ない場合があるので、
// そのような手を指し手生成してはいけない。
template <bool MUSTNOTDROP, bool FROMMUSTNOTKING,Color US,Color THEM>
bool Position::IsPseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const {
	// 駒打ちは、打ち歩詰めや二歩は指し手生成時や、killerを ＭｏｖｅＰｉｃｋｅｒ::nextMove() 内で排除しているので、常に合法手
	// (連続王手の千日手は省いていないけれど。)
	if (!MUSTNOTDROP && move.IsDrop()) {
		return true;
	}
	assert(!move.IsDrop());

	const Square from = move.From();

	if (!FROMMUSTNOTKING && ConvPiece::TO_PIECE_TYPE10(GetPiece(from)) == N08_King) {
		// 玉の移動先に相手の駒の利きがあれば、合法手でないので、false
		return !IsAttackersToIsNot0(THEM, move.To());
	}
	// 玉以外の駒の移動
	return !IsPinnedIllegal(from, move.To(), GetKingSquare(US), pinned);
}
template bool Position::IsPseudoLegalMoveIsLegal<false, false, Color::Black,Color::White>(const Move move, const Bitboard& pinned) const;
template bool Position::IsPseudoLegalMoveIsLegal<false, false, Color::White, Color::Black>(const Move move, const Bitboard& pinned) const;
template bool Position::IsPseudoLegalMoveIsLegal<false, true, Color::Black, Color::White>(const Move move, const Bitboard& pinned) const;
template bool Position::IsPseudoLegalMoveIsLegal<false, true, Color::White, Color::Black>(const Move move, const Bitboard& pinned) const;
template bool Position::IsPseudoLegalMoveIsLegal<true, false, Color::Black, Color::White>(const Move move, const Bitboard& pinned) const;
template bool Position::IsPseudoLegalMoveIsLegal<true, false, Color::White, Color::Black>(const Move move, const Bitboard& pinned) const;


template<Color US,Color THEM>
bool Position::IsPseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const {
	assert(IsOK());

	// 玉の移動
	if (move.GetPieceTypeFrom() == N08_King) {
		// 遠隔駒で王手されたとき、王手している遠隔駒の利きには移動しないように指し手を生成している。
		// その為、移動先に他の駒の利きが無いか調べるだけで良い。
		const bool canMove = !IsAttackersToIsNot0(THEM, move.To());
		assert(canMove == IsPseudoLegalMoveIsLegal<false, US,THEM>(move, pinned));
		return canMove;
	}

	// 玉の移動以外
	Bitboard target = GetCheckersBB();
	const Square checkSq = target.PopFirstOneFromI9();

	if (target.Exists1Bit()) {
		// 両王手のとき、玉の移動以外の手は指せない。
		return false;
	}

	//const Color us = GetTurn();
	const Square to = move.To();
	// 移動、又は打った駒が、王手をさえぎるか、王手している駒を取る必要がある。
	target = g_betweenBb.GetBetweenBB(checkSq, GetKingSquare(US)) | GetCheckersBB();
	return g_setMaskBb.IsSet(&target, to) && IsPseudoLegalMoveIsLegal<false, true,US,THEM>(move, pinned);
}
template bool Position::IsPseudoLegalMoveIsEvasion<Color::Black, Color::White>(const Move move, const Bitboard& pinned) const;
template bool Position::IsPseudoLegalMoveIsEvasion<Color::White, Color::Black>(const Move move, const Bitboard& pinned) const;






// checkPawnDrop : 二歩と打ち歩詰めも調べるなら true
//                 これが true のとき、駒打ちの場合のみ Legal であることが確定する。
template<Color US, Color THEM>
bool Position::MoveIsPseudoLegal(const Move move, const bool checkPawnDrop) const {
	const Square to = move.To();

	if (move.IsDrop()) {
		const PieceType ptFrom = move.GetPieceTypeDropped();
		if (!this->GetHand<US>().Exists(ConvHandPiece::FromPieceType(ptFrom)) || GetPiece(to) != N00_Empty) {
			return false;
		}

		if (this->InCheck()) {
			// 王手されているので、合駒でなければならない。
			Bitboard target = this->GetCheckersBB();
			const Square checksq = target.PopFirstOneFromI9();

			if (target.Exists1Bit()) {
				// 両王手は合駒出来無い。
				return false;
			}

			target = g_betweenBb.GetBetweenBB(checksq, this->GetKingSquare<US>());
			if (!g_setMaskBb.IsSet(&target, to)) {
				// 玉と、王手した駒との間に駒を打っていない。
				return false;
			}
		}

		if (ptFrom == N01_Pawn && checkPawnDrop) {
			if ((this->GetBbOf20<US>(N01_Pawn) & g_fileMaskBb.GetFileMask(ConvSquare::TO_FILE10(to))).Exists1Bit()) {
				// 二歩
				return false;
			}
			const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);
			if (to + TDeltaN == this->GetKingSquare(THEM) && this->IsPawnDropCheckMate<US,THEM>(to)) {
				// 王手かつ打ち歩詰め
				return false;
			}
		}
	}
	else {
		const Square from = move.From();
		const PieceType ptFrom = move.GetPieceTypeFrom();
		if (GetPiece(from) != ConvPiece::FROM_COLOR_AND_PIECE_TYPE10<US>(ptFrom) || g_setMaskBb.IsSet(&this->GetBbOf10<US>(), to)) {
			return false;
		}

		if (!g_setMaskBb.IsSet( &UtilAttack::GetAttacksFrom(ptFrom, US, from, this->GetOccupiedBB()), to)) {
			return false;
		}

		if (InCheck()) {
			if (ptFrom == N08_King) {
				Bitboard occ = GetOccupiedBB();
				g_setMaskBb.ClearBit(&occ, from);
				if (this->IsAttackersToIsNot0(THEM, to, occ)) {
					// 王手から逃げていない。
					return false;
				}
			}
			else {
				// 玉以外の駒を移動させたとき。
				Bitboard target = GetCheckersBB();
				const Square checksq = target.PopFirstOneFromI9();

				if (target.Exists1Bit()) {
					// 両王手なので、玉が逃げない手は駄目
					return false;
				}

				target = g_betweenBb.GetBetweenBB(checksq, this->GetKingSquare<US>()) | GetCheckersBB();
				if (!g_setMaskBb.IsSet(&target, to)) {
					// 玉と、王手した駒との間に移動するか、王手した駒を取る以外は駄目。
					return false;
				}
			}
		}
	}

	return true;
}
template bool Position::MoveIsPseudoLegal<Color::Black,Color::White>(const Move move, const bool checkPawnDrop) const;
template bool Position::MoveIsPseudoLegal<Color::White, Color::Black>(const Move move, const bool checkPawnDrop) const;


#if !defined NDEBUG
// 過去(又は現在)に生成した指し手が現在の局面でも有効か判定。
// あまり速度が要求される場面で使ってはいけない。
bool Position::MoveIsLegal(const Move GetMove) const {
	return MoveList<N09_LegalAll>(*this).Contains(GetMove);
}
#endif

// 局面の更新
template<Color US, Color THEM>
void Position::DoMove(const Move move, StateInfo& newSt) {
	const CheckInfo ci(*this);
	DoMove<US, THEM>(move, newSt, ci, IsMoveGivesCheck(move, ci));
}
template void Position::DoMove<Color::Black, Color::White>(const Move move, StateInfo& newSt);
template void Position::DoMove<Color::White, Color::Black>(const Move move, StateInfo& newSt);

// 局面の更新
template<Color US, Color THEM>
void Position::DoMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck) {
	assert(IsOK());
	assert(!move.IsNone());
	assert(&newSt != m_st_);

	Key boardKey = GetBoardKey();
	Key handKey = GetHandKey();
	boardKey ^= GetZobTurn();

	memcpy(&newSt, m_st_, sizeof(StateInfoMin));
	newSt.m_previous = m_st_;
	m_st_ = &newSt;

	m_st_->m_cl.m_size = 1;

	const Square to = move.To();
	const PieceType ptCaptured = move.GetCap();
	PieceType ptTo;
	if (move.IsDrop()) {
		ptTo = move.GetPieceTypeDropped();
		const HandPiece hpTo = ConvHandPiece::FromPieceType(ptTo);

		handKey -= this->GetZobHand<US>(hpTo);
		boardKey += this->GetZobrist<US>(ptTo, to);

		prefetch(GetConstRucksack()->m_tt.FirstEntry(boardKey + handKey));

		const int handnum = this->GetHand<US>().NumOf(hpTo);
		const int listIndex = m_evalList_.m_squareHandToList[g_HandPieceToSquareHand[US][hpTo] + handnum];
		const Piece pcTo = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10<US>(ptTo);
		m_st_->m_cl.m_listindex[0] = listIndex;
		m_st_->m_cl.m_clistpair[0].m_oldlist[0] = m_evalList_.m_list0[listIndex];
		m_st_->m_cl.m_clistpair[0].m_oldlist[1] = m_evalList_.m_list1[listIndex];

		m_evalList_.m_list0[listIndex] = kppArray[pcTo] + to;
		m_evalList_.m_list1[listIndex] = kppArray[ConvPiece::INVERSE10(pcTo)] + ConvSquare::INVERSE10(to);
		m_evalList_.m_listToSquareHand[listIndex] = to;
		m_evalList_.m_squareHandToList[to] = listIndex;

		m_st_->m_cl.m_clistpair[0].m_newlist[0] = m_evalList_.m_list0[listIndex];
		m_st_->m_cl.m_clistpair[0].m_newlist[1] = m_evalList_.m_list1[listIndex];

		m_hand_[US].MinusOne(hpTo);
		this->XorBBs<US>(ptTo, to);
		m_piece_[to] = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10<US>(ptTo);

		if (moveIsCheck) {
			// Direct checks
			m_st_->m_checkersBB = g_setMaskBb.GetSetMaskBb(to);
			m_st_->m_continuousCheck[US] += 2;
		}
		else {
			m_st_->m_checkersBB = Bitboard::CreateAllZeroBB();
			m_st_->m_continuousCheck[US] = 0;
		}
	}
	else {
		const Square from = move.From();
		const PieceType ptFrom = move.GetPieceTypeFrom();
		ptTo = move.GetPieceTypeTo(ptFrom);

		g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptFrom], from);
		g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptTo], to);
		g_setMaskBb.XorBit(&m_BB_ByColor_[US], from, to);
		m_piece_[from] = N00_Empty;
		m_piece_[to] = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10<US>(ptTo);
		boardKey -= this->GetZobrist<US>(ptFrom, from);
		boardKey += this->GetZobrist<US>(ptTo, to);

		if (ptCaptured) {
			// 駒を取ったとき
			const HandPiece hpCaptured = ConvHandPiece::FromPieceType(ptCaptured);

			boardKey -= this->GetZobrist<THEM>(ptCaptured, to);
			handKey += this->GetZobHand<US>(hpCaptured);

			g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptCaptured], to);
			g_setMaskBb.XorBit(&m_BB_ByColor_[THEM], to);

			m_hand_[US].PlusOne(hpCaptured);
			const int toListIndex = m_evalList_.m_squareHandToList[to];
			m_st_->m_cl.m_listindex[1] = toListIndex;
			m_st_->m_cl.m_clistpair[1].m_oldlist[0] = m_evalList_.m_list0[toListIndex];
			m_st_->m_cl.m_clistpair[1].m_oldlist[1] = m_evalList_.m_list1[toListIndex];
			m_st_->m_cl.m_size = 2;

			const int handnum = this->GetHand<US>().NumOf(hpCaptured);
			m_evalList_.m_list0[toListIndex] = kppHandArray[US][hpCaptured] + handnum;
			m_evalList_.m_list1[toListIndex] = kppHandArray[THEM][hpCaptured] + handnum;
			const Square squarehand = g_HandPieceToSquareHand[US][hpCaptured] + handnum;
			m_evalList_.m_listToSquareHand[toListIndex] = squarehand;
			m_evalList_.m_squareHandToList[squarehand] = toListIndex;

			m_st_->m_cl.m_clistpair[1].m_newlist[0] = m_evalList_.m_list0[toListIndex];
			m_st_->m_cl.m_clistpair[1].m_newlist[1] = m_evalList_.m_list1[toListIndex];

			m_st_->m_material += (US == Black ?
				PieceScore::GetCapturePieceScore(ptCaptured) :
				-PieceScore::GetCapturePieceScore(ptCaptured)
			);
		}
		prefetch(GetConstRucksack()->m_tt.FirstEntry(boardKey + handKey));
		// Occupied は to, from の位置のビットを操作するよりも、
		// Black と White の or を取る方が速いはず。
		m_BB_ByPiecetype_[N00_Occupied] = this->GetBbOf10<Color::Black>() | this->GetBbOf10<Color::White>();

		if (ptTo == N08_King) {
			m_kingSquare_[US] = to;
		}
		else {
			const Piece pcTo = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10<US>(ptTo);
			const int fromListIndex = m_evalList_.m_squareHandToList[from];

			m_st_->m_cl.m_listindex[0] = fromListIndex;
			m_st_->m_cl.m_clistpair[0].m_oldlist[0] = m_evalList_.m_list0[fromListIndex];
			m_st_->m_cl.m_clistpair[0].m_oldlist[1] = m_evalList_.m_list1[fromListIndex];

			m_evalList_.m_list0[fromListIndex] = kppArray[pcTo] + to;
			m_evalList_.m_list1[fromListIndex] = kppArray[ConvPiece::INVERSE10(pcTo)] + ConvSquare::INVERSE10(to);
			m_evalList_.m_listToSquareHand[fromListIndex] = to;
			m_evalList_.m_squareHandToList[to] = fromListIndex;

			m_st_->m_cl.m_clistpair[0].m_newlist[0] = m_evalList_.m_list0[fromListIndex];
			m_st_->m_cl.m_clistpair[0].m_newlist[1] = m_evalList_.m_list1[fromListIndex];
		}

		if (move.IsPromotion()) {
			m_st_->m_material += (US == Black ?
				(PieceScore::GetPieceScore(ptTo) - PieceScore::GetPieceScore(ptFrom))
				: -(PieceScore::GetPieceScore(ptTo) - PieceScore::GetPieceScore(ptFrom)));
		}

		if (moveIsCheck) {
			// Direct checks
			m_st_->m_checkersBB = ci.m_checkBB[ptTo] & g_setMaskBb.GetSetMaskBb(to);

			// Discovery checks
			const Square ksq = this->GetKingSquare<THEM>();
			if (IsDiscoveredCheck(from, to, ksq, ci.m_dcBB)) {
				g_bonaDirArray[g_squareRelation.GetSquareRelation(from, ksq)]->Do2Move(*this, from, ksq, US);
			}
			m_st_->m_continuousCheck[US] += 2;
		}
		else {
			m_st_->m_checkersBB = Bitboard::CreateAllZeroBB();
			m_st_->m_continuousCheck[US] = 0;
		}
	}
	m_goldsBB_ = this->GetBbOf(N07_Gold, N09_ProPawn, N10_ProLance, N11_ProKnight, N12_ProSilver);

	m_st_->m_boardKey = boardKey;
	m_st_->m_handKey = handKey;
	++m_st_->m_pliesFromNull;

	m_turn_ = THEM;
	m_st_->m_hand = GetHand(this->GetTurn());

	assert(IsOK());
}
template void Position::DoMove<Color::Black, Color::White>(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck);
template void Position::DoMove<Color::White, Color::Black>(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck);


void Position::UndoMove(const Move move) {
	assert(IsOK());
	assert(!move.IsNone());

	// themとusが普段とは逆☆（＾ｑ＾）
	const Color them = GetTurn();
	const Color us = ConvColor::OPPOSITE_COLOR10b(them);

	const Square to = move.To();
	m_turn_ = us;
	// ここで先に turn_ を戻したので、以下、move は us の指し手とする。
	if (move.IsDrop()) {
		const PieceType ptTo = move.GetPieceTypeDropped();
		g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptTo], to);
		g_setMaskBb.XorBit(&m_BB_ByColor_[us], to);
		m_piece_[to] = N00_Empty;

		const HandPiece hp = ConvHandPiece::FromPieceType(ptTo);
		m_hand_[us].PlusOne(hp);

		const int toListIndex = m_evalList_.m_squareHandToList[to];
		const int handnum = GetHand(us).NumOf(hp);
		m_evalList_.m_list0[toListIndex] = kppHandArray[us][hp] + handnum;
		m_evalList_.m_list1[toListIndex] = kppHandArray[them][hp] + handnum;
		const Square squarehand = g_HandPieceToSquareHand[us][hp] + handnum;
		m_evalList_.m_listToSquareHand[toListIndex] = squarehand;
		m_evalList_.m_squareHandToList[squarehand] = toListIndex;
	}
	else {
		const Square from = move.From();
		const PieceType ptFrom = move.GetPieceTypeFrom();
		const PieceType ptTo = move.GetPieceTypeTo(ptFrom);
		const PieceType ptCaptured = move.GetCap(); // todo: st_->capturedType 使えば良い。

		if (ptTo == N08_King) {
			m_kingSquare_[us] = from;
		}
		else {
			const Piece pcFrom = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10(us, ptFrom);
			const int toListIndex = m_evalList_.m_squareHandToList[to];
			m_evalList_.m_list0[toListIndex] = kppArray[pcFrom] + from;
			m_evalList_.m_list1[toListIndex] = kppArray[ConvPiece::INVERSE10(pcFrom)] + ConvSquare::INVERSE10(from);
			m_evalList_.m_listToSquareHand[toListIndex] = from;
			m_evalList_.m_squareHandToList[from] = toListIndex;
		}

		if (ptCaptured) {
			// 駒を取ったとき
			g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptCaptured], to);
			g_setMaskBb.XorBit(&m_BB_ByColor_[them], to);
			const HandPiece hpCaptured = ConvHandPiece::FromPieceType(ptCaptured);
			const Piece pcCaptured = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10(them, ptCaptured);
			m_piece_[to] = pcCaptured;

			const int handnum = GetHand(us).NumOf(hpCaptured);
			const int toListIndex = m_evalList_.m_squareHandToList[g_HandPieceToSquareHand[us][hpCaptured] + handnum];
			m_evalList_.m_list0[toListIndex] = kppArray[pcCaptured] + to;
			m_evalList_.m_list1[toListIndex] = kppArray[ConvPiece::INVERSE10(pcCaptured)] + ConvSquare::INVERSE10(to);
			m_evalList_.m_listToSquareHand[toListIndex] = to;
			m_evalList_.m_squareHandToList[to] = toListIndex;

			m_hand_[us].MinusOne(hpCaptured);
		}
		else {
			// 駒を取らないときは、UtilPiece::colorAndPieceTypeToPiece(us, ptCaptured) は 0 または 16 になる。
			// 16 になると困るので、駒を取らないときは明示的に Empty にする。
			m_piece_[to] = N00_Empty;
		}
		g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptFrom], from);
		g_setMaskBb.XorBit(&m_BB_ByPiecetype_[ptTo], to);
		g_setMaskBb.XorBit(&m_BB_ByColor_[us], from, to);
		m_piece_[from] = ConvPiece::FROM_COLOR_AND_PIECE_TYPE10(us, ptFrom);
	}
	// Occupied は to, from の位置のビットを操作するよりも、
	// Black と White の or を取る方が速いはず。
	m_BB_ByPiecetype_[N00_Occupied] = this->GetBbOf10(Black) | this->GetBbOf10(White);
	m_goldsBB_ = this->GetBbOf(N07_Gold, N09_ProPawn, N10_ProLance, N11_ProKnight, N12_ProSilver);

	// key などは StateInfo にまとめられているので、
	// previous のポインタを st_ に代入するだけで良い。
	m_st_ = m_st_->m_previous;

	assert(IsOK());
}



template<Color US, Color THEM>
ScoreIndex Position::GetSee1(const Move move, const int asymmThreshold) const {
	const Square to = move.To();
	Square from;
	PieceType ptCaptured;
	Bitboard occ = GetOccupiedBB();
	Bitboard attackers;
	Bitboard opponentAttackers;


	ScoreIndex swapList[32];
	if (move.IsDrop()) {
		opponentAttackers = this->GetAttackersTo_clr(THEM, to, occ);
		if (!opponentAttackers.Exists1Bit()) {
			return ScoreZero;
		}
		attackers = opponentAttackers | this->GetAttackersTo_clr(US, to, occ);
		swapList[0] = ScoreZero;
		ptCaptured = move.GetPieceTypeDropped();
	}
	else {
		from = move.From();
		g_setMaskBb.XorBit(&occ, from);
		opponentAttackers = this->GetAttackersTo_clr(THEM, to, occ);
		if (!opponentAttackers.Exists1Bit()) {
			if (move.IsPromotion()) {
				const PieceType ptFrom = move.GetPieceTypeFrom();
				return PieceScore::GetCapturePieceScore(move.GetCap()) + PieceScore::GetPromotePieceScore(ptFrom);
			}
			return PieceScore::GetCapturePieceScore(move.GetCap());
		}
		attackers = opponentAttackers | this->GetAttackersTo_clr(US, to, occ);
		swapList[0] = PieceScore::GetCapturePieceScore(move.GetCap());
		ptCaptured = move.GetPieceTypeFrom();
		if (move.IsPromotion()) {
			const PieceType ptFrom = move.GetPieceTypeFrom();
			swapList[0] += PieceScore::GetPromotePieceScore(ptFrom);
			ptCaptured += PTPromote;
		}
	}

	// 相手の駒がぶつかっている所？の数だけ回っているのかだぜ☆？（＾ｑ＾）？
	int slIndex = 1;
	Color iCurrTurn = THEM; // ループ中にひっくり返るぜ☆（＾ｑ＾）
	do {
		swapList[slIndex] = -swapList[slIndex - 1] + PieceScore::GetCapturePieceScore(ptCaptured);

		// 再帰関数のスタート地点だぜ☆！（＾ｑ＾）
		// todo: 実際に移動した方向を基にattackersを更新すれば、template, inline を使用しなくても良さそう。
		//       その場合、キャッシュに乗りやすくなるので逆に速くなるかも。
		const PieceTypeSeeEvent ptsEvent(
			*this,
			to,
			opponentAttackers,
			iCurrTurn
		);
		ptCaptured = PiecetypePrograms::m_PIECETYPE_PROGRAMS[PieceType::N01_Pawn]->AppendToNextAttackerAndTryPromote(
			occ,
			attackers,
			PieceType::N02_Lance,
			ptsEvent
		);

		attackers &= occ;
		++slIndex;
		iCurrTurn = ConvColor::OPPOSITE_COLOR10b(iCurrTurn); // ループ中にひっくり返すぜ☆（＾ｑ＾）
		opponentAttackers = attackers & this->GetBbOf10(iCurrTurn);

		if (ptCaptured == N08_King) {
			if (opponentAttackers.Exists1Bit()) {
				swapList[slIndex++] = PieceScore::m_CaptureKingScore;
			}
			break;
		}
	} while (opponentAttackers.Exists1Bit());

	if (asymmThreshold) {
		for (int i = 0; i < slIndex; i += 2) {
			if (swapList[i] < asymmThreshold) {
				swapList[i] = -PieceScore::m_CaptureKingScore;
			}
		}
	}

	// nega max 的に駒の取り合いの点数を求める。
	while (--slIndex) {
		swapList[slIndex - 1] = std::min(-swapList[slIndex], swapList[slIndex - 1]);
	}
	return swapList[0];
}
template ScoreIndex Position::GetSee1<Color::Black, Color::White>(const Move move, const int asymmThreshold) const;
template ScoreIndex Position::GetSee1<Color::White, Color::Black>(const Move move, const int asymmThreshold) const;


ScoreIndex Position::GetSeeSign(const Move move) const {
	if (move.IsCapture()) {
		const PieceType ptFrom = move.GetPieceTypeFrom();
		const Square to = move.To();
		if (PieceScore::GetCapturePieceScore(ptFrom) <= PieceScore::GetCapturePieceScore(GetPiece(to))) {
			return static_cast<ScoreIndex>(1);
		}
	}
	return
		this->GetTurn()
		?
		GetSee1<Color::Black,Color::White>(move)
		:
		GetSee1<Color::White,Color::Black>(move)
		;
}

namespace {
	// them(相手) 側の玉が逃げられるか。
	// sq : 王手した相手の駒の位置。紐付きか、桂馬の位置とする。よって、玉は sq には行けない。
	// bb : sq の利きのある場所のBitboard。よって、玉は bb のビットが立っている場所には行けない。
	// sq と ksq の位置の Occupied Bitboard のみは、ここで更新して評価し、元に戻す。
	// (実際にはテンポラリのOccupied Bitboard を使うので、元には戻さない。)
	template<Color US,Color THEM>
	bool G_CanKingEscape(const Position& pos, const Square sq, const Bitboard& bb) {
		const Square ksq = pos.GetKingSquare(THEM);
		Bitboard kingMoveBB = bb.NotThisAnd(pos.GetBbOf10<THEM>().NotThisAnd(g_kingAttackBb.GetControllBb(ksq)));
		g_setMaskBb.ClearBit(&kingMoveBB, sq); // sq には行けないので、クリアする。xorBit(sq)ではダメ。

		if (kingMoveBB.Exists1Bit()) {
			Bitboard tempOccupied = pos.GetOccupiedBB();
			g_setMaskBb.AddBit(&tempOccupied, sq);
			g_setMaskBb.ClearBit(&tempOccupied, ksq);
			do {
				const Square to = kingMoveBB.PopFirstOneFromI9();
				// 玉の移動先に、us 側の利きが無ければ、true
				if (!pos.IsAttackersToIsNot0(US, to, tempOccupied)) {
					return true;
				}
			} while (kingMoveBB.Exists1Bit());
		}
		// 玉の移動先が無い。
		return false;
	}
	template bool G_CanKingEscape<Color::Black, Color::White>(const Position& pos, const Square sq, const Bitboard& bb);
	template bool G_CanKingEscape<Color::White, Color::Black>(const Position& pos, const Square sq, const Bitboard& bb);


	// them(相手) 側の玉以外の駒が sq にある us 側の駒を取れるか。
	bool canPieceCapture(const Position& pos, const Color them, const Square sq, const Bitboard& dcBB) {
		// 玉以外で打った駒を取れる相手側の駒の Bitboard
		Bitboard fromBB = pos.GetAttackersToExceptKing(them, sq);

		if (fromBB.Exists1Bit()) {
			const Square ksq = pos.GetKingSquare(them);
			do {
				const Square from = fromBB.PopFirstOneFromI9();
				if (!pos.IsDiscoveredCheck(from, sq, ksq, dcBB)) {
					// them 側から見て、pin されていない駒で、打たれた駒を取れるので、true
					return true;
				}
			} while (fromBB.Exists1Bit());
		}
		// 玉以外の駒で、打った駒を取れない。
		return false;
	}

	// pos.discoveredCheckBB<false>() を遅延評価するバージョン。
	bool canPieceCapture(const Position& pos, const Color them, const Square sq) {
		Bitboard fromBB = pos.GetAttackersToExceptKing(them, sq);

		if (fromBB.Exists1Bit()) {
			const Square ksq = pos.GetKingSquare(them);

			const Bitboard dcBB = pos.GetTurn() == Color::Black
				?
				pos.DiscoveredCheckBB<Color::Black, Color::White, false>()
				:
				pos.DiscoveredCheckBB<Color::White, Color::Black, false>()
				;

			do {
				const Square from = fromBB.PopFirstOneFromI9();
				if (!pos.IsDiscoveredCheck(from, sq, ksq, dcBB)) {
					// them 側から見て、pin されていない駒で、打たれた駒を取れるので、true
					return true;
				}
			} while (fromBB.Exists1Bit());
		}
		// 玉以外の駒で、打った駒を取れない。
		return false;
	}
}

bool Position::NoPawns(const Color us, const File toFile) const
{
	return !this->GetBbOf20(N01_Pawn, us).AndIsNot0(g_fileMaskBb.GetFileMask(toFile));
}

// us が sq へ歩を打ったとき、them の玉が詰むか。
// us が sq へ歩を打つのは王手であると仮定する。
// 打ち歩詰めのとき、true を返す。
template<Color US, Color THEM>
bool Position::IsPawnDropCheckMate(const Square sq) const {
	// 玉以外の駒で、打たれた歩が取れるなら、打ち歩詰めではない。
	if (canPieceCapture(*this, THEM, sq)) {
		return false;
	}
	// todo: ここで玉の位置を求めるのは、上位で求めたものと2重になるので無駄。後で整理すること。
	const Square ksq = this->GetKingSquare(THEM);

	// 玉以外で打った歩を取れないとき、玉が歩を取るか、玉が逃げるか。

	// 利きを求める際に、occupied の歩を打った位置の bit を立てた Bitboard を使用する。
	// ここでは歩の Bitboard は更新する必要がない。
	// color の Bitboard も更新する必要がない。(相手玉が動くとき、こちらの打った歩で玉を取ることは無い為。)
	const Bitboard tempOccupied = this->GetOccupiedBB() | g_setMaskBb.GetSetMaskBb(sq);
	Bitboard kingMoveBB = this->GetBbOf10<THEM>().NotThisAnd(g_kingAttackBb.GetControllBb(ksq));

	// 少なくとも歩を取る方向には玉が動けるはずなので、do while を使用。
	assert(kingMoveBB.Exists1Bit());
	do {
		const Square to = kingMoveBB.PopFirstOneFromI9();
		if (!IsAttackersToIsNot0(US, to, tempOccupied)) {
			// 相手玉の移動先に自駒の利きがないなら、打ち歩詰めではない。
			return false;
		}
	} while (kingMoveBB.Exists1Bit());

	return true;
}
template bool Position::IsPawnDropCheckMate<Color::Black, Color::White>(const Square sq) const;
template bool Position::IsPawnDropCheckMate<Color::White, Color::Black>(const Square sq) const;


inline void Position::XorBBs(const PieceType pt, const Square sq, const Color c) {
	g_setMaskBb.XorBit(&this->m_BB_ByPiecetype_[N00_Occupied], sq);
	g_setMaskBb.XorBit(&this->m_BB_ByPiecetype_[pt], sq);
	g_setMaskBb.XorBit(&this->m_BB_ByColor_[c], sq);
}

// 相手玉が1手詰みかどうかを判定。
// 1手詰みなら、詰みに至る指し手の一部の情報(from, to のみとか)を返す。
// 1手詰みでないなら、Move::moveNone() を返す。
// Bitboard の状態を途中で更新する為、const 関数ではない。(更新後、元に戻すが。)
template <Color US, Color THEM>
Move Position::GetMateMoveIn1Ply() {
	const Square ksq = GetKingSquare(THEM);
	const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);

	assert(!IsAttackersToIsNot0(THEM, GetKingSquare(US)));

	// 駒打ちを調べる。
	const Bitboard dropTarget = GetNOccupiedBB(); // emptyBB() ではないので注意して使うこと。
	const Hand ourHand = GetHand(US);
	// 王手する前の状態の dcBB。
	// 間にある駒は相手側の駒。
	// 駒打ちのときは、打った後も、打たれる前の状態の dcBB を使用する。
	const Bitboard dcBB_betweenIsThem = DiscoveredCheckBB<US, THEM, false>();

	// 飛車打ち
	if (Hand::Exists_HRook(ourHand)) {
		// 合駒されるとややこしいので、3手詰み関数の中で調べる。
		// ここでは離れた位置から王手するのは考えない。
		Bitboard toBB = dropTarget & g_rookAttackBb.RookStepAttacks(ksq);
		while (toBB.Exists1Bit()) {
			const Square to = toBB.PopFirstOneFromI9();
			// 駒を打った場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
			if (IsAttackersToIsNot0(US, to)) {
				// 玉が逃げられず、他の駒で取ることも出来ないか
				if (!G_CanKingEscape<US,THEM>(*this, to, g_rookAttackBb.GetControllBbToEdge(to))
					&& !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem))
				{
					return ConvMove::Convert30_MakeDropMove_da(g_PTROOK_DA_AS_MOVE, to);
				}
			}
		}
	}
	// 香車打ち
	// 飛車で詰まなければ香車でも詰まないので、else if を使用。
	// 玉が 9(1) 段目にいれば香車で王手出来無いので、それも省く。
	else if (
		Hand::Exists_HLance(ourHand) &&
		ConvSquare::IS_IN_FRONT_OF10(US, Rank1, Rank9, ConvSquare::TO_RANK10(ksq))
	) {
		const Square to = ksq + TDeltaS;
		if (GetPiece(to) == N00_Empty && IsAttackersToIsNot0(US, to)) {
			if (!G_CanKingEscape<US,THEM>(*this, to, g_lanceAttackBb.GetControllBbToEdge(US, to))
				&& !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem))
			{
				return ConvMove::Convert30_MakeDropMove_da(g_PTLANCE_DA_AS_MOVE, to);
			}
		}
	}

	// 角打ち
	if (Hand::Exists_HBishop(ourHand)) {
		Bitboard toBB = dropTarget & g_bishopAttackBb.BishopStepAttacks(ksq);
		while (toBB.Exists1Bit()) {
			const Square to = toBB.PopFirstOneFromI9();
			if (IsAttackersToIsNot0(US, to)) {
				if (!G_CanKingEscape<US,THEM>(*this, to, g_bishopAttackBb.GetControllBbToEdge(to))
					&& !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem))
				{
					return ConvMove::Convert30_MakeDropMove_da(g_PTBISHOP_DA_AS_MOVE, to);
				}
			}
		}
	}

	// 金打ち
	if (Hand::Exists_HGold(ourHand)) {
		Bitboard toBB;
		if (Hand::Exists_HRook(ourHand)) {
			// 飛車打ちを先に調べたので、尻金だけは省く。
			toBB = dropTarget & (g_goldAttackBb.GetControllBb(THEM, ksq) ^ g_pawnAttackBb.GetControllBb(US, ksq));
		}
		else {
			toBB = dropTarget & g_goldAttackBb.GetControllBb(THEM, ksq);
		}
		while (toBB.Exists1Bit()) {
			const Square to = toBB.PopFirstOneFromI9();
			if (IsAttackersToIsNot0(US, to)) {
				if (!G_CanKingEscape<US,THEM>(*this, to, g_goldAttackBb.GetControllBb(US, to))
					&& !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem))
				{
					return ConvMove::Convert30_MakeDropMove_da(g_PTGOLD_DA_AS_MOVE, to);
				}
			}
		}
	}

	if (Hand::Exists_HSilver(ourHand)) {
		Bitboard toBB;
		if (Hand::Exists_HGold(ourHand)) {
			// 金打ちを先に調べたので、斜め後ろから打つ場合だけを調べる。

			if (Hand::Exists_HBishop(ourHand)) {
				// 角打ちを先に調べたので、斜めからの王手も除外できる。銀打ちを調べる必要がない。
				goto silver_drop_end;
			}
			// 斜め後ろから打つ場合を調べる必要がある。
			toBB = dropTarget & (g_silverAttackBb.GetControllBb(THEM, ksq) & g_inFrontMaskBb.GetInFrontMask(US, ConvSquare::TO_RANK10(ksq)));
		}
		else {
			if (Hand::Exists_HBishop(ourHand)) {
				// 斜め後ろを除外。前方から打つ場合を調べる必要がある。
				toBB = dropTarget & g_goldAndSilverAttackBb.GoldAndSilverAttacks(THEM, ksq);
			}
			else {
				toBB = dropTarget & g_silverAttackBb.GetControllBb(THEM, ksq);
			}
		}
		while (toBB.Exists1Bit()) {
			const Square to = toBB.PopFirstOneFromI9();
			if (IsAttackersToIsNot0(US, to)) {
				if (!G_CanKingEscape<US, THEM>(*this, to, g_silverAttackBb.GetControllBb(US, to))
					&& !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem))
				{
					return ConvMove::Convert30_MakeDropMove_da(g_PTSILVER_DA_AS_MOVE, to);
				}
			}
		}
	}
silver_drop_end:

	if (Hand::Exists_HKnight(ourHand)) {
		Bitboard toBB = dropTarget & g_knightAttackBb.GetControllBb(THEM, ksq);
		while (toBB.Exists1Bit()) {
			const Square to = toBB.PopFirstOneFromI9();
			// 桂馬は紐が付いている必要はない。
			// よって、このG_CanKingEscape() 内での to の位置に逃げられないようにする処理は無駄。
			if (!G_CanKingEscape<US, THEM>(*this, to, Bitboard::CreateAllZeroBB())
				&& !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem))
			{
				return ConvMove::Convert30_MakeDropMove_da(g_PTKNIGHT_DA_AS_MOVE, to);
			}
		}
	}

	// 歩打ちで詰ますと反則なので、調べない。

	// 駒を移動する場合
	// moveTarget は桂馬以外の移動先の大まかな位置。飛角香の遠隔王手は含まない。
	const Bitboard moveTarget = this->GetBbOf10(US).NotThisAnd(g_kingAttackBb.GetControllBb(ksq));
	const Bitboard pinned = this->GetPinnedBB();
	const Bitboard dcBB_betweenIsUs = this->DiscoveredCheckBB<US, THEM, true>();

	{
		// 竜による移動
		Bitboard fromBB = this->GetBbOf20(N14_Dragon, US);
		while (fromBB.Exists1Bit()) {
			const Square from = fromBB.PopFirstOneFromI9();
			// 遠隔王手は考えない。
			const PieceTypeEvent ptEvent1(this->GetOccupiedBB(), Color::Null, from);
			Bitboard toBB = moveTarget & PiecetypePrograms::m_DRAGON.GetAttacks2From(ptEvent1);
			if (toBB.Exists1Bit()) {
				XorBBs(N14_Dragon, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

				const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

				// to の位置の Bitboard は G_CanKingEscape の中で更新する。
				do {
					const Square to = toBB.PopFirstOneFromI9();
					// 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
					if (IsUnDropCheckIsSupported(US, to)) {
						// 玉が逃げられない
						// かつ、(空き王手 または 他の駒で取れない)
						// かつ、王手した駒が pin されていない
						const PieceTypeEvent ptEvent2(
							this->GetOccupiedBB() ^ g_setMaskBb.GetSetMaskBb(ksq), Color::Null, to);
						if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_DRAGON.GetAttacks2From(ptEvent2))
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							XorBBs(N14_Dragon, from, US);
							return UtilMovePos::MakeCaptureMove(g_PTDRAGON_ONBOARD_AS_MOVE,	from, to, *this);
						}
					}
				} while (toBB.Exists1Bit());
				XorBBs(N14_Dragon, from, US);
			}
		}
	}

	// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
	const Rank TRank6 = (US == Black ? Rank6 : Rank4);
	const Bitboard TRank789BB = g_inFrontMaskBb.GetInFrontMask(US, TRank6);
	{
		// 飛車による移動
		Bitboard fromBB = this->GetBbOf20(N06_Rook, US);
		Bitboard fromOn789BB = fromBB & TRank789BB;
		// from が 123 段目
		if (fromOn789BB.Exists1Bit()) {
			fromBB.AndEqualNot(TRank789BB);
			do {
				const Square from = fromOn789BB.PopFirstOneFromI9();
				const PieceTypeEvent ptEvent1(this->GetOccupiedBB(), Color::Null, from);
				Bitboard toBB = moveTarget & PiecetypePrograms::m_ROOK.GetAttacks2From(ptEvent1);
				if (toBB.Exists1Bit()) {
					XorBBs(N06_Rook, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

					// to の位置の Bitboard は G_CanKingEscape の中で更新する。
					do {
						const Square to = toBB.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							const PieceTypeEvent ptEvent2(this->GetOccupiedBB() ^ g_setMaskBb.GetSetMaskBb(ksq), Color::Null, to);
							if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_DRAGON.GetAttacks2From(ptEvent2))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								XorBBs(N06_Rook, from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTROOK_ONBOARD_AS_MOVE,from, to, *this);
							}
						}
					} while (toBB.Exists1Bit());
					XorBBs(N06_Rook, from, US);
				}
			} while (fromOn789BB.Exists1Bit());
		}

		// from が 4~9 段目
		while (fromBB.Exists1Bit()) {
			const Square from = fromBB.PopFirstOneFromI9();
			const PieceTypeEvent ptEvent3(this->GetOccupiedBB(), Color::Null, from);
			Bitboard toBB = moveTarget & PiecetypePrograms::m_ROOK.GetAttacks2From(ptEvent3) &
				(g_rookAttackBb.RookStepAttacks(ksq) | TRank789BB);
			if (toBB.Exists1Bit()) {
				XorBBs(N06_Rook, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

				const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();
				Bitboard toOn789BB = toBB & TRank789BB;
				// 成り
				if (toOn789BB.Exists1Bit()) {
					do {
						const Square to = toOn789BB.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							const PieceTypeEvent ptEvent4(
								this->GetOccupiedBB() ^ g_setMaskBb.GetSetMaskBb(ksq), Color::Null, to);
							if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_DRAGON.GetAttacks2From(ptEvent4))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								XorBBs(N06_Rook, from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTROOK_ONBOARD_AS_MOVE,from, to, *this);
							}
						}
					} while (toOn789BB.Exists1Bit());

					toBB.AndEqualNot(TRank789BB);
				}
				// 不成
				while (toBB.Exists1Bit()) {
					const Square to = toBB.PopFirstOneFromI9();
					if (IsUnDropCheckIsSupported(US, to)) {
						if (!G_CanKingEscape<US, THEM>(*this, to, g_rookAttackBb.GetControllBbToEdge(to))
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							XorBBs(N06_Rook, from, US);
							return UtilMovePos::MakeCaptureMove(g_PTROOK_ONBOARD_AS_MOVE,from, to, *this);
						}
					}
				}
				XorBBs(N06_Rook, from, US);
			}
		}
	}

	{
		// 馬による移動
		Bitboard fromBB = this->GetBbOf20(N13_Horse, US);
		while (fromBB.Exists1Bit()) {
			const Square from = fromBB.PopFirstOneFromI9();
			// 遠隔王手は考えない。
			const PieceTypeEvent ptEvent1(this->GetOccupiedBB(), Color::Null, from);
			Bitboard toBB = moveTarget & PiecetypePrograms::m_HORSE.GetAttacks2From(ptEvent1);
			if (toBB.Exists1Bit()) {
				XorBBs(N13_Horse, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

				const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

				// to の位置の Bitboard は G_CanKingEscape の中で更新する。
				do {
					const Square to = toBB.PopFirstOneFromI9();
					// 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
					if (IsUnDropCheckIsSupported(US, to)) {
						// 玉が逃げられない
						// かつ、(空き王手 または 他の駒で取れない)
						// かつ、動かした駒が pin されていない)
						if (!G_CanKingEscape<US, THEM>(*this, to, g_horseAttackBb.GetControllBbToEdge(to)) // 竜の場合と違って、常に最大の利きを使用して良い。
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							XorBBs(N13_Horse, from, US);
							return UtilMovePos::MakeCaptureMove(g_PTHORSE_ONBOARD_AS_MOVE,from, to, *this);
						}
					}
				} while (toBB.Exists1Bit());
				XorBBs(N13_Horse, from, US);
			}
		}
	}

	{
		// 角による移動
		Bitboard fromBB = this->GetBbOf20(N05_Bishop, US);
		Bitboard fromOn789BB = fromBB & TRank789BB;
		// from が 123 段目
		if (fromOn789BB.Exists1Bit()) {
			fromBB.AndEqualNot(TRank789BB);
			do {
				const Square from = fromOn789BB.PopFirstOneFromI9();
				const PieceTypeEvent ptEvent1(this->GetOccupiedBB(), Color::Null, from);
				Bitboard toBB = moveTarget & PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent1);
				if (toBB.Exists1Bit()) {
					XorBBs(N05_Bishop, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

					// to の位置の Bitboard は G_CanKingEscape の中で更新する。
					do {
						const Square to = toBB.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							if (!G_CanKingEscape<US, THEM>(*this, to, g_horseAttackBb.GetControllBbToEdge(to))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								XorBBs(N05_Bishop, from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTBISHOP_ONBOARD_AS_MOVE,	from, to, *this);
							}
						}
					} while (toBB.Exists1Bit());
					XorBBs(N05_Bishop, from, US);
				}
			} while (fromOn789BB.Exists1Bit());
		}

		// from が 4~9 段目
		while (fromBB.Exists1Bit()) {
			const Square from = fromBB.PopFirstOneFromI9();
			const PieceTypeEvent ptEvent2(this->GetOccupiedBB(), Color::Null, from);
			Bitboard toBB = moveTarget & PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent2) & (g_bishopAttackBb.BishopStepAttacks(ksq) | TRank789BB);
			if (toBB.Exists1Bit()) {
				XorBBs(N05_Bishop, from, US);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

				const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

				Bitboard toOn789BB = toBB & TRank789BB;
				// 成り
				if (toOn789BB.Exists1Bit()) {
					do {
						const Square to = toOn789BB.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							if (!G_CanKingEscape<US, THEM>(*this, to, g_horseAttackBb.GetControllBbToEdge(to))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								XorBBs(N05_Bishop, from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTBISHOP_ONBOARD_AS_MOVE,	from, to, *this);
							}
						}
					} while (toOn789BB.Exists1Bit());

					toBB.AndEqualNot(TRank789BB);
				}
				// 不成
				while (toBB.Exists1Bit()) {
					const Square to = toBB.PopFirstOneFromI9();
					if (IsUnDropCheckIsSupported(US, to)) {
						if (!G_CanKingEscape<US, THEM>(*this, to, g_bishopAttackBb.GetControllBbToEdge(to))
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							XorBBs(N05_Bishop, from, US);
							return UtilMovePos::MakeCaptureMove(g_PTBISHOP_ONBOARD_AS_MOVE,	from, to, *this);
						}
					}
				}
				XorBBs(N05_Bishop, from, US);
			}
		}
	}

	{
		// 金、成り金による移動
		Bitboard fromBB = GetGoldsBB(US) & g_goldAttackBb.GoldCheckTable(US, ksq);
		while (fromBB.Exists1Bit()) {
			const Square from = fromBB.PopFirstOneFromI9();
			const PieceTypeEvent ptEvent1a(g_nullBitboard, US, from);
			const PieceTypeEvent ptEvent1b(g_nullBitboard, THEM, ksq);
			Bitboard toBB = moveTarget & PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1a) &
				PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1b);
			if (toBB.Exists1Bit()) {
				const PieceType pt = ConvPiece::TO_PIECE_TYPE10(GetPiece(from));
				XorBBs(pt, from, US);
				g_setMaskBb.XorBit(&m_goldsBB_, from);
				// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

				const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

				// to の位置の Bitboard は G_CanKingEscape の中で更新する。
				do {
					const Square to = toBB.PopFirstOneFromI9();
					// 王手した駒の場所に自駒の利きがあるか。(無ければ玉で取られて詰まない)
					if (IsUnDropCheckIsSupported(US, to)) {
						// 玉が逃げられない
						// かつ、(空き王手 または 他の駒で取れない)
						// かつ、動かした駒が pin されていない)
						const PieceTypeEvent ptEvent2(g_nullBitboard, US, to);
						if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent2))
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							XorBBs(pt, from, US);
							g_setMaskBb.XorBit(&m_goldsBB_, from);
							return UtilMovePos::MakeCaptureMove(ConvMove::FROM_PIECETYPE_ONBOARD10(pt),from, to, *this);
						}
					}
				} while (toBB.Exists1Bit());
				XorBBs(pt, from, US);
				g_setMaskBb.XorBit(&m_goldsBB_, from);
			}
		}
	}

	{
		// 銀による移動
		Bitboard fromBB = this->GetBbOf20(N04_Silver, US) & g_silverAttackBb.SilverCheckTable(US, ksq);
		if (fromBB.Exists1Bit()) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const Bitboard TRank1_5BB = g_inFrontMaskBb.GetInFrontMask(THEM, TRank6);
			const PieceTypeEvent ptEvent1(g_nullBitboard, THEM, ksq);
			const Bitboard chkBB = PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent1);
			const Bitboard chkBB_promo = PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1);

			Bitboard fromOn789BB = fromBB & TRank789BB;
			// from が敵陣
			if (fromOn789BB.Exists1Bit()) {
				fromBB.AndEqualNot(TRank789BB);
				do {
					const Square from = fromOn789BB.PopFirstOneFromI9();
					const PieceTypeEvent ptEvent2(g_nullBitboard, US, from);
					Bitboard toBB = moveTarget & PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent2);
					Bitboard toBB_promo = toBB & chkBB_promo;

					toBB &= chkBB;
					if ((toBB_promo | toBB).Exists1Bit()) {
						XorBBs(N04_Silver, from, US);
						// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

						const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

						// to の位置の Bitboard は G_CanKingEscape の中で更新する。
						while (toBB_promo.Exists1Bit()) {
							const Square to = toBB_promo.PopFirstOneFromI9();
							if (IsUnDropCheckIsSupported(US, to)) {
								// 成り
								const PieceTypeEvent ptEvent3(g_nullBitboard, US, to);
								if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent3))
									&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
										|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
									&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
								{
									XorBBs(N04_Silver, from, US);
									return UtilMovePos::MakeCapturePromoteMove(	g_PTSILVER_ONBOARD_AS_MOVE,	from, to, *this);
								}
							}
						}

						// 玉の前方に移動する場合、成で詰まなかったら不成でも詰まないので、ここで省く。
						// sakurapyon の作者が言ってたので実装。
						toBB.AndEqualNot(g_inFrontMaskBb.GetInFrontMask(THEM, ConvSquare::TO_RANK10(ksq)));
						while (toBB.Exists1Bit()) {
							const Square to = toBB.PopFirstOneFromI9();
							if (IsUnDropCheckIsSupported(US, to)) {
								// 不成
								const PieceTypeEvent ptEvent4(g_nullBitboard, US, to);
								if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent4))
									&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
										|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
									&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
								{
									XorBBs(N04_Silver, from, US);
									return UtilMovePos::MakeCaptureMove(g_PTSILVER_ONBOARD_AS_MOVE,	from, to, *this);
								}
							}
						}

						this->XorBBs(N04_Silver, from, US);
					}
				} while (fromOn789BB.Exists1Bit());
			}

			// from が 5~9段目 (必ず不成)
			Bitboard fromOn1_5BB = fromBB & TRank1_5BB;
			if (fromOn1_5BB.Exists1Bit()) {
				fromBB.AndEqualNot(TRank1_5BB);
				do {
					const Square from = fromOn1_5BB.PopFirstOneFromI9();
					const PieceTypeEvent ptEvent5(g_nullBitboard, US, from);
					Bitboard toBB = moveTarget & PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent5) & chkBB;

					if (toBB.Exists1Bit()) {
						this->XorBBs(N04_Silver, from, US);
						// 動いた後の dcBB, pinned: to の位置の occupied や checkers は関係ないので、ここで生成できる。

						const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

						// to の位置の Bitboard は G_CanKingEscape の中で更新する。
						while (toBB.Exists1Bit()) {
							const Square to = toBB.PopFirstOneFromI9();
							if (IsUnDropCheckIsSupported(US, to)) {
								// 不成
								const PieceTypeEvent ptEvent6(g_nullBitboard, US, to);
								if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent6))
									&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
										|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
									&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
								{
									this->XorBBs(N04_Silver, from, US);
									return UtilMovePos::MakeCaptureMove(g_PTSILVER_ONBOARD_AS_MOVE,	from, to, *this);
								}
							}
						}

						this->XorBBs(N04_Silver, from, US);
					}
				} while (fromOn1_5BB.Exists1Bit());
			}

			// 残り 4 段目のみ
			// 前進するときは成れるが、後退するときは成れない。
			while (fromBB.Exists1Bit()) {
				const Square from = fromBB.PopFirstOneFromI9();
				const PieceTypeEvent ptEvent7(g_nullBitboard, US, from);
				Bitboard toBB = moveTarget & PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent7);
				Bitboard toBB_promo = toBB & TRank789BB & chkBB_promo; // 3 段目にしか成れない。

				toBB &= chkBB;
				if ((toBB_promo | toBB).Exists1Bit()) {
					this->XorBBs(N04_Silver, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();
					// to の位置の Bitboard は G_CanKingEscape の中で更新する。
					while (toBB_promo.Exists1Bit()) {
						const Square to = toBB_promo.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							// 成り
							const PieceTypeEvent ptEvent8(g_nullBitboard, US, to);
							if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent8))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								this->XorBBs(N04_Silver, from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTSILVER_ONBOARD_AS_MOVE,	from, to, *this);
							}
						}
					}

					while (toBB.Exists1Bit()) {
						const Square to = toBB.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							// 不成
							const PieceTypeEvent ptEvent9(g_nullBitboard, US, to);
							if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent9))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								this->XorBBs(N04_Silver, from, US);
								return UtilMovePos::MakeCaptureMove(g_PTSILVER_ONBOARD_AS_MOVE,	from, to, *this);
							}
						}
					}

					this->XorBBs(N04_Silver, from, US);
				}
			}
		}
	}

	{
		// 桂による移動
		Bitboard fromBB = this->GetBbOf20(N03_Knight, US) & g_knightAttackBb.KnightCheckTable(US, ksq);
		if (fromBB.Exists1Bit()) {
			const PieceTypeEvent ptEvent1(g_nullBitboard, THEM, ksq);
			const Bitboard chkBB_promo = PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1) & TRank789BB;
			const Bitboard chkBB = PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEvent1);

			do {
				const Square from = fromBB.PopFirstOneFromI9();
				const PieceTypeEvent ptEvent2(g_nullBitboard, US, from);
				Bitboard toBB = this->GetBbOf10(US).NotThisAnd(PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEvent2));
				Bitboard toBB_promo = toBB & chkBB_promo;
				toBB &= chkBB;
				if ((toBB_promo | toBB).Exists1Bit()) {
					this->XorBBs(N03_Knight, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

					// to の位置の Bitboard は G_CanKingEscape の中で更新する。
					while (toBB_promo.Exists1Bit()) {
						const Square to = toBB_promo.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							// 成り
							const PieceTypeEvent ptEvent3(g_nullBitboard, US, to);
							if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent3))
								&& (IsDiscoveredCheck<true>(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal<true>(from, to, GetKingSquare(US), pinned))
							{
								this->XorBBs(N03_Knight, from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTKNIGHT_ONBOARD_AS_MOVE,	from, to, *this);
							}
						}
					}

					while (toBB.Exists1Bit()) {
						const Square to = toBB.PopFirstOneFromI9();
						// 桂馬は紐が付いてなくて良いので、紐が付いているかは調べない。
						// 不成
						if (!G_CanKingEscape<US, THEM>(*this, to, Bitboard::CreateAllZeroBB())
							&& (IsDiscoveredCheck<true>(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal<true>(from, to, GetKingSquare(US), pinned))
						{
							this->XorBBs(N03_Knight, from, US);
							return UtilMovePos::MakeCaptureMove(g_PTKNIGHT_ONBOARD_AS_MOVE, from, to, *this);
						}
					}
					this->XorBBs(N03_Knight, from, US);
				}
			} while (fromBB.Exists1Bit());
		}
	}

	{
		// 香車による移動
		Bitboard fromBB = this->GetBbOf20(N02_Lance, US) & g_lanceAttackBb.LanceCheckTable(US, ksq);
		if (fromBB.Exists1Bit()) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
			const Rank TRank8 = (US == Black ? Rank8 : Rank2);
			const PieceTypeEvent ptEvent1(g_nullBitboard, THEM, ksq);
			const Bitboard chkBB_promo = PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1) & TRank789BB;
			// 玉の前方1マスのみ。
			// 玉が 1 段目にいるときは、成のみで良いので省く。
			const Bitboard chkBB = PiecetypePrograms::m_PAWN.GetAttacks2From(ptEvent1) &
				g_inFrontMaskBb.GetInFrontMask(THEM, TRank8);

			do {
				const Square from = fromBB.PopFirstOneFromI9();
				const PieceTypeEvent ptEvent2(this->GetOccupiedBB(), US, from);
				Bitboard toBB = moveTarget & PiecetypePrograms::m_LANCE.GetAttacks2From(ptEvent2);
				Bitboard toBB_promo = toBB & chkBB_promo;

				toBB &= chkBB;

				if ((toBB_promo | toBB).Exists1Bit()) {
					this->XorBBs(N02_Lance, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();
					// to の位置の Bitboard は G_CanKingEscape の中で更新する。

					while (toBB_promo.Exists1Bit()) {
						const Square to = toBB_promo.PopFirstOneFromI9();
						if (IsUnDropCheckIsSupported(US, to)) {
							// 成り
							const PieceTypeEvent ptEvent3(g_nullBitboard, US, to);
							if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent3))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								this->XorBBs(N02_Lance,	from, US);
								return UtilMovePos::MakeCapturePromoteMove(	g_PTLANCE_ONBOARD_AS_MOVE,from, to, *this);
							}
						}
					}

					if (toBB.Exists1Bit()) {
						assert(toBB.IsOneBit());
						// 不成で王手出来るのは、一つの場所だけなので、ループにする必要が無い。
						const Square to = ksq + TDeltaS;
						if (IsUnDropCheckIsSupported(US, to)) {
							// 不成
							if (!G_CanKingEscape<US, THEM>(*this, to, g_lanceAttackBb.GetControllBbToEdge(US, to))
								&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
									|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
								&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
							{
								this->XorBBs(N02_Lance, from, US);
								return UtilMovePos::MakeCaptureMove(g_PTLANCE_ONBOARD_AS_MOVE,from, to, *this);
							}
						}
					}
					this->XorBBs(N02_Lance, from, US);
				}
			} while (fromBB.Exists1Bit());
		}
	}

	{
		// 歩による移動
		// 成れる場合は必ずなる。
		// todo: PawnCheckBB 作って簡略化する。
		const Rank krank = ConvSquare::TO_RANK10(ksq);
		// 歩が移動して王手になるのは、相手玉が1~7段目の時のみ。
		if (ConvSquare::IS_IN_FRONT_OF10(US, Rank2, Rank8, krank)) {
			// Txxx は先手、後手の情報を吸収した変数。数字は先手に合わせている。
			const SquareDelta TDeltaS = (US == Black ? DeltaS : DeltaN);
			const SquareDelta TDeltaN = (US == Black ? DeltaN : DeltaS);

			Bitboard fromBB = this->GetBbOf20(N01_Pawn, US);
			// 玉が敵陣にいないと成で王手になることはない。
			if (ConvSquare::IS_IN_FRONT_OF10(US, Rank6, Rank4, krank)) {
				// 成った時に王手になる位置
				const PieceTypeEvent ptEvent1(g_nullBitboard, THEM, ksq);
				const Bitboard toBB_promo = moveTarget & PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1) & TRank789BB;
				Bitboard fromBB_promo = fromBB & Bitboard::PawnAttack(toBB_promo, THEM);
				while (fromBB_promo.Exists1Bit()) {
					const Square from = fromBB_promo.PopFirstOneFromI9();
					const Square to = from + TDeltaN;

					this->XorBBs(N01_Pawn, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。

					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

					// to の位置の Bitboard は G_CanKingEscape の中で更新する。
					if (IsUnDropCheckIsSupported(US, to)) {
						// 成り
						const PieceTypeEvent ptEvent2(g_nullBitboard, US, to);
						if (!G_CanKingEscape<US, THEM>(*this, to, PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent2))
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							this->XorBBs(N01_Pawn, from, US);
							return UtilMovePos::MakeCapturePromoteMove(	g_PTPAWN_ONBOARD_AS_MOVE, from, to, *this);
						}
					}
					this->XorBBs(N01_Pawn, from, US);
				}
			}

			// 不成
			// 玉が 8,9 段目にいることは無いので、from,to が隣の筋を指すことは無い。
			const Square to = ksq + TDeltaS;
			const Square from = to + TDeltaS;
			if (g_setMaskBb.IsSet(&fromBB, from) && !g_setMaskBb.IsSet(&this->GetBbOf10(US), to)) {
				// 玉が 1, 2 段目にいるなら、成りで王手出来るので不成は調べない。
				if (ConvSquare::IS_BEHIND10<US>(Rank8, Rank2, krank)) {
					this->XorBBs(N01_Pawn, from, US);
					// 動いた後の dcBB: to の位置の occupied や checkers は関係ないので、ここで生成できる。
					const Bitboard dcBB_betweenIsThem_after = DiscoveredCheckBB<US, THEM, false>();

					// to の位置の Bitboard は G_CanKingEscape の中で更新する。
					if (IsUnDropCheckIsSupported(US, to)) {
						// 不成
						if (!G_CanKingEscape<US, THEM>(*this, to, Bitboard::CreateAllZeroBB())
							&& (IsDiscoveredCheck(from, to, ksq, dcBB_betweenIsUs)
								|| !canPieceCapture(*this, THEM, to, dcBB_betweenIsThem_after))
							&& !IsPinnedIllegal(from, to, GetKingSquare(US), pinned))
						{
							this->XorBBs(N01_Pawn, from, US);
							return UtilMovePos::MakeCaptureMove(g_PTPAWN_ONBOARD_AS_MOVE,from, to, *this);
						}
					}
					this->XorBBs(N01_Pawn, from, US);
				}
			}
		}
	}

	return g_MOVE_NONE;
}
template Move Position::GetMateMoveIn1Ply<Color::Black,Color::White>();
template Move Position::GetMateMoveIn1Ply<Color::White,Color::Black>();

/*
Move Position::GetMateMoveIn1Ply() {
	return (this->GetTurn() == Black
		?
		GetMateMoveIn1Ply<Color::Black, Color::White>()
		:
		GetMateMoveIn1Ply<Color::White, Color::Black>()
		);
}
*/

Ply Position::GetGamePly() const
{
	return this->m_gamePly_;
}

Key Position::GetBoardKey() const
{
	return this->m_st_->m_boardKey;
}

Key Position::GetHandKey() const
{
	return this->m_st_->m_handKey;
}

Key Position::GetKey() const
{
	return this->m_st_->GetKey();
}

Key Position::GetExclusionKey() const
{
	return this->m_st_->GetKey() ^ m_ZOB_EXCLUSION_;
}

Key Position::GetKeyExcludeTurn() const
{
	static_assert(this->m_zobTurn_ == 1, "");
	return this->GetKey() >> 1;
}

void Position::InitZobrist() {
	// zobTurn_ は 1 であり、その他は 1桁目を使わない。
	// zobTurn のみ xor で更新する為、他の桁に影響しないようにする為。
	// hash値の更新は普通は全て xor を使うが、持ち駒の更新の為に +, - を使用した方が都合が良い。
	for (PieceType pt = N00_Occupied; pt < g_PIECETYPE_NUM; ++pt) {
		for (Square sq = I9; sq < SquareNum; ++sq) {
			for (Color c = Black; c < g_COLOR_NUM; ++c) {
				m_ZOBRIST_[pt][sq][c] = g_mt64bit.GetRandom() & ~UINT64_C(1);
			}
		}
	}
	for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
		m_ZOB_HAND_[hp][Black] = g_mt64bit.GetRandom() & ~UINT64_C(1);
		m_ZOB_HAND_[hp][White] = g_mt64bit.GetRandom() & ~UINT64_C(1);
	}
	m_ZOB_EXCLUSION_ = g_mt64bit.GetRandom() & ~UINT64_C(1);
}

// Print() で使う。
namespace {
	const char* PieceToCharCSATable[N31_PieceNone] = {
		" * ", "+FU", "+KY", "+KE", "+GI", "+KA", "+HI", "+KI", "+OU", "+TO", "+NY", "+NK", "+NG", "+UM", "+RY", "", "",
		"-FU", "-KY", "-KE", "-GI", "-KA", "-HI", "-KI", "-OU", "-TO", "-NY", "-NK", "-NG", "-UM", "-RY"
	};
	inline const char* pieceToCharCSA(const Piece pc) {
		return PieceToCharCSATable[pc];
	}
}
void Position::Print() const {
	std::cout << "'  9  8  7  6  5  4  3  2  1" << std::endl;
	int i = 0;
	for (Rank r = Rank9; r < RankNum; ++r) {
		++i;
		std::cout << "P" << i;
		for (File f = FileA; FileI <= f; --f) {
			std::cout << pieceToCharCSA(GetPiece(ConvSquare::FROM_FILE_RANK10(f, r)));
		}
		std::cout << std::endl;
	}
	PrintHand(Black);
	PrintHand(White);
	std::cout << (GetTurn() == Black ? "+" : "-") << std::endl;
	std::cout << std::endl;
	std::cout << "key = " << GetKey() << std::endl;
}

u64 Position::GetNodesSearched() const
{
	return this->m_nodes_;
}

void Position::SetNodesSearched(const u64 n)
{
	this->m_nodes_ = n;
}

#if !defined NDEBUG
bool Position::IsOK() const {
	static Key prevKey;
	const bool debugAll = true;

	const bool debugBitboards = debugAll || false;
	const bool debugKingCount = debugAll || false;
	const bool debugKingCapture = debugAll || false;
	const bool debugCheckerCount = debugAll || false;
	const bool debugKey = debugAll || false;
	const bool debugStateHand = debugAll || false;
	const bool debugPiece = debugAll || false;
	const bool debugMaterial = debugAll || false;

	int failedStep = 0;
	if (debugBitboards) {
		if ((GetBbOf(Black) & GetBbOf(White)).Exists1Bit()) {
			goto incorrect_position;
		}
		if ((GetBbOf(Black) | GetBbOf(White)) != GetOccupiedBB()) {
			goto incorrect_position;
		}
		if ((GetBbOf(N01_Pawn) ^ GetBbOf(N02_Lance) ^ GetBbOf(N03_Knight) ^ GetBbOf(N04_Silver) ^ GetBbOf(N05_Bishop) ^
			GetBbOf(N06_Rook) ^ GetBbOf(N07_Gold) ^ GetBbOf(N08_King) ^ GetBbOf(N09_ProPawn) ^ GetBbOf(N10_ProLance) ^
			GetBbOf(N11_ProKnight) ^ GetBbOf(N12_ProSilver) ^ GetBbOf(N13_Horse) ^ GetBbOf(N14_Dragon)) != GetOccupiedBB())
		{
			goto incorrect_position;
		}
		for (PieceType pt1 = N01_Pawn; pt1 < g_PIECETYPE_NUM; ++pt1) {
			for (PieceType pt2 = pt1 + 1; pt2 < g_PIECETYPE_NUM; ++pt2) {
				if ((GetBbOf(pt1) & GetBbOf(pt2)).Exists1Bit()) {
					goto incorrect_position;
				}
			}
		}
	}

	++failedStep;
	if (debugKingCount) {
		int kingCount[g_COLOR_NUM] = { 0, 0 };
		if (GetBbOf(N08_King).PopCount() != 2) {
			goto incorrect_position;
		}
		if (!GetBbOf(N08_King, Black).IsOneBit()) {
			goto incorrect_position;
		}
		if (!GetBbOf(N08_King, White).IsOneBit()) {
			goto incorrect_position;
		}
		for (Square sq = I9; sq < SquareNum; ++sq) {
			if (GetPiece(sq) == N08_BKing) {
				++kingCount[Black];
			}
			if (GetPiece(sq) == N24_WKing) {
				++kingCount[White];
			}
		}
		if (kingCount[Black] != 1 || kingCount[White] != 1) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if (debugKingCapture) {
		// 相手玉を取れないことを確認
		const Color us = GetTurn();
		const Color them = ConvColor::OPPOSITE_COLOR10(us);
		const Square ksq = GetKingSquare(them);
		if (this->GetAttackersTo(us, ksq, g_nullBitboard).Exists1Bit()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if (debugCheckerCount) {
		if (2 < m_st_->m_checkersBB.PopCount()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if (debugKey) {
		if (GetKey() != GetComputeKey()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if (debugStateHand) {
		if (m_st_->m_hand != this->GetHand(GetTurn())) {
			goto incorrect_position;
		}
	}

	++failedStep;
	if (debugPiece) {
		for (Square sq = I9; sq < SquareNum; ++sq) {
			const Piece pc = GetPiece(sq);
			if (pc == N00_Empty) {
				if (!g_setMaskBb.IsSet( &GetEmptyBB(), sq) ) {
					goto incorrect_position;
				}
			}
			else {
				if (!g_setMaskBb.IsSet( &GetBbOf(ConvPiece::TO_PIECE_TYPE10(pc), ConvPiece::TO_COLOR10(pc)), sq) ) {
					goto incorrect_position;
				}
			}
		}
	}

	++failedStep;
	if (debugMaterial) {
		if (this->GetMaterial() != ComputeMaterial()) {
			goto incorrect_position;
		}
	}

	++failedStep;
	{
		int i;
		if ((i = GetDebugSetEvalList()) != 0) {
			std::cout << "GetDebugSetEvalList() error = " << i << std::endl;
			goto incorrect_position;
		}
	}

	prevKey = GetKey();
	return true;

incorrect_position:
	std::cout << "Error! failedStep = " << failedStep << std::endl;
	std::cout << "prevKey = " << prevKey << std::endl;
	std::cout << "currKey = " << GetKey() << std::endl;
	Print();
	return false;
}
#endif

#if !defined NDEBUG
int Position::GetDebugSetEvalList() const {
	// not implement
	return 0;
}
#endif
void Position::SetEvalList()
{
	this->m_evalList_.Set(*this);
}

Key Position::GetComputeBoardKey() const {
	Key result = 0;
	for (Square sq = I9; sq < SquareNum; ++sq) {
		if (this->GetPiece(sq) != N00_Empty) {
			result += this->GetZobrist(ConvPiece::TO_PIECE_TYPE10(GetPiece(sq)), sq, ConvPiece::TO_COLOR10(GetPiece(sq)));
		}
	}
	if (this->GetTurn() == White) {
		result ^= this->GetZobTurn();
	}
	return result;
}

Key Position::GetComputeHandKey() const {
	Key result = 0;
	for (HandPiece hp = HPawn; hp < HandPieceNum; ++hp) {
		for (Color c = Black; c < g_COLOR_NUM; ++c) {
			const int num = this->GetHand(c).NumOf(hp);
			for (int i = 0; i < num; ++i) {
				result += this->GetZobHand(hp, c);
			}
		}
	}
	return result;
}

Key Position::GetComputeKey() const
{
	return this->GetComputeBoardKey() + this->GetComputeHandKey();
}

// todo: isRepetition() に名前変えた方が良さそう。
//       同一局面4回をきちんと数えていないけど問題ないか。
RepetitionType Position::IsDraw(const int checkMaxPly) const {
	const int startNum = 4;
	int i = startNum;
	const int e = std::min(this->m_st_->m_pliesFromNull, checkMaxPly);

	// 4手掛けないと千日手には絶対にならない。
	if (i <= e) {
		// 現在の局面と、少なくとも 4 手戻らないと同じ局面にならない。
		// ここでまず 2 手戻る。
		StateInfo* stp = this->m_st_->m_previous->m_previous;

		do {
			// 更に 2 手戻る。
			stp = stp->m_previous->m_previous;
			if (stp->GetKey() == this->m_st_->GetKey()) {
				if (i <= this->m_st_->m_continuousCheck[this->GetTurn()]) {
					return N03_RepetitionLose;
				}
				else if (i <= this->m_st_->m_continuousCheck[ConvColor::OPPOSITE_COLOR10b(this->GetTurn())]) {
					return N02_RepetitionWin;
				}
#if defined BAN_BLACK_REPETITION
				return (this->GetTurn() == Black ? N03_RepetitionLose : N02_RepetitionWin);
#elif defined BAN_WHITE_REPETITION
				return (this->GetTurn() == White ? N03_RepetitionLose : N02_RepetitionWin);
#else
				return N01_RepetitionDraw;
#endif
			}
			else if (stp->m_boardKey == this->m_st_->m_boardKey) {
				if (this->m_st_->m_hand.IsEqualOrSuperior(stp->m_hand)) { return N04_RepetitionSuperior; }
				if (stp->m_hand.IsEqualOrSuperior(this->m_st_->m_hand)) { return N05_RepetitionInferior; }
			}
			i += 2;
		} while (i <= e);
	}
	return N00_NotRepetition;
}

Military * Position::GetThisThread() const
{
	return this->m_thisThread_;
}

void Position::SetStartPosPly(const Ply ply)
{
	this->m_gamePly_ = ply;
}

int Position::GetList0(const int index) const
{
	return this->m_evalList_.m_list0[index];
}

int Position::GetList1(const int index) const
{
	return this->m_evalList_.m_list1[index];
}

int Position::GetSquareHandToList(const Square sq) const
{
	return this->m_evalList_.m_squareHandToList[sq];
}

Square Position::GetListToSquareHand(const int i) const
{
	return this->m_evalList_.m_listToSquareHand[i];
}

int * Position::GetPlist0()
{
	return &this->m_evalList_.m_list0[0];
}

int * Position::GetPlist1()
{
	return &this->m_evalList_.m_list1[0];
}

const int * Position::GetCplist0() const
{
	return &this->m_evalList_.m_list0[0];
}

const int * Position::GetCplist1() const
{
	return &this->m_evalList_.m_list1[0];
}

const ChangedLists & Position::GetCl() const
{
	return this->m_st_->m_cl;
}

const Rucksack * Position::GetConstRucksack() const
{
	return this->m_pRucksack_;
}

Rucksack * Position::GetRucksack() const
{
	return this->m_pRucksack_;
}

void Position::SetRucksack(Rucksack * s)
{
	this->m_pRucksack_ = s;
}

namespace {
	void printHandPiece(const Position& pos, const HandPiece hp, const Color c, const std::string& str) {
		if (pos.GetHand(c).NumOf(hp)) {
			const char* sign = (c == Black ? "+" : "-");
			std::cout << "P" << sign;
			for (u32 i = 0; i < pos.GetHand(c).NumOf(hp); ++i) {
				std::cout << "00" << str;
			}
			std::cout << std::endl;
		}
	}
}
void Position::PrintHand(const Color c) const {
	printHandPiece(*this, HandPiece::HPawn, c, "FU");
	printHandPiece(*this, HandPiece::HLance, c, "KY");
	printHandPiece(*this, HandPiece::HKnight, c, "KE");
	printHandPiece(*this, HandPiece::HSilver, c, "GI");
	printHandPiece(*this, HandPiece::HGold, c, "KI");
	printHandPiece(*this, HandPiece::HBishop, c, "KA");
	printHandPiece(*this, HandPiece::HRook, c, "HI");
}

Key Position::GetZobrist(const PieceType pt, const Square sq, const Color c)
{
	return Position::m_ZOBRIST_[pt][sq][c];
}

Key Position::GetZobTurn()
{
	return Position::m_zobTurn_;
}

Key Position::GetZobHand(const HandPiece hp, const Color c)
{
	return Position::m_ZOB_HAND_[hp][c];
}

Position::Position()
{
	// デフォルト・コンストラクタは空っぽ☆（＾ｑ＾）
}

Position::Position(Rucksack * s) : m_pRucksack_(s)
{
}

Position::Position(const Position & pos)
{
	*this = pos;
}

Position::Position(const Position & pos, Military * th)
{
	*this = pos;
	this->m_thisThread_ = th;
}

Position::Position(const std::string & sfen, Military * th, Rucksack * s)
{
	this->Set(sfen, th);
	this->SetRucksack(s);
}

Position& Position::operator = (const Position& pos) {
	memcpy(this, &pos, sizeof(Position));
	this->m_startState_ = *this->m_st_;
	this->m_st_ = &this->m_startState_;
	this->m_nodes_ = 0;

	assert(IsOK());

	return *this;
}

void Position::Set(const std::string& sfen, Military* th) {
	Piece promoteFlag = UnPromoted;
	std::istringstream ss(sfen);
	char token;
	Square sq = A9;

	Rucksack* s = std::move(m_pRucksack_);
	this->Clear();
	this->SetRucksack(s);

	// 盤上の駒
	while (ss.get(token) && token != ' ') {
		if (isdigit(token)) {
			sq += SquareDelta::DeltaE * (token - '0');
		}
		else if (token == '/') {
			sq += (SquareDelta::DeltaW * 9) + SquareDelta::DeltaS;
		}
		else if (token == '+') {
			promoteFlag = Promoted;
		}
		else if (g_charToPieceUSI.IsLegalChar(token)) {
			if (ConvSquare::CONTAINS_OF10(sq)) {
				SetPiece(g_charToPieceUSI.GetValue(token) + promoteFlag, sq);
				promoteFlag = Piece::UnPromoted;
				sq += SquareDelta::DeltaE;
			}
			else {
				goto INCORRECT;
			}
		}
		else {
			goto INCORRECT;
		}
	}
	this->m_kingSquare_[Black] = this->GetBbOf20(N08_King, Black).GetFirstOneFromI9();
	this->m_kingSquare_[White] = this->GetBbOf20(N08_King, White).GetFirstOneFromI9();
	this->m_goldsBB_ = this->GetBbOf(N07_Gold, N09_ProPawn, N10_ProLance, N11_ProKnight, N12_ProSilver);

	// 手番
	while (ss.get(token) && token != ' ') {
		if (token == 'b') {
			this->m_turn_ = Black;
		}
		else if (token == 'w') {
			this->m_turn_ = White;
		}
		else {
			goto INCORRECT;
		}
	}

	// 持ち駒
	for (int digits = 0; ss.get(token) && token != ' '; ) {
		if (token == '-') {
			memset(m_hand_, 0, sizeof(m_hand_));
		}
		else if (isdigit(token)) {
			digits = digits * 10 + token - '0';
		}
		else if (g_charToPieceUSI.IsLegalChar(token)) {
			// 持ち駒を32bit に pack する
			const Piece piece = g_charToPieceUSI.GetValue(token);
			this->SetHand(piece, (digits == 0 ? 1 : digits));

			digits = 0;
		}
		else {
			goto INCORRECT;
		}
	}

	// 次の手が何手目か
	ss >> this->m_gamePly_;
	this->m_gamePly_ = std::max(2 * (this->m_gamePly_ - 1), 0) + static_cast<int>(this->GetTurn() == White);

	// 残り時間, hash key, (もし実装するなら)駒番号などをここで設定
	this->m_st_->m_boardKey = this->GetComputeBoardKey();
	this->m_st_->m_handKey = this->GetComputeHandKey();
	this->m_st_->m_hand = this->GetHand(this->GetTurn());

	this->SetEvalList();
	this->FindCheckers();
	this->m_st_->m_material = this->ComputeMaterial();
	this->m_thisThread_ = th;

	return;
INCORRECT:
	std::cout << "incorrect SFEN string : " << sfen << std::endl;
}

Bitboard Position::GetBbOf10(const PieceType pt) const
{
	return this->m_BB_ByPiecetype_[pt];
}

Bitboard Position::GetBbOf10(const Color c) const
{
	return this->m_BB_ByColor_[c];
}

Bitboard Position::GetBbOf20(const PieceType pt, const Color c) const
{
	return this->GetBbOf10(pt) & this->GetBbOf10(c);
}

Bitboard Position::GetBbOf20(const PieceType pt1, const PieceType pt2) const
{
	return this->GetBbOf10(pt1) | this->GetBbOf10(pt2);
}

Bitboard Position::GetBbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3) const
{
	return this->GetBbOf20(pt1, pt2) | this->GetBbOf10(pt3);
}

Bitboard Position::GetBbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3, const PieceType pt4) const
{
	return this->GetBbOf(pt1, pt2, pt3) | this->GetBbOf10(pt4);
}

Bitboard Position::GetBbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3, const PieceType pt4, const PieceType pt5) const
{
	return this->GetBbOf(pt1, pt2, pt3, pt4) | this->GetBbOf10(pt5);
}

Bitboard Position::GetOccupiedBB() const
{
	return this->GetBbOf10(N00_Occupied);
}

Bitboard Position::GetNOccupiedBB() const
{
	return ~this->GetOccupiedBB();
}

Bitboard Position::GetEmptyBB() const
{
	return this->GetOccupiedBB() ^ Bitboard::CreateAllOneBB();
}

Bitboard Position::GetGoldsBB() const
{
	return this->m_goldsBB_;
}

Bitboard Position::GetGoldsBB(const Color c) const
{
	return this->GetGoldsBB() & this->GetBbOf10(c);
}

Piece Position::GetPiece(const Square sq) const
{
	return this->m_piece_[sq];
}

Hand Position::GetHand(const Color c) const
{
	return this->m_hand_[c];
}

Bitboard Position::GetPinnedBB() const
{
	if (this->GetTurn() == Color::Black)
	{
		return this->GetHiddenCheckers<true, true, Color::Black, Color::White>();
	}
	else
	{
		return this->GetHiddenCheckers<true, true, Color::White, Color::Black>();
	}
}

Bitboard Position::GetCheckersBB() const
{
	return this->m_st_->m_checkersBB;
}

Bitboard Position::GetPrevCheckersBB() const
{
	return this->m_st_->m_previous->m_checkersBB;
}

bool Position::InCheck() const
{
	return this->GetCheckersBB().Exists1Bit();
}

ScoreIndex Position::GetMaterial() const
{
	return this->m_st_->m_material;
}

ScoreIndex Position::GetMaterialDiff() const
{
	return this->m_st_->m_material - this->m_st_->m_previous->m_material;
}

bool Position::IsMoveGivesCheck(const Move move) const {
	return this->IsMoveGivesCheck(move, CheckInfo(*this));
}

// move が王手なら true
bool Position::IsMoveGivesCheck(const Move move, const CheckInfo& ci) const {
	assert(IsOK());
	assert(ci.m_dcBB == this->DiscoveredCheckBB());

	const Square to = move.To();
	if (move.IsDrop()) {
		const PieceType ptTo = move.GetPieceTypeDropped();
		// Direct Check ?
		if (g_setMaskBb.IsSet(&ci.m_checkBB[ptTo], to)) {
			return true;
		}
	}
	else {
		const Square from = move.From();
		const PieceType ptFrom = move.GetPieceTypeFrom();
		const PieceType ptTo = move.GetPieceTypeTo(ptFrom);
		assert(ptFrom == ConvPiece::TO_PIECE_TYPE10(GetPiece(from)));
		// Direct Check ?
		if (g_setMaskBb.IsSet(&ci.m_checkBB[ptTo], to)) {
			return true;
		}

		// Discovery Check ?
		if (this->IsDiscoveredCheck(from, to, this->GetKingSquare(ConvColor::OPPOSITE_COLOR10b(GetTurn())), ci.m_dcBB)) {
			return true;
		}
	}

	return false;
}

void Position::Clear() {
	memset(this, 0, sizeof(Position));
	this->m_st_ = &this->m_startState_;
}

void Position::SetPiece(const Piece piece, const Square sq)
{
	const Color c = ConvPiece::TO_COLOR10(piece);
	const PieceType pt = ConvPiece::TO_PIECE_TYPE10(piece);

	this->m_piece_[sq] = piece;

	g_setMaskBb.AddBit(&this->m_BB_ByPiecetype_[pt], sq);
	g_setMaskBb.AddBit(&this->m_BB_ByColor_[c], sq);
	g_setMaskBb.AddBit(&this->m_BB_ByPiecetype_[PieceType::N00_Occupied], sq);
}

void Position::SetHand(const HandPiece hp, const Color c, const int num)
{
	this->m_hand_[c].OrEqual(num, hp);
}

void Position::SetHand(const Piece piece, const int num)
{
	const Color c = ConvPiece::TO_COLOR10(piece);
	const PieceType pt = ConvPiece::TO_PIECE_TYPE10(piece);
	const HandPiece hp = ConvHandPiece::FromPieceType(pt);
	this->SetHand(hp, c, num);
}

// 先手、後手に関わらず、sq へ移動可能な Bitboard を返す。
Bitboard Position::GetAttackersTo(const Square sq, const Bitboard& occupied) const {

	const Bitboard golds = this->GetGoldsBB();

	const PieceTypeEvent ptEventB(occupied, Color::Black, sq);
	const PieceTypeEvent ptEventW(occupied, Color::White, sq);
	const PieceTypeEvent ptEvent0(occupied, Color::Null, sq);

	return (((PiecetypePrograms::m_PAWN.GetAttacks2From(ptEventB) & this->GetBbOf10(N01_Pawn))
		| (PiecetypePrograms::m_LANCE.GetAttacks2From(ptEventB) & this->GetBbOf10(N02_Lance))
		| (PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEventB) & this->GetBbOf10(N03_Knight))
		| (PiecetypePrograms::m_SILVER.GetAttacks2From(ptEventB) & this->GetBbOf10(N04_Silver))
		| (PiecetypePrograms::m_GOLD.GetAttacks2From(ptEventB) & golds))
		& this->GetBbOf10(White))
		| (((PiecetypePrograms::m_PAWN.GetAttacks2From(ptEventW) & this->GetBbOf10(N01_Pawn))
			| (PiecetypePrograms::m_LANCE.GetAttacks2From(ptEventW) & this->GetBbOf10(N02_Lance))
			| (PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEventW) & this->GetBbOf10(N03_Knight))
			| (PiecetypePrograms::m_SILVER.GetAttacks2From(ptEventW) & this->GetBbOf10(N04_Silver))
			| (PiecetypePrograms::m_GOLD.GetAttacks2From(ptEventW) & golds))
			& this->GetBbOf10(Black))
		| (PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent0) & this->GetBbOf20(N05_Bishop, N13_Horse))
		| (PiecetypePrograms::m_ROOK.GetAttacks2From(ptEvent0) & this->GetBbOf20(N06_Rook, N14_Dragon))
		| (PiecetypePrograms::m_KING.GetAttacks2From(ptEvent0) & this->GetBbOf(N08_King, N13_Horse, N14_Dragon));
}

// occupied を Position::occupiedBB() 以外のものを使用する場合に使用する。
Bitboard Position::GetAttackersTo_clr(const Color turn1, const Square sq, const Bitboard& occupied) const {
	const Color turn2 = ConvColor::OPPOSITE_COLOR10b(turn1);
	const PieceTypeEvent ptEvent1(occupied, turn2, sq);
	return ((PiecetypePrograms::m_PAWN.GetAttacks2From(ptEvent1) & this->GetBbOf10(N01_Pawn))
		| (PiecetypePrograms::m_LANCE.GetAttacks2From(ptEvent1) & this->GetBbOf10(N02_Lance))
		| (PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEvent1) & this->GetBbOf10(N03_Knight))
		| (PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent1) & this->GetBbOf10(N04_Silver))
		| (PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1) & GetGoldsBB())
		| (PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent1) & this->GetBbOf20(N05_Bishop, N13_Horse))
		| (PiecetypePrograms::m_ROOK.GetAttacks2From(ptEvent1) & this->GetBbOf20(N06_Rook, N14_Dragon))
		| (PiecetypePrograms::m_KING.GetAttacks2From(ptEvent1) & this->GetBbOf(N08_King, N13_Horse, N14_Dragon)))
		& this->GetBbOf10(turn1);
}

// 玉以外で sq へ移動可能な c 側の駒の Bitboard を返す。
Bitboard Position::GetAttackersToExceptKing(const Color c, const Square sq) const {
	const Color opposite = ConvColor::OPPOSITE_COLOR10b(c);
	const PieceTypeEvent ptEvent1(this->GetOccupiedBB(), opposite, sq);
	return ((PiecetypePrograms::m_PAWN.GetAttacks2From(ptEvent1) & this->GetBbOf10(N01_Pawn))
		| (PiecetypePrograms::m_LANCE.GetAttacks2From(ptEvent1) & this->GetBbOf10(N02_Lance))
		| (PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEvent1) & this->GetBbOf10(N03_Knight))
		| (PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent1) & this->GetBbOf20(N04_Silver, N14_Dragon))
		| (PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1) & (GetGoldsBB() | this->GetBbOf10(N13_Horse)))
		| (PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent1) & this->GetBbOf20(N05_Bishop, N13_Horse))
		| (PiecetypePrograms::m_ROOK.GetAttacks2From(ptEvent1) & this->GetBbOf20(N06_Rook, N14_Dragon)))
		& this->GetBbOf10(c);
}

bool Position::IsAttackersToIsNot0(const Color c, const Square sq) const
{
	return this->GetAttackersTo_clr(c, sq, this->GetOccupiedBB()).Exists1Bit();
}

bool Position::IsAttackersToIsNot0(const Color c, const Square sq, const Bitboard & occupied) const
{
	return this->GetAttackersTo_clr(c, sq, occupied).Exists1Bit();
}

bool Position::IsUnDropCheckIsSupported(const Color c, const Square sq) const
{
	return this->GetAttackersTo_clr(c, sq, this->GetOccupiedBB()).Exists1Bit();
}

void Position::FindCheckers()
{
	m_st_->m_checkersBB = GetAttackersToExceptKing(ConvColor::OPPOSITE_COLOR10b(GetTurn()), GetKingSquare(GetTurn()));
}

ScoreIndex Position::ComputeMaterial() const {
	ScoreIndex s = ScoreZero;
	for (PieceType pt = N01_Pawn; pt < g_PIECETYPE_NUM; ++pt) {
		const int num = this->GetBbOf20(pt, Black).PopCount() - this->GetBbOf20(pt, White).PopCount();
		s += num * PieceScore::GetPieceScore(pt);
	}
	for (PieceType pt = N01_Pawn; pt < N08_King; ++pt) {
		const int num = GetHand(Black).NumOf(ConvHandPiece::FromPieceType(pt)) - GetHand(White).NumOf(ConvHandPiece::FromPieceType(pt));
		s += num * PieceScore::GetPieceScore(pt);
	}
	return s;
}
