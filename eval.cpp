#include "globals.h"

constexpr int BACKWARDS_PAWN_PENALTY = 8;
constexpr int MAJORITY_PENALTY = 20;

constexpr int bishop_pair[2][3] = { {0,0,20},{0,0,20} };

constexpr int knightsquares[8] = { -48,-48,2,3,4,5,6,7 };
constexpr int bishopmoves[14] = { -5,1,2,3,4,5,6,7,8,9,10,11,12,13 };
constexpr int rookmoves[15] = { -5,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
constexpr int queenmoves[28] = { -5,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9 };

//masks for unmoved centre pawns
const BITBOARD bit_unmoved_white = mask[D2] | mask[E2];
const BITBOARD bit_unmoved_black = mask[D7] | mask[E7];

int EvalPawns(const int s, const int xs);
static int EvalPawn(const int s, const int xs, const int sq, const BITBOARD, const BITBOARD);

int EvalEndgame(const int, const int);

int CountBits(BITBOARD b1);

void z();

int Eval(const int real_s, const int real_xs, const int alpha, const int beta)
{
	if (piece_mat[0] <= Q_VALUE && piece_mat[1] <= Q_VALUE)
	{
		if (pawn_mat[0] == 0 && pawn_mat[1] == 0)
		{
			return EvalPawnless(real_s, real_xs);
		}
		if ((piece_mat[0] < BB_VALUE || piece_mat[0] == Q_VALUE) &&
			(piece_mat[1] < BB_VALUE || piece_mat[1] == Q_VALUE))
		{
			return EvalEndgame(real_s, real_xs);
		}
	}

	int score[2] = { 0,0 };

	hashpawn& pawn_eval = LookUpPawn();

	if (PawnHashHit(pawn_eval))
	{
		score[0] = pawn_eval.score[0];
		score[1] = pawn_eval.score[1];

		passed_list[0] = pawn_eval.passed_pawns[0];
		passed_list[1] = pawn_eval.passed_pawns[1];
	}
	else
	{
		passed_list[0] = 0; passed_list[1] = 0;
		kingside[0] = 0; kingside[1] = 0;
		queenside[0] = 0; queenside[1] = 0;
		kingattack[0] = 0; kingattack[1] = 0;
		queenattack[0] = 0; queenattack[1] = 0;

		score[0] += EvalPawns(0, 1);
		score[1] += EvalPawns(1, 0);

		pawn_eval.hashlock = currentpawnlock;

		for (int s = 0; s < 2; s++)
		{
			pawn_eval.score[s] = score[s];
			pawn_eval.passed_pawns[s] = passed_list[s];

			pawn_eval.defence[s][0] = scale[queenside[s]] + queenattack[s];
			pawn_eval.defence[s][1] = scale[kingside[s]] + kingattack[s];
		}
	}

	for (int s = 0; s < 2; s++)
	{
		const int xs = s ^ 1;

		score[s] += piece_mat[s] + pawn_mat[s] + table_score[s];

		if (bit_pieces[xs][Q])
		{
			const int kz = king_zone[s][kingloc[s]];

			if (kz < 2)
				score[s] += pawn_eval.defence[s][kz];

			score[xs] += kingqueen[pieces[xs][Q][0]][kingloc[s]];
		}
		else
		{
			score[s] += king_endgame_score[kingloc[s]];
		}
	}

	if (mask[A7] & bit_pieces[0][B] && mask[B6] & bit_pieces[1][P])
	{
		score[0] -= 150;
	}
	if (mask[H7] & bit_pieces[0][B] && mask[G6] & bit_pieces[1][P])
	{
		score[0] -= 150;
	}
	if (mask[A2] & bit_pieces[1][B] && mask[B3] & bit_pieces[0][P])
	{
		score[1] -= 150;
	}
	if (mask[H2] & bit_pieces[1][B] && mask[G3] & bit_pieces[0][P])
	{
		score[1] -= 150;
	}
	
	int diff = score[real_s] - score[real_xs];
	if (diff + 80 <= alpha || diff - 80 > beta)
	{
		return score[real_s] - score[real_xs];
	}

	bit_pawnattacks[0] = (bit_pieces[0][P] & not_a_file) << 7;
	bit_pawnattacks[0] |= (bit_pieces[0][P] & not_h_file) << 9;
	bit_pawnattacks[1] = (bit_pieces[1][P] & not_a_file) >> 9;
	bit_pawnattacks[1] |= (bit_pieces[1][P] & not_h_file) >> 7;

	for (int s = 0; s < 2; s++)
	{
		const int xs = s ^ 1;
		const BITBOARD denied_squares = ~(bit_pawnattacks[xs] | bit_units[s]);

		for (int x = 0; x < total[s][N]; x++)
		{
			const int sq = pieces[s][N][x];
			int nc = 0;
			BITBOARD b1 = bit_knightmoves[sq] & ~bit_units[s] & ~bit_pawnattacks[xs];
			nc = CountBits(b1);
			score[s] += knightsquares[nc];
			if (bit_knightmoves[sq] & bit_kingmoves[kingloc[xs]])
				score[s] += 2;
		}
		for (int x = 0; x < total[s][B]; x++)
		{
			const int sq = pieces[s][B][x];
			if (bit_bishopmoves[sq] & bit_kingmoves[kingloc[xs]])
				score[s] += 2;
			score[s] += bishopmoves[CountBits(MagicBishopAttacks(sq, bit_all) & denied_squares)];
		}
		score[s] += bishop_pair[s][total[s][B]];
		for (int x = 0; x < total[s][R]; x++)
		{
			const int sq = pieces[s][R][x];
			if (!(mask_cols[sq] & bit_pieces[s][P]))
			{
				score[s] += 10;
				if (!(mask_cols[sq] & bit_pieces[xs][P]))
				{
					score[s] += 10;
					if (mask_cols[sq] & bit_pieces[xs][K])
					{
						score[s] += 10;
					}
				}
			}
			if (adjfile[sq][kingloc[xs]] &&
				!(mask_path[s][sq] & bit_pawnattacks[xs] & bit_pieces[xs][P]))
			{
				score[s] += 5;
			}
			score[s] += rookmoves[CountBits(MagicRookAttacks(sq, bit_all) & denied_squares)];
		}
		if (bit_pieces[s][Q])
		{
			const int sq = pieces[s][Q][0];
			score[s] += queenmoves[CountBits(MagicQueenAttacks(sq, bit_all) & denied_squares)];
		}
	}

	if (((bit_pieces[0][P] & bit_unmoved_white) << 8) & bit_all)
	{
		score[0] -= 20;
	}
	if (((bit_pieces[1][P] & bit_unmoved_black) >> 8) & bit_all)
	{
		score[1] -= 20;
	}
	return score[real_s] - score[real_xs];
}

int EvalPawns(const int s, const int xs)
{
	int score = 0;

	BITBOARD pawns_s = bit_pieces[s][P];
	BITBOARD pawns_xs = bit_pieces[xs][P];
	BITBOARD b1 = bit_pieces[s][P];
	while (b1)
	{
		const int sq = NextBit(b1);
		b1 &= b1 - 1;
		score += EvalPawn(s, xs, sq, pawns_s, pawns_xs);
	}
	static const int central_sqs[4] = { D4, E4, D5, E5 };

	for (int x = 0; x < 4; ++x)
	{
		const int sq = central_sqs[x];
		if ((pawns_s & mask[sq]) && (pawns_s & bit_adjacent[sq]))
		{
			score += 15;
			break; 
		}
	}

	return score;
}

int EvalPawn(const int s, const int xs, const int sq, const BITBOARD pawn_s, const BITBOARD pawn_xs)
{
	int score = 0;
	if (!(mask_passed[s][sq] & pawn_xs) &&
		!(mask_path[s][sq] & pawn_s))
	{
		if (pawn_s & bit_adjacent[sq])
		{
			score += adjacent_passed[s][sq];
		}
		if (bit_pawncaptures[xs][sq] & pawn_s)
		{
			score += defended_passed[s][sq];
		}
		score += passed[s][sq];
		score += PieceScore[s][0][sq];
		passed_list[s] |= mask[sq];
		kingside[s] += KingSide[s][sq];
		queenside[s] += QueenSide[s][sq];
		kingattack[xs] += KingSide2[s][sq];
		queenattack[xs] += QueenSide2[s][sq];
		return score;
	}
	if ((mask_isolated[sq] & pawn_s) == 0)
	{
		score -= isolated[sq];
		if ((mask_cols[sq] & pawn_xs) == 0)
		{
			score -= isolated[sq];
			if (pawn_s & mask_path[s][sq])
			{
				score -= 10;
			}
		}
	}
	else
	{
		if (pawn_s & mask_path[s][sq])
		{
			score -= 10;
			if (!((pawn_s | pawn_xs) & mask_left_col[sq]) ||
				!((pawn_s | pawn_xs) & mask_right_col[sq]))
			{
				score -= MAJORITY_PENALTY;
			}
		}
		if ((mask_backward[s][sq] & pawn_s) == 0)
		{
			score -= BACKWARDS_PAWN_PENALTY;
			if (bit_pawncaptures[s][pawnplus[s][sq]] & pawn_xs)
			{
				score -= BACKWARDS_PAWN_PENALTY;
			}
			if (pawn_s & mask_path[xs][sq])
			{
				score -= isolated[sq];
			}
			if ((mask_cols[sq] & pawn_xs) == 0)
				score -= BACKWARDS_PAWN_PENALTY;
		}
	}
	score += PieceScore[s][0][sq];

	kingside[s] += KingSide[s][sq];
	queenside[s] += QueenSide[s][sq];

	kingattack[xs] += KingSide2[s][sq];
	queenattack[xs] += QueenSide2[s][sq];
	return score;
}





