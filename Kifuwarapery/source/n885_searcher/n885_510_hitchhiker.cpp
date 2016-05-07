#include <iostream>
#include "../../header/n119_score___/n119_200_pieceScore.hpp"
#include "../../header/n160_board___/n160_106_inFrontMaskBb.hpp"
#include "../../header/n160_board___/n160_220_queenAttackBb.hpp"
#include "../../header/n160_board___/n160_230_setMaskBb.hpp"

#include "../../header/n220_position/n220_100_repetitionType.hpp"
#include "../../header/n220_position/n220_640_utilAttack.hpp"
#include "../../header/n220_position/n220_650_position.hpp"
#include "../../header/n220_position/n220_665_utilMoveStack.hpp"
#include "../../header/n220_position/n220_750_charToPieceUSI.hpp"

#include "../../header/n223_move____/n223_040_nodeType.hpp"
#include "../../header/n223_move____/n223_300_moveAndScoreIndex.hpp"

#include "../../header/n350_pieceTyp/n350_030_makePromoteMove.hpp"
#include "../../header/n350_pieceTyp/n350_500_ptPrograms.hpp"
#include "../../header/n440_movStack/n440_500_nextmoveEvent.hpp"
#include "../../header/n520_evaluate/n520_700_evaluation09.hpp"
#include "../../header/n560_timeMng_/n560_500_timeManager.hpp"
#include "../../header/n600_book____/n600_500_book.hpp"

#include "../../header/n640_searcher/n640_440_splitedNode.hpp"
#include "../../header/n640_searcher/n640_500_reductions.hpp"
#include "../../header/n640_searcher/n640_510_futilityMargins.hpp"
#include "../../header/n640_searcher/n640_520_futilityMoveCounts.hpp"
#include "../../header/n680_egOption/n680_240_engineOptionsMap.hpp"
#include "../../header/n680_egOption/n680_300_engineOptionSetup.hpp"
#include "../../header/n760_thread__/n760_400_herosPub.hpp"

#include "../../header/n800_learn___/n800_100_stopwatch.hpp"
#include "../../header/n883_nodeType/n883_070_nodetypeAbstract.hpp"

#include "../../header/n885_searcher/n885_040_rucksack.hpp"
#include "../../header/n885_searcher/n885_480_hitchhikerNyugyoku.hpp"
#include "../../header/n885_searcher/n885_510_hitchhiker.hpp"
#include "../../header/n885_searcher/n885_600_iterativeDeepeningLoop.hpp"//FIXME:

#include "../../header/n886_repeType/n886_100_rtNot.hpp"
#include "../../header/n886_repeType/n886_110_rtDraw.hpp"
#include "../../header/n886_repeType/n886_120_rtWin.hpp"
#include "../../header/n886_repeType/n886_130_rtLose.hpp"
#include "../../header/n886_repeType/n886_140_rtSuperior.hpp"
#include "../../header/n886_repeType/n886_150_rtInferior.hpp"
#include "../../header/n886_repeType/n886_500_rtArray.hpp"

#include "../../header/n887_nodeType/n887_500_nodetypePrograms.hpp"//FIXME:
//class IterativeDeepeningLoop;
//static inline void IterativeDeepeningLoop::Execute(Rucksack& rucksack, Position& pos);

using namespace std;


extern const InFrontMaskBb g_inFrontMaskBb;
extern NodetypeAbstract* g_NODETYPE_PROGRAMS[];
extern RepetitionTypeArray g_repetitionTypeArray;



void Hitchhiker::Think(
	Rucksack& captainsRucksack // �L���v�e���̎����Ă���r����������������
	) {

	Position& pos = captainsRucksack.m_rootPosition;

	bool isMoveTime0Clear = false;
	captainsRucksack.m_timeManager.InitializeTimeManager_OnHitchhikerThinkStarted(
		isMoveTime0Clear,
		captainsRucksack.m_limits,
		pos.GetGamePly(),
		pos.GetTurn(),
		&captainsRucksack
		);
	if (isMoveTime0Clear) {
		captainsRucksack.m_limits.ZeroClearMoveTime();
	}
	SYNCCOUT << "info string limits move time = " << captainsRucksack.m_limits.GetMoveTime() << SYNCENDL;//���\���͂Ȃ�������


	//����������������������������������������������������������������������������������������������������������������������������������������������������������������
	// ���ʏ������𒲂ׂ邺���i�O���O�j
	//����������������������������������������������������������������������������������������������������������������������������������������������������������������
#if !defined(LEARN)
	if (Hitchhiker::IsNyugyokuWin(pos)) {
		//����������������������������������������������������������������������������������������������������������������������������������������������������������������
		// �T�����H�i�O���O�j
		//����������������������������������������������������������������������������������������������������������������������������������������������������������������
		HitchhikerNyugyoku::Travel_885_480(captainsRucksack,pos);
		return;
	}
#endif

	//����������������������������������������������������������������������������������������������������������������������������������������������������������������
	// ���ʏ�������Ȃ����������i�O���O�j
	//����������������������������������������������������������������������������������������������������������������������������������������������������������������
	static Book book;
	std::uniform_int_distribution<int> dist(
		captainsRucksack.m_engineOptions["Min_Book_Ply"], captainsRucksack.m_engineOptions["Max_Book_Ply"]);

	const Ply book_ply = dist(g_randomTimeSeed);

	pos.SetNodesSearched(0);

#if defined LEARN
	captainsRucksack.m_ownerHerosPub[0]->m_searching = true;
#else
	captainsRucksack.m_tt.SetSize(captainsRucksack.m_engineOptions["USI_Hash"]); // operator int() �Ăяo���B

	SYNCCOUT << "info string book_ply " << book_ply << SYNCENDL;
	if (
		// ��Ղ��g����萔�̂Ƃ����i�O���O�j�H
		captainsRucksack.m_engineOptions["OwnBook"] &&
		pos.GetGamePly() <= book_ply
	) {
		const MoveAndScoreIndex bookMoveScore = book.GetProbe(
			pos, captainsRucksack.m_engineOptions["Book_File"], captainsRucksack.m_engineOptions["Best_Book_Move"]);

		if (
			!bookMoveScore.m_move.IsNone()
			&&
			std::find(
				captainsRucksack.m_rootMoves.begin(),
				captainsRucksack.m_rootMoves.end(),
				bookMoveScore.m_move
			) != captainsRucksack.m_rootMoves.end()
		){
			std::swap(captainsRucksack.m_rootMoves[0], *std::find(captainsRucksack.m_rootMoves.begin(),
				captainsRucksack.m_rootMoves.end(),
				bookMoveScore.m_move));
			SYNCCOUT << "info"
				<< " score " << captainsRucksack.scoreToUSI(bookMoveScore.m_scoreIndex)
				<< " pv " << bookMoveScore.m_move.ToUSI()
				<< SYNCENDL;

			goto finalize;
		}
	}

	// �S�X���b�h�̏��������������H
	captainsRucksack.m_ownerHerosPub.WakeUp(&captainsRucksack);

	// ������m�̎����i�~���b�j��ݒ肷�邺��
	captainsRucksack.m_ownerHerosPub.GetCurrWarrior()->m_lifetimeMilliseconds =
		(
			captainsRucksack.m_limits.IsBrandnewTimeManagement() ? // �����[�������������H�i�O���O�j
			// ����ꍇ
			std::min(100,
				std::max(
					captainsRucksack.m_timeManager.GetWarriorLifeTime(),
					captainsRucksack.TimerResolution
				))
			:
			// �ł��Ȃ��ꍇ��
			captainsRucksack.m_limits.m_nodes01 ?
			2 * captainsRucksack.TimerResolution :
			100
			);

	captainsRucksack.m_ownerHerosPub.GetCurrWarrior()->NotifyOne();

#if defined INANIWA_SHIFT
	detectInaniwa(GetPos);
#endif
#if defined BISHOP_IN_DANGER
	detectBishopInDanger(GetPos);
#endif
#endif
	//����������������������������������������������������������������������������������������������������������������������������������������������������������������
	// �����[���T�����n�߂邺���i�O���O�j
	//����������������������������������������������������������������������������������������������������������������������������������������������������������������
	IterativeDeepeningLoop::Execute885_500(captainsRucksack, pos);//ExecuteIterativeDeepeningLoop(pos);

#if defined LEARN
#else
	captainsRucksack.m_ownerHerosPub.GetCurrWarrior()->m_lifetimeMilliseconds = 0; // timer ���~�߂�B
	captainsRucksack.m_ownerHerosPub.Sleep();

finalize:

	SYNCCOUT << "info nodes " << pos.GetNodesSearched()
		<< " time " << captainsRucksack.m_stopwatch.GetElapsed() << SYNCENDL;

	if (!captainsRucksack.m_signals.m_stop && (captainsRucksack.m_limits.m_ponder || captainsRucksack.m_limits.m_infinite)) {
		captainsRucksack.m_signals.m_stopOnPonderHit = true;
		pos.GetThisThread()->WaitFor(captainsRucksack.m_signals.m_stop);
	}

	if (captainsRucksack.m_rootMoves[0].m_pv_[0].IsNone()) {
		SYNCCOUT << "bestmove resign" << SYNCENDL;
	}
	else {
		SYNCCOUT << "bestmove " << captainsRucksack.m_rootMoves[0].m_pv_[0].ToUSI()
			<< " ponder " << captainsRucksack.m_rootMoves[0].m_pv_[1].ToUSI()
			<< SYNCENDL;
	}
#endif
}



// ���ʏ������ǂ����𔻒�
bool Hitchhiker::IsNyugyokuWin(const Position& pos) {
	// CSA ���[���ł́A�� ���� �Z �̏�����S�Ė������Ƃ��A���ʏ����錾���o����B

	// �� �錾���̎�Ԃł���B

	// ���̊֐����Ăяo���͎̂����̎�Ԃ݂̂Ƃ���Bponder �ł͌Ăяo���Ȃ��B

	const Color us = pos.GetTurn();

	// �G�w�̃}�X�N
	const Bitboard opponentsField = (us == Black ? g_inFrontMaskBb.GetInFrontMask(Black, Rank6) : g_inFrontMaskBb.GetInFrontMask(White, Rank4));

	// �� �錾���̋ʂ��G�w�O�i�ڈȓ��ɓ����Ă���B
	if (!pos.GetBbOf20(N08_King, us).AndIsNot0(opponentsField))
	{
		return false;
	}

	// �O �錾�����A���5�_����1�_�Ōv�Z����
	//     ���̏ꍇ28�_�ȏ�̎��_������B
	//     ���̏ꍇ27�_�ȏ�̎��_������B
	//     �_���̑ΏۂƂȂ�̂́A�錾���̎���ƓG�w�O�i�ڈȓ��ɑ��݂���ʂ������錾���̋�݂̂ł���B
	const Bitboard bigBB = pos.GetBbOf(N06_Rook, N14_Dragon, N05_Bishop, N13_Horse) & opponentsField & pos.GetBbOf10(us);

	const Bitboard smallBB = (pos.GetBbOf(N01_Pawn, N02_Lance, N03_Knight, N04_Silver) |
		pos.GetGoldsBB()) & opponentsField & pos.GetBbOf10(us);

	const Hand hand = pos.GetHand(us);

	const int val = (bigBB.PopCount() + hand.NumOf<HRook>() + hand.NumOf<HBishop>()) * 5
		+ smallBB.PopCount()
		+ hand.NumOf<HPawn>() + hand.NumOf<HLance>() + hand.NumOf<HKnight>()
		+ hand.NumOf<HSilver>() + hand.NumOf<HGold>();

#if defined LAW_24
	if (val < 31)
	{
		return false;
	}
#else
	if (val < (us == Black ? 28 : 27))
	{
		return false;
	}
#endif

	// �l �錾���̓G�w�O�i�ڈȓ��̋�́A�ʂ�������10���ȏ㑶�݂���B

	// �ʂ͓G�w�ɂ���̂ŁA����G�w��11���ȏ゠��΂悢�B
	if ((pos.GetBbOf10(us) & opponentsField).PopCount() < 11)
	{
		return false;
	}

	// �� �錾���̋ʂɉ��肪�������Ă��Ȃ��B
	if (pos.InCheck())
	{
		return false;
	}

	// �Z �錾���̎������Ԃ��c���Ă���B

	// �������Ԃ�������Ί��ɕ����Ă��ā@�����ɂ͂��Ȃ��̂ŁA�����`�F�b�N���Ȃ��i�O���O�j

	return true;
}
