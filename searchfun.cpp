#include <chrono>

#include "globals.h"

std::chrono::steady_clock::time_point g_deadline;
bool g_use_deadline = false;

static inline bool time_up();

void PrintPV();
void DisplayPV(int depth, int score_cp, unsigned long long nodes);

bool PawnCounterAttack(const int from, const int to, const BITBOARD bit_threshold);
bool CounterAttack(const int from, const int to, const int, const BITBOARD bit_threshold);
BITBOARD GetThreshold(const int s, const int attacked);
int GetThreat(const int target);
BITBOARD PinnersPossible(const int s, const int xs);

void UpdateSequence(const int ply, const int depth, const move_data& m);
int ContinuationScore(int ply, int piece, int to);
inline void UpdateWithGravity(int& cell, int bonus);

bool LightSEE(const int s, const int xs, const int att1, const int sq);
int GetLowestQuietAttacker(const int s, const int to);

void UpdateCheckHistory(const int from, const int to, const int x, const int depth);

int GetAttackingSquare(const int s, const int sq);

extern int pv_len[MAX_PLY];
extern int old_pv_len[MAX_PLY];

extern move_data pv[MAX_PLY][MAX_PLY];
extern move_data old_pv[MAX_PLY][MAX_PLY];

extern move_data counter[64][64];
extern move_data killer[MAX_PLY];
extern move_data killer2[MAX_PLY];
extern move_data mate_move[MAX_PLY];

extern int total_killers[2];
extern int currentdepth;
extern int currentmax;

extern int frontier[8];

extern int cont_hist[6][64][6][64];

// Optional: keep a second table for "prev2" (two plies back) for extra strength
extern int cont2_hist[6][64][6][64];

void SortLastDepth(const int first, const int last)
{
	int from, to;
	for (int i = first; i < last; i++)
	{
		from = move_list[i].from;
		to = move_list[i].to;
		move_list[i].score = PieceScore[side][b[from]][to] - PieceScore[side][b[from]][from] + frontier[b[from]];
	}
}

int Sort(const int from, const int top, const int last)
{
	int bestscore = move_list[from].score;
	int best_index = from;
	const int start = from + 1;
	for (int i = start; i < last; i++)
		if (move_list[i].score > bestscore)
		{
			bestscore = move_list[i].score;
			best_index = i;
			if (bestscore >= top)
			{
				//Alg(move_list[i].from, move_list[i].to);
				//ShowAll(ply);
				break;
			}
		}

	move_data g = move_list[from];
	move_list[from] = move_list[best_index];
	move_list[best_index] = g;

	return bestscore;
}

int SortCaptures(const int from, const int top, const int last)
{
	int bestscore = move_list[from].score;
	int best_index = from;
	const int start = from + 1;
	for (int i = start; i < last; i++)
		if (move_list[i].score > bestscore)
		{
			bestscore = move_list[i].score;
			best_index = i;
			if (bestscore >= top)
			{
				break;
			}
		}

	move_data g = move_list[from];
	move_list[from] = move_list[best_index];
	move_list[best_index] = g;

	return bestscore;
}

void SelectCheck(const int from, const int last)
{
	int best = from;
	const int first = from + 1;

	for (int i = first; i < last; i++)
		if (move_list[i].score > move_list[best].score)
		{
			best = i;
		}

	move_data g = move_list[from];
	move_list[from] = move_list[best];
	move_list[best] = g;
}

void CheckUp()
{
	U64 tim = GetTime();
	if (tim >= stop_time)
	{
		stop_search = true;
	}
}

static inline bool time_up()
{
	//if (stop_search.load(std::memory_order_relaxed))
	//	return true;

	if (g_use_deadline && std::chrono::steady_clock::now() >= g_deadline)
		return true;

	return false;
}

static void set_deadline(int time_limit_ms)
{
	if (time_limit_ms < 0)
	{
		g_use_deadline = false;
		return;
	}
	g_use_deadline = true;
	g_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(time_limit_ms);
}

// In your Search() loop:
//if ((nodes & 2047) == 0) { // every 2048 nodes
//	if (time_up()) return alpha; // or return best so far
//}

int Reduce(const int i)
{
	if (move_list[i].flags & (CAPTURE | CHECK | PROMOTE | PASSED7))
		return 0;
	if (piece_mat[xside] == 0)
		return 0;
	if (game_list[hply - 1].flags & MATETHREAT)
	{
		return 0;
	}
	//if (piece_mat[side] <= 300 && fifty > 8)
		//cout << "+";
	//	return 2;
	if (piece_mat[side] <= 300 && b[move_list[i].to] == K)
	{
		return 0;
	}
	return 1;
}

void DisplayPV2()
{
	if (pv_len[0] <= 0)
		return;

	for (int i = 0; i < pv_len[0]; ++i)
	{
		Alg(pv[0][i].from, pv[0][i].to);
		printf(" ");
	}
	printf("\n");
}

static inline void SquareToChars(int sq, char* out)
{
	out[0] = char('a' + (sq & 7));
	out[1] = char('1' + (sq >> 3));
}

static inline void PrintUciMove(const move_data& m)
{
	char buf[6];
	SquareToChars(m.from, buf);
	SquareToChars(m.to, buf + 2);

	int n = 4;
	// promo must be lowercase 'q','r','b','n' when present
	//if (m.promo) buf[n++] = m.promo;

	buf[n] = 0;
	std::printf("%s", buf);
}

void PrintPV()
{
	if (pv_len[0] <= 0)
		return;

	for (int i = 0; i < pv_len[0]; ++i)
	{
		PrintUciMove(pv[0][i]);
		if (i + 1 < pv_len[0]) std::printf(" ");
	}
}

void DisplayPV(int depth, int score_cp, unsigned long long nodes)
{
	int time_ms = GetTime() - start_time;  // adapt if your time vars differ

	printf("info depth %d score cp %d time %d nodes %llu pv ",
		depth, score_cp, time_ms, nodes);

	PrintPV();

	printf("\n");
	fflush(stdout);
}

void UpdateCheckHistory(const int from, const int to, const int x, const int depth)
{
	if (check_history[b[from]][to] < HISTORY_LIMIT)
		check_history[b[from]][to] += depth;
	else
		check_history[b[from]][to] >>= 1;
	if (x > WON_ENDGAME && check_history[b[from]][to] < CHECK_SCORE)
		check_history[b[from]][to] = CHECK_SCORE;
}

void UpdateKillers(const int i, const int from, const int to, const int x, const int depth)
{
	if (hist_from[side][b[from]][from] + depth < HISTORY_LIMIT &&
		hist_to[side][b[from]][to] + depth < HISTORY_LIMIT)
	{
		hist_from[side][b[from]][from] += depth;
		hist_to[side][b[from]][to] += depth * depth;
		if (game_list[hply - 1].capture == EMPTY)
		{
			/*
			printf("\n counter ");
			Alg(game_list[hply - 1].from,game_list[hply - 1].to);
			printf(" ");
			Alg(from, to);
			z();
			//*/
			counter[game_list[hply - 1].from][game_list[hply - 1].to].from = from;
			counter[game_list[hply - 1].from][game_list[hply - 1].to].to = to;
		}
	}
	else
	{
		hist_from[side][b[from]][from] >>= 1;
		hist_to[side][b[from]][to] >>= 1;
	}

	if (!(killer[ply].from == from && killer[ply].to == to))
	{
		killer2[ply] = killer[ply];
		killer[ply].from = from;
		killer[ply].to = to;
		killer[ply].score = b[from];
	}
}

void ClearKillers()
{
	memset(mate_move, 0, sizeof(mate_move));
	memset(killer, 0, sizeof(killer));
	memset(killer2, 0, sizeof(killer2));
}

int GetCurrentDepth()
{
	return currentdepth;
}

int Reps2()
{
	for (int i = hply - 4; i >= hply - fifty; i -= 2)
	{
		if (game_list[i].hash == currentkey)
		{
			return 1;
		}
	}
	return 0;
}

bool IsLegal(const int from, const int to)
{
	if (b[from] == K)
	{
		if (bit_kingmoves[from] & mask[to])
			return true;
		if (abs(from - to) == 2)
		{
			if (to == G1 && castle & 1)
				return true;
			else if (to == C1 && castle & 2)
				return true;
			else if (to == G8 && castle & 4)
				return true;
			else if (to == C8 && castle & 8)
				return true;
		}
		return false;
	}
	if (b[from] == R)
	{
		if (bit_rookmoves[from] & mask[to] &&
			(bit_between[from][to] & bit_all) == 0)
			return true;
		else
		{
			return false;
		}
	}
	if (b[from] == B)
	{
		if (bit_bishopmoves[from] & mask[to] &&
			(bit_between[from][to] & bit_all) == 0)
			return true;
		else
			return false;
	}
	if (b[from] == Q)
	{
		if (bit_queenmoves[from] & mask[to] &&
			(bit_between[from][to] & bit_all) == 0)
			return true;
		else
		{
			return false;
		}
	}
	if (b[from] == N)
	{
		if (bit_knightmoves[from] & mask[to])
			return true;
		else
		{
			return false;
		}
	}
	if (b[from] == P)
	{
		if (pawnplus[side][from] == to)
		{
			if (b[to] == EMPTY)
			{
				return true;
			}
			return false;
		}
		if (bit_pawndefends[xside][from] & mask[to] && mask[to] & bit_units[xside])
		{
			return true;
		}
		if (pawndouble[side][from] == to)
		{
			if (b[pawnplus[side][from]] == EMPTY && b[to] == EMPTY)
			{
				return true;
			}
			return false;
		}
		if (b[to] == EMPTY && col[from] != col[to] &&
			bit_pawndefends[xside][from] & mask[to])
		{
			if ((b[game_list[hply - 1].to] == P &&
				abs(game_list[hply - 1].from - game_list[hply - 1].to) == 16 &&
				col[game_list[hply - 1].to] == col[to]))
			{
				return true;
			}
			return false;
		}
	}
	return true;
}

int GetThreat(const int target)
{
	int attacker = GetNextAttackerSquare(side, xside, target, bit_all);
	if (attacker == -1)
		return -1;

	int count = 0;

	if (piece_value[b[attacker]] < piece_value[b[target]] ||
		Attack(xside, target) == 0// ||
		//SEE(xside, attacker, target) > 0)//
		)
	{
		//Alg(attacker, target);
		//	z();
		//	printf(" targets ");
		return 1;
	}

	return -1;
}

int GetLineTarget()
{
	BITBOARD bit_target;
	int sq = -1;
	bit_target = (bit_attacked[xside][R] | bit_attacked[xside][B]) & bit_pieces[side][4];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	bit_target = (bit_attacked[xside][Q] | bit_attacked[xside][B]) & bit_pieces[side][3];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	bit_target = (bit_attacked[xside][Q] | bit_attacked[xside][R]) & bit_pieces[side][2];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	bit_target = (bit_attacked[xside][Q] | bit_attacked[xside][R] | bit_attacked[xside][B]) & bit_pieces[side][1];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	return -1;
}

bool PawnCounterAttack(const int from, const int to, const BITBOARD bit_threshold)
{
	if (bit_pawncaptures[xside][to] & bit_threshold)
	{
		return true;
	}
	return false;
}//could add knight king attacks

bool CounterAttack(const int from, const int to, const int piece, const BITBOARD bit_threshold)
{
	BITBOARD b1 = bit_moves[piece][to] & bit_threshold;
	if (b1)
	{
		int counter_sq = NextBit(b1);
		if (piece != b[counter_sq] && !(bit_moves[piece][from] & mask[counter_sq]))
		{
			if (!(bit_between[to][counter_sq] & bit_all))
			{
				if (piece_value[piece] < piece_value[b[counter_sq]] ||
					!Attack(xside, counter_sq))
				{
					/*
					PrintBitBoard(bit_threshold);
					cout << " att ";
					Algebraic(to);
					cout << " counter ";
					Algebraic(counter_sq);
					cout << " ";
					Alg(from, to);
					z();
					//*/
					return true;
				}
			}
		}
	}
	return false;
}

BITBOARD GetThreshold(const int s, const int attacked)
{
	if (attacked == Q)
		return 0;
	if (attacked == R)
		return bit_pieces[s][Q];
	if (attacked == B || attacked == N)
		return bit_pieces[s][Q] | bit_pieces[s][R];
	if (attacked == P)
		return bit_pieces[s][Q] | bit_pieces[s][R] | bit_pieces[s][B] | bit_pieces[s][N];
	return 0;
}

void UpdatePV(move_data m)
{
	pv[ply][0] = m; // move_list[i];              // first move in PV
	pv_len[ply] = pv_len[ply + 1] + 1;      // length = 1 + child PV

	for (int j = 0; j < pv_len[ply + 1]; ++j)
		pv[ply][j + 1] = pv[ply + 1][j];    // copy child PV up

}

bool BestThreat(const int s, const int xs, const int diff)
{
	if (diff >= Q_VALUE)
		return false;
	int from, to, x;

	BITBOARD b1 = bit_pieces[s][P] & mask_ranks[s][6];

	while (b1)
	{
		from = NextBit(b1);
		b1 &= b1 - 1;
		to = pawnplus[s][from];
		if (b[to] == EMPTY && !Attack2(xs, to, bit_all & not_mask[from], not_mask[from]))
		{
			return true;
		}
	}

	BITBOARD bit_targets = bit_pieces[xs][Q];

	if (diff < R_VALUE)//Rooks are potentially a target.
	{
		bit_targets |= bit_pieces[xs][R];
		if (diff < B_VALUE)//Minor pieces are potentially a target.
		{
			bit_targets |= bit_pieces[xs][N] | bit_pieces[xs][B];
			if (diff < P_VALUE)//Pawns are potentially a target.
				bit_targets |= bit_pieces[xs][P];
		}
	}
	if (bit_targets == 0)
		return false;

	BITBOARD b2;
	if (s == 0)//Are there any pawn attacks?
	{
		b1 = bit_targets & (((bit_pieces[0][P] & not_h_file) << 9));
		b2 = bit_targets & (((bit_pieces[0][P] & not_a_file) << 7));
	}
	else
	{
		b1 = bit_targets & (((bit_pieces[1][P] & not_h_file) >> 7));
		b2 = bit_targets & (((bit_pieces[1][P] & not_a_file) >> 9));
	}

	while (b1)
	{
		to = NextBit(b1);
		b1 &= b1 - 1;
		if (b[to] > P || Attack(xs, to) == 0)
		{
			return true;
		}
	}

	while (b2)
	{
		to = NextBit(b2);
		b2 &= b2 - 1;
		if (b[to] > P || Attack(xs, to) == 0)
		{
			return true;
		}
	}

	for (x = 0; x < total[s][N]; x++)
	{
		from = pieces[s][N][x];
		b1 = bit_knightmoves[from] & bit_targets;
		while (b1)
		{
			to = NextBit(b1);
			b1 &= b1 - 1;
			if (b[to] > B || Attack(xs, to) == 0)
			{
				return true;
			}
			else if (SEE(s, from, to, 0, 0) > 0)
			{
				return true;
			}
		}
	}

	for (x = 0; x < total[s][B]; x++)
	{
		from = pieces[s][B][x];
		b1 = bit_bishopmoves[from] & bit_targets;
		while (b1)//Are there any bishop attacks?
		{
			to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (b[to] > B || Attack(xs, to) == 0)
				{
					return true;
				}
				else if (SEE(s, from, to, 0, 0) > 0)
				{
					return true;
				}
			}
		}
	}

	for (x = 0; x < total[s][R]; x++)
	{
		from = pieces[s][R][x];
		b1 = bit_rookmoves[from] & bit_targets;
		while (b1)//Are there any rook attacks?
		{
			to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (b[to] > R || Attack(xs, to) == 0)
				{
					return true;
				}
				else if (SEE(s, from, to, 0, 0) > 0)
				{
					return true;
				}
			}
		}
	}

	for (x = 0; x < total[s][Q]; x++)
	{
		from = pieces[s][Q][x];
		b1 = bit_queenmoves[from] & bit_targets;
		while (b1)//Are there any queen attacks?
		{
			to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (Attack(xs, to) == 0)
				{
					return true;
				}
			}
		}
	}

	from = kingloc[s];
	b1 = bit_kingmoves[from] & bit_targets;
	while (b1)
	{
		to = NextBit(b1);
		b1 &= b1 - 1;
		if (Attack(xs, to) == 0)
		{
			return true;
		}
	}
	return false;
}

bool IsThreat(const int s, const int xs, const int diff)
{
	int sq, i, x;

	BITBOARD b1, b2;

	b1 = bit_pieces[s][P] & mask_ranks[s][6];

	while (b1)
	{
		i = NextBit(b1);
		b1 &= b1 - 1;
		sq = pawnplus[s][i];
		if (b[sq] == EMPTY && Attack(xs, sq) == 0 &&
			!(mask_cols[i] & mask[pieces[xs][R][0]] &&
				!(bit_between[i][NextBit(mask[pieces[xs][R][0]])] & bit_all)))
		{
			return true;
		}
	}
	/*
	if(diff>300)
	{
	cout << " diff " << diff;
	cout << " s " << s;
	z();
	}
	*/

	BITBOARD bit_targets = 0;

	if (diff < 800)
		bit_targets |= bit_pieces[xs][Q];

	if (diff < 500)
		bit_targets |= bit_pieces[xs][R];

	if (diff < 300)
		bit_targets |= bit_pieces[xs][N] | bit_pieces[xs][B];

	if (bit_targets == 0)
		return false;

	if (s == 0)
	{
		b1 = bit_targets & (((bit_pieces[0][P] & not_h_file) << 9));
		b2 = bit_targets & (((bit_pieces[0][P] & not_a_file) << 7));
	}
	else
	{
		b1 = bit_targets & (((bit_pieces[1][P] & not_h_file) >> 7));
		b2 = bit_targets & (((bit_pieces[1][P] & not_a_file) >> 9));
	}

	while (b1)
	{
		sq = NextBit(b1);
		b1 &= b1 - 1;
		if (b[sq] > P || Attack(xs, sq) == 0)
		{
			return true;
		}
	}

	while (b2)
	{
		sq = NextBit(b2);
		b2 &= b2 - 1;
		if (b[sq] > P || Attack(xs, sq) == 0)
		{
			return true;
		}
	}

	for (x = 0; x < total[s][N]; x++)
	{
		i = pieces[s][N][x];
		b1 = bit_knightmoves[i] & bit_targets;
		while (b1)
		{
			sq = NextBit(b1);
			b1 &= b1 - 1;
			if (b[sq] > B || Attack(xs, sq) == 0)
			{
				return true;
			}
			else if (SEE(s, i, sq, 0, 0) > 0)
			{
				return true;
			}
		}
	}

	for (x = 0; x < total[s][B]; x++)
	{
		i = pieces[s][B][x];
		b1 = bit_bishopmoves[i] & bit_targets;
		while (b1)//Are there any bishop attacks?
		{
			sq = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[i][sq] & bit_all))
			{
				if (b[sq] > B || Attack(xs, sq) == 0)
				{
					return true;
				}
				else if (SEE(s, i, sq, 0, 0) > 0)
				{
					return true;
				}
			}
		}
	}

	for (x = 0; x < total[s][R]; x++)
	{
		i = pieces[s][R][x];
		b1 = bit_rookmoves[i] & bit_targets;
		while (b1)//Are there any rook attacks?
		{
			sq = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[i][sq] & bit_all))
			{
				if (b[sq] > R || Attack(xs, sq) == 0)
				{
					return true;
				}
				else if (SEE(s, i, sq, 0, 0) > 0)
				{
					return true;
				}
			}
		}
	}

	for (x = 0; x < total[s][Q]; x++)
	{
		i = pieces[s][Q][x];
		b1 = bit_queenmoves[i] & bit_targets;
		while (b1)//Are there any queen attacks?
		{
			sq = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[i][sq] & bit_all))
			{
				if (Attack(xs, sq) == 0)
				{
					return true;
				}
			}
		}
	}

	i = kingloc[s];
	b1 = bit_kingmoves[i] & bit_targets;
	while (b1)//Are there any king attacks?
	{
		sq = NextBit(b1);
		b1 &= b1 - 1;
		if (Attack(xs, sq) == 0)
		{
			return true;
		}
	}
	return false;
}

void SortPromotion(const int startmoves, const int endmoves)
{
	BITBOARD b2 = bit_pieces[xside][P] & mask_ranks[xside][6];
	int start = -1, dest = -1;
	int from, to;

	while (b2)
	{
		start = NextBit(b2);
		dest = pawnplus[xside][start];
		break;
	}
	if (dest > -1 && (b[dest] == 6))
	{
		if (!Attack(side, dest))
		{
			for (int i = startmoves; i < endmoves; i++)
			{
				from = move_list[i].from;
				to = move_list[i].to;
				if (b[from] > 0)
				{
					if (bit_moves[b[from]][to] & mask[dest])
					{
						if (!(bit_between[to][dest] & (bit_all & not_mask[start])))
						{
							move_list[i].score += ESCAPE_SCORE + 8;
						}
					}
				}
			}
		}
		else
		{
			for (int i = startmoves; i < endmoves; i++)
			{
				from = move_list[i].from;
				to = move_list[i].to;
				if (b[from] > 0 && to != dest && !(move_list[i].flags & CHECK))
				{
					if (bit_moves[b[from]][from] & mask[dest])
					{
						if (!(bit_moves[b[from]][to] & mask[dest]))
						{
							move_list[i].score = -8;
						}
					}
				}
			}
		}
	}
}

void SortEndgame(const int startmoves, const int endmoves)
{
	return;//
	if (piece_mat[side] < Q_VALUE)//endgames
	{
		int bonus, piece;
		for (int i = startmoves; i < endmoves; i++)
		{
			int from = move_list[i].from;
			int to = move_list[i].to;
			piece = b[from];
			bonus = 0;
			if (piece == K)
			{
				bonus = king_endgame_score[to] - king_endgame_score[from];
				if (bit_kingmoves[to] & bit_units[xside])// & bit_undefended[xside])
				{
					bonus += 25;
				}
				if (bit_kingmoves[to] & bit_units[side] & bit_undefended[side])
				{
					bonus += 25 + ATTACK_SCORE;
				}
			}
			else
			{
				if (piece == P)
				{
					if (mask[from] & passed_list[side])
					{
						move_list[i].score += passed[side][to] + ATTACK_SCORE;
					}
					continue;
				}
				if (bit_moves[piece][to] & bit_pieces[xside][P] & bit_undefended[xside])
				{
					int sq = NextBit(bit_pieces[xside][P]);
					if (!(bit_between[to][sq] & bit_all))
					{
						bonus += 25 + ATTACK_SCORE;
					}
				}
				bonus += PieceScore[side][piece][to] - PieceScore[side][piece][from];
			}
			move_list[i].score += bonus;
		}
	}
}

void SortQuiet(const int startmoves, const int endmoves, move_data ttmove, move_data countermove, move_data killer, move_data killer2)
{
	for (int i = startmoves; i < endmoves; i++)
	{
		int from = move_list[i].from;
		int to = move_list[i].to;

		if (from == ttmove.from && to == ttmove.to)
			continue;
		if (from == countermove.from && to == countermove.to)
		{
			if (move_list[i].score < COUNTER_SCORE)
				move_list[i].score += COUNTER_SCORE;
		}
		else if (from == killer.from && to == killer.to)
		{
			if (move_list[i].score < KILLER1_SCORE)
				move_list[i].score += KILLER1_SCORE;
		}
		else if (from == killer2.from && to == killer2.to)
		{
			if (move_list[i].score < KILLER2_SCORE)
				move_list[i].score += KILLER2_SCORE;
		}
		if (move_list[i].score >= 1000000)//patch
		{
			//Alg(from, to);
			//z();
			move_list[i].score = 900000;
		}
	}
}

BITBOARD GetTargets(const int s, const int xs)
{
	BITBOARD target = 0;
	BITBOARD b1, b2;
	BITBOARD br = 0, bm = 0, bp = 0;

	b1 = b2 = bit_pieces[s][Q] & bit_total_attacks[xs];
	while (b1)
	{
		int sq = NextBit(b1);

		if (mask[sq] & bit_undefended[s])
		{
			return mask[sq];
		}
		int att = GetLowestQuietAttacker(xs, sq);
		if (piece_value[att] < piece_value[Q])
		{
			return mask[sq];
		}
		else if (LightSEE(xs, s, att, sq))
		{
			target |= mask[sq];
		}
		else
		{
			int att_sq = GetAttackingSquare(xs, sq);
			if (SEE(xs, att_sq, sq, 0, 0) > 0)
			{
				target |= mask[sq];
			}
		}
		b1 &= b1 - 1;
	}

	br = bit_pieces[s][R] & bit_total_attacks[xs];
	bm = (bit_pieces[s][N] | bit_pieces[s][B]) & bit_total_attacks[xs];
	b1 = br;
	while (b1)
	{
		int sq = NextBit(b1);

		if (mask[sq] & bit_undefended[s])
		{
			target |= mask[sq];
		}
		int att = GetLowestQuietAttacker(xs, sq);
		if (att == P)
		{
			target |= mask[sq];
		}
		else if (LightSEE(xs, s, att, sq))
		{
			target |= mask[sq];
		}
		else
		{
			int att_sq = GetAttackingSquare(xs, sq);
			if (SEE(xs, att_sq, sq, 0, 0) > 0)
			{
				target |= mask[sq];
			}
		}
		b1 &= b1 - 1;
	}

	b1 = bm;
	while (b1)
	{
		int sq = NextBit(b1);

		if (mask[sq] & bit_undefended[s])
		{
			target |= mask[sq];
		}
		else
		{
			int att = GetLowestQuietAttacker(xs, sq);
			if (att == P)
			{
				target |= mask[sq];
			}
			else
			{
				if (LightSEE(xs, s, att, sq))
				{
					target |= mask[sq];
				}
				else
				{
					int att_sq = GetAttackingSquare(xs, sq);
					if (SEE(xs, att_sq, sq, 0, 0) > 0)
					{
						target |= mask[sq];
					}
				}
			}
		}
		b1 &= b1 - 1;
	}
	if (target)
		return target;

	b1 = bit_pieces[s][P] & bit_total_attacks[xs];
	while (b1)
	{
		int sq = NextBit(b1);

		if (mask[sq] & bit_undefended[s])
		{
			target |= mask[sq];
		}
		else
		{
			int att = GetLowestQuietAttacker(xs, sq);
			if (LightSEE(xs, s, att, sq))
			{
				target |= mask[sq];
			}
			else
			{
				int att_sq = GetAttackingSquare(xs, sq);
				if (SEE(xs, att_sq, sq, 0, 0) > 0)
				{
					target |= mask[sq];
				}
			}
		}
		b1 &= b1 - 1;
	}
	return target;
}

bool LightSEE(const int s, const int xs, const int att1, const int sq)
{
	const int piece = b[sq];

	int defender = GetLowestAttacker(s, sq);

	if (defender < 0) return true;

	// Net after first recapture
	int gain = piece_value[piece] - piece_value[att1];

	// Defender recaptures attacker
	gain -= piece_value[defender];

	// Remove defender temporarily and look for second attacker
	int att2 = GetNextAttackerSquare(xs, s, sq,
		bit_all & ~mask[GetAttackingSquare(s, sq)]);

	if (att2 > -1)
		gain += piece_value[defender] - piece_value[att2];

	//	printf(" light ");
	//	Algebraic(sq);
	//	z();

	return gain > 0;
}

bool ContactMate(const int s, const int xs, const int attack_sq)
{
	BITBOARD b1 = bit_kingmoves[kingloc[xs]] & ~bit_kingmoves[attack_sq] & ~bit_units[xs];
	if (attack_sq == H7 && b[F8] == 6)
	{
		//z();
	}
	while (b1)
	{
		int sq = NextBit(b1);
		if (!(Attack(s, sq)))
			return false;
		b1 &= b1 - 1;
	}
	return true;
}

///

int ContinuationScore(int hply, int piece, int to)
{
	int s = 0;

	if (hply > 0)
	{
		const int p0 = game_list[hply - 1].piece;
		const int t0 = game_list[hply - 1].to;
		if (p0 < 0 || t0 < 0 || piece < 0 || to < 0 || p0>5 || t0>63 || piece>5 || to>63)
		{
			/*
			printf(" info cont bug ");
			printf(" p %d ", p0);
			printf(" t0 %d ", t0);
			printf(" to %d ", to);
			printf(" piece %d ", piece);
			z();
			*/
			return 0;
		}
		if (game_list[hply - 1].from != 0 || game_list[hply - 1].to != 0)
			s += cont_hist[p0][t0][piece][to];
	}

	// optional second continuation (2 plies back):
	if (hply > 1)
	{
		const int p1 = game_list[hply - 2].piece;
		const int t1 = game_list[hply - 2].to;
		if (game_list[hply - 2].from != 0 || game_list[hply - 2].to != 0)
			s += cont2_hist[p1][t1][piece][to] >> 1; // half weight
	}

	return s;
}

// in your loop over generated quiet moves:
/*
if (is_quiet)
{
	int sc = QUIET_BASE;

	// Normal history (optional, keep it)
	// sc += HIST_SCALE * history[side][piece][to];

	// Continuation history replaces killers+counters:
	sc += CONT_SCALE * ContinuationScore(ply, piece, to);

	move_list[i].score = sc;
}
*/

static inline int Bonus(int depth)
{
	// common shape: depth^2-ish
	return depth * depth + 2 * depth;
}
/*
void UpdateContinuationOnCutoff(int ply, int depth, const move_data& m, int was_quiet)
{
	if (!was_quiet) return;

	const int b = Bonus(depth);

	const int mp = m.piece;
	const int mt = m.to;

	if (ply > 0)
	{
		const int p0 = game_list[ply - 1].piece;
		const int t0 = game_list[ply - 1].to;
		UpdateWithGravity(cont_hist[p0][t0][mp][mt], +b);
	}

	if (ply > 1)
	{
		const int p1 = game_list[ply - 2].piece;
		const int t1 = game_list[ply - 2].to;
		UpdateWithGravity(cont2_hist[p1][t1][mp][mt], +b);
	}
}
*/

void UpdateSequence(const int ply, const int depth, const move_data& m)
{
	const int bonus = Bonus(depth);

	const int to = m.to;
	int from = m.from;
	int piece = b[from];
	/*
	if (hply > 0)
	{
		printf("\n seq ");
		Alg(m.from, m.to);
	}
	*/

	if (hply > 0)
	{
		const int p0 = game_list[hply - 1].piece;
		const int t0 = game_list[hply - 1].to;
		//printf(" 1 ");
		//Alg(game_list[hply - 1].from, t0);
		UpdateWithGravity(cont_hist[p0][t0][piece][to], +bonus);
	}

	//printf(" ");
	if (hply > 1)
	{
		const int p1 = game_list[hply - 2].piece;
		const int t1 = game_list[hply - 2].to;
		//printf(" 2 ");
		//Alg(game_list[hply - 2].from, t1);
		if (p1 < 6)
			UpdateWithGravity(cont2_hist[p1][t1][piece][to], +bonus);
	}
	//z();
}

// Suppose you have stored tried quiet moves (or you can loop from startmoves..current index)
void PenaliseTriedQuiets(int ply, int depth, int start, int tried_end, const move_data& best)
{
	const int pen = Bonus(depth) / 2;

	for (int j = start; j < tried_end; ++j)
	{
		const move_data& m = move_list[j];

		if (m.from == best.from && m.to == best.to && m.flags == best.flags) continue;

		const int mp = m.piece, mt = m.to;

		if (ply > 0)
		{
			const int p0 = game_list[ply - 1].piece;
			const int t0 = game_list[ply - 1].to;
			UpdateWithGravity(cont_hist[p0][t0][mp][mt], -pen);
		}
		if (ply > 1)
		{
			const int p1 = game_list[ply - 2].piece;
			const int t1 = game_list[ply - 2].to;
			UpdateWithGravity(cont2_hist[p1][t1][mp][mt], -pen);
		}
	}
}

static inline int Clamp(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

// "Gravity" / decay update: adds bonus but pulls existing values back toward 0.
// This keeps scores bounded and responsive.
static inline void UpdateWithGravity(int& cell, int bonus)
{
	// tune these:
	// LIMIT: max magnitude in table
	// G: decay factor (bigger = slower decay)
	const int LIMIT = 16000;   // good starting point
	const int G = 32;      // 16..64 typical

	// gravity toward 0
	cell -= cell / G;

	// add new bonus
	cell += bonus;

	// clamp
	cell = Clamp(cell, -LIMIT, LIMIT);
}
