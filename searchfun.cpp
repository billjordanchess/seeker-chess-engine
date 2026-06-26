#include <chrono>

#include "globals.h"

std::chrono::steady_clock::time_point g_deadline;
bool g_use_deadline = false;

static inline bool time_up();

void PrintPV();
void DisplayPV2(int depth, int score_cp, unsigned long long nodes);

bool PawnCounterAttack(const int from, const int to, const BITBOARD bit_threshold);
bool CounterAttack(const int from, const int to, const int, const BITBOARD bit_threshold);
BITBOARD GetThreshold(const int s, const int attacked);
BITBOARD PinnersPossible(const int s, const int xs);

void UpdateContinuation(int depth, const int, const int);
int ContinuationScore(int ply, int piece, int to);
inline void UpdateWithGravity(int& cell, int bonus);

bool LightSEE(const int s, const int xs, const int att1, const int sq);
int GetLowestQuietAttacker(const int s, const int to);

void UpdateCheckHistory(const int from, const int to, const int x, const int depth);

int GetAttackingSquare(const int s, const int sq);

void AddAllMove(const int from, const int to);
void GenAllMoves(const int s, const int xs, BITBOARD pin_mask, const BITBOARD(&bit_check)[6], const int diff);
bool IsAnyMoves(const int s, const BITBOARD pin_mask);

extern move_data* g;

extern int move_count;

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
extern int cont2_hist[6][64][6][64];

void SortLastDepth(const int first, const int last)
{
	for (int i = first; i < last; i++)
	{
		int from = move_list[i].from;
		int to = move_list[i].to;
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

int Reduce(const int i)
{
	if (move_list[i].flags & (CAPTURE | CHECK | PROMOTE | PASSED7))
		return 0;
	if (game_list[hply - 1].flags & MATETHREAT)
	{
		return 0;
	}
	if (piece_mat[side] <= B_VALUE && b[move_list[i].to] == K)
	{
		return 0;
	}
	return 1;
}

void DisplayPV(int i)
{
	for (int x = 0; x < i; x++)
	{
		if (LookUp2(side) == false)
			break;
		printf(" ");
		Alg(hash_move.from, hash_move.to);
		MakeMove(hash_move.from, hash_move.to, hash_move.flags);
	}
	while (ply)
		UnMakeMove();
}

void DisplayPV3()
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

	buf[n] = 0;
	printf("%s", buf);
}

void PrintPV()
{
	if (pv_len[0] <= 0)
		return;

	for (int i = 0; i < pv_len[0]; ++i)
	{
		PrintUciMove(pv[0][i]);
		if (i + 1 < pv_len[0]) 
			printf(" ");
	}
}

void DisplayPV2(int depth, int score_cp, unsigned long long nodes)
{
	int time_ms = GetTime() - start_time;

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

void UpdateHistory(const int i, const int from, const int to, const int x, const int depth)
{
	if (hist_from[side][b[from]][from] + depth < HISTORY_LIMIT &&
		hist_to[side][b[from]][to] + depth < HISTORY_LIMIT)
	{
		hist_from[side][b[from]][from] += depth;
		hist_to[side][b[from]][to] += depth * depth;
		if (game_list[hply - 1].capture == EMPTY)
		{
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
	const int piece = b[from];
	if (piece == K)
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
	if (piece == R)
	{
		if (bit_rookmoves[from] & mask[to] &&
			(bit_between[from][to] & bit_all) == 0)
			return true;
		else
		{
			return false;
		}
	}
	if (piece == B)
	{
		if (bit_bishopmoves[from] & mask[to] &&
			(bit_between[from][to] & bit_all) == 0)
			return true;
		else
			return false;
	}
	if (piece == Q)
	{
		if (bit_queenmoves[from] & mask[to] &&
			(bit_between[from][to] & bit_all) == 0)
			return true;
		else
		{
			return false;
		}
	}
	if (piece == N)
	{
		if (bit_knightmoves[from] & mask[to])
			return true;
		else
		{
			return false;
		}
	}
	if (piece == P)
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
	return false;
}

int GetLineTarget()
{
	BITBOARD bit_target;
	int sq = -1;
	bit_target = (bit_attacked[xside][R] | bit_attacked[xside][B]) & bit_pieces[side][Q];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	bit_target = (bit_attacked[xside][Q] | bit_attacked[xside][B]) & bit_pieces[side][R];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	bit_target = (bit_attacked[xside][Q] | bit_attacked[xside][R]) & bit_pieces[side][B];
	if (bit_target)
	{
		return NextBit(bit_target);
	}
	bit_target = (bit_attacked[xside][Q] | bit_attacked[xside][R] | bit_attacked[xside][B]) & bit_pieces[side][N];
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
}

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
	pv[ply][0] = m;
	pv_len[ply] = pv_len[ply + 1] + 1;

	for (int j = 0; j < pv_len[ply + 1]; j++)
		pv[ply][j + 1] = pv[ply + 1][j];

}

bool IsThreat(const int s, const int xs, const int diff)
{
	if (diff >= Q_VALUE)
		return false;

	BITBOARD b1 = bit_pieces[s][P] & mask_ranks[s][6];

	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		int to = pawnplus[s][from];
		if (b[to] == EMPTY && !Attack2(xs, to, bit_all & not_mask[from], not_mask[from]))
		{
			return true;
		}
	}

	BITBOARD bit_targets = bit_pieces[xs][Q];

	if (diff < R_VALUE)
	{
		bit_targets |= bit_pieces[xs][R];
		if (diff < B_VALUE)
		{
			bit_targets |= bit_pieces[xs][N] | bit_pieces[xs][B];
			if (diff < P_VALUE)
				bit_targets |= bit_pieces[xs][P];
		}
	}
	if (bit_targets == 0)
		return false;

	BITBOARD b2;
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
		int to = NextBit(b1);
		b1 &= b1 - 1;
		if (b[to] > P || Attack(xs, to) == 0)
		{
			return true;
		}
	}

	while (b2)
	{
		int to = NextBit(b2);
		b2 &= b2 - 1;
		if (b[to] > P || Attack(xs, to) == 0)
		{
			return true;
		}
	}

	for (int x = 0; x < total[s][N]; x++)
	{
		int from = pieces[s][N][x];
		b1 = bit_knightmoves[from] & bit_targets;
		while (b1)
		{
			int to = NextBit(b1);
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

	for (int x = 0; x < total[s][B]; x++)
	{
		int from = pieces[s][B][x];
		b1 = bit_bishopmoves[from] & bit_targets;
		while (b1)
		{
			int to = NextBit(b1);
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

	for (int x = 0; x < total[s][R]; x++)
	{
		int from = pieces[s][R][x];
		b1 = bit_rookmoves[from] & bit_targets;
		while (b1)
		{
			int to = NextBit(b1);
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

	for (int x = 0; x < total[s][Q]; x++)
	{
		int from = pieces[s][Q][x];
		b1 = bit_queenmoves[from] & bit_targets;
		while (b1)
		{
			int to = NextBit(b1);
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

	b1 = bit_kingmoves[kingloc[s]] & bit_targets;
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		if (Attack(xs, to) == 0)
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
				int from = move_list[i].from;
				int to = move_list[i].to;
				int piece = b[from];
				if (piece > P)
				{
					if (bit_moves[piece][to] & mask[dest])
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
				int from = move_list[i].from;
				int to = move_list[i].to;
				int piece = b[from];
				if (piece > P && to != dest && !(move_list[i].flags & CHECK))
				{
					if (bit_moves[piece][from] & mask[dest])
					{
						if (!(bit_moves[piece][to] & mask[dest]))
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
	const BITBOARD undefended_pawns = bit_pieces[xside][P] & bit_undefended[xside];

	for (int i = startmoves; i < endmoves; i++)
	{
		int from = move_list[i].from;
		int to = move_list[i].to;
		int piece = b[from];
		int bonus = 0;

		if (piece == K)
		{
			bonus = king_endgame_score[to] - king_endgame_score[from];
			if (bit_kingmoves[to] & bit_units[xside])
			{
				bonus += 25;
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
			BITBOARD b1 = bit_moves[piece][to] & undefended_pawns;
			while (b1)
			{
				int sq = NextBit(b1);
				if (piece < B)
				{
					bonus += 15 + ATTACK_SCORE;
				}
				else if (!(bit_between[to][sq] & bit_all))
				{
					bonus += 25 + ATTACK_SCORE;
				}
				b1 &= b1 - 1;
			}
			bonus += PieceScore[side][piece][to] - PieceScore[side][piece][from];
		}
		move_list[i].score += bonus;
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
				move_list[i].score = COUNTER_SCORE;
		}
		else if (from == killer.from && to == killer.to)
		{
			if (move_list[i].score < KILLER1_SCORE)
				move_list[i].score = KILLER1_SCORE;
		}
		else if (from == killer2.from && to == killer2.to)
		{
			if (move_list[i].score < KILLER2_SCORE)
				move_list[i].score = KILLER2_SCORE;
		}
	}
}

BITBOARD GetTargets(const int s, const int xs)
{
	BITBOARD target = 0;
	BITBOARD b1;
	BITBOARD br = 0, bm = 0, bp = 0;

	b1 = bit_pieces[s][Q] & bit_total_attacks[xs];
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

	const int defender = GetLowestAttacker(s, sq);

	if (defender < 0) return true;

	int gain = piece_value[piece] - piece_value[att1];

	gain -= piece_value[defender];

	int att2 = GetNextAttackerSquare(xs, s, sq,
		bit_all & ~mask[GetAttackingSquare(s, sq)]);

	if (att2 > -1)
		gain += piece_value[defender] - piece_value[att2];

	return gain > 0;
}

int ContinuationScore(int hply, int piece, int to)
{
	int s = 0;

	if (hply > 0)
	{
		const int p0 = game_list[hply - 1].piece;
		const int t0 = game_list[hply - 1].to;
		if (game_list[hply - 1].from != 0 || game_list[hply - 1].to != 0)
			s += cont_hist[p0][t0][piece][to];
	}

	if (hply > 1)
	{
		const int p1 = game_list[hply - 2].piece;
		const int t1 = game_list[hply - 2].to;
		if (game_list[hply - 2].from != 0 || game_list[hply - 2].to != 0)
		{
			s += cont2_hist[p1][t1][piece][to] >> 1;
		}
	}

	return s;
}

static inline int Bonus(int depth)
{
	return depth * depth + 2 * depth;
}

void UpdateContinuation(int depth, const int from, const int to)
{
	const int bonus = Bonus(depth);

	const int mp = b[from];
	const int mt = to;

	if (ply > 0)
	{
		const int p1 = game_list[hply - 1].piece;
		const int t1 = game_list[hply - 1].to;
		UpdateWithGravity(cont_hist[p1][t1][mp][mt], bonus);
	}

	if (ply > 1)
	{
		const int p2 = game_list[hply - 2].piece;
		const int t2 = game_list[hply - 2].to;
		UpdateWithGravity(cont2_hist[p2][t2][mp][mt], bonus);
	}
}

static inline int Clamp(const int v, const int lo, const int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

static inline void UpdateWithGravity(int& cell, const int bonus)
{
	const int LIMIT = 16000;
	const int G = 32;

	cell -= cell / G;
	cell += bonus;
	cell = Clamp(cell, -LIMIT, LIMIT);
}

void GenAllMoves(const int s, const int xs, const BITBOARD pin_mask, const BITBOARD(&bit_check)[6], const int diff)
{
	first_move[ply + 1] = first_move[ply];
	move_count = first_move[ply];

	BITBOARD b1;

	if (s == 0)
	{
		b1 = bit_pieces[0][P] & not_rank6 & ~(bit_all >> 8) & ~bit_check[P];
	}
	else
	{
		b1 = bit_pieces[1][P] & not_rank1 & ~(bit_all << 8) & ~bit_check[P];
	}

	while (b1)
	{
		const int from = NextBit(b1);
		const int to = pawnplus[s][from];
		b1 &= b1 - 1;

		if (!(mask[from] & pin_mask) || col[from] == col[kingloc[s]])
		{
			if (PieceScore[side][P][to] - PieceScore[side][P][from] > diff)
				AddAllMove(from, to);
			const int to2 = pawndouble[s][from];
			if (row2[s][from] == 1 && b[to2] == EMPTY)
			{
				if (PieceScore[side][P][to] - PieceScore[side][P][from] > diff)
					AddAllMove(from, to2);
			}
		}
	}

	if (s == 0) {
		if (castle & 1 && !(bit_e1h1 & bit_all) && Attack(1u, F1) == 0)
			AddAllMove(E1, G1);
		if (castle & 2 && !(bit_e1a1 & bit_all) && Attack(1u, D1) == 0)
			AddAllMove(E1, C1);
	}
	else {
		if (castle & 4 && !(bit_e8h8 & bit_all) && Attack(0, F8) == 0)
			AddAllMove(E8, G8);
		if (castle & 8 && !(bit_e8a8 & bit_all) && Attack(0, D8) == 0)
			AddAllMove(E8, C8);
	}
	for (int x = 0; x < total[s][N]; x++)
	{
		const int from = pieces[s][N][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = bit_knightmoves[from] & ~bit_all & ~bit_check[N];
		while (b1)
		{
			const int to = NextBit(b1);
			b1 &= b1 - 1;
			if (PieceScore[side][N][to] - PieceScore[side][N][from] > diff)
				AddAllMove(from, to);
		}
	}

	for (int x = 0; x < total[s][B]; x++)
	{
		const int from = pieces[s][B][x];
		if (mask[from] & pin_mask)
			continue;
		b1 = MagicBishopAttacks(from, bit_all) & ~bit_all & ~bit_check[B];
		while (b1)
		{
			const int to = NextBit(b1);
			b1 &= b1 - 1;
			if (PieceScore[side][B][to] - PieceScore[side][B][from] > diff)
				AddAllMove(from, to);
		}
	}

	for (int x = 0; x < total[s][R]; x++)
	{
		const int from = pieces[s][R][x];
		if (mask[from] & pin_mask)
			continue;
		b1 = MagicRookAttacks(from, bit_all) & ~bit_all & ~bit_check[R];
		while (b1)
		{
			const int to = NextBit(b1);
			b1 &= b1 - 1;
			if (PieceScore[side][R][to] - PieceScore[side][R][from] > diff)
				AddAllMove(from, to);
		}
	}

	for (int x = 0; x < total[s][Q]; x++)
	{
		const int from = pieces[s][Q][x];
		if (mask[from] & pin_mask)
			continue;
		b1 = MagicQueenAttacks(from, bit_all) & ~bit_all & ~bit_check[Q];
		while (b1)
		{
			const int to = NextBit(b1);
			b1 &= b1 - 1;
			if (PieceScore[side][Q][to] - PieceScore[side][Q][from] > diff)
				AddAllMove(from, to);
		}
	}

	const int from = kingloc[s];
	b1 = bit_kingmoves[from] & ~bit_all;
	while (b1)
	{
		const int to = NextBit(b1);
		b1 &= b1 - 1;
		if (PieceScore[side][K][to] - PieceScore[side][K][from] > diff && !(Attack(xs,to)))
			AddAllMove(from, to);
	}
	first_move[ply + 1] = move_count;
	//if(first_move[ply + 1] > first_move[ply])
	//z();
}

void AddAllMove(const int from, const int to)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = 0;
	//Alg(from, to); printf("\n");
}

bool IsAnyMoves(const int s, const BITBOARD pin_mask)
{
	for (int x = 0; x < total[s][N]; x++)
	{
		const int from = pieces[s][N][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = bit_knightmoves[from] & ~bit_all;
		if (b1)
			return true;
	}
	for (int x = 0; x < total[s][B]; x++)
	{
		const int from = pieces[s][B][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = MagicBishopAttacks(from, bit_all) & ~bit_all;
		if (b1)
			return true;
	}
	for (int x = 0; x < total[s][R]; x++)
	{
		const int from = pieces[s][R][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = MagicRookAttacks(from, bit_all) & ~bit_all;
		if (b1)
			return true;
	}
	for (int x = 0; x < total[s][Q]; x++)
	{
		const int from = pieces[s][Q][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = MagicQueenAttacks(from, bit_all) & ~bit_all;
		if (b1)
			return true;
	}
	return false;
}
