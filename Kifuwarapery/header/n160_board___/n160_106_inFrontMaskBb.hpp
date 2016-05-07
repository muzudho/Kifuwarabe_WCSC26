﻿#pragma once

#include "../n080_common__/n080_100_common.hpp"
#include "../n105_color___/n105_500_convColor.hpp"
#include "../n110_square__/n110_200_rank.hpp"
#include "../n160_board___/n160_100_bitboard.hpp"
#include "../n160_board___/n160_104_RankMaskBb.hpp"



extern const RankMaskBb g_rankMaskBb;



class InFrontMaskBb {
public:
	// g_InFrontOfRank9Black...
	const Bitboard m_ofRank9Black = Bitboard::CreateAllZeroBB();
	const Bitboard m_ofRank8Black = g_rankMaskBb.GetRankMask<Rank9>();
	const Bitboard m_ofRank7Black = m_ofRank8Black | g_rankMaskBb.GetRankMask<Rank8>();
	const Bitboard m_ofRank6Black = m_ofRank7Black | g_rankMaskBb.GetRankMask<Rank7>();
	const Bitboard m_ofRank5Black = m_ofRank6Black | g_rankMaskBb.GetRankMask<Rank6>();
	const Bitboard m_ofRank4Black = m_ofRank5Black | g_rankMaskBb.GetRankMask<Rank5>();
	const Bitboard m_ofRank3Black = m_ofRank4Black | g_rankMaskBb.GetRankMask<Rank4>();
	const Bitboard m_ofRank2Black = m_ofRank3Black | g_rankMaskBb.GetRankMask<Rank3>();
	const Bitboard m_ofRank1Black = m_ofRank2Black | g_rankMaskBb.GetRankMask<Rank2>();

	const Bitboard m_ofRank1White = Bitboard::CreateAllZeroBB();
	const Bitboard m_ofRank2White = g_rankMaskBb.GetRankMask<Rank1>();
	const Bitboard m_ofRank3White = m_ofRank2White | g_rankMaskBb.GetRankMask<Rank2>();
	const Bitboard m_ofRank4White = m_ofRank3White | g_rankMaskBb.GetRankMask<Rank3>();
	const Bitboard m_ofRank5White = m_ofRank4White | g_rankMaskBb.GetRankMask<Rank4>();
	const Bitboard m_ofRank6White = m_ofRank5White | g_rankMaskBb.GetRankMask<Rank5>();
	const Bitboard m_ofRank7White = m_ofRank6White | g_rankMaskBb.GetRankMask<Rank6>();
	const Bitboard m_ofRank8White = m_ofRank7White | g_rankMaskBb.GetRankMask<Rank7>();
	const Bitboard m_ofRank9White = m_ofRank8White | g_rankMaskBb.GetRankMask<Rank8>();


	const Bitboard m_inFrontMask[g_COLOR_NUM][RankNum] = {
		{
			this->m_ofRank9Black,
			this->m_ofRank8Black,
			this->m_ofRank7Black,
			this->m_ofRank6Black,
			this->m_ofRank5Black,
			this->m_ofRank4Black,
			this->m_ofRank3Black,
			this->m_ofRank2Black,
			this->m_ofRank1Black
		},
		{
			this->m_ofRank9White,
			this->m_ofRank8White,
			this->m_ofRank7White,
			this->m_ofRank6White,
			this->m_ofRank5White,
			this->m_ofRank4White,
			this->m_ofRank3White,
			this->m_ofRank2White,
			this->m_ofRank1White
		}
	};

public:

	template <Color US>
	inline Bitboard GetInFrontMask(const Rank r) const {
		return this->m_inFrontMask[US][r];
	}
	inline Bitboard GetInFrontMask(const Color c, const Rank r) const {
		return this->m_inFrontMask[c][r];
	}

};


// クラス定義のあとに書くとビルドできるぜ☆（＾ｑ＾）
extern const InFrontMaskBb g_inFrontMaskBb;