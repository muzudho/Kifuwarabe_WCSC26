﻿#pragma once


#include <algorithm>
#include "../n080_common__/n080_100_common.hpp"
#include "../n223_move____/n223_200_depth.hpp"


class Reductions {
public:
	s8 m_reductions[2][64][64]; // [pv][depth][moveNumber]

public:
	void Initialize();


	inline Depth DoReduction(
		bool PVNode,
		const Depth depth, const int moveCount
	) {
		return static_cast<Depth>(this->m_reductions[PVNode][std::min(Depth(depth / OnePly), Depth(63))][std::min(moveCount, 63)]);
	}
	inline Depth DoReduction_PvNode(
		const Depth depth, const int moveCount
		) {
		return static_cast<Depth>(this->m_reductions[true//PVNode
		][std::min(Depth(depth / OnePly), Depth(63))][std::min(moveCount, 63)]);
	}
	inline Depth DoReduction_NotPvNode(
		const Depth depth, const int moveCount
		) {
		return static_cast<Depth>(this->m_reductions[false//not PVNode
		][std::min(Depth(depth / OnePly), Depth(63))][std::min(moveCount, 63)]);
	}

};


extern Reductions g_reductions;
