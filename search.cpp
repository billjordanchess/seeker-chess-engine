//11/9/26
#include <stdlib.h>
#include <iostream>
#include <setjmp.h>
#include <chrono>

#include "globals.h"

using namespace std;

#define UC1 0

void SortBlunders(int left, int right);

//void GenLastQuietMoves(const int s, const int xs, BITBOARD pin_mask, const BITBOARD(&bit_check)[6]);

void AddCont(const int startmoves, const int endmoves);
int SafeKingMoves(const int s, const int xs);
bool IsAnyMoves(const int s, const BITBOARD pin_mask);
void MakeEvasion(const int from, const int to);
void UnMakeEvasion();
void MakeCheck(const int from, const int to, const int flags);
void UnMakeCheck();

void DisplayPV(int);

int RecaptureSearch(int s, const int attacker, const int, const int sq, BITBOARD p1, BITBOARD p2);

int GetLowestAttackerPins(const int s, const int sq, const BITBOARD pin_between);

U64 max_time;
U64 start_time;
U64 stop_time;

int fixedtime = 0;

BITBOARD bit_undefendable;
BITBOARD bit_defendable;
BITBOARD bit_unblock;
BITBOARD bit_line;

//ofstream file("debug.txt");

int target_bonus[64];

int slider[] = { 0,0,1,1,1,0 };

int block[64], unblock[64];

int check_history[6][64];

int cont_hist[6][64][6][64];
int cont2_hist[6][64][6][64];

static unsigned char prev_piece[MAX_PLY];
static unsigned char prev_to[MAX_PLY];
static unsigned char prev_valid[MAX_PLY];

int hits, misses;
int bestdiff = 0;
int worstdiff = 1000;
int bestcut = 0;

bool TimeUp();

void RemoveDiscoChecks(const int startmoves);
bool LightSEE(const int s, const int xside, const int att1, const int sq);

void UnMakeNull();

void SortPromotion(const int startmoves, const int endmoves);
void SortEndgame(const int startmoves, const int endmoves);

void SelectCheck(const int from, const int last);

void BuildAttackMap();
BITBOARD GetTargets(const int s, const int xside);

int GetAttackingSquare(const int s, const int sq);

int Search(int alpha, int beta, int depth, int pvflag, int nullflag);

int GetThreat(const int target);
BITBOARD PinnersPossible(const int s, const int xside);

void UpdateCheckHistory(const int from, const int to, const int x, const int depth);

BITBOARD GenChecks(const int, const int, BITBOARD);

BITBOARD GetKnightAttacks(const int s);
BITBOARD GetKingAttacks(const int s);

void UpdateContinuation(int depth, const int, const int);
int ContinuationScore(int ply, int piece, int to);

extern int test_mode;

U64 qnodes;
U64 all_nodes;

constexpr int ALPHA_THRESHOLD = 100;// 150;
constexpr int BETA_THRESHOLD = 100;// 150;// 150;

constexpr int  NO_PV = 0;
constexpr int  PV = 1;
constexpr int  CUT = 2;
constexpr int  ALL = 3;
constexpr int  NO_NULL = 0;
constexpr int  DO_NULL = 1;

constexpr int  RANK_7 = 6;

constexpr int  UNDEFENDED = 1;
constexpr int  WEAKER_ATTACK = 2;
constexpr int  SEE_ATTACK = 3;

constexpr int  INF = 10000;

constexpr int INVALID = 11111;
constexpr int START_SCORE = -20000;

const int MAX1 = MAX_PLY - 1;
const int MAX2 = MAX_PLY - 2;

const int LOW = -10000;
const int HIGH = 10000;

int startmat[2];
int currentdepth;
int currentmax;

int frontier[8] = { 0,0,8,20,10,20,0 };
int piece_value[6] = { 100, 300, 300, 500, 900, 0 };

int stats_depth[20];
int stats_count[100];
int total_depth[20];

move_data root_list[100];
move_data root_move;
static int rootscore = LOW;

int deep;

static int debug;

int c_nodes[MAX_PLY];

int reduce[MAX_PLY];//

#include <setjmp.h>
static jmp_buf env;
bool stop_search;
int root_score;
int real_side;

bool IsLegal(const int, const int);
bool IsCheck(const int p, const int sq, const int king);

void LoadBook();
int Book();

int SelectMove(const int from, const int, const int);

void SelectCapture(const int from, const int last);
void z();
void ClearHistory();
void SetTime(int);

void GenAllMoves(const int s, const int xs, BITBOARD pin_mask, const BITBOARD(&bit_check)[6], const int diff);
void GenQuietMoves(const int, BITBOARD, const BITBOARD(&bit_check)[6]);
void GenRoot(const int, const int);
int RootSearch(int depth, int alpha, int beta, const int);

void ShowMoves(int);

int Reduce(const int i);

void UpdateHistory(const int i, const int from, const int to, const int x, const int depth);

move_data GetHashMove();

void SortLastDepth(const int first, const int last);

void FreeAllHash();

int all, cut;

void z();

int mates;

int null_depth[64] = {
-2, -1, 0, 1, 1, 2, 2, 3,
3, 4, 4, 5, 5, 6, 6, 7,
7, 8, 8, 9, 9, 10, 10, 11,
11, 12, 12, 13, 13, 14, 14, 15,
15, 16, 16, 17, 17, 18, 18, 19,
19, 20, 20, 21, 21, 22, 22, 23,
23, 23, 23, 24, 23, 23, 23, 23,
23, 23, 23, 24, 23, 23, 23, 23
};

unsigned long total_tt_percent;
unsigned long total_tt_moves;
unsigned long total_not_percent;
unsigned long total_not_moves;
unsigned long total_percent;
unsigned long total_moves;

void ClearContHistory()
{
	memset(cont_hist, 0, sizeof(cont_hist));
	memset(cont2_hist, 0, sizeof(cont2_hist));
}

void ReduceHistory()
{
	for (int x = 0; x < 2; x++)
		for (int y = 0; y < 2; y++)
			for (int z = 0; z < 64; z++)
			{
				hist_from[x][y][z] >>= 1;
				hist_to[x][y][z] >>= 1;
			}
}

void ClearHistory()
{
	ply = 0;
	nodes = 0;
	qnodes = 0;
	cut_nodes = 0;
	all_nodes = 0;
	av_nodes = 0;
	first_nodes = 0;
	a_nodes = 0;
	b_nodes = 0;
	total_percent = 0;
	total_moves = 0;
	stop_search = false;
	debug = 0;

	startmat[0] = piece_mat[0];
	startmat[1] = piece_mat[1];

	memset(PlyMove, 0, sizeof(PlyMove));

	memset(c_nodes, 0, sizeof(c_nodes));
	memset(stats_depth, 0, sizeof(stats_depth));
	memset(stats_count, 0, sizeof(stats_count));

	memset(check_history, 0, sizeof(check_history));
	memset(reduce, 0, sizeof(reduce));

	memset(hist_from, 0, sizeof(hist_from));
	memset(hist_to, 0, sizeof(hist_to));
}

void SetTime(int fixed_time)
{
	fixedtime = fixed_time;
	start_time = GetTime();

	if (fixed_time == 0)
	{
		stop_time = start_time + 24 * 60 * 60 * 1000;
		return;
	}

	U64 move_time;
	int mat = (pawn_mat[0] + pawn_mat[1] + piece_mat[0] + piece_mat[1]) / 100;
	if (mat < 10)
		mat = 10;
	move_time = max_time / mat;
	move_time = move_time * 10;
	if (fixed_time == 1)
	{
		if (game_list[hply - 1].capture == Q_VALUE &&
			b[game_list[hply - 1].to] == Q)
		{
			move_time = move_time / 2;
		}
		else if (game_list[hply - 1].capture == R_VALUE &&
			b[game_list[hply - 1].to] == R &&
			Attack(side, game_list[hply - 1].to, bit_all) &&
			Attack(xside, game_list[hply - 1].to, bit_all) == 0)
		{
			move_time = move_time / 2;
		}
		else if (piece_value[game_list[hply - 1].capture] == B_VALUE &&
			piece_value[b[game_list[hply - 1].to]] == B_VALUE &&
			Attack(side, game_list[hply - 1].to, bit_all) &&
			Attack(xside, game_list[hply - 1].to, bit_all) == 0)
		{
			move_time = move_time / 2;
		}
		else if (Attack(xside, kingloc[side], bit_all))
		{
			move_time = move_time / 2;
		}
	}
	if (max_time < 200)
		move_time = 20;          // 0.2 sec

	if (max_time < 5)
		move_time = 5;           // 0.05 sec

	if (move_time > max_time / 4)
		move_time = max_time / 4;
	stop_time = start_time + move_time;
}

move_data Think(int fixed_time, int max_depth)
{
	hits = 0;
	misses = 0;
	mates = 0;
	stop_search = false;

	int bookflag = 0;

	if (hply < 8)
	{
		if (hply == 0)
			LoadBook();
		bookflag = Book();
		if (bookflag > 0)
		{
			return hash_move;
		}
	}

	ClearHistory();
	SetTime(fixed_time);

	int score = 0;
	int prevScore = 0;

	move_data best_stable;
	best_stable.from = 0;
	best_stable.to = 0;
	best_stable.flags = 0;
	rootscore = LOW;

	printf("ply      nodes  score  pv\n"); fflush(stdout);

	for (int depth = 1; depth <= max_depth; ++depth)
	{
		if (fixed_depth == 0 && max_depth > 1)
			if (fixed_time == 1)
			{
				if (GetTime() >= start_time + max_time)
				{
					stop_search = true;
					return best_stable;
				}
			}
			else if (GetTime() >= start_time + max_time / 4)
			{
				stop_search = true;
				return best_stable;
			}
		if (GetTime() >= stop_time)
			break;

		int alpha = LOW;
		int beta = HIGH;
		currentmax = depth;

		if (depth > 1)
		{
			int delta = 50;
			int tries = 0;
			alpha = prevScore - delta;
			beta = prevScore + delta;

			if (depth > 7)
			{
				ReduceHistory();
			}

			while (true)
			{
				tries++;
				if (tries >= 3)
					break;
				ply = 0;
				for (int i = 0; i < first_move[1]; i++)
					move_list[i] = root_list[i];

				score = RootSearch(depth, alpha, beta, prevScore);

				std::cout << depth << " " << score << " " << (GetTime() - start_time) / 10 << " " << nodes;
				DisplayPV(depth);
				std::cout << std::endl;

				if (stop_search)
					break;

				if (score > alpha && score < beta)
					break;

				if (score < -9900 || score > 9900)
					break;

				if (score <= alpha)
				{
					if (tries == 1)
					{
						alpha -= delta;
						delta <<= 1;
					}
					if (tries >= 2)
					{
						alpha = LOW; beta = HIGH;
					}
					continue;
				}

				if (score >= beta)
				{
					if (tries == 1)
					{
						beta += delta;
						delta <<= 1;
					}
					if (tries >= 2)
					{
						alpha = LOW; beta = HIGH;
					}
					continue;
				}
				break;
			}
		}
		else
		{
			ply = 0;
			GenRoot(side, xside);
			score = RootSearch(depth, alpha, beta, 0);
			std::cout << depth << " " << score << " " << (GetTime() - start_time) / 10 << " " << nodes;
			DisplayPV(depth);
			std::cout << std::endl;
		}

		if (stop_search)
			break;

		prevScore = score;
		best_stable = root_move;

		if (score > 9000 || score < -9000)
			break;
	}

	return best_stable;
}

void GenRoot(const int s, const int xside)
{
	BITBOARD bit_disco_pieces;
	BITBOARD pin_mask = GetPinMask(s, xside);
	BITBOARD bit_check_root[6];
	memset(bit_check_root, 0, sizeof(bit_check_root));

	SetSliderMoves();

	int check = Check(xside, kingloc[s]);
	if (check > -1)
	{
		EvadeCapture(s, xside, check, pin_mask);
		EvadeQuiet(s, xside, check, pin_mask);
	}
	else
	{
		GenCaptures(s, xside, pin_mask);
		bit_disco_pieces = GenChecks(s, xside, pin_mask);
		BuildAttackMap();
		GenQuietMoves(s, xside, pin_mask, bit_check_root);
	}
	for (int i = 0; i < first_move[1]; i++)
	{
		root_list[i] = move_list[i];
	}
}

int RootSearch(int depth, int alpha, int beta, const int prevScore)
{
	int bestScore = LOW;

	move_data best;
	best.from = -1;
	best.to = -1;
	best.flags = 0;

	int start = first_move[0];
	int end = first_move[1];
	int top = HASH_SCORE;
	int count = 0;
	int score = 0;

	ply = 0;
	int d = depth - 1;

	for (int i = start; i < end; ++i)
	{
		top = SelectMove(i, top, end);
		int from = move_list[i].from;
		int to = move_list[i].to;
		unsigned int flags = move_list[i].flags;

		MakeMove(from, to, flags);

		if (count == 0)
		{
			score = -Search(-beta, -alpha, d, PV, DO_NULL);
		}
		else
		{
			score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
			if (score > alpha)
			{
				score = -Search(-beta, -alpha, d, PV, DO_NULL);
			}
		}

		UnMakeMove();

		count++;

		if (fixedtime == 1 && TimeUp())
		{
			if (best.from != -1)
				root_move = best;
			return rootscore;
		}

		root_list[i] = move_list[i];
		root_list[i].score = score;

		if (score > bestScore)
		{
			bestScore = score;
			best = move_list[i];
			root_move = best;

			if (score > alpha)
			{
				alpha = score;
			}
		}
	}
	if (!stop_search && best.from != -1)
	{
		root_move = best;
		rootscore = bestScore;
		AddHash(side, depth, bestScore, 0, best.from, best.to, best.flags);
	}
	return bestScore;
}

bool TimeUp()
{
	if (stop_search)
		return true;
	if ((nodes & 2047) == 0)
	{
		if (GetTime() >= stop_time)
		{
			fflush(stdout);
			stop_search = true;
		}
	}
	return stop_search;
}

int Search(int alpha, int beta, int depth, int pvs, int null)
{
	if (fixed_depth == 0 && TimeUp())
		return alpha;
	nodes++;

	if (hply >= 4 && Reps2())
	{
		return 0;
	}
	if (fifty >= 10 && piece_mat[0] <= 300 && piece_mat[1] <= 300)
	{
		//printf("+");
		//return 0;
	}
	if (fifty >= 100)
	{
		return 0;
	}
	if (pawn_mat[0] == 0 && pawn_mat[1] == 0 &&
		game_list[hply - 1].capture != EMPTY)
	{
		if (endmatrix[piece_mat[side]][total[side][N]][piece_mat[xside]][total[xside][N]] == DRAWN)
		{
			if (alpha >= 0)
			{
				return alpha;
			}
			return 0;
		}
	}
	if (piece_mat[xside] == 0 && pawn_mat[xside] == 0)
	{
		if (piece_mat[xside] < startmat[xside] || piece_mat[side] > startmat[side])
		{
			if (bit_pieces[side][R] | bit_pieces[side][Q])
				return WON_ENDGAME - ply;
		}
		if (piece_mat[side] + pawn_mat[side] == 100)
		{
			int king = kingloc[xside];
			if (b[pawnplus[xside][king]] == P ||
				b[pawndouble[xside][king]] == P)
				if (row2[xside][king] != 0 || bit_pieces[side][P] & mask_rookfiles)
				{
					return 0;
				}
		}
	}

	if (depth < 1)
	{
		return QuietSearch(alpha, beta);
	}
	int pvflag = 0;

	int lookup = LookUp(side, depth, alpha, beta);
	move_data ttmove;
	ttmove.from = 0;
	ttmove.to = 0;

	if (lookup > -1)
	{
		ttmove = GetHashMove();
		if ((mask[ttmove.from] & bit_units[side]) == 0 ||
			(mask[ttmove.to] & bit_units[side]) ||
			IsLegal(ttmove.from, ttmove.to) == 0)
		{
			lookup = -1;
		}
	}
	if (lookup > -1)
	{
		if (lookup == BETA)
			return beta;
		if (lookup == ALPHA)
			return alpha;
		if (lookup == EXACT)
		{
			if (hash_move.score >= beta)
			{
				return beta;
			}
			if (hash_move.score > alpha)
			{
				alpha = hash_move.score;
				pvflag = 1;
				PlyMove[ply] = PV;
			}
		}
	}

	if (lookup > -1)
	{
		hits++;
	}
	else misses++;

	currentdepth = depth;
	int score;
	int check = Check(xside, kingloc[side]);
	int ev1 = INVALID;

	if (depth > 2
		&& null
		&& !pvs
		&& piece_mat[side] > Q_VALUE
		&& check == -1
		)
	{
		ev1 = Eval(alpha, beta);
		if (ev1 >= beta - 50)
		{
			first_move[ply + 1] = first_move[ply];
			game_list[hply].from = 0;
			game_list[hply].to = 0;
			game_list[hply].flags = 0;
			game_list[hply].piece = 0;

			int old_side = side;
			int old_xside = xside;
			side ^= 1;
			xside ^= 1;
			ply++; hply++;
			int old_castle = castle;
			int old_fifty = fifty;
			BITBOARD old_currentkey = currentkey;

			score = -Search(-beta, -beta + 1, null_depth[depth], NO_PV, NO_NULL);

			side = old_side;
			xside = old_xside;
			castle = old_castle;
			fifty = old_fifty;
			currentkey = old_currentkey;

			ply--; hply--;

			if (stop_search)
				return alpha;

			if (score >= beta)
			{
				return beta;
			}
		}
	}

	int count = 0;
	const int initial_alpha = alpha;
	int bestscore = START_SCORE;
	int from, to, flags;
	int first = first_move[ply];
	int d;
	int move_score;
	int piece;

	move_data bestmove;

	first_move[ply + 1] = first_move[ply];

	if (lookup > -1)
	{
		from = ttmove.from;
		to = ttmove.to;
		flags = ttmove.flags;
		ttmove.score = HASH_SCORE;

		move_list[first].from = from;
		move_list[first].to = to;
		move_list[first].flags = flags;
		move_list[first].score = HASH_SCORE;
		first_move[ply + 1] = first + 1;

		MakeMove(from, to, flags);
		count++;

		if (flags & INCHECK && b[to] == K && piece_mat[side] > 1200)//&& !(flags & CAPTURE)
		{
			d = depth;
		}
		else
			d = depth - 1;

		score = -Search(-beta, -alpha, d, pvs, DO_NULL);

		UnMakeMove();

		if (stop_search)
			return alpha;

		if (score > alpha)
		{
			if (score >= beta)
			{
				AddHash(side, depth, score, BETA, from, to, flags);
				if (ply > 0) PlyMove[ply - 1] = CUT;
				cut_tt_nodes++;
				first_tt_nodes++;
				av_nodes++;
				return beta;
			}
			alpha = score;
		}

		bestscore = score;
		bestmove = ttmove;
	}

	int r = 0;
	int ev = -10000;
	int top = HASH_SCORE;

	BITBOARD pins[2];

	pins[0] = PinnersPossible(0, 1);
	pins[1] = PinnersPossible(1, 0);

	BITBOARD pin_mask = GetPinMask(side, xside);

	if (check > -1)
	{
		EvadeCapture(side, xside, check, pin_mask);
		game_list[hply].flags |= INCHECK;

		for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
		{
			top = SelectMove(i, top, first_move[ply + 1]);

			from = move_list[i].from;
			to = move_list[i].to;

			if (from == ttmove.from && to == ttmove.to)
				continue;

			flags = move_list[i].flags;

			MakeCapture(from, to, flags);

			d = depth - 1;

			if (count == 0)
				score = -Search(-beta, -alpha, d, pvs, DO_NULL);
			else
			{
				score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
				if (score > alpha && score < beta)
				{
					score = -Search(-beta, -alpha, d, PV, DO_NULL);
				}
			}

			UnMakeCapture();

			if (stop_search)
				return alpha;

			count++;

			if (score > bestscore)
			{
				bestscore = score;
				bestmove = move_list[i];
			}
			if (score > alpha)
			{
				if (score >= beta)
				{
					AddHash(side, depth, score, BETA, from, to, flags);
					PlyMove[ply - 1] = CUT;
					if (lookup > -1)
						cut_tt_nodes++;
					else
						cut_nodes++;
					if (count == 1)
					{
						if (lookup > -1)
							first_tt_nodes++;
						else
							first_nodes++;
					}
					av_nodes += count;
					return beta;
				}
				alpha = score;
			}
		}

		int start = first_move[ply + 1];
		EvadeQuiet(side, xside, check, pin_mask);
		int end = first_move[ply + 1];

		ev1 = Eval(alpha, beta);
		int diff = alpha - ev1;

		if (depth == 1 && !(bit_pieces[side][P] & mask_ranks[side][6]))
		{
			if (diff >= ALPHA_THRESHOLD)
			{
				//if (count)
				//return alpha;
				if (end > first_move[ply])
					//z();
					//printf("+");
					return alpha;
			}
		}
		//10 148 26 133269 c8c3 g3h4 d8d2 e2d2 b7e4 d2g2 e4g2 h1g2 c3c2 g2f3
		//10 148 24 132642 c8c3 g3h4 d8d2 e2d2 b7e4 d2g2 e4g2 h1g2 c3c2 g2f3

		for (int i = start; i < end; i++)
		{
			top = SelectMove(i, top, end);

			from = move_list[i].from;
			to = move_list[i].to;

			if (from == ttmove.from && to == ttmove.to)
				continue;

			flags = move_list[i].flags;
			piece = b[from];

			MakeEvasion(from, to);

			if (count > 0 && !pvs)
			{
				if (depth <= 2)
				{
					if (ev1 == INVALID)
						ev1 = Eval(alpha, beta);
				}
				if (depth <= 1)
				{
					ev = ev1 + PieceScore[side][piece][to] - PieceScore[side][piece][from];
					if (ev + frontier[piece] <= alpha)
					{
						UnMakeEvasion();
						if (alpha - ev > 100)
						{
							break;
						}
						continue;
					}
				}
				if (depth == 2 && ply > 1)
				{
					ev = ev1 + PieceScore[xside][piece][to] - PieceScore[xside][piece][from];
					if (ev <= alpha && IsThreat(xside, side, alpha - ev) == 0)
					{
						UnMakeEvasion();
						continue;
					}
				}
			}

			if (b[to] == K && piece_mat[side] > 1200)
				d = depth;
			else
				d = depth - 1;

			if (count == 0)
				score = -Search(-beta, -alpha, d, pvs, DO_NULL);
			else
			{
				score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
				if (score > alpha && score < beta)
				{
					score = -Search(-beta, -alpha, d, PV, DO_NULL);
				}
			}

			UnMakeEvasion();

			if (stop_search)
				return alpha;

			count++;

			if (score > bestscore)
			{
				bestscore = score;
				bestmove = move_list[i];
			}
			if (score > alpha)
			{
				if (score >= beta)
				{
					AddHash(side, depth, score, BETA, from, to, flags);
					PlyMove[ply - 1] = CUT;
					if (lookup > -1)
						cut_tt_nodes++;
					else
						cut_nodes++;
					if (count == 1)
					{
						if (lookup > -1)
							first_tt_nodes++;
						else
							first_nodes++;
					}
					av_nodes += count;
					return beta;
				}
				alpha = score;
			}
		}
		if (!count)
		{
			if (!(game_list[hply - 2].flags & INCHECK) && ply > 2)
				mates++;
			PlyMove[ply - 1] = CUT;
			return -10000 + ply;
		}
		PlyMove[ply] = CUT;
		if (fifty >= 100)
			return 0;

		if (alpha > initial_alpha)
		{
			AddHash(side, depth, alpha, EXACT, bestmove.from, bestmove.to, bestmove.flags);
		}
		else
		{
			if (depth > 3)
				AddHash(side, depth, alpha, ALPHA, bestmove.from, bestmove.to, bestmove.flags);
		}
		return alpha;
	}
	//end in check

	pin_mask = GetPinMask(side, xside);

	SetSliderMoves();

	GenCaptures(side, xside, pin_mask);

	int val_to, val_from;
	int lowest;

	const int startcaptures = first_move[ply];
	const int endcaptures = first_move[ply + 1];

	int capture_score = 0;
	for (int i = startcaptures; i < endcaptures; i++)
	{
		if (b[move_list[i].to] != EMPTY)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			if (from == ttmove.from && to == ttmove.to)
				continue;

			piece = b[from];

			if (pins[xside] == 0)
			{
				lowest = GetLowestAttacker(xside, to);
			}
			else
			{
				lowest = GetLowestAttackerPins(xside, to, pin_mask);
			}

			val_to = piece_value[b[to]];
			capture_score = val_to;
			//12 36 46 928794 d3h7 g8h7 e2h5 h7g8 b2g7 g8g7 h5g4 g7h6 f1f3 e7h4 g4h4 h6g6
			//12 86 47 945387 d3h7 g8h7 e2h5 h7g8 b2g7 g8g7 h5g4 g7h6 f1f3 e7h4 g4h4 h6g6
			//12 36 44 921753 d3h7 g8h7 e2h5 h7g8 b2g7 g8g7 h5g4 g7h6 f1f3 e7h4 g4h4 h6g6
			//	12 86 45 938300 d3h7 g8h7 e2h5 h7g8 b2g7 g8g7 h5g4 g7h6 f1f3 e7h4 g4h4 h6g6
			//12 36 40 896984 d3h7 g8h7 e2h5 h7g8 b2g7 g8g7 h5g4 g7h6 f1f3 e7h4 g4h4 h6g6
			//12 86 41 913499 d3h7 g8h7 e2h5 h7g8 b2g7 g8g7 h5g4 g7h6 f1f3 e7h4 g4h4 h6g6
		
			if (lowest > -1)
			{
				val_from = piece_value[piece];
				if (val_from < val_to)
				{
					capture_score -= val_from;
				}
				else if (lowest == K)
				{
					if (Attack2(side, to, bit_all & ~mask[from], ~mask[from]) == 0)
					{
						capture_score = val_to - val_from;
					}
					else
					{
						capture_score = val_to;
					}
				}
				else if (val_from > val_to + piece_value[lowest])
				{
					capture_score = (val_to + piece_value[lowest]) - val_from;
				}
				else if (piece == P && b[to] == P)
				{
					capture_score = 0;
				}
				else
				{
					int defender_sq = GetAttackingSquare(xside, to);
					if (defender_sq > -1)
					{
						capture_score = RecaptureSearch(side, from, to, defender_sq, pins[0], pins[1]);
					}
					if (capture_score == 0)
						capture_score = val_to - val_from;
				}
			}
			if (capture_score >= 0)
			{
				if (move_list[i].flags & CHECK)
					capture_score += 10 + b[from];
				else if (hply > 1 && game_list[hply - 1].to == to)
				{
					capture_score++;
				}
			}
			if (piece == b[to])
			{
				if (!Attack(side, from, bit_all))
				{
					capture_score += 10 + b[from];
				}
			}
			move_list[i].score = capture_score;
		}
	}

	for (int i = startcaptures; i < endcaptures; i++)
	{
		top = SelectMove(i, top, endcaptures);

		from = move_list[i].from;
		to = move_list[i].to;

		if (from == ttmove.from && to == ttmove.to)
			continue;

		flags = move_list[i].flags;
		piece = b[from];
		int captured = b[to];

		if (count > 0 && !pvs)
		{
			if (depth <= 2)
			{
				if (ev1 == INVALID)
					ev1 = Eval(alpha, beta);
			}
			if (depth <= 1 && !(flags & CHECK))
			{
				ev = ev1 + PieceScore[side][piece][to] - PieceScore[side][piece][from];
				ev += piece_value[captured] + PieceScore[xside][captured][to] + frontier[captured];
				if (ev + frontier[piece] <= alpha)
				{
					continue;
				}
			}
		}

		MakeCapture(from, to, flags);

		if (count > 0 && depth == 2 && ply > 1 && !(flags & CHECK))
		{
			ev = ev1 + PieceScore[xside][piece][to] - PieceScore[xside][piece][from];
			ev += piece_value[game_list[hply - 1].capture] + PieceScore[side][game_list[hply - 1].capture][to] + frontier[piece];
			if (ev <= alpha && IsThreat(xside, side, alpha - ev) == 0)
			{
				UnMakeCapture();
				continue;
			}
		}

		d = depth - 1;

		if (count == 0)
			score = -Search(-beta, -alpha, d, pvs, DO_NULL);
		else
		{
			score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
			if (score > alpha && score < beta)
			{
				score = -Search(-beta, -alpha, d, PV, DO_NULL);
			}
		}

		UnMakeCapture();

		if (stop_search)
			return alpha;

		count++;

		if (score > bestscore)
		{
			bestscore = score;
			bestmove = move_list[i];
		}
		if (score > alpha)
		{
			if (score >= beta)
			{
				AddHash(side, depth, score, BETA, from, to, flags);
				PlyMove[ply - 1] = CUT;
				if (lookup > -1)
					cut_tt_nodes++;
				else
					cut_nodes++;
				if (count == 1)
				{
					if (lookup > -1)
						first_tt_nodes++;
					else
						first_nodes++;
				}
				av_nodes += count;
				return beta;
			}
			alpha = score;
		}
	}
	//end captures

	const int startchecks = first_move[ply + 1];

	BITBOARD bit_disco_pieces = GenChecks(side, xside, pin_mask);

	const int endchecks = first_move[ply + 1];

	BITBOARD bit_check_squares[6];
	memset(bit_check_squares, 0, sizeof(bit_check_squares));
	int k1 = kingloc[xside];

	d = depth - 1;

	for (int i = startchecks; i < endchecks; i++)
	{
		from = move_list[i].from;
		to = move_list[i].to;
		piece = b[from];
		flags = move_list[i].flags;

		if (!(flags & DISCO))
		{
			bit_check_squares[piece] |= mask[to];
		}
		else
		{
			if (bit_moves[piece][to] & mask[k1] && !(bit_between[to][k1] & bit_all))
				bit_check_squares[piece] |= mask[to];
		}
		if (endchecks > startchecks + 1)
		{
			int low = GetLowestAttacker(xside, to);
			if (low > -1)
			{
				int val = piece_value[piece];
				if (piece_value[low] < val)
					move_list[i].score = -val;
				else if (piece_value[low] == val)
					move_list[i].score = -val + 1;
				else
				{
					if (Attack2(side, to, bit_all & ~mask[from], ~mask[from]) == 0)
					{
						move_list[i].score = -val;
					}
				}
			}
		}
	}

	//ShowAll(ply);

	for (int i = startchecks; i < endchecks; i++)
	{
		SelectCheck(i, endchecks);

		from = move_list[i].from;
		to = move_list[i].to;

		if (from == ttmove.from && to == ttmove.to)
			continue;

		flags = move_list[i].flags;

		MakeCheck(from, to, flags);

		if (count == 0)
			score = -Search(-beta, -alpha, d, pvs, DO_NULL);
		else
		{
			score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
			if (score > alpha && score < beta)
			{
				score = -Search(-beta, -alpha, d, PV, DO_NULL);
			}
		}
		UnMakeCheck();

		if (stop_search)
			return alpha;

		count++;

		if (score > bestscore)
		{
			bestscore = score;
			bestmove = move_list[i];
		}
		if (score > alpha)
		{
			if (score >= beta)
			{
				UpdateCheckHistory(from, to, score, depth);
				AddHash(side, depth, score, BETA, from, to, flags);
				PlyMove[ply] = CUT;
				if (lookup > -1)
					cut_tt_nodes++;
				else
					cut_nodes++;
				if (count == 1)
				{
					if (lookup > -1)
						first_tt_nodes++;
					else
						first_nodes++;
				}
				av_nodes += count;
				return beta;
			}
			alpha = score;
		}
	}
	//end checks

	ev1 = Eval(alpha, beta);
	int diff = alpha - ev1;
	if (depth == 1 && !(bit_pieces[side][P] & mask_ranks[side][6]))
	{
		if (diff >= ALPHA_THRESHOLD)
		{
			if (count || SafeKingMoves(side, xside) || IsAnyMoves(side, pin_mask))
				return alpha;
		}
		int diff2 = beta - ev1;
		if (diff2 >= BETA_THRESHOLD)
		{
			int startmoves = first_move[ply];
			GenAllMoves(side, xside, pin_mask, bit_check_squares, diff);
			int endmoves = first_move[ply + 1];

			for (int i = startmoves; i < endmoves; i++)
			{
				int from = move_list[i].from;
				int to = move_list[i].to;

				unsigned int flags = move_list[i].flags;

				MakeQuietMove(from, to, flags);

				score = -Search(-alpha - 1, -alpha, 0, NO_PV, NO_NULL);

				UnMakeQuietMove();

				count++;

				if (score > bestscore)
				{
					bestscore = score;
					bestmove = move_list[i];
				}
				if (score > alpha)
				{
					alpha = score;
				}
			}
			all_nodes++;
			PlyMove[ply] = ALL;

			if (!count)
			{
				return 0;
			}

			if (fifty >= 100)
				return 0;

			if (alpha > initial_alpha)
			{
				AddHash(side, depth, alpha, EXACT, bestmove.from, bestmove.to, bestmove.flags);
			}
			return alpha;
		}
	}
	if (depth == 2 && diff >= 650 && !(bit_pieces[side][P] & mask_ranks[side][6])
		&& piece_mat[side] > 1200)
	{
		if (count || SafeKingMoves(side, xside) || IsAnyMoves(side, pin_mask))
			return alpha;
	}
	//end all moves

	BuildAttackMap();

	BITBOARD bit_targets = GetTargets(side, xside);

	bit_undefendable = (bit_units[side] & ~bit_pieces[side][P]) & bit_attacked[xside][P];
	bit_undefendable |= (bit_pieces[side][R] | bit_pieces[side][Q]) & (bit_attacked[xside][N] | bit_attacked[xside][B]);
	bit_undefendable |= bit_pieces[side][Q] & bit_attacked[xside][R];

	bool capture_check = 0;
	int target_score = 0;
	bit_unblock = 0;
	BITBOARD b1 = bit_targets & ~bit_undefendable;
	bit_defendable = b1;

	while (b1)
	{
		int square = NextBit(b1);
		BITBOARD b2 =
			(bit_moves[B][square] & (bit_pieces[side][B] | bit_pieces[side][Q])) |
			(bit_moves[R][square] & (bit_pieces[side][R] | bit_pieces[side][Q]));

		while (b2)
		{
			int square2 = NextBit(b2);
			b2 &= b2 - 1;
			BITBOARD bit_ray = bit_between[square2][square];
			if (!(bit_ray & bit_units[xside]))
			{
				BITBOARD blockers = bit_ray & bit_units[side];
				if (blockers && !(blockers & (blockers - 1)))
				{
					bit_unblock |= bit_ray;
					unblock[square2] = square;
				}
			}
		}
		b1 &= b1 - 1;
	}
	bit_line = 0;

	b1 = (bit_attacked[xside][B] | bit_attacked[xside][R] | bit_attacked[xside][Q]) & bit_targets;
	while (b1)
	{
		int square = NextBit(b1);
		b1 &= b1 - 1;
		BITBOARD b2 = bit_moves[B][square] & (bit_pieces[xside][B] | bit_pieces[xside][Q]);
		b2 |= bit_moves[R][square] & (bit_pieces[xside][R] | bit_pieces[xside][Q]);
		while (b2)
		{
			int attacker = NextBit(b2);
			b2 &= b2 - 1;
			if ((bit_between[square][attacker] & bit_all) == 0)
			{
				bit_line |= bit_between[square][attacker];
				block[attacker] = square;
			}
		}
	}

	const int startmoves = first_move[ply + 1];
	int quiet_count = 0;
	//if(depth==1)
	//	GenLastQuietMoves(side, xside, pin_mask, bit_check_squares);
	//else
	GenQuietMoves(side, xside, pin_mask, bit_check_squares);

	if (bit_disco_pieces)
	{
		RemoveDiscoChecks(startmoves);
	}

	int endmoves = first_move[ply + 1];

	SortPromotion(startmoves, endmoves);

	if (depth > 1)
	{
		if (piece_mat[side] < Q_VALUE)
		{
			SortEndgame(startmoves, endmoves);
		}
	}
	if (bit_targets == 0)
	{
		AddCont(startmoves, endmoves);
	}

	int blunder_start = 0;
	//*/

	int left = startmoves;
	int right = endmoves - 1;
	SortBlunders(left, right);

	/*
	for (int i = startmoves; i < endmoves; i++)
	{
		Alg(move_list[i].from, move_list[i].to);
		printf(" score %d \n", move_list[i].score);
	}
	Alg(move_list[left].from, move_list[left].to);
	printf(" score %d \n", move_list[left].score);
	z();
	/*/
	blunder_start = left;
	endmoves = blunder_start;
	//*/

	top = HASH_SCORE;

	for (int i = startmoves; i < endmoves; i++)
	{
		top = SelectMove(i, top, endmoves);

		from = move_list[i].from;
		to = move_list[i].to;

		if (from == ttmove.from && to == ttmove.to)
			continue;

		flags = move_list[i].flags;
		move_score = move_list[i].score;
		piece = b[from];

		if (depth == 1 && count > 0 && !pvs)
		{
			ev = ev1 + PieceScore[side][piece][to] - PieceScore[side][piece][from] + frontier[piece];

			if (ev <= alpha)
			{
				if (alpha - ev > 100)
				{
					break;
				}
				continue;
			}
		}

		MakeQuietMove(from, to, flags);

		if (depth == 2 && piece_mat[side] > N_VALUE && count > 0)
		{

			ev = ev1 + PieceScore[xside][piece][to] - PieceScore[xside][piece][from] + frontier[piece] +
				frontier[game_list[hply - 1].capture];
			if (ev <= alpha && IsThreat(xside, side, alpha - ev) == 0)
			{
				UnMakeQuietMove();
				continue;
			}
		}
		if (depth == 3 && piece_mat[side] > N_VALUE && count > 0)
		{
			if (ev1 + 400 <= alpha && IsThreat(xside, side, alpha - ev) == 0)
			{
				if (!(bit_pieces[xside][P] & mask_ranks[xside][6]))
				{
					UnMakeQuietMove();
					continue;
				}
			}
		}
		if (depth == 4 && piece_mat[side] > N_VALUE && count > 0)
		{
			if (ev1 + 800 <= alpha && IsThreat(xside, side, alpha - ev) == 0)
			{
				if (!(bit_pieces[xside][P] & mask_ranks[xside][6]))
				{
					UnMakeQuietMove();
					continue;
				}
			}
		}
		if (quiet_count > 0)
		{
			if (depth > 2)
			{
				r = Reduce(i);
				if (depth >= 10 && !pvs && !(flags & ATTACK) && count >= 12)//10 12 45
				{
					if (move_score < COUNTER_SCORE)
					{
						r = 2;
					}
				}
			}
			if (depth == 2)
			{
				if (piece_mat[side] <= B_VALUE && piece_mat[xside] <= B_VALUE)
				{
					if (!(bit_moves[K][kingloc[xside]] & bit_units[side]))
					{
						r = 1;
					}
				}
			}
			reduce[ply] = r;
		}

		quiet_count++;
		extend[ply] = -r;
		d = depth - 1;
		int rd = d - r;
		if (rd < 0)
			rd = 0;

		if (depth < 3 || r == 0)
		{
			if (count == 0)
			{
				score = -Search(-beta, -alpha, d, pvs, DO_NULL);
			}
			else
			{
				score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
				if (score > alpha && score < beta)
					score = -Search(-beta, -alpha, d, PV, DO_NULL);
			}
		}
		else
		{
			score = -Search(-alpha - 1, -alpha, rd, NO_PV, DO_NULL);

			if (score > alpha && score < beta)
			{
				extend[ply] = 0;
				score = -Search(-beta, -alpha, d, PV, DO_NULL);
			}
		}

		UnMakeQuietMove();

		if (stop_search)
			return alpha;

		count++;

		if (score > bestscore)
		{
			bestscore = score;
			bestmove = move_list[i];
		}
		if (score > alpha)
		{
			if (score >= beta)
			{
				/*/
				if (quiet_count > 10 && depth>2)//depth == 1 && bit_targets > 0 && score < 0)//>2
				{
					cout << quiet_count << endl;
					Alg(from, to);
					ShowAll(ply);
					_getch();
				}
				/*/
				UpdateContinuation(depth, from, to);
				UpdateHistory(i, from, to, score, depth);
				AddHash(side, depth, score, BETA, from, to, flags);
				PlyMove[ply] = CUT;
				if (lookup > -1)
					cut_tt_nodes++;
				else
					cut_nodes++;
				if (count == 1)
				{
					if (lookup > -1)
						first_tt_nodes++;
					else
						first_nodes++;
				}

				av_nodes += count;

				return beta;
			}
			alpha = score;
		}
	}
	//
	//
	//
	//
	//
	//

	if (blunder_start > 0)
	{
		top = HASH_SCORE;
		endmoves = first_move[ply + 1];
		for (int i = blunder_start; i < endmoves; i++)
		{
			top = SelectMove(i, top, endmoves);

			from = move_list[i].from;
			to = move_list[i].to;

			if (from == ttmove.from && to == ttmove.to)
				continue;

			flags = move_list[i].flags;
			move_score = move_list[i].score;
			piece = b[from];

			if (depth == 1 && count > 0 && !pvs)
			{
				ev = ev1 + PieceScore[side][piece][to] - PieceScore[side][piece][from] + frontier[piece];

				if (ev <= alpha)
				{
					if (alpha - ev > 100)
					{
						break;
					}
					continue;
				}
			}

			MakeQuietMove(from, to, flags);

			if (depth == 2 && piece_mat[side] > N_VALUE && count > 0)
			{
				ev = ev1 + PieceScore[xside][piece][to] - PieceScore[xside][piece][from] + frontier[piece] +
					frontier[game_list[hply - 1].capture];
				if (ev <= alpha && IsThreat(xside, side, alpha - ev) == 0)
				{
					UnMakeQuietMove();
					continue;
				}
			}
			if (depth == 3 && piece_mat[side] > N_VALUE && count > 0)
			{
				if (ev1 + 400 <= alpha && IsThreat(xside, side, alpha - ev) == 0)
				{
					if (!(bit_pieces[xside][P] & mask_ranks[xside][6]))
					{
						UnMakeQuietMove();
						continue;
					}
				}
			}
			if (depth == 4 && piece_mat[side] > N_VALUE && count > 0)
			{
				if (ev1 + 800 <= alpha && IsThreat(xside, side, alpha - ev) == 0)
				{
					if (!(bit_pieces[xside][P] & mask_ranks[xside][6]))
					{
						UnMakeQuietMove();
						continue;
					}
				}
			}
			if (quiet_count > 0)
			{
				if (depth > 2)
				{
					r = Reduce(i);
					if (depth >= 10 && !pvs && !(flags & ATTACK) && count >= 12)//10 12 45
					{
						if (move_score < COUNTER_SCORE)
						{
							r = 2;
						}
					}
				}
				if (depth == 2)
				{
					if (piece_mat[side] <= B_VALUE && piece_mat[xside] <= B_VALUE)
					{
						if (!(bit_moves[K][kingloc[xside]] & bit_units[side]))
						{
							r = 1;
						}
					}
				}
				reduce[ply] = r;
			}

			quiet_count++;
			extend[ply] = -r;
			d = depth - 1;
			int rd = d - r;
			if (rd < 0)
				rd = 0;

			if (depth < 3 || r == 0)
			{
				if (count == 0)
				{
					score = -Search(-beta, -alpha, d, pvs, DO_NULL);
				}
				else
				{
					score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);
					if (score > alpha && score < beta)
						score = -Search(-beta, -alpha, d, PV, DO_NULL);
				}
			}
			else
			{
				score = -Search(-alpha - 1, -alpha, rd, NO_PV, DO_NULL);

				if (score > alpha && score < beta)
				{
					extend[ply] = 0;
					score = -Search(-beta, -alpha, d, PV, DO_NULL);
				}
			}

			UnMakeQuietMove();

			if (stop_search)
				return alpha;

			count++;

			if (score > bestscore)
			{
				bestscore = score;
				bestmove = move_list[i];
			}
			if (score > alpha)
			{
				if (score >= beta)
				{
					//
					if (depth > 2)
					{
						//Alg(from, to);
						//ShowAll(ply);
						//_getch();
					}
					/*/
					if (quiet_count > 10 && depth > 2)//depth == 1 && bit_targets > 0 && score < 0)//>2
					{
						cout << quiet_count << endl;
						Alg(from, to);
						ShowAll(ply);
						_getch();
					}
					/*/
					//
					UpdateContinuation(depth, from, to);
					UpdateHistory(i, from, to, score, depth);
					AddHash(side, depth, score, BETA, from, to, flags);
					PlyMove[ply] = CUT;
					if (lookup > -1)
						cut_tt_nodes++;
					else
						cut_nodes++;
					if (count == 1)
					{
						if (lookup > -1)
							first_tt_nodes++;
						else
							first_nodes++;
					}

					av_nodes += count;

					return beta;
				}
				alpha = score;
			}
		}
	}

	all_nodes++;
	PlyMove[ply] = ALL;

	if (!count)
	{
		return 0;
	}

	if (alpha > initial_alpha)
	{
		AddHash(side, depth, alpha, EXACT, bestmove.from, bestmove.to, bestmove.flags);
	}
	else
	{
		if (depth > 3)
			AddHash(side, depth, alpha, ALPHA, bestmove.from, bestmove.to, bestmove.flags);
	}
	return alpha;
}

void SortBlunders(int left, int right)
{
	while (left <= right)
	{
		if (move_list[left].score > -EN_PRISE_SCORE)
		{
			left++;
		}
		else
		{
			std::swap(move_list[left], move_list[right]);
			right--;
		}
	}
}

//22 113 22569 546305295 c1f4 b8d7 e1h4 h7h5 g5e4 d8f8 f1e1 c5c4 b5d7 c7d7 e4d6 d7d6 e1e5 d6d8 h4f2 c8b7 d5d6 b7g2 f2g2 d8d6
//23 163 73932 1874170821 g5h7 g8h7 e1h4 h7g8 c1h6 c8g4 h6g7 g4h5 h4g5 e5g4 h2h3 g4f6 g7f6 d8f8 g2g4 h5g4 h3g4 b8d7 b5d7 c7d7 g5h4
