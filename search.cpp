#include <stdlib.h>
#include <iostream>
#include <setjmp.h>

#include "globals.h"

using namespace std;

//ofstream file("debug.txt");

// Example constants: tune to your scoring scale
const int CONT_SCALE = 8;   // multiply continuation table values
const int  HIST_SCALE = 1;    // if you still use normal history
const int  QUIET_BASE = 0;

// Continuation history: prev (piece,to) -> curr (piece,to)
static int cont_hist[6][64][6][64];

// Optional: keep a second table for "prev2" (two plies back) for extra strength
static int cont2_hist[6][64][6][64];

// what move was made to reach this node (for the side who just moved)
static unsigned char prev_piece[MAX_PLY];
static unsigned char prev_to[MAX_PLY];
static unsigned char prev_valid[MAX_PLY]; // 0/1, in case ply==0 or null-move etc.

void UpdateSequence(const int ply, const int depth, const move_data& m);
static inline int ContinuationScore(int ply, int piece, int to);
static inline void UpdateWithGravity(int& cell, int bonus);

int hits, misses;
int bestdiff = 0;
int worstdiff = 1000;

int bestcut = 0;

void ClearContHistory()
{
	memset(cont_hist, 0, sizeof(cont_hist));
	memset(cont2_hist, 0, sizeof(cont2_hist));
}

int GetNextAttackerPins2(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);

bool ContactMate(const int s, const int xs, const int attack_sq);

void RemoveDiscoChecks(const int startmoves);
bool LightSEE(const int s, const int xs, const int att1, const int sq);
//int GetNextAttackerPins(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);

void UnMakeNull();

bool IsThreat(const int s, const int xs, const int);

int SortCaptures(const int from, const int top, const int last);
void SortPromotion(const int startmoves, const int endmoves);
void SortEndgame(const int startmoves, const int endmoves);
void SortQuiet(const int startmoves, const int endmoves, move_data ttmove, move_data countermove, move_data killer, move_data killer2);

void SortChecks(const int from, const int last);

void BuildAttackMap();
int GetLineTarget();
BITBOARD GetTargets(const int s, const int xs);

int GetAttackingSquare(const int s, const int sq);

int Search(int alpha, int beta, int depth, int pvflag, int nullflag);

bool PawnCounterAttack(const int from, const int to, const BITBOARD bit_threshold);
bool CounterAttack(const int from, const int to, const int, const BITBOARD bit_threshold);
BITBOARD GetThreshold(const int s, const int attacked);

void UpdateCheckHistory(const int from, const int to, const int x, const int depth);

BITBOARD GenChecks(const int, const int, BITBOARD);

BITBOARD GetKnightAttacks(const int s);
BITBOARD GetBishopAttacks(const int s);
BITBOARD GetRookAttacks(const int s);
BITBOARD GetQueenAttacks(const int s);
BITBOARD GetKingAttacks(const int s);

extern BITBOARD bit_adjacent_king[MAX_PLY][2];
extern int king_attackers[MAX_PLY][2];

extern int test_mode;

BITBOARD qnodes;
BITBOARD all_nodes;

int GetThreat(const int target);
BITBOARD PinnersPossible(const int s, const int xs);

bool IsCheck(const int p, const int sq, const int king);

constexpr int  NO_PV =0;
constexpr int  PV =1;
constexpr int  CUT =2;
constexpr int  ALL =3;
constexpr int  NO_NULL =0;
constexpr int  DO_NULL =1;

constexpr int  RANK_7 =6;
constexpr int  WON_ENDGAME =9900;
constexpr int  LOST_ENDGAME = - 9900;

constexpr int  UNDEFENDED =1;
constexpr int  WEAKER_ATTACK =2;
constexpr int  SEE_ATTACK =3;

constexpr int  INF =10000;

constexpr int INVALID = 11111;
constexpr int START_SCORE = -20000;

int startmat[2];

const int MAX1 = MAX_PLY - 1;
const int MAX2 = MAX_PLY - 2;

const int LOW = -10000;
const int HIGH = 10000;
const int HI = 10000;//

int currentdepth;
int currentmax;

int frontier[8] = { 0,0,8,20,10,20,0 };//k as 30 june 17
int value[] = { 100, 300, 300, 500, 900, 0 };

int stats_depth[20];
int stats_count[100];
int stats_killers[2];
int total_depth[20];

move_data counter[64][64];

int total_killers[2];

move_data killer[MAX_PLY];
move_data killer2[MAX_PLY];

move_data mate_move[MAX_PLY];

move_data root_list[100];

int deep;

static int debug;

int c_nodes[MAX_PLY];

int reduce[MAX_PLY];//

#include <setjmp.h>
static jmp_buf env;
bool stop_search;
int root_score;
int real_side;

move_data root_move;

move_data pv[MAX_PLY][MAX_PLY];
int pv_len[MAX_PLY];
move_data old_pv[MAX_PLY][MAX_PLY];
int old_pv_len[MAX_PLY];

void UpdatePV(move_data m);

bool IsLegal(const int, const int);

void LoadBook();
int Book();

int GetLowestQuietAttacker(const int s, const int to);
int Sort(const int from, const int, const int);
void CheckUp();

void SelectCapture(const int from, const int last);
void z();
void ClearKillers();
void ClearHistory();
void SetTime(int);

void GenQuietMoves(const int, const int, BITBOARD, BITBOARD, const BITBOARD* bit_cs);
void GenRoot(const int, const int);
int RootSearch(int depth, int alpha, int beta, const int);

void ShowMoves(int);

int Reduce(const int i);

void DisplayPV();

void UpdateKillers(const int i, const int from, const int to, const int x, const int depth);

move_data GetHashMove();

void SortLastDepth(const int first, const int last);

void FreeAllHash();

int all, cut;

void z();

int InCheck[MAX_PLY];

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
	memset(stats_killers, 0, sizeof(stats_killers));

	ClearKillers();

	memset(check_history, 0, sizeof(check_history));
	memset(counter, 0, sizeof(counter));
	memset(reduce, 0, sizeof(reduce));

	memset(hist_from, 0, sizeof(hist_from));//
	memset(hist_to, 0, sizeof(hist_to));//
	//SetFromTo();//??
}

void SetTime(int fixed_time)
{
	if (fixed_depth == 1)
	{
		//??		return;
	}
	if (fixed_time == 0 && hply > 1)
	{
		if (game_list[hply - 1].capture == QVAL &&
			b[game_list[hply - 1].to] == Q)
		{
			max_time = max_time / 2;
		}
		else if (game_list[hply - 1].capture == RVAL &&
			b[game_list[hply - 1].to] == R &&
			Attack(side, game_list[hply - 1].to) &&
			Attack(xside, game_list[hply - 1].to) == 0)
		{
			max_time = max_time / 2;
		}
		else if (piece_value[game_list[hply - 1].capture] == BVAL &&
			piece_value[b[game_list[hply - 1].to]] == BVAL &&
			Attack(side, game_list[hply - 1].to) &&
			Attack(xside, game_list[hply - 1].to) == 0)
		{
			max_time = max_time / 2;
		}
		else if (Attack(xside, kingloc[side]))
		{
			max_time = max_time / 2;
		}
	}

	start_time = GetTime();
	stop_time = start_time + max_time;
}

move_data Think(int fixed_time)
{
	hits = 0;
	misses = 0;
	//
	stop_search = false;

	if (setjmp(env) != 0)      // came from longjmp
	{
		stop_search = true;

		//z();

		while (ply)
		{
			UnMakeMove();      // must be rock-solid
			//z();
		}
		/*
		while (ply)
		{
		if(game_list[hply].from !=0 || game_list[hply].to !=0)
		{
		UnMakeMove();
		}
		else
		{
			UnMakeNull();
			//z();
			}
		}
		*/
		return root_move;
	}

	int bookflag = 0;
	//*
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
	//*/

	ClearHistory();
	fixed_depth = 1;
	SetTime(fixed_time);

	int score = 0;
	int prevScore = 0;

	move_data best_stable;
	best_stable.from = 0;
	best_stable.to = 0;
	best_stable.flags = 0;

	for (int depth = 1; depth <= max_depth; ++depth)
	{
		if (GetTime() >= stop_time)
			break;

		int alpha = LOW;
		int beta = HIGH;
		currentmax = depth;

		old_pv_len[0] = pv_len[0];
		for (int i = 0; i < old_pv_len[0]; ++i)
			old_pv[0][i] = pv[0][i];

		if (depth > 1)
		{
			int delta = 50;
			alpha = prevScore - delta;
			beta = prevScore + delta;

			if (depth > 7)
			{
				ReduceHistory();
			}

			bool tried_full_window = false;

			while (true)
			{
				ply = 0;
				for (int i = 0; i < first_move[1]; i++)
					move_list[i] = root_list[i];

				score = RootSearch(depth, alpha, beta, prevScore);

				if (stop_search)
					break;

				if (score <= alpha)
				{
					alpha -= delta;
					delta <<= 1;

					if (alpha <= LOW)
					{
						if (tried_full_window) break;
						alpha = LOW; beta = HIGH;
						delta = 50;
						tried_full_window = true;
					}
					continue;
				}

				if (score >= beta)
				{
					beta += delta;
					delta <<= 1;

					if (beta >= HIGH)
					{
						if (tried_full_window) break;
						alpha = LOW; beta = HIGH;
						delta = 50;
						tried_full_window = true;
					}
					continue;
				}

				break;
			}
			if (test_mode != 1)
			{
				//std::cerr << depth << "/" << deep << " " << score << " " << (GetTime() - start_time) / 10 << " " << nodes << " ";
				//printf(" %d %d %d %d ", depth, deep, score,(GetTime() - start_time) / 10, nodes);
				//DisplayPV();
			}

			if (score > 9000 || score < -9000)
			{
				break;
			}
		}
		else
		{
			ply = 0;
			GenRoot(side, xside);
			score = RootSearch(depth, alpha, beta, 0);
		}

		prevScore = score;

		best_stable = root_move;

		// here: record best root move from globals, print info, etc.
		if (stop_search)
			break;
	}

	//printf(" hits %d \nmisses %d ", hits, misses);

	return best_stable;
}

void GenRoot(const int s, const int xs)
{
	BITBOARD bit_disco_pieces;
	BITBOARD pin_mask = GetPinMask(s, xs);
	BITBOARD bit_check_squares[6];
	memset(bit_check_squares, 0, sizeof(bit_check_squares));

	int check = Check(xs, kingloc[s]);
	if (check > -1)
	{
		EvadeCapture(s, xs, check, pin_mask);
		EvadeQuiet(s, xs, check, pin_mask);
		//ShowAll(ply);
	}
	else
	{
		GenCaptures(s, xs, pin_mask);//
		bit_disco_pieces = GenChecks(s, xs, pin_mask);
		BuildAttackMap();
		GenQuietMoves(s, xs, 0, pin_mask, bit_check_squares);
	}
	pv_len[0] = 0;
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

	bool firstMove = true;
	bool hasPVHint = (old_pv_len[0] > 0);
	move_data pvmove;
	if (hasPVHint)
		pvmove = old_pv[0][0];

	ply = 0;
	pv_len[0] = 0;

	for (int i = start; i < end; ++i)
	{
		top = Sort(i, top, end);
		int from = move_list[i].from;
		int to = move_list[i].to;
		unsigned int flags = move_list[i].flags;

		if (!MakeMove(from, to, flags))
			continue;

		int score;

		bool matchesPV =
			hasPVHint &&
			move_list[i].from == pvmove.from &&
			move_list[i].to == pvmove.to &&
			move_list[i].flags == pvmove.flags;

		if (firstMove)
		{
			score = -Search(-beta, -alpha, depth - 1, PV, DO_NULL);
			firstMove = false;
		}
		else
		{
			score = -Search(-alpha - 1, -alpha, depth - 1, NO_PV, DO_NULL);
			if (score > alpha)
			{
				score = -Search(-beta, -alpha, depth - 1, PV, DO_NULL);
			}
		}

		UnMakeMove();

		if (stop_search) {
			if (best.from != -1) 
				root_move = best;
			return bestScore;
		}

		root_list[i] = move_list[i];
		root_list[i].score = score;

		if (score > bestScore)
		{
			bestScore = score;
			best = move_list[i];

			if (test_mode != 1 && depth > 7)
			{
				if (root_move.from != best.from || root_move.to != best.to)
				{
					std::cerr << depth << "/" << deep << " " << score << " " << (GetTime() - start_time) / 10 << " " << nodes << " ";
					DisplayPV();
				}
			}
			root_move = best;

			if (score > alpha)
			{
				alpha = score;
				pv[0][0] = move_list[i];
				pv_len[0] = pv_len[1] + 1;
				for (int k = 0; k < pv_len[1]; k++)
					pv[0][k + 1] = pv[1][k];
			}
		}
	}
	if (!stop_search && best.from != -1)
		root_move = best;
	return bestScore;
}

int Search(int alpha, int beta, int depth, int pvs, int null)
{
	pv_len[ply] = 0;
	nodes++;
	if ((nodes & 4095) == 0)// && null == 1)
	{
		CheckUp();
	}
	if (stop_search)
		return alpha;

	mate_move[ply].from = -1;//
	mate_move[ply].to = -1;//

	if (ply && Reps2())
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
	if (piece_mat[xside] == 0 && (bit_pieces[side][R] | bit_pieces[side][Q]) &&
		pawn_mat[xside] == 0)
		if (piece_mat[xside] < startmat[xside] || piece_mat[side] > startmat[side])
			return WON_ENDGAME - ply;

	if (depth < 1)
	{
		return QuietSearch(side, xside, alpha, beta);
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

	if (ply > MAX2)
		return Eval(alpha, beta);

	if (lookup > -1)
	{
		hits++;
	}
	else misses++;

	currentdepth = depth;
	int score;
	int check = Check(xside, kingloc[side]);
	int ev1 = INVALID;
	int threat = 0;

	if (depth > 2
		&& null
		&& !pvs
		&& piece_mat[side] > QVAL
		&& check == -1
		)
	{
		ev1 = Eval(alpha, beta);
		if (ev1 >= beta - 50)
		{
			//if (!(IsThreat(xside, side, ev1 - beta)) && !(IsThreat(side, xside, 0)))
			{
				//printf("\n is threat");

				//printf("+");

				//threat = 1;
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
					if (threat == 1)
					{
						//cout << " threat ";
						//z();
						//cout << "+";
					}

					return beta;
				}
				if (threat == 1)
					;// cout << "-";
			}
		}
	}

	int count = 0;
	const int initial_alpha = alpha;
	int bestscore = START_SCORE;
	int from, to, flags;
	int first = first_move[ply];
	int d;
	int s, xs;
	int move_score;
	int piece;

	move_data bestmove;

	BITBOARD bit_safe_captures = 0;

	first_move[ply + 1] = first_move[ply];

	if (lookup > -1)
	{
		from = ttmove.from;
		to = ttmove.to;
		flags = ttmove.flags;
		ttmove.score = HASH_SCORE;//

		move_list[first].from = from;
		move_list[first].to = to;
		move_list[first].flags = flags;
		move_list[first].score = HASH_SCORE;
		first_move[ply + 1] = first + 1;

		if (MakeMove(from, to, flags))
		{
			count++;

			if (b[to] == K && b[from] == 6 && piece_mat[side] > 12)
				d = depth;
			else
				d = depth - 1;

			score = -Search(-beta, -alpha, d, pvs, DO_NULL);

			UnMakeMove();

			if (stop_search)
				return alpha;

			if (score > alpha)
			{
				if (score < beta)
				{
					UpdatePV(move_list[first]);
				}
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
		}
		if (b[to] < 6)
		{
			bit_safe_captures |= mask[to];
			/*
			printf("tt ");
			PrintBitBoard(bit_safe_captures);
			Alg(from, to);
			z();
			*/
		}
		bestmove = ttmove;
	}

	int r = 0;
	int ev = -10000;
	int top = HASH_SCORE;
	int lowest;

	mate_move[ply].from = -1;//
	mate_move[ply].to = -1;//

	BITBOARD pins[2];

	pins[0] = PinnersPossible(0, 1);
	pins[1] = PinnersPossible(1, 0);

	int k = kingloc[side];
	BITBOARD b1 = pins[side];
	BITBOARD pin_mask = 0;
	/*
	while (b1)
	{
		int sq = NextBit(b1);
		BITBOARD b2 = bit_between[sq][k] & bit_units[side];
		if (IsOneBit(b2) != 1 || (bit_between[sq][k] & bit_units[xside]))
		{
			pins[side] &= not_mask[sq];
		}
		else
			pin_mask |= mask[NextBit(b2)];
		b1 &= b1 - 1;
	}
*/

	pin_mask = GetPinMask(side, xside);//

	if (check > -1)
	{
		EvadeCapture(side, xside, check, pin_mask);
		game_list[hply].flags |= INCHECK;

		for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
		{
			top = Sort(i, top, first_move[ply + 1]);

			from = move_list[i].from;
			to = move_list[i].to;

			if (from == ttmove.from && to == ttmove.to)
				continue;

			flags = move_list[i].flags;

			if (!MakeCapture(from, to, flags))
			{
				continue;
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
				if (score < beta)
				{
					UpdatePV(move_list[i]);
				}
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

		for (int i = start; i < end; i++)
		{
			top = Sort(i, top, end);

			from = move_list[i].from;
			to = move_list[i].to;

			if (from == ttmove.from && to == ttmove.to)
				continue;

			flags = move_list[i].flags;
			piece = b[from];

			if (!MakeMove(from, to, flags))
			{
				continue;
			}

			s = side;
			xs = xside;

			if (count > 0 && !pvs)
			{
				if (depth <= 2)
				{
					if (ev1 == INVALID)
						ev1 = Eval(alpha, beta);
				}
				if (depth <= 1)
				{
					ev = ev1 + PieceScore[s][piece][to] - PieceScore[s][piece][from];
					if (ev + frontier[piece] <= alpha)
					{
						UnMakeMove();
						if (alpha - ev > 100)
						{
							break;
						}
						continue;
					}
				}
			}
			if (count > 0 && depth == 2 && ply > 1 && !pvs)
			{
				ev = ev1 + PieceScore[xs][piece][to] - PieceScore[xs][piece][from];
				if (ev <= alpha && BestThreat(xs, s, alpha - ev) == 0)
				{
					UnMakeMove();
					continue;
				}
			}

			if (b[to] == K && piece_mat[s] > 12)
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

			UnMakeMove();

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
					if (i > first_move[ply] && b[from] != K)
					{
						//printf(" check block ");
						//Alg(from, to);
						//ShowAll(ply);
					}
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
				if (score < beta)
				{
					UpdatePV(move_list[i]);
				}
			}
		}
		if (!count)
		{/*
			if (!(game_list[hply - 2].flags & INCHECK) && ply > 2)
			{
				int mate_from = game_list[hply - 1].from;
				int mate_to = game_list[hply - 1].to;
				int mfrom2 = game_list[hply - 2].from;
				int mto2 = game_list[hply - 2].to;

				bool mateflag = 0;

				if (mto2 == mate_from)
				{
					Alg(mfrom2, mto2);
					cout << " mate ";
					Alg(mate_from, mate_to);
					z();
				}

				if ((bit_moves[b[mto2]][mfrom2] & mask[mate_to] && !(bit_between[mfrom2][mate_to] & bit_all)) ||
					mto2 == mate_to)
				{
					mateflag = 1;
				}
				if (mateflag == 0 && !(bit_between[mate_from][mate_to] & (mask[mfrom2])))
				{
					mate_move[ply-1].from = mate_from;
					mate_move[ply-1].to = mate_to;
					mate_move[ply-1].flags = game_list[hply - 1].flags;
					for (int i = first_move[ply - 2]; i < first_move[ply - 1]; i++)
						move_list[i].flags |= MATETHREAT;
				}
			}
			*/
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
	//end 	

	if (b[C7]==0 && b[D6]==0 && bit_pieces[1][N]==0 && b[C4]==R)
	{
		//printf(" rxb7 ");
		//z();
	}
	pin_mask = GetPinMask(side, xside);//
	if (pin_mask)
	{
		//PrintBitBoard(pin_mask);
		//z();
	}

	GenCaptures(side, xside, pin_mask);

	int val_to, val_from;

	const int startcaptures = first_move[ply];
	const int endcaptures = first_move[ply + 1];

	int capture_score = 0;//

	for (int i = startcaptures; i < endcaptures; i++)
	{
		if (b[move_list[i].to] != EMPTY)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			if (from == ttmove.from && to == ttmove.to)
				continue;

			piece = b[from];
			capture_score = 0;//

			if (pins[xside] == 0)
			{
				lowest = GetLowestAttacker(xside, to);
			}
			else
			{
				lowest = GetNextAttackerPins2(xside, side, to, bit_all & not_mask[from], pin_mask);
			}

			val_to = value[b[to]];
			//capture_score = val_to;//
			if (lowest > -1)
			{
				val_from = value[piece];
				if (val_from < val_to)
				{
					capture_score = val_to - val_from;
				}
				else if (lowest == K)
				{
					if (GetNextAttacker(side, xside, to, bit_all & not_mask[from]) == -1)
					{
						//printf(" king defends ");
						//Alg(from, to);
						//z();
						capture_score = val_to - val_from;
					}
					else
					{
						capture_score = val_to;
						//if (piece==Q && ContactMate(side,xside,to))
						{
							//printf(" contact ");
							//Alg(from, to);
							//z();
						//	capture_score = 10000 - ply;
						}
					}
				}
				else if (val_from > val_to + value[lowest])
				{
					capture_score = (val_to + value[lowest]) - val_from;
				}
				else if (piece == P && b[to] == P)
				{
					capture_score = 0;
				}
				else
				{
					capture_score = centi_pieces[SEE(side, from, to, pins[0], pins[1])];
					if (capture_score == 0)
						capture_score = val_to - val_from;//11/25
				}
			}
			else
			{
				capture_score = val_to;
			}
			if (capture_score > 0)
			{
				bit_safe_captures |= mask[to];
				/*
				PrintBitBoard(bit_safe_captures);
				printf(" safe %d ", capture_score);
				Alg(from, to);
				z();
				*/
			}
			if (capture_score >= 0)
			{
				if (move_list[i].flags & CHECK)
					capture_score += 10 + b[from];
				else if (hply > 1 && game_list[hply - 1].to == to)
				{
					capture_score++;//25/9/25
				}
			}
			/*
			if (piece == b[to])
			{
				if (!Attack(side, from))
				{
					printf("moving away ");
					Alg(from, to);
					z();
					capture_score += 10 + b[from];
				}
			}
			*/
			move_list[i].score = capture_score;
		}
	}
	/*
	int mfrom = mate_move[ply + 1].from;
	int mto = mate_move[ply + 1].to;
	if (mfrom != mto)
	{
		for (int i = startcaptures; i < endcaptures; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			if (from == ttmove.from && to == ttmove.to)
				continue;
			//
			if (from == mfrom && to == mto)
			{
				move_list[i].score = 24000000;
				cout << " strike 1 ";
				Alg(from, to);
				z();
				break;
			}
		}
	}
	//*/

	int captured;

	for (int i = startcaptures; i < endcaptures; i++)
	{
		//SelectCapture(i, endcaptures);
		top = SortCaptures(i, top, endcaptures);

		from = move_list[i].from;
		to = move_list[i].to;

		if (from == ttmove.from && to == ttmove.to)
			continue;

		flags = move_list[i].flags;
		piece = b[from];
		captured = b[to];

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
				ev += value[captured] + PieceScore[xside][captured][to] + frontier[captured];
				if (ev + frontier[piece] <= alpha)
				{
					continue;
				}
			}
		}
		if (!MakeCapture(from, to, flags))
		{
			continue;
		}

		if (count > 0 && depth == 2 && ply > 1 && !(flags & CHECK))
		{
			ev = ev1 + PieceScore[xside][piece][to] - PieceScore[xside][piece][from];
			ev += value[game_list[hply - 1].capture] + PieceScore[side][game_list[hply - 1].capture][to] + frontier[piece];
			if (ev <= alpha && BestThreat(xside, side, alpha - ev) == 0)
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
				/*
				if (i > startcaptures)
				{
					int n;
					if (ttmove.from == 0 && ttmove.to == 0)
					{
						printf(" no tt ");
						n = startcaptures;
					}
					else
						n = startcaptures;
					if (i > n)
					{
						Alg(from, to);
						ShowAll(ply);
					}
				}
				*/
				return beta;
			}
			alpha = score;
			if (score < beta)
			{
				UpdatePV(move_list[i]);
			}
		}
	}
	//end captures

	const int startchecks = first_move[ply + 1];

	BITBOARD bit_disco_pieces = GenChecks(side, xside, pin_mask);

	const int endchecks = first_move[ply + 1];

	d = depth - 1;
	/*
		int mfrom = mate_move[ply + 1].from;
		int mto = mate_move[ply + 1].to;
		if (mfrom != mto && b[mto] == 6 &&
			!(mfrom == ttmove.from && mto == ttmove.to) && mto != game_list[hply - 1].from &&
			!KingLessAttack(xside, mto))
		{
			for (int i = startchecks; i < endchecks; i++)
			{
				from = move_list[i].from;
				to = move_list[i].to;

				if (from == mfrom && to == mto)
				{
					move_list[i].score = 25000000;
					break;
				}
			}
		}
		*/
	BITBOARD bit_check_squares[6];
	memset(bit_check_squares, 0, sizeof(bit_check_squares));
	int k1 = kingloc[xside];

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
		if (startchecks > endchecks + 1)
		{
			int low = GetLowestAttacker(xside, to);
			if (low > -1)
			{
				int val = p_value[piece];
				if (p_value[low] < val)
					move_list[i].score = -val;
				else if (p_value[low] == val)
					move_list[i].score = -val + 1;
				else
				{
					if (Attack2(side, to, bit_all & not_mask[from], not_mask[from]) == 0)
					{
						move_list[i].score = -val;
					}
				}
			}
		}
	}
	//ShowAll(ply);

	top = HASH_SCORE;

	for (int i = startchecks; i < endchecks; i++)
	{
		//top = Sort(i, top, endchecks);
		SortChecks(startchecks, endchecks);

		from = move_list[i].from;
		to = move_list[i].to;

		if (from == ttmove.from && to == ttmove.to)
			continue;

		flags = move_list[i].flags;

		if (!MakeMove(from, to, flags))
		{
			continue;
		}

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
		UnMakeMove();

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
				if (i > startchecks)
				{
					//printf(" check ");
					//Alg(from, to);
					//ShowAll(ply);
				}
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
			if (score < beta)
			{
				UpdatePV(move_list[i]);
			}
			alpha = score;
		}
	}
	//end checks

	ev1 = Eval(alpha, beta);
	int diff = alpha - ev1;
	int old_alpha = alpha;

	int margin = 50;

	if (bit_pieces[side][N] & mask_corner)
	{
		margin = 180;
	} 

	int flag = 0;
	if (depth == 1 && diff > margin && count > 0 && (piece_mat[side]> BBVAL && piece_mat[xside] > BBVAL))
	{
		//printf("+");
		//flag = 1;
		//return alpha;
	}

	int killer_from = killer[ply].from;
	int killer_to = killer[ply].to;
	int killer2_from = killer2[ply].from;
	int killer2_to = killer2[ply].to;

	int game_from = game_list[hply - 1].from;
	int game_to = game_list[hply - 1].to;
	int counter_from = counter[game_from][game_to].from;
	int counter_to = counter[game_from][game_to].to;

	const int startmoves = first_move[ply + 1];
	int quiet_count = 0;

	BuildAttackMap();

	if (bit_safe_captures)
	{
		//PrintBitBoard(bit_safe_captures);
		//z();
	}

	GenQuietMoves(side, xside, ~bit_safe_captures, pin_mask, bit_check_squares);
	if (bit_disco_pieces)
	{
		RemoveDiscoChecks(startmoves);
	}

	const int endmoves = first_move[ply + 1];

	/*
	 BITBOARD b2;
	if(mate_move[ply+1].from>0 || mate_move[ply + 1].to > 0)
	{
		printf(" mm ");
		Alg(mate_move[ply+1].from, mate_move[ply+1].to);
		z();
		int mfrom = mate_move[ply + 1].from;
		int mto = mate_move[ply + 1].to;
		//test threat
		//if (bit_moves[b[mfrom]][mto] & mask[mto])
		{
			//if (!(bit_between[mfrom][mto] & bit_all))
		}
		//
		for (int i = startmoves; i < endmoves; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			if (bit_between[mfrom][mto] & mask[to])
			{
				Alg(mfrom, mto);
				cout << " block ";
				Alg(from, to);
				z();
				move_list[i].score = 1000000;
				move_list[i].flags |= DEFEND;
			}
			else if (b[from] != K && bit_moves[b[from]][to] & mask[mto])
			{
				if (!(bit_between[to][mto] & bit_all))
				{
					Alg(mfrom, mto);
					cout << " defend ";
					Alg(from, to);
					z();
					move_list[i].score = 1000000;
				}
			}
			else if(b[from]==5)
			{
				Alg(mfrom, mto);
				cout << " king move ";
				Alg(from, to);
				z();
				move_list[i].score = 1000000;
			}
			else if (bit_kingmoves[kingloc[side]] & mask[from] && b[from] != 5)
			{
				Alg(mfrom, mto);
				cout << " move adj ";
				Alg(from, to);
				z();
				move_list[i].score = 1000000;
			}
		}
		//ShowAll(ply);
	}
	//*/

	SortPromotion(startmoves, endmoves);

	BITBOARD bit_targets = GetTargets(side, xside);

	bool capture_check = 0;
	int target_score = 0;
	int attacking_piece_square = -1;

	BITBOARD bit_threshold = 0;
	BITBOARD bit_unblock = 0;
	BITBOARD bit_unblock2 = 0;
	BITBOARD bit_line = 0;
	int unblock_square = -1;
	int defendable = 1;

	s = side;
	xs = xside;

	b1 = bit_targets;
	BITBOARD myPiecesOnly = bit_units[s] & ~bit_units[xs];
	while (b1)
	{
		int square = NextBit(bit_targets);
		//bit_line |= bit_between[attacking_piece_square][square];
		bit_unblock =
			(bit_moves[B][square] & (bit_pieces[s][B] | bit_pieces[s][Q])) |
			(bit_moves[R][square] & (bit_pieces[s][R] | bit_pieces[s][Q]));

		while (bit_unblock)
		{
			int hd = NextBit(bit_unblock);
			BITBOARD blockers = bit_between[hd][square] & myPiecesOnly;

			if (blockers && !(blockers & (blockers - 1)))
				bit_unblock2 |= blockers;

			bit_unblock &= bit_unblock - 1;
		}
		b1 &= b1 - 1;
	}

	if (bit_targets)
	{
		target_score = value[b[NextBit(bit_targets)]];
		for (int i = startmoves; i < endmoves; i++)
		{
			from = move_list[i].from;
			if (bit_targets & mask[from])
			{
				//if (target_score == 1)
				{
					move_list[i].score += 50000;
				}
				continue;
			}

			to = move_list[i].to;
			piece = b[from];
			if (defendable == 1)
			{
				if (piece > P)
				{
					if (piece == B || piece == R || piece == Q)
					{
						BITBOARD b1 = bit_targets;
						while (b1)
						{
							b1 &= b1 - 1;
							int square = NextBit(bit_targets);
							if (p_value[piece] >= p_value[square])
							{
								if (bit_moves[piece][to] & mask[square] &&
									!(bit_between[to][square] & bit_all) &&
									!(bit_moves[piece][from] & mask[square] &&
										!(bit_between[from][square] & bit_all)))
								{
									move_list[i].score += 50000 + value[piece];
								}
							}
						}
					}
					else
					{
						if ((bit_moves[piece][to] & bit_targets) &&
							!(bit_moves[piece][from] & bit_targets))
						{
							move_list[i].score += 50000 + target_score;
							continue;
						}
					}
				}
				else
				{
					if (bit_pawncaptures[s][to] & bit_targets)//pawn att
					{
						move_list[i].score += 50000 + target_score;
						continue;
					}
				}
			}
			if (mask[to] & bit_line)
			{
				move_list[i].score += 10000 + target_score;
				{
					move_list[i].score += 30000 + target_score;
				}
				continue;
			}
			if (mask[from] & bit_unblock2)
			{
				move_list[i].score += 40000 + target_score;
			}
		}
		//ShowAll(ply);
	}
	//*/

	BITBOARD bit_reveal;
	bit_reveal = bit_units[s] & bit_attacked[s][B] | bit_attacked[s][R] | bit_attacked[s][Q];
	/*
	while (bit_reveal)
	{

		bit_reveal &= bit_reveal - 1;
	}
	*/

	if (piece_mat[s] < 9)//endgames
	{
		SortEndgame(startmoves, endmoves);
	}

	//if (target_square == -1 || bit_targets==0)//
	/*
	if (bit_targets == 0)//
	{
		for (int i = startmoves; i < endmoves; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;

			if (from == ttmove.from && to == ttmove.to)
				continue;
			if (from == counter_from && to == counter_to)
			{
				if (move_list[i].score < COUNTER_SCORE)
					move_list[i].score += COUNTER_SCORE;
			}
			else if (from == killer_from && to == killer_to)
			{
				if (move_list[i].score < KILLER1_SCORE)
					move_list[i].score += KILLER1_SCORE;
			}
			else if (from == killer2_from && to == killer2_to)
			{
				if (move_list[i].score < KILLER2_SCORE)
					move_list[i].score += KILLER2_SCORE;
			}
			if (move_list[i].score >= 900000)//patch
			{
				//Alg(from, to);
				//z();
				move_list[i].score = 900000;
			}
		}
	}
	//*/
	int cont;
	if (bit_targets == 0)//
	{
		for (int i = startmoves; i < endmoves; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			cont = CONT_SCALE * ContinuationScore(hply, b[from], to);
			if (cont > CONT_SCORE)
				cont = CONT_SCORE;
			move_list[i].score += cont;
		}
	}


int quiet_alpha = alpha - ev1;

if (ev1 == INVALID && depth <= 2)
{
	ev1 = Eval(alpha, beta);
}

bool counter_flag = false;

top = HASH_SCORE;

for (int i = startmoves; i < endmoves; i++)
{
	top = Sort(i, top, endmoves);

	from = move_list[i].from;
	to = move_list[i].to;

	if (from == ttmove.from && to == ttmove.to)
		continue;

	flags = move_list[i].flags;
	move_score = move_list[i].score;
	piece = b[from];

	s = side;
	xs = xside;
	if (move_score > -1)
	{
		if (depth == 1 && count > 0 && !pvs)
		{
			ev = ev1 + PieceScore[s][piece][to] - PieceScore[s][piece][from] + frontier[piece];

			if (ev <= alpha)
			{
				if (alpha - ev > 100)
				{
					break;
				}
				continue;
			}
		}
	}

	if (!MakeQuietMove(from, to, flags))
	{
		continue;
	}

	s = side;
	xs = xside;

	counter_flag = false;

	if (move_score >= 0)// !pvs && depth >= 4
	{
		if (bit_targets == 0 && bit_line_attackers[ply - 1] & mask[from])
		{
			BITBOARD b1 = (bit_pieces[s][R] | bit_pieces[s][Q]) & bit_rookmoves[from];
			b1 |= (bit_pieces[s][B] | bit_pieces[s][Q]) & bit_bishopmoves[from];
			while (b1)
			{
				int sq = NextBit(b1);
				b1 &= b1 - 1;
				BITBOARD screened = bit_after[sq][from] & bit_units[xs];
				if (screened)
				{
					int flag = 0;
					int capture_sq = NextBit(screened);
					if (!(bit_between[sq][capture_sq] & (bit_all | mask[to])))
					{
						if (p_value[b[sq]] < p_value[b[capture_sq]] ||
							!Attack(xs, capture_sq))
						{
							flag = 1;
						}
						else
						{
							int att_sq = GetAttackingSquare(s, capture_sq);
							if (att_sq > -1)
							{
								int see = SEE(s, att_sq, capture_sq, 0, 0);
								if (see > 0)
								{
									flag = 1;
								}
							}
						}
						if (flag == 1)
						{
							int att_sq = GetAttackingSquare(s, capture_sq);
							if (att_sq > -1)
							{
								capture_check = IsCheck(att_sq, capture_sq, kingloc[xs]);
								if (!capture_check)
								{
									bit_threshold = GetThreshold(s, b[capture_sq]);
									counter_flag = false;
									if (piece > 0)
									{
										counter_flag = CounterAttack(from, to, piece, bit_threshold & not_mask[sq]);
									}
									else
									{
										counter_flag = PawnCounterAttack(from, to, bit_threshold & not_mask[sq]);
									}
								}
								if (counter_flag == false)
								{
									move_list[i].score -= p_value[b[capture_sq]] * BLUNDER_SCORE;
								}
							}
						}
					}
				}
			}
			if (move_list[i].score < 0)
			{
				i--;
				UnMakeQuietMove();
				continue;
			}
		}

		if (move_list[i].score >= 0 && bit_targets == 0)//moving defensive piece
		{
			counter_flag = false;
			if (piece == 0)
			{
				if (row2[xs][to] < 6)
				{
					BITBOARD b1 = bit_pawncaptures[xs][from] & bit_defended[xs][ply - 1];
					while (b1)
					{
						int sq = NextBit(b1);
						int att_sq = GetAttackingSquare(s, sq);
						if (att_sq > -1)
						{
							if (!Attack(xs, sq))
							{
								capture_check = IsCheck(att_sq, sq, kingloc[xs]);
								if (!capture_check)
								{
									bit_threshold = GetThreshold(s, b[sq]) & not_mask[att_sq];
									counter_flag = PawnCounterAttack(from, to, bit_threshold);
								}
								if (counter_flag == false)
								{
									move_list[i].score -= p_value[b[sq]] * BLUNDER_SCORE;
									break;
								}
							}
							else
							{
								int see = SEE(s, att_sq, sq, 0, 0);
								if (see > 0)
								{
									move_list[i].score -= p_value[b[sq]] * BLUNDER_SCORE;
									break;
								}
							}
						}
						b1 &= b1 - 1;
					}
				}
				if (move_list[i].score < 0)
				{
					i--;
					UnMakeQuietMove();
					continue;
				}
			}
			else
			{
				counter_flag = false;
				BITBOARD b1 = bit_moves[piece][from] & bit_defended[xs][ply - 1];
				while (b1)
				{
					int sq = NextBit(b1);
					b1 &= b1 - 1;
					if (!(bit_between[from][sq] & bit_all))
					{
						int att = GetAttackingSquare(s, sq);
						if (att > -1)
						{
							if (!Attack(xs, sq))
							{
								capture_check = IsCheck(att, sq, kingloc[xs]);
								if (!capture_check)
								{
									bit_threshold = GetThreshold(s, b[sq]) & not_mask[att];
									counter_flag = CounterAttack(from, piece, to, bit_threshold);
								}
								if (counter_flag == false)
								{
									move_list[i].score -= p_value[b[sq]] * BLUNDER_SCORE;
									break;
								}
							}
							else
							{
								int see = SEE(s, att, sq, 0, 0);
								if (see > 0)
								{
									break;
								}
							}
						}
					}
					if (move_list[i].score < 0)
					{
						i--;
						UnMakeQuietMove();
						continue;
					}
				}
			}
		}
	}
	if (depth == 2 && piece_mat[s] > 3 && count > 0)
	{
		if (ev1 == INVALID)
		{
			ev1 = -Eval(alpha, beta);
		}
		ev = ev1 + PieceScore[xs][piece][to] - PieceScore[xs][piece][from] + frontier[piece] +
			frontier[game_list[hply - 1].capture];
		if (ev <= alpha && BestThreat(xs, s, alpha - ev) == 0)// ( && b[to] != K))
		{
			UnMakeQuietMove();
			continue;
		}
	}
	if (depth == 3 && piece_mat[s] > 3 && count > 0)
	{
		if (ev1 == INVALID)
		{
			ev1 = -Eval(alpha, beta);
		}

		if (ev1 + 400 <= alpha && BestThreat(xs, s, alpha - ev) == 0)// ( && b[to] != K))
		{
			if (!(bit_pieces[xs][P] & mask_ranks[xs][6]))
			{
				UnMakeQuietMove();
				continue;
			}
		}
	}
	if (depth == 4 && piece_mat[s] > 3 && count > 0)
	{
		if (ev1 == INVALID)
		{
			ev1 = -Eval(alpha, beta);
		}

		if (ev1 + 800 <= alpha && BestThreat(xs, s, alpha - ev) == 0)// ( && b[to] != K))
		{
			if (!(bit_pieces[xs][P] & mask_ranks[xs][6]))
			{
				UnMakeQuietMove();
				continue;
			}
		}
	}
	if (quiet_count > 0 && depth > 2)
	{
		r = Reduce(i);
		//if (b[to] == 0 && row2[s][to] == 5)
		//	r = 0;//
		if (r == 1)
		{
			if (ev1 == INVALID)
				ev1 = Eval(alpha, beta);
			//??if (depth > 4 && alpha - ev1 >= 200)
			//	r = 2;
			if (alpha - ev1 >= 800)
				;// printf("+");
		//	r = 3;
		}
		//if (quietIndex < 4) return 0;
		//if (depth >= 10 && quiet_count >= 12) r = 2;
		//if (depth >= 12 && quiet_count >= 20) r = 3;
		//*
		if (depth >= 10 && !pvs && !(flags & ATTACK) && count >= 12)
		{
			if (move_score < KILLER2_SCORE) 
			{ r = 2; }
			//if (quiet_count > 16 && move_score < 20000)   r = 3;   // truly awful tail
			//if (tactical_flag)                         r = 0;   // your threat/en-prise check says volatile
		}
		//*/
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
	/*
	else
	{
		score = -Search(-alpha - 1, -alpha, rd, NO_PV, DO_NULL);

		if (score > alpha && score < beta)
		{
			extend[ply] = 0;
			score = -Search(-alpha - 1, -alpha, d, NO_PV, DO_NULL);

			if (score > alpha)
				score = -Search(-beta, -alpha, d, PV, DO_NULL);
		}
	}
	//*/
	//*
	else
	{
		// LMR trial
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

	s = side;
	xs = xside;

	if (score > bestscore)
	{
		bestscore = score;
		bestmove = move_list[i];
	}
	if (score > alpha)
	{
		if (score >= beta)
		{
			UpdateSequence(ply, depth, move_list[i]);
			UpdateKillers(i, from, to, score, depth);
			AddHash(s, depth, score, BETA, from, to, flags);
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
			if (flag==1 && depth == 1)
			{
				printf(" diff %d ", diff);
				printf(" ev1 %d ", ev1);
				printf(" alpha %d ", alpha);
				printf(" score %d ", score);
				Alg(from, to);
				printf(" xxx ");
				z();
			}
			if (diff > bestcut && depth == 1)
			{
				bestcut = diff;
				if (diff > 50)
				{
					;
				}
			}

			av_nodes += count;
			//if (game_list[hply - 1].flags & MATETHREAT)

			if (nodes > 1 && quiet_count > 7 && depth > 1)// && move_list[i].score < 0)
			{
				//debug++;
				/*
				file << " count " << debug << '\n';
				file << " move " << quiet_count << '\n';
				file << " startmoves " << startmoves << '\n';
				file << " endmoves " << endmoves << '\n';

				file << " score " << move_list[i].score << '\n';

				file << " nodes " << nodes << '\n';

				file << " depth " << depth << '\n';

				file << " pvs " << pvs << '\n';
				file << '\n';
				*/
				/*
				if (nodes > 100000)
				{
					cout << " nodes " << nodes << '\n';
					cout << " depth " << depth << '\n';
					printf("\n move %d ", quiet_count);
					Alg(from, to); printf(" score %d \n", move_list[i].score);

					DisplayBoard();
					cout << " number " << endmoves - startmoves << '\n';
					for (int j = startmoves; j < endmoves; j++)
					{
						from = move_list[j].from;
						to = move_list[j].to;
						Alg(from, to);
						printf(" score %d \n", move_list[j].score);
					}
					_getch();
				}
				//*/
			}
			return beta;
		}
		UpdatePV(move_list[i]);
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

if (depth == 1 && old_alpha < alpha && flag == 1)
{
	printf(" diff %d ", diff);
	printf(" ev1 %d ", ev1);
	printf(" alpha %d ", alpha);
	printf(" score %d ", score);
	Alg(from, to);
	z();
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

void SortChecks(const int from, const int last)
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

static void CheckUp2()
{
	if (fixed_depth != 0)
		;// return;

	if (GetTime() >= stop_time)// || (max_time < 50 && ply > 1))
	{
		stop_search = true;
		longjmp(env, 1);              // must be non-zero
	}
}

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
	//if (piece_mat[side] <= 3 && fifty > 8)
		//cout << "+";
	//	return 2;
	if (piece_mat[side] <= 3 && b[move_list[i].to] == K)
	{
		return 0;
	}
	return 1;
}

void DisplayPV()
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
	int attacker = GetNextAttacker(side, xside, target, bit_all);
	if (attacker == -1)
		return -1;

	int count = 0;

	if (p_value[b[attacker]] < p_value[b[target]] ||
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
				if (p_value[piece] < p_value[b[counter_sq]] ||
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
	if (diff >= QVAL)
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

	if (diff < RVAL)//Rooks are potentially a target.
	{
		bit_targets |= bit_pieces[xs][R];
		if (diff < BVAL)//Minor pieces are potentially a target.
		{
			bit_targets |= bit_pieces[xs][N] | bit_pieces[xs][B];
			if (diff < PVAL)//Pawns are potentially a target.
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
	if (piece_mat[side] < 9)//endgames
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
		if (p_value[att] < p_value[Q])
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

	int def1 = GetLowestAttacker(s, sq);

	// If no defender → already handled earlier
	if (def1 < 0) return true;

	// Net after first recapture
	int gain = p_value[piece] - p_value[att1];

	// Defender recaptures attacker
	gain -= p_value[def1];

	// Remove def1 temporarily and look for second attacker
	int att2 = GetNextAttacker(xs, s, sq,
		bit_all & ~mask[GetAttackingSquare(s, sq)]);

	if (att2 > -1)
		gain += p_value[def1] - p_value[att2];

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

static inline int ContinuationScore(int hply, int piece, int to)
{
	int s = 0;

	if (hply > 0)
	{
		const int p0 = game_list[hply - 1].piece;
		const int t0 = game_list[hply - 1].to;
		if (p0 < 0 || t0 < 0 || piece < 0 || to < 0 || p0>5 || t0>63 || piece>5 || to>63)
		{
			printf(" bug ");
			printf(" p %d ", p0);
			printf(" t0 %d ", t0);
			printf(" to %d ", to);
			printf(" piece %d ", piece);
			z();
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

