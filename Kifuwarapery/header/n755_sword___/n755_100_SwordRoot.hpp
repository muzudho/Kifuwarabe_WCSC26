#pragma once


#include "n755_070_SwordAbstract.hpp"


class SwordRoot : public SwordAbstract {
public:

	// �X�v���b�g�E�|�C���g���������邺���i�O���O�j
	// �ˑ��֌W�̓s����A�C�����C���ɂ͂��Ȃ������i�O���O�j
	virtual void GoSearch_AsSplitedNode(
		SplitedNode& ownerSplitedNode,
		Rucksack& searcher,
		Position& pos,
		Flashlight* pFlashlight
		) const override;

};


extern const SwordRoot g_SWORD_ROOT;
