#pragma once


#include "../n119_score___/n119_090_scoreIndex.hpp"

#include "../n223_move____/n223_500_flashlight.hpp"// &�Q�Ƃ͎g���Ȃ��B*�Q�Ƃ͎g����B
#include "../n885_searcher/n885_040_rucksack.hpp"


class RepetitionTypeAbstract {
public:

	virtual void CheckStopAndMaxPly(
		bool& isReturn, ScoreIndex& resultScore, const Rucksack* pSearcher, Flashlight* pFlashlightBox) const = 0;

};

