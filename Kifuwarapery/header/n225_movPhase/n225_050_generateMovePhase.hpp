#pragma once

#include "../n080_common__/n080_150_overloadEnumOperators.hpp"


// �w���萶���t�F�[�Y���H�i�O���O�j320_540_�l�������o�����������Ƃ��Ŏg����
enum GenerateMovePhase {
	N00_MainSearch,
	N01_PH_TacticalMoves0,
	N02_PH_Killers,
	N03_PH_NonTacticalMoves0,
	N04_PH_NonTacticalMoves1,
	N05_PH_BadCaptures,
	N06_EvasionSearch,
	N07_PH_Evasions,
	N08_QSearch,
	N09_PH_QCaptures0,
	N10_QEvasionSearch,
	N11_PH_QEvasions,
	N12_ProbCut,
	N13_PH_TacticalMoves1,
	N14_QRecapture,
	N15_PH_QCaptures1,
	N16_PH_Stop,
	GenerateMovePhaseNum	//(^q^)�ǉ���
};
OverloadEnumOperators(GenerateMovePhase); // ++phase_ �ׁ̈B
