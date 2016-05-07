﻿#pragma once


#include <stack>
#include "../n080_common__/n080_100_common.hpp"
#include "../n105_color___/n105_100_color.hpp"
#include "../n110_square__/n110_100_square.hpp"
#include "../n112_pieceTyp/n112_050_pieceType.hpp"
#include "../n113_piece___/n113_150_piece.hpp"
#include "../n113_piece___/n113_500_hand.hpp"
#include "../n119_score___/n119_090_scoreIndex.hpp"
#include "../n119_score___/n119_200_pieceScore.hpp"
#include "../n160_board___/n160_100_bitboard.hpp"
#include "../n160_board___/n160_102_FileMaskBb.hpp"
#include "../n160_board___/n160_106_inFrontMaskBb.hpp"
#include "../n160_board___/n160_240_betweenBb.hpp"
#include "../n160_board___/n160_400_printBb.hpp"
#include "../n165_movStack/n165_400_move.hpp"
#include "../n220_position/n220_100_repetitionType.hpp"
#include "../n220_position/n220_150_checkInfo.hpp"
#include "../n220_position/n220_350_stateInfo.hpp"
#include "../n220_position/n220_400_evalList.hpp"
#include "../n220_position/n220_640_utilAttack.hpp"


using StateStackPtr = std::unique_ptr<std::stack<StateInfo> >;


class Military;
class Rucksack;


class Position {
public:

	Position();

	explicit Position(Rucksack* s);

	Position(const Position& pos);

	Position(const Position& pos, Military* th);

	Position(const std::string& sfen, Military* th, Rucksack* s);

	Position& operator = (const Position& pos);

	void Set(const std::string& sfen, Military* th);

	template<const Color US>
	Bitboard GetBbOf10() const
	{
		return this->m_BB_ByColor_[US];
	}
	Bitboard GetBbOf10(const PieceType pt) const;

	Bitboard GetBbOf10(const Color c) const;


	template<Color US>
	Bitboard GetBbOf20(const PieceType pt) const {
		return this->GetBbOf10(pt) & this->GetBbOf10(US);
	}


	Bitboard GetBbOf20(const PieceType pt, const Color c) const;



	Bitboard GetBbOf20(const PieceType pt1, const PieceType pt2) const;

	template<Color CLR>
	Bitboard Position::GetBbOf30(const PieceType pt1, const PieceType pt2) const
	{
		return this->GetBbOf20(pt1, pt2) & this->GetBbOf10(CLR);
	}

	Bitboard GetBbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3) const;

	Bitboard GetBbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3, const PieceType pt4) const;

	Bitboard GetBbOf(const PieceType pt1, const PieceType pt2, const PieceType pt3,	const PieceType pt4, const PieceType pt5) const;

	Bitboard GetOccupiedBB() const;

	// emptyBB() よりもわずかに速いはず。
	// emptyBB() とは異なり、全く使用しない位置(0 から数えて、right の 63bit目、left の 18 ~ 63bit目)
	// の bit が 1 になっても構わないとき、こちらを使う。
	// todo: SSEにビット反転が無いので実はそんなに速くないはず。不要。
	Bitboard GetNOccupiedBB() const;

	Bitboard GetEmptyBB() const;

	// 金、成り金 の Bitboard
	Bitboard GetGoldsBB() const;

	Bitboard GetGoldsBB(const Color c) const;

	Piece GetPiece(const Square sq) const;

	// hand
	template<Color CLR>
	Hand GetHand() const
	{
		return this->m_hand_[CLR];
	}
	Hand GetHand(const Color c) const;

	// turn() 側が pin されている Bitboard を返す。
	// checkersBB が更新されている必要がある。
	Bitboard GetPinnedBB() const;

	// 間の駒を動かすことで、turn() 側が空き王手が出来る駒のBitboardを返す。
	// checkersBB が更新されている必要はない。
	// BetweenIsUs == true  : 間の駒が自駒。
	// BetweenIsUs == false : 間の駒が敵駒。
	template <Color US, Color THEM, bool BetweenIsUs = true>
	Bitboard DiscoveredCheckBB() const {

		return GetHiddenCheckers<false, BetweenIsUs, US, THEM>();
		/*
		if (this->GetTurn() == Color::Black)
		{
			return GetHiddenCheckers<false, BetweenIsUs,Color::Black,Color::White>();
		}
		else
		{
			return GetHiddenCheckers<false, BetweenIsUs,Color::White,Color::Black>();
		}
		*/

	}

	// toFile と同じ筋に us の歩がないなら true
	bool NoPawns(const Color us, const File toFile) const;

	template<Color US,Color THEM>
	bool IsPawnDropCheckMate(const Square sq) const;

	// Pinされているfromの駒がtoに移動出来なければtrueを返す。
	template <bool IsKnight = false>
	bool IsPinnedIllegal(const Square from, const Square to, const Square ksq, const Bitboard& pinned) const {
		// 桂馬ならどこに動いても駄目。
		return g_setMaskBb.IsSet(&pinned,from) && (IsKnight || !g_squareRelation.IsAligned<true>(from, to, ksq));
	}

	// 空き王手かどうか。
	template <bool IsKnight = false>
	bool IsDiscoveredCheck(const Square from, const Square to, const Square ksq, const Bitboard& dcBB) const {
		// 桂馬ならどこに動いても空き王手になる。
		return g_setMaskBb.IsSet(&dcBB,from) && (IsKnight || !g_squareRelation.IsAligned<true>(from, to, ksq));
	}

	Bitboard GetCheckersBB() const;

	Bitboard GetPrevCheckersBB() const;

	// 王手が掛かっているか。
	bool InCheck() const;

	ScoreIndex GetMaterial() const;

	ScoreIndex GetMaterialDiff() const;

	template<const Color CLR>
	FORCE_INLINE Square GetKingSquare() const {
		assert(m_kingSquare_[CLR] == this->GetBbOf<CLR>(N08_King).GetFirstOneFromI9());
		return m_kingSquare_[CLR];
	}
	FORCE_INLINE Square GetKingSquare(const Color c) const {
		assert(m_kingSquare_[c] == this->GetBbOf(N08_King, c).GetFirstOneFromI9());
		return m_kingSquare_[c];
	}

	bool IsMoveGivesCheck(const Move m) const;

	bool IsMoveGivesCheck(const Move move, const CheckInfo& ci) const;

	// attacks
	Bitboard GetAttackersTo(const Square sq, const Bitboard& occupied) const;

	template<
		Color TURN1,
		Color TURN2//TURN1の相手の色
	>
	Bitboard GetAttackersTo_a(const Square sq, const Bitboard& occupied) const{
		const PieceTypeEvent ptEvent1(occupied, TURN2, sq);
		return ((PiecetypePrograms::m_PAWN.GetAttacks2From(ptEvent1) & this->GetBbOf10(N01_Pawn))
			| (PiecetypePrograms::m_LANCE.GetAttacks2From(ptEvent1) & this->GetBbOf10(N02_Lance))
			| (PiecetypePrograms::m_KNIGHT.GetAttacks2From(ptEvent1) & this->GetBbOf10(N03_Knight))
			| (PiecetypePrograms::m_SILVER.GetAttacks2From(ptEvent1) & this->GetBbOf10(N04_Silver))
			| (PiecetypePrograms::m_GOLD.GetAttacks2From(ptEvent1) & GetGoldsBB())
			| (PiecetypePrograms::m_BISHOP.GetAttacks2From(ptEvent1) & this->GetBbOf20(N05_Bishop, N13_Horse))
			| (PiecetypePrograms::m_ROOK.GetAttacks2From(ptEvent1) & this->GetBbOf20(N06_Rook, N14_Dragon))
			| (PiecetypePrograms::m_KING.GetAttacks2From(ptEvent1) & this->GetBbOf(N08_King, N13_Horse, N14_Dragon)))
			& this->GetBbOf10(TURN1);
	}

	Bitboard GetAttackersTo_clr(const Color c, const Square sq, const Bitboard& occupied) const;

	Bitboard GetAttackersToExceptKing(const Color c, const Square sq) const;

	// todo: 利きをデータとして持ったとき、attackersToIsNot0() を高速化すること。
	bool IsAttackersToIsNot0(const Color c, const Square sq) const;

	bool IsAttackersToIsNot0(const Color c, const Square sq, const Bitboard& occupied) const;

	// 移動王手が味方の利きに支えられているか。false なら相手玉で取れば詰まない。
	bool IsUnDropCheckIsSupported(const Color c, const Square sq) const;
	// 利きの生成




	// 次の手番
	Color GetTurn() const;

	// pseudoLegal とは
	// ・玉が相手駒の利きがある場所に移動する
	// ・pin の駒を移動させる
	// ・連続王手の千日手の手を指す
	// これらの反則手を含めた手の事と定義する。
	// よって、打ち歩詰めや二歩の手は pseudoLegal では無い。
	template <bool MUSTNOTDROP, bool FROMMUSTNOTBEKING, Color US, Color THEM>
	bool IsPseudoLegalMoveIsLegal(const Move move, const Bitboard& pinned) const;

	// FIXME: これ使ってないのでは☆？（＾ｑ＾）？
	template<Color US, Color THEM>
	bool IsPseudoLegalMoveIsEvasion(const Move move, const Bitboard& pinned) const;

	// checkPawnDrop : 二歩と打ち歩詰めも調べるなら true
	template<Color US, Color THEM>
	bool MoveIsPseudoLegal(const Move move, const bool checkPawnDrop = false) const;

#if !defined NDEBUG
	bool MoveIsLegal(const Move GetMove) const;
#endif

	template<Color US,Color THEM>
	void DoMove(const Move move, StateInfo& newSt);

	template<Color US, Color THEM>
	void DoMove(const Move move, StateInfo& newSt, const CheckInfo& ci, const bool moveIsCheck);

	void UndoMove(const Move move);

	void DoNullMove(bool DO, StateInfo& backUpSt);

	template<Color US,Color THEM>
	ScoreIndex GetSee1(const Move move, const int asymmThreshold = 0) const;

	ScoreIndex GetSeeSign(const Move move) const;

	template <Color US,Color THEM>
	Move GetMateMoveIn1Ply();
	//Move GetMateMoveIn1Ply();

	Ply GetGamePly() const;

	Key GetBoardKey() const;

	Key GetHandKey() const;

	Key GetKey() const;

	Key GetExclusionKey() const;

	Key GetKeyExcludeTurn() const;

	void Print() const;

	u64 GetNodesSearched() const;

	void SetNodesSearched(const u64 n);

	RepetitionType IsDraw(const int checkMaxPly = std::numeric_limits<int>::max()) const;

	Military* GetThisThread() const;

	void SetStartPosPly(const Ply ply);

	static constexpr int GetNlist() {
		return EvalList::m_ListSize;
	}

	int GetList0(const int index) const;

	int GetList1(const int index) const;

	int GetSquareHandToList(const Square sq) const;

	Square GetListToSquareHand(const int i) const;

	int* GetPlist0();

	int* GetPlist1();

	const int* GetCplist0() const;

	const int* GetCplist1() const;

	const ChangedLists& GetCl() const;

	const Rucksack* GetConstRucksack() const;

	Rucksack* GetRucksack() const;

	void SetRucksack(Rucksack* s);

#if !defined NDEBUG
	// for debug
	bool IsOK() const;
#endif

	static void InitZobrist();

	// メンバーを変更するのに使われる。
	inline StateInfo* GetStateInfo() { return m_st_; }

private:

	void Clear();

	void SetPiece(const Piece piece, const Square sq);

	void SetHand(const HandPiece hp, const Color c, const int num);

	void SetHand(const Piece piece, const int num);

	// 手番側の玉へ check している駒を全て探して checkersBB_ にセットする。
	// 最後の手が何か覚えておけば、attackersTo() を使用しなくても良いはずで、処理が軽くなる。
	void FindCheckers();

	ScoreIndex ComputeMaterial() const;

	template<Color CLR>
	inline void XorBBs(const PieceType pt, const Square sq) {
		g_setMaskBb.XorBit(&this->m_BB_ByPiecetype_[N00_Occupied], sq);
		g_setMaskBb.XorBit(&this->m_BB_ByPiecetype_[pt], sq);
		g_setMaskBb.XorBit(&this->m_BB_ByColor_[CLR], sq);
	}
	void XorBBs(const PieceType pt, const Square sq, const Color c);

	// turn() 側が
	// pin されて(して)いる駒の Bitboard を返す。
	// BetweenIsUs == true  : 間の駒が自駒。
	// BetweenIsUs == false : 間の駒が敵駒。
	template <bool FindPinned, bool BetweenIsUs, Color US, Color THEM>
	Bitboard GetHiddenCheckers() const {
		Bitboard result = Bitboard::CreateAllZeroBB();
		// pin する遠隔駒
		// まずは自駒か敵駒かで大雑把に判別
		Bitboard pinners = this->GetBbOf10(FindPinned ? THEM : US);

		const Square ksq = GetKingSquare(FindPinned ? US : THEM);

		// 障害物が無ければ玉に到達出来る駒のBitboardだけ残す。
		pinners &=	(
						this->GetBbOf10(N02_Lance) &
						g_lanceAttackBb.GetControllBbToEdge((FindPinned ? US : THEM), ksq)
					) |
					(
						this->GetBbOf20(N06_Rook, N14_Dragon) &
						g_rookAttackBb.GetControllBbToEdge(ksq)
					) |
					(
						this->GetBbOf20(N05_Bishop, N13_Horse) &
						g_bishopAttackBb.GetControllBbToEdge(ksq)
					);

		while (pinners.Exists1Bit()) {
			const Square sq = pinners.PopFirstOneFromI9();
			// pin する遠隔駒と玉の間にある駒の位置の Bitboard
			const Bitboard between = g_betweenBb.GetBetweenBB(sq, ksq) & GetOccupiedBB();

			// pin する遠隔駒と玉の間にある駒が1つで、かつ、引数の色のとき、その駒は(を) pin されて(して)いる。
			if (between.Exists1Bit()
				&& between.IsOneBit<false>()
				&& between.AndIsNot0( this->GetBbOf10(BetweenIsUs ? US : THEM)))
			{
				result |= between;
			}
		}

		return result;
	}

#if !defined NDEBUG
	int GetDebugSetEvalList() const;
#endif

	void SetEvalList();

	Key GetComputeBoardKey() const;

	Key GetComputeHandKey() const;

	Key GetComputeKey() const;

	void PrintHand(const Color c) const;

	template<Color CLR>
	static Key GetZobrist(const PieceType pt, const Square sq)
	{
		return Position::m_ZOBRIST_[pt][sq][CLR];
	}
	static Key GetZobrist(const PieceType pt, const Square sq, const Color c);

	static Key GetZobTurn();

	template<Color CLR>
	static Key GetZobHand(const HandPiece hp)
	{
		return Position::m_ZOB_HAND_[hp][CLR];
	}
	static Key GetZobHand(const HandPiece hp, const Color c);

	// このポジションをピースタイプ毎にビットボードにしたもの。（byTypeBB は敵、味方の駒を区別しない）
	Bitboard m_BB_ByPiecetype_[g_PIECETYPE_NUM];

	// このポジションを白黒毎にビットボードにしたもの。（byColorBB は駒の種類を区別しない）
	Bitboard m_BB_ByColor_[g_COLOR_NUM];

	// このポジションのカナゴマをビットボードにしたもの。
	Bitboard m_goldsBB_;

	// 各マスの状態
	Piece m_piece_[SquareNum];

	Square m_kingSquare_[g_COLOR_NUM];

	// 手駒
	Hand m_hand_[g_COLOR_NUM];

	Color m_turn_;

	EvalList m_evalList_;

	StateInfo m_startState_;

	StateInfo* m_st_;

	// 時間管理に使用する。
	Ply m_gamePly_;

	Military* m_thisThread_;

	u64 m_nodes_;

	Rucksack* m_pRucksack_;

	static Key m_ZOBRIST_[g_PIECETYPE_NUM][SquareNum][g_COLOR_NUM];

	static const Key m_zobTurn_ = 1;

	static Key m_ZOB_HAND_[HandPieceNum][g_COLOR_NUM];

	static Key m_ZOB_EXCLUSION_; // todo: これが必要か、要検討

};
