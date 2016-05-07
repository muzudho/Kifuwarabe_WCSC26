﻿#pragma once

#include "../n080_common__/n080_100_common.hpp"
#include "../n080_common__/n080_150_overloadEnumOperators.hpp"
#include "../n111_colorSq_/n111_500_ColorFileRank.h"
#include "../n119_score___/n119_200_pieceScore.hpp"
#include "../n119_score___/n119_500_EvalSum.hpp"
#include "../n220_position/n220_650_position.hpp"
#include "../n220_position/n220_750_charToPieceUSI.hpp"

#include "../n223_move____/n223_500_flashlight.hpp"



const size_t EvaluateTableSize = 0x400000; // 134MB
//const size_t EvaluateTableSize = 0x10000000; // 8GB
//const size_t EvaluateTableSize = 0x20000000; // 17GB

using EvaluateHashEntry = EvalSum;
struct EvaluateHashTable : HashTable<EvaluateHashEntry, EvaluateTableSize> {};
extern EvaluateHashTable g_evalTable;


class Evaluation09 {
public:
	ScoreIndex evaluateUnUseDiff(const Position& pos);
	ScoreIndex evaluate(Position& pos, Flashlight* ss);
private:

	EvalSum doapc(const Position& pos, const int index[2]);
	std::array<s32, 2> doablack(const Position& pos, const int index[2]);
	std::array<s32, 2> doawhite(const Position& pos, const int index[2]);

#if defined INANIWA_SHIFT
	ScoreIndex inaniwaScoreBody(const Position& GetPos);
	inline ScoreIndex inaniwaScore(const Position& GetPos);
#endif

	bool calcDifference(Position& pos, Flashlight* ss);
	int make_list_unUseDiff(const Position& pos, int list0[EvalList::m_ListSize], int list1[EvalList::m_ListSize], int nlist);
	void evaluateBody(Position& pos, Flashlight* ss);
};

