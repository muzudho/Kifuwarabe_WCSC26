#pragma once


#include "../n119_score___/n119_090_scoreIndex.hpp"
#include "../n119_score___/n119_100_utilScore.hpp"
#include "n886_070_rtAbstract.hpp"

#include "../n223_move____/n223_500_flashlight.hpp"// &参照は使えない。*参照は使える。
#include "../n885_searcher/n885_040_rucksack.hpp"


class RepetitionTypeWin : public RepetitionTypeAbstract {
public:

	void CheckStopAndMaxPly(
		bool& isReturn, ScoreIndex& resultScore, const Rucksack* pSearcher, Flashlight* pFlashlightBox
	) const override {
		isReturn = true;
		resultScore = UtilScore::MateIn(pFlashlightBox->m_ply);
		return;
	}
};

