#include "globals.h"

constexpr int  HALFKENDMOB = 4;

constexpr int  DRAWPLUS = 1;
constexpr int  DRAWMINUS = -1;
constexpr int  ROOKBEHIND = 15;

constexpr int  RANK_6 = 5;
constexpr int  RANK_7 = 6;

constexpr int  QUEENING = 650;

int endgame_score[MAX_PLY][2];
int endmatrix[10][3][10][3];

int EvalPawns(const int s, const int xs);
int EvalPawn(const int s, const int xs, const int sq, const BITBOARD pawn_s, const BITBOARD pawn_xs);
int RookMoveCount(const int x, const BITBOARD denied_squares);

int PawnEndingScore(const int s, const int xs);
int PawnRace(const int s, const int, const int p);
int OpposedPawns(const int s, const int);
int MostAdvancedPawn(const int s, const int);
int LeastDifference(const int s, const int xs);
int PawnPositional(const int s, const int xs);

int PiecesPassedPawnScore(const int s, const int xs, const int king, const int xking);

int OppositeBishops();

int DrawnEnding(const int, const int);
int KingVersusPawns(const int s, const int xs, const int);

bool RookBehind(const int s, const int from, const int to);

int GetDistance(const int xs, const int x, const int king);

int EvalPawnless(const int, const int);

int EvalEndgame(const int s, const int xs)
{
	int score[2];
	score[0] = pawn_mat[0] + piece_mat[0] + table_score[0] + king_endgame_score[kingloc[0]];
	score[1] = pawn_mat[1] + piece_mat[1] + table_score[1] + king_endgame_score[kingloc[1]];

	hashpawn& pawn_eval = LookUpPawn();

	if (PawnHashHit(pawn_eval))
	{
		score[0] += pawn_eval.score[0];
		score[1] += pawn_eval.score[1];

		passed_list[0] = pawn_eval.passed_pawns[0];
		passed_list[1] = pawn_eval.passed_pawns[1];
	}
	else
	{
		passed_list[0] = 0;
		passed_list[1] = 0;
		pawn_eval.hashlock = currentpawnlock;

		int s1 = EvalPawns(0, 1);
		int s2 = EvalPawns(1, 0);
		score[0] += s1;
		score[1] += s2;

		pawn_eval.score[0] = s1;
		pawn_eval.score[1] = s2;
		pawn_eval.passed_pawns[0] = passed_list[0];
		pawn_eval.passed_pawns[1] = passed_list[1];
	}

	bit_pawnattacks[0] = (bit_pieces[0][P] & not_a_file) << 7;
	bit_pawnattacks[0] |= (bit_pieces[0][P] & not_h_file) << 9;

	bit_pawnattacks[1] = (bit_pieces[1][P] & not_a_file) >> 9;
	bit_pawnattacks[1] |= (bit_pieces[1][P] & not_h_file) >> 7;

	BITBOARD b1 = bit_pieces[s][P] & mask_ranks[s][6];

	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		int to = pawnplus[s][from];
		if (b[to] == EMPTY && !Attack(xs, to) && !(RookBehind(xs, from, to)))
		{
			score[s] += 800;
			return score[s] - score[xs];
		}
	}

	if (piece_mat[0] == 0 && piece_mat[1] == 0)
	{
		if (pawn_mat[0] == 0 && pawn_mat[1] > 0)
		{
			return KingVersusPawns(0, 1, s);
		}
		else if (pawn_mat[1] == 0 && pawn_mat[0] > 0)
		{
			return KingVersusPawns(1, 0, s);
		}
		score[s] += PawnEndingScore(s, xs);
		score[0] += PawnPositional(s, xs);
		score[1] += PawnPositional(xs, s);
		score[0] += pawn_mat[0] / 2;//24
		score[1] += pawn_mat[1] / 2;//24
			//King attack pawn
		for (int x = 0; x < 2; x++)
			if (bit_kingmoves[kingloc[x]] & bit_pieces[x ^ 1][P] & ~(bit_pawnattacks[x ^ 1]))
			{
				score[x] += 25;
			}

		return score[s] - score[xs];
	}
	//static score
	score[0] += pawn_mat[0] / 4;
	score[1] += pawn_mat[1] / 4;

	if (pawn_mat[0] > 0 && piece_mat[0] > 0 &&
		piece_mat[0] + pawn_mat[0] > piece_mat[1] + pawn_mat[1])
	{
		if (mask_abc & bit_pieces[0][P] && mask_def & bit_pieces[0][P])
		{
			score[0] += 10;
		}
	}
	else if (pawn_mat[1] > 0 && piece_mat[1] > 0 &&
		piece_mat[1] + pawn_mat[1] > piece_mat[0] + pawn_mat[0])
	{
		if (mask_abc & bit_pieces[1][P] && mask_def & bit_pieces[1][P])
		{
			score[1] += 10;
		}
	}
	if (OppositeBishops() > 0)
	{
		score[0] -= (pawn_mat[0] / 2);
		score[1] -= (pawn_mat[1] / 2);
	}
	for (int s = 0; s < 2; s++)
	{
		if (pawn_mat[s] == 0)
		{
			if (piece_mat[s] == B_VALUE && score[s] > score[!s])//
			{
				if (score[s] >= 0)
					return 0;
			}
			if (piece_mat[s] < BB_VALUE && pawn_mat[!s] == P_VALUE &&
				(piece_mat[!s] < BB_VALUE || piece_mat[!s] == Q_VALUE)
				&& DrawnEnding(s, !s) && ply>1)
			{
				return 0;
			}

		}
	}
	for (int s = 0; s < 2; s++)
	{
		if (passed_list[s])
			score[s] += PiecesPassedPawnScore(s, s ^ 1, kingloc[s], kingloc[s ^ 1]);
	}
	for (int s = 0; s < 2; s++)
	{
		for (int x = 0; x < total[s][R]; x++)
		{
			int sq = pieces[s][R][x];
			score[s] += RookMoveCount(sq, bit_pawnattacks[!s]);
		}
	}
	int diff, dec;
	if (fifty > 10)
	{
		if (score[s] > score[xs])
		{
			diff = score[s] - score[xs];
			dec = (fifty * diff) / 10;
			if (dec > diff)
				dec = diff;
			score[s] -= dec;
		}
		else
		{
			diff = score[xs] - score[s];
			dec = (fifty * diff) / 10;
			if (dec > diff)
				dec = diff;
			score[xs] -= dec;
		}
	}
	
	return score[s] - score[xs];
}

bool RookBehind(const int s, const int from, const int to)
{
	BITBOARD b1 = bit_after[to][from] & bit_pieces[s][R];
	if (b1 && !(bit_between[from][NextBit(b1)] & bit_all))
	{
		return true;
	}
	return false;
}

int DrawnEnding(const int s, const int xs)
{
	int pawn = 0;
	int king = kingloc[s];
	int xking = kingloc[xs];

	if (pawn_mat[s] == 1 && piece_mat[s] == 0 && pawn_mat[xs] == 0)
	{
		pawn = NextBit(bit_pieces[s][P]);

		//knight v p
		if (total[xs][N] == 1)
		{
			int knight_square = NextBit(bit_pieces[xs][N]);
			if (!(mask[knight_square] & mask_corner) && col[knight_square] == col[pawn] &&
				row2[xs][knight_square] < row2[xs][pawn] && !(bit_kingmoves[king] & mask[xking] && s == xside))
			{
				return 1;
			}
			return 0;
		}
		//bishop v p
		if (total[xs][B] == 1)
		{
			if ((bit_bishopmoves[pawnplus[s][pawn]] & bit_pieces[xs][B] ||
				mask[pawnplus[s][pawn]] & bit_units[xs]) &&
				!(bit_kingmoves[king] & bit_pieces[xs][B]))
			{
				return 1;
			}

			return 0;
		}
		//rook v p
		if (piece_mat[xs] == R_VALUE)
		{
			int to = lastsquare[s][pawn];
			int dist = row2[xs][pawn];
			if (difference[king][pawn] == 1 && king != to &&
				difference[king][to] == 1 &&
				difference[xking][to] >= dist + difference[king][to] &&
				!(bit_kingmoves[king] & bit_pieces[xs][R]))
			{
				return 1;
			}
			return 0;
		}
		//queen v p
		if (piece_mat[xs] == Q_VALUE)
		{
			if ((col[pawn] == 0 || col[pawn] == 7) && row2[s][pawn] == 6 &&
				difference[king][pawnplus[s][pawn]] < 2 && !(bit_pieces[xs][Q] & bit_kingmoves[king]) &&
				(abs(col[xking] - col[pawn]) > 4 || row2[xs][xking] > 4))
			{
				return 1;
			}
			if (((col[pawn] == 2 && (col[king] == 0 || col[king] == 1))
				|| (col[pawn] == 5 && (col[king] == 6 || col[king] == 7))) &&
				row2[s][pawn] == RANK_7 &&
				difference[king][pawnplus[s][pawn]] < 2 &&
				!(bit_pieces[xs][Q] & bit_kingmoves[king]) &&
				(abs(col[xking] - col[pawn]) > 4 || row2[xs][xking] > 4))
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	//phildor position
	if (piece_mat[xs] == R_VALUE && piece_mat[s] == R_VALUE &&
		pawn_mat[xs] == P_VALUE && pawn_mat[s] == 0)
	{
		int pawn = NextBit(bit_pieces[xs][P]);
		int rook = NextBit(bit_pieces[s][R]);
		if (mask_passed[xs][pawn] & bit_pieces[s][K] &&
			!(bit_rookmoves[xking] & bit_pieces[xs][R]) &&
			!(bit_rookmoves[xking] & bit_pieces[s][R]) &&
			!(bit_kingmoves[xking] & bit_pieces[xs][R]) &&
			!(bit_kingmoves[xking] & bit_pieces[s][R]) &&
			row2[xs][pawn] < 5 && row[rook] == row[pawnplus[s][pawn]])
			return 1;
		return 0;
	}

	//  rook pawn of wrong colour  
	if (total[xs][B] == 1 &&
		!(bit_pieces[xs][P] & not_mask_rookfiles) &&
		piece_mat[s] <= B_VALUE)
	{
		int pawn = NextBit(bit_pieces[xs][P]);
		if (difference[king][lastsquare[xs][pawn]] < 2)
			if (colors[pieces[xs][B][0]] != colors[lastsquare[xs][pawn]])
			{
				return 1;
			}
	}
	return 0;
}

int PiecesPassedPawnScore(const int s, const int xs, const int king, const int xking)
{
	int score = 0;
	int x;
	BITBOARD b1 = passed_list[s] & ~(mask_squarepawn[xs][xking]);
	if (s == 1)
	{
		//PrintBitBoard(mask_squareking[1][kingloc[1]]);
		//z();
	}

	while (b1)
	{
		x = NextBit(b1);
		b1 &= b1 - 1;

		score += passed[s][x];

		if (row2[s][x] == RANK_7)
		{
			if (bit_pawncaptures[xs][x] & passed_list[s])
			{
				if (total[xs][N] + total[xs][R] <= 1 &&
					difference[xking][x] > 2)
				{
					score += 200;
				}
			}
			if (bit_adjacent[x] & passed_list[s])
			{
				if (total[xs][N] + total[xs][R] <= 1 &&
					difference[xking][x] > 1)
					score += 800;
			}
		}
		if (row2[s][x] == RANK_6)
		{
			if (bit_adjacent[x] & passed_list[s])
			{
				if (total[xs][N] + total[xs][R] <= 1 &&
					difference[xking][x] > 3)
					score += 100;
			}
		}
		if (piece_mat[xs] == 0 && !(mask_path[s][x] & bit_all))
		{
			score += PawnRace(s, xs, x);
		}
		else if (total[xs][N] == 1)
		{
			score -= pawn_difference[king][x] + pawn_difference[xking][x];
		}
	}
	return score;
}

int OppositeBishops()
{
	if (total[0][B] == 1 && total[1][B] == 1)
	{
		if (colors[pieces[0][B][0]] != colors[pieces[1][B][0]])
			return 1;
	}
	return 0;
}

int KingVersusPawns(const int s, const int xs, const int real_side)
{
	if (pawn_mat[xs] == 1)
	{
		int pawn = NextBit(bit_pieces[xs][P]);
		int xking = kingloc[xs];
		int king = kingloc[s];
		if (b[pawnplus[s][king]] == P ||
			b[pawndouble[s][king]] == P)
			if (row2[s][king] != 0 || bit_pieces[xs][P] & mask_rookfiles)
			{
				return 1;
			}
		if (bit_pieces[xs][K] & mask_rookfiles &&
			col[xking] == col[pawn] &&
			difference[king][xking] == 2 && row2[xs][king] >= row2[xs][xking] &&
			row2[xs][xking] > row2[xs][pawn])
		{
			return 1;
		}
		if ((col[pawn] == 0 || col[pawn] == 7)
			&& abs(col[king] - col[pawn]) < 2 &&
			row2[s][king] < row2[s][pawn])
		{
			return 1;
		}
		if (col[pawn] == 0 || col[pawn] == 7)//could be in main loop
		{
			if (row2[xs][pawn] > 3 && difference[pawn][king] > 1)
			{
				if (col[pawn] == 0 && xking == squares[xs][B7])
				{
					return -600;
				}
				if (col[pawn] == 7 && xking == squares[xs][G7])
				{
					return -600;
				}
			}
		}
		int p2 = pawndouble[xs][pawn];
		if ((p2 == xking || p2 - 1 == xking || p2 + 1 == xking)
			&& difference[pawn][xking] > 1)
		{
			return -600;
		}
		if (s == real_side && !(mask_squarepawn[s][pawn] & bit_pieces[xs][K]))
			return -600;
	}
	else
	{
		//doubled rook pawns
		BITBOARD xp = bit_pieces[xs][P];
		if (!(~mask_files[0] & xp) &&
			!(~mask_files[7] & xp))
		{
			int xking = kingloc[xs];
			int king = kingloc[s];
			if (!(mask_passed[xs][king] & xp) &&
				!(mask_passed[xs][xking] & xp))
			{
				if ((b[pawnplus[s][king]] == P ||
					b[pawndouble[s][king]] == P))
				{
					return 0;
				}
				if (bit_pieces[xs][K] & mask_rookfiles &&
					row2[s][king] <= row2[s][xking] &&
					difference[king][xking] < 3)
				{
					return 0;
				}
			}
		}
	}
	return 0;
}

int PawnEndingScore(const int s, const int xs)
{
	int score = 0;
	BITBOARD b1 = passed_list[s];
	BITBOARD b2 = passed_list[xs];
	int king = kingloc[s];
	int xking = kingloc[xs];

	int best = 64;
	int bestx = 64;
	int runner = 0;
	int runnerx = 0;

	while (b1)
	{
		int x = NextBit(b1);
		b1 &= b1 - 1;
		if (!(mask_squarepawn[s][x] & bit_pieces[xs][K]))
		{
			int distance = row2[xs][x];
			if (col[x] == col[king])
				distance++;
			if (distance < best)
				best = distance;
			runner = 1;
			continue;
		}
		if (!(mask_path[s][x] & ~bit_kingmoves[king]) &&
			!(bit_kingmoves[xking] & mask[x] &&
				!(bit_kingmoves[king] & mask[x])))
		{
			int distance = row2[xs][x];
			if (col[x] == col[king])
				distance++;
			if (distance < best)
				best = distance;
			continue;
		}
		if (!(mask_path[s][king] & bit_pawnattacks[xs]) &&
			row2[s][king] >= row2[s][xking] &&
			difference[king][x] == 1 &&
			col[king] != col[x])
		{
			int distance = row2[xs][x];
			if (col[x] == col[king])
				distance++;
			if (distance < best)
				best = distance;
			continue;
		}
		if (!(mask_rookfiles & mask[x]))
		{
			if (mask_path[s][x] & bit_pieces[s][K] &&
				row[king] == row[xking])
			{
				int distance = row2[xs][x];
				if (col[x] == col[king])
					distance++;
				if (distance < best)
					best = distance;
				continue;
			}
		}
	}

	king = kingloc[xs];
	xking = kingloc[s];

	while (b2)
	{
		int x = NextBit(b2);
		b2 &= b2 - 1;
		if (!(mask_squarepawn[xs][pawnplus[s][x]] & bit_pieces[s][K]))
		{
			int distance = row2[s][x];
			if (col[x] == col[king])
				distance++;
			if (distance < bestx)
				bestx = distance;
			runnerx = 1;
			continue;
		}
		if (!(mask_path[xs][x] & ~bit_kingmoves[king]) &&
			!(bit_kingmoves[xking] & mask[x] &&
				!(bit_kingmoves[king] & mask[x])))
		{
			int distance = row2[s][x];
			if (col[x] == col[king])
				distance++;
			if (distance < bestx)
				bestx = distance;
			continue;
		}
		if (!(mask_path[s][king] & bit_pawnattacks[xs]) &&
			row2[s][king] >= row2[s][xking] &&
			difference[king][x] == 1 &&
			col[king] != col[x])
		{
			int distance = row2[s][x];
			if (col[x] == col[king])
				distance++;
			if (distance < bestx)
				bestx = distance;
			continue;
		}
		if (!(mask_rookfiles & mask[x]))
		{
			if (mask_path[xs][x] & bit_pieces[xs][K] &&
				row[king] == row[xking])
			{
				int distance = row2[s][x];
				if (col[x] == col[king])
					distance++;
				if (distance < bestx)
					bestx = distance;
				continue;
			}
		}
	}

	if (best < bestx)
	{
		int most_advanced = MostAdvancedPawn(xs, s);
		if (best < most_advanced)
		{
			/*
			printf(" runner %d ", runner);
			printf(" runnerx %d ", runnerx);
			printf(" best %d ", best);
			printf(" bestx %d ", bestx);
			printf(" counter! %d ", most_advanced);
			z();
			*/
			return 600;
		}
		/*
		else if (bestx < 64)
		{
			printf(" runner %d ", runner);
			printf(" runnerx %d ", runnerx);
			printf(" best %d ", best);
			printf(" bestx %d ", bestx);
			printf(" counter! %d ", most_advanced);
			z();
		}
		//*/
	}

	if (bestx + 1 < best)
	{
		int most_advanced = MostAdvancedPawn(xs, s);
		if (bestx + 1 < most_advanced)
		{
			;// printf(" most_advanced %d ", most_advanced);
			//z();
			return -600;
		}
		/*
		else if (bestx < 64)
		{
			printf(" runner %d ", runner);
			printf(" runnerx %d ", runnerx);
			printf(" best %d ", best);
			printf(" bestx %d ", bestx);
			printf(" counter! %d ", most_advanced);
			z();
		}
		//*/
	}
	return 0;
}

int GetDistance(const int xs,const int x, const int king)
{
int distance = row2[xs][x];
if (col[x] == col[king])
	distance++;
return distance;
}

int PawnPositional(const int s, const int xs)
{
	int score = 0;
	BITBOARD b1 = passed_list[s];

	while (b1)
	{
		int x = NextBit(b1);
		b1 &= b1 - 1;
		//supported passed pawn
		if (row2[s][x] > 2)
		{
			if (bit_left[xs][x] & bit_pieces[s][P])
			{
				int a = pawnleft[xs][x];
				if ((mask_passed[s][a] & not_mask[x - 1] & bit_pieces[xs][P]) == 0)
				{
					score += 50;
				}
				else
				{
					if (bit_right[xs][x] & bit_pieces[s][P])
					{
						a = pawnright[xs][x];
						if ((mask_passed[s][a] & not_mask[x + 1] & bit_pieces[xs][P]) == 0)
						{
							score += 50;
						}
					}
				}
			}
		}
		//outside passed pawn
		if ((col[x] == 0 || (col[x] == 1 && (mask_files[0] & bit_pieces[xs][P]) == 0)) &&
			(bit_pieces[s][P] & mask_def))
		{
			score += 20;
		}
		if ((col[x] == 7 || (col[x] == 6 && (mask_files[7] & bit_pieces[xs][P]) == 0)) &&
			(bit_pieces[s][P] & mask_abc))
		{
			score += 20;
		}
	}
	return score;
}

/*
		if ((pawndouble[s][x] == king ||
				pawndouble[s][x] == king - 1 ||
				pawndouble[s][x] == king + 1) &&
				difference[xking][x] > 1)
			{
				//printf(" 5 ");
				//Algebraic(x);
				//z();
				runner[s] |= mask[x];
				continue;
			}
*/

int PawnRace(const int s, const int xs, const int p)
{
	if (!pawn_mat[xs])
	{
		return QUEENING - ply;
	}
	int path = row2[xs][p];

	if (mask_path[s][p] & bit_pieces[s][K])
	{
		path++;
	}

	int path2;
	if (OpposedPawns(xs, s) == 1 || BlockedPawns(s, xs) == 1)
	{
		path2 = LeastDifference(s, xs);
	}
	else
		path2 = MostAdvancedPawn(xs, s);

	if (s == xside)
		path2--;

	if (path < path2)
	{
		return QUEENING - ply;
	}
	//queen with check
	if (path == path2)
	{
		int target = lastsquare[s][p];
		if (bit_queenmoves[target] & mask[kingloc[xs]] &&
			!(bit_between[target][kingloc[xs]] & bit_all) &&
			!(bit_kingmoves[kingloc[xs]] & mask[target])
			)
		{
			//Algebraic(p);
			//z();
			return QUEENING - ply;
		}
	}
	return 0;
}

int LeastDifference(const int s, const int xs)
{
	BITBOARD b1 = bit_pieces[s][P] & ~passed_list[s];
	int diff = 100;
	int least = 0;
	int x;

	while (b1)
	{
		x = NextBit(b1);
		b1 &= b1 - 1;
		least = difference[kingloc[xs]][x] + 2 + row2[s][x];
		if (least < diff)
			diff = least;
	}
	return diff;
}

int MostAdvancedPawn(const int s, const int xs)
{
	BITBOARD b1 = bit_pieces[s][P] & ~passed_list[s];

	BITBOARD b2;
	int sq;
	int advanced = 64;
	int x, current;
	
	current = 0;
	while (b1)
	{
		x = NextBit(b1);
		b1 &= b1 - 1;
		current = row2[xs][x];
		if (current == 6)
			current = 5;//double pawn move;
		if (mask_path[s][x] & bit_pieces[s][K])
			current++;
		b2 = mask_path[s][x] & bit_pieces[xs][P];
		if (current == 3 && !(bit_adjacent[x] & ~bit_pieces[s][P]))
		{
			current = 3;
			if (current < advanced)
			{
				advanced = current;
				continue;
			}				
		}
		if (b2)
		{
			sq = NextBit(b2);
			current += difference[kingloc[s]][sq] + 1;
		}
		if (current < advanced)
			advanced = current;
	}
	return advanced;
}

int BlockedPawns(const int s, const int xs)
{
	if ((bit_pawnattacks[s] & bit_units[xs]) != 0)
	{
		return 0;
	}
	if (s == 0 && ((bit_pieces[s][P] << 8) & bit_pieces[xs][P]) == bit_pieces[xs][P])
	{
		return 1;
	}
	if (s == 1 && ((bit_pieces[s][P] >> 8) & bit_pieces[xs][P]) == bit_pieces[xs][P])
	{
		return 1;
	}
	return 0;
}

int OpposedPawns(const int s, const int xs)
{
	if ((bit_pawnattacks[s] & bit_units[xs]) != 0)
	{
		return 0;
	}
	for (int x = 0; x < 8; x++)
	{
		if (bit_pieces[s][P] & mask_files[x] &&
			!(bit_pieces[xs][P] & mask_files[x]))
			return 0;
	}
	return 1;
}

int EvalPawnless(const int s, const int xs)
{
	const int king = kingloc[s];
	const int xking = kingloc[xside];
		
	if (startmat[0] != piece_mat[0] || startmat[1] != piece_mat[1])
	{
		int result = endmatrix[piece_mat[s]][total[s][N]][piece_mat[xs]][total[xs][N]];
		if (result == DRAWN)
		{
			return 0;
		}
		if (result != 0 &&
			SafeKingMoves(s, xs) > 0 &&
			!(bit_kingmoves[king] & bit_units[xs]) &&
			!(bit_kingmoves[xking] & bit_units[s]))
		{
			return result;
		}
	}
	int score[2];
	score[0] = piece_mat[0] + table_score[0];
	score[1] = piece_mat[1] + table_score[1];
	score[s] += KingPawnLess[king];
	score[xs] += KingPawnLess[xking];
	if (abs(piece_mat[0] - piece_mat[1]) < R_VALUE)
	{
		score[0] -= (piece_mat[0] >> 1);
		score[1] -= (piece_mat[1] >> 1);
	}
	if (piece_mat[s] > piece_mat[xs])
		score[s] += kingking[king][xking];
	else if (piece_mat[s] < piece_mat[xs])
		score[xs] += kingking[xking][king];
	return score[s] - score[xs];
}



