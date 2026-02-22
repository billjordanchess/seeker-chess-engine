#include "globals.h"

constexpr int BACKWARDS_PAWN_PENALTY = 8;
constexpr int MAJORITY_PENALTY = 20;

int bishop_pair[2][3] = { {0,0,20},{0,0,20} };

int RookMoveCount(const int x, const BITBOARD denied_squares);
static int BishopMoveCount(const int x, const BITBOARD denied_squares);
static int QueenMoveCount(const int x, const BITBOARD denied_squares);

int EvalPawns(const int s, const int xs);
static int EvalPawn(const int s, const int xs, const int sq, const BITBOARD, const BITBOARD);

int EvalEndgame();
int LostEnding(const int s, const int xs);

int knightsquares[8] = {-48,-48,2,3,4,5,6,7};//
int bishopmoves[14] = {-5,1,2,3,4,5,6,7,8,9,10,11,12,13};
int rookmoves[15] = {-5,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
int queenmoves[28] = {-5,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9};

int king_defend[2][2][2][2][2][2][2][2][2];

static BITBOARD mask_d2e2= mask[D2] | mask[E2];
static BITBOARD mask_d7e7 = mask[D7] | mask[E7];

int CountBits(BITBOARD b1);

void z();

int Eval(const int alpha, const int beta)
{ 
	if (!piece_mat[xside] && startmat[xside] > 0)
	{
		if (bit_pieces[side][Q])
			if (!(bit_pieces[xside][0] & (mask_ranks[side][1] | mask_ranks[side][2])))
			{
				return 9900 - ply;
			}				
		if (bit_pieces[side][R])
			if (!(bit_pieces[xside][0] &
				(mask_ranks[side][1] | mask_ranks[side][2] | mask_ranks[side][3])))
			{
				return 9900 - ply;
			}			
	}

	if (piece_mat[0] <= QVAL && piece_mat[1] <= QVAL)
	{
		if (pawn_mat[0] == 0 && pawn_mat[1] == 0)
		{
			return EvalPawnless();
		}
		if ((piece_mat[0] < BBVAL || piece_mat[0] == QVAL) && 
			(piece_mat[1] < BBVAL || piece_mat[1] == QVAL))
		{
			return EvalEndgame(); 
		}
	}

	int score[2] = {0,0};

	hashpawn& pawn_eval = LookUpPawn();

	if (PawnHashHit(pawn_eval))
	{
		score[0] = pawn_eval.score[0];
		score[1] = pawn_eval.score[1];

		passed_list[0] = pawn_eval.passed_pawns[0];
		passed_list[1] = pawn_eval.passed_pawns[1];
		// defence access is direct too:
		// pawn_eval.defence[s][board_side]
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
		pawn_eval.score[0] = score[0];
		pawn_eval.score[1] = score[1];
		pawn_eval.passed_pawns[0] = passed_list[0];
		pawn_eval.passed_pawns[1] = passed_list[1];

		pawn_eval.defence[0][0] = scale[queenside[0]] + queenattack[0];
		pawn_eval.defence[0][1] = scale[kingside[0]] + kingattack[0];
		pawn_eval.defence[1][0] = scale[queenside[1]] + queenattack[1];
		pawn_eval.defence[1][1] = scale[kingside[1]] + kingattack[1];
	}

	/*
	if (LookUpPawn())
	{
		score[0] = GetHashPawn(0);
		score[1] = GetHashPawn(1);
	}
	else
	{
		score[0] += EvalPawns(0, 1);
		score[1] += EvalPawns(1, 0);
	
		AddPawnHash(score[0], score[1], passed_list[0], passed_list[1]);
		AddQueenHash(0, scale[queenside[0]] + queenattack[0]);
		AddKingHash(0, scale[kingside[0]] + kingattack[0]);
		AddQueenHash(1, scale[queenside[1]] + queenattack[1]);
		AddKingHash(1, scale[kingside[1]] + kingattack[1]);
	}
	*/
	score[0] += centi_pieces[piece_mat[0]] + centi_pawns[pawn_mat[0]] + table_score[0];
	score[1] += centi_pieces[piece_mat[1]] + centi_pawns[pawn_mat[1]] + table_score[1];

	if (bit_pieces[1][Q])
	{
		//score[0] += KingScore[0][kingloc[0]];
		int pawn_defence[3];
		pawn_defence[0] = pawn_eval.defence[0][0];
		pawn_defence[1] = pawn_eval.defence[0][1];
		pawn_defence[2] = 0;

		int kz = king_zone[0][kingloc[0]];
		score[0] += pawn_defence[kz];
		score[1] += kingqueen[pieces[1][Q][0]][kingloc[0]];
	}
	else
	{
		score[0] += king_endgame_score[kingloc[0]];
	}

	if (bit_pieces[0][Q])
	{
		//score[1] += KingScore[1][kingloc[1]];
		int pawn_defence[3];
		pawn_defence[0] = pawn_eval.defence[1][0];
		pawn_defence[1] = pawn_eval.defence[1][1];
		pawn_defence[2] = 0;

		int kz = king_zone[1][kingloc[1]];
		score[0] += pawn_defence[kz];
		score[1] += kingqueen[pieces[1][Q][0]][kingloc[0]];
		score[0] += kingqueen[pieces[0][Q][0]][kingloc[1]];
	}
	else
	{
		score[1] += king_endgame_score[kingloc[1]];
	}
/*
		score[0] -= 150 * (mask[A7] & bit_pieces[0][B] && mask[B6] & bit_pieces[1][P]);
		score[0] -= 150 * (mask[H7] & bit_pieces[0][B] && mask[G6] & bit_pieces[1][P]);
		score[1] -= 150 * (mask[A2] & bit_pieces[1][B] && mask[B3] & bit_pieces[0][P]);
		score[1] -= 150 * (mask[H2] & bit_pieces[1][B] && mask[G3] & bit_pieces[0][P]);
*/
//*
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
//*/
	int diff = score[side] - score[xside];
	if (diff + 80 <= alpha || diff - 80 > beta)
	{
	    return score[side] - score[xside];
	}
	
	bit_pawnattacks[0] = (bit_pieces[0][P] & not_a_file) << 7;
	bit_pawnattacks[0] |= (bit_pieces[0][P] & not_h_file) << 9;
	bit_pawnattacks[1] = (bit_pieces[1][P] & not_a_file) >> 9;
	bit_pawnattacks[1] |= (bit_pieces[1][P] & not_h_file) >> 7;

	int x, sq, xs;
	int i, nc;
	for (int s = 0; s < 2; s++)
	{
		xs = s ^1;
		for (x = 0; x < total[s][R]; x++)
		{
			sq = pieces[s][R][x];
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
			score[s] += rookmoves[RookMoveCount(sq, bit_pawnattacks[xs] | bit_units[s])];
		}
		for (x = 0; x < total[s][B]; x++)
		{
			if (bit_bishopmoves[pieces[s][B][x]] & bit_kingmoves[kingloc[xs]])
				 score[s] += 2;
			score[s] += bishopmoves[BishopMoveCount(pieces[s][B][x], bit_pawnattacks[xs] | bit_units[s])];
		}
		score[s] += bishop_pair[s][total[s][B]];
		
		for (x = 0; x < total[s][N]; x++)
		{
			i = pieces[s][N][x];
			nc = 0;
			BITBOARD b1 = bit_knightmoves[i] & ~bit_units[s] & ~bit_pawnattacks[s ^ 1];
			nc = CountBits(b1);
			if (bit_knightmoves[pieces[s][N][x]] & bit_kingmoves[kingloc[xs]])
				score[s] += 2;
			score[s] += knightsquares[nc];
		}
		if (bit_pieces[s][Q])
			score[s] += queenmoves[QueenMoveCount(pieces[s][Q][0], bit_pawnattacks[xs] | bit_units[s])];
	}
		if ((bit_pieces[0][P] & mask_d2e2) << 8 & bit_all)
		{
			score[0] -= 20;
		}
		if ((bit_pieces[1][P] & mask_d7e7) >> 8 & bit_all)
		{
			score[1] -= 20;
		}
	return score[side] - score[xside];
}

int EvalPawns(const int s, const int xs)
{
	int i, score = 0;

	BITBOARD pawns_s = bit_pieces[s][P];
	BITBOARD pawns_xs = bit_pieces[xs][P];
	BITBOARD b1 = bit_pieces[s][P];
	while (b1)
	{
		i = NextBit(b1);
		b1 &= b1 - 1;
		score += EvalPawn(s, xs, i, pawns_s, pawns_xs);
	}
	static const int central_sqs[4] = { D4, E4, D5, E5 };

	for (int x = 0; x < 4; ++x)
	{
		int sq = central_sqs[x];
		if ((pawns_s & mask[sq]) && (pawns_s & bit_adjacent[sq]))
		{
			score += 15;
			break; // only one bonus total, same as your if/else chain
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

static int BishopMoveCount(const int sq, const BITBOARD denied_squares)
{
	BITBOARD b2 = bit_bishopmoves[sq];
	BITBOARD b3 = b2 & bit_all;
	BITBOARD b4;
	while (b3)
	{
		int sq2 = NextBit(b3);
		b4 = ~bit_after[sq][sq2];
		b3 &= b4;
		b2 &= b4 | mask[sq2];
	}
	return CountBits(b2 & denied_squares);
}

int RookMoveCount(const int sq, const BITBOARD denied_squares)
{
	BITBOARD b2 = bit_rookmoves[sq];
	BITBOARD b3 = b2 & bit_all;
	BITBOARD b4;
	while (b3)
	{
		int sq2 = NextBit(b3);
		b4 = ~bit_after[sq][sq2];
		b3 &= b4;
		b2 &= b4 | mask[sq2];
	}
	return CountBits(b2 & denied_squares);
}

static int QueenMoveCount(const int sq, const BITBOARD denied_squares)
{
	BITBOARD b2 = bit_queenmoves[sq];
	BITBOARD b3 = b2 & bit_all;
	BITBOARD b4;
	while (b3)
	{
		int sq2 = NextBit(b3);
		b4 = ~bit_after[sq][sq2];
		b3 &= b4;
		b2 &= b4 | mask[sq2];
	}
	return CountBits(b2 & denied_squares);
}

static int MoveCount(const int sq, const BITBOARD squares, const BITBOARD denied_squares)
{
	BITBOARD b2 = squares;
	BITBOARD b3 = b2 & bit_all;
	BITBOARD b4;
	while (b3)
	{
		int sq2 = NextBit(b3);
		b4 = ~bit_after[sq][sq2];
		b3 &= b4;
		b2 &= b4 | mask[sq2];
	}
	return CountBits(b2 & denied_squares);
}

int CountBits2(BITBOARD b1)
{
	int c = 0;
	while (b1)
	{
		b1 &= b1 - 1;
		c++;
	}
	return c;
}


