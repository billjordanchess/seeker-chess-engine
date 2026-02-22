#include <stdlib.h>
#include "globals.h"

constexpr int  STALEMATE = 0;

const int MAX1 = MAX_PLY - 1;
const int MAX2 = MAX_PLY - 2;

extern int deep;
extern int currentdepth;
extern BITBOARD bit_weaker[2][6];
//extern BITBOARD bit_line_attackers[MAX_PLY];

BITBOARD GetKnightAttacks(const int s);

void RemoveBlunders(const int s, const int xs, const int startmoves, BITBOARD);
int GetTarget(const int s, const int xs);

int Sort(const int from, const int, const int);
void SelectCapture(const int from, const int last);
int SEEQuiet(int s, const int attacker, const int sq, BITBOARD p1, BITBOARD p2, const int, const int, const int, const int);

int check_history[6][64];

void z();//

void AddRecapture(const int from, const int to);

BITBOARD GetPinMask(const int s, const int xs);

void ShowMoves(int);

bool MakeCapture(const int, const int, const int);
void UnMakeCapture();
void MakeRecapture(const int, const int);
void UnMakeRecapture();
bool MakeEvasion(const int from, const int to);
void UnMakeEvasion();
int QuietEvasion(int alpha, int beta, BITBOARD);

int CaptureSearch(const int, const int, int alpha, const int beta, const int);

int BlockedPawns(const int s);

BITBOARD PinnersPossible(const int s, const int xs);
BITBOARD GetDiscoCaptures(const int s, const int xs);

extern int pv_len[MAX_PLY];
/*
static int c = CAPTURE_SCORE;
static const int px[6] = { 0 + c,c + 100,c + 200,c + 300,c + 400,0 + c };
static const int nx[6] = { c - 30,c + 70,c + 170,c + 270,c + 370,c + 0 };
static const int bx[6] = { c - 30,c + 70,c + 170,c + 270,c + 370,c + 0 };
static const int rx[6] = { c - 50,c + 50,c + 150,c + 250,c + 350,c + 0 };
static const int qx[6] = { c - 90,c + 10,c + 110,c + 210,c + 310,c + 0 };
static const int kx[6] = { c + 00,c + 100,c + 200,c + 300,c + 400,c + 0 };
*/

int QuietEvasion(int alpha, int beta, BITBOARD pin_mask)
{
	first_move[ply + 1] = first_move[ply];

	int count = 0;
	int top = HASH_SCORE;
	int start = first_move[ply];
	int check = Check(xside, kingloc[side]);

	EvadeCapture(side, xside, check, pin_mask);
	game_list[hply].flags |= INCHECK;
	int end = first_move[ply + 1];

	int from, to, score;
	int flags;

	for (int i = start; i < end; i++)
	{
		top = Sort(i, top, end);
		from = move_list[i].from;
		to = move_list[i].to;

		if (!MakeCapture(from, to, move_list[i].flags))
		{
			continue;
		}

		count++;
		score = -QuietSearch(side, xside, -beta, -alpha);
		UnMakeCapture();

		if (score > alpha)
		{
			if (score >= beta)
			{
				//ShowAll(ply);
				return beta;
			}
			alpha = score;
		}
	}

	start = first_move[ply + 1];

	EvadeQuiet(side, xside, check, pin_mask);

	end = first_move[ply + 1];

	for (int i = start; i < end; i++)
	{
		top = Sort(i, top, end);

		from = move_list[i].from;
		to = move_list[i].to;
		flags = move_list[i].flags;

		if (!MakeEvasion(from, to))
		{
			continue;
		}

		count++;

		score = -QuietSearch(side, xside, -beta, -alpha);
		UnMakeEvasion();

		if (score > alpha)
		{
			if (score >= beta)
			{
				return beta;
			}
			alpha = score;
		}
	}
	if (count == 0)
	{
		return ply - 10000;
	}
	return alpha;
}

int QuietSearch(const int s, const int xs, int alpha, int beta)
{
	pv_len[ply] = 0;
	if (ply > MAX2)
		return Eval(alpha, beta);

	if (Attack(xs, kingloc[s]))
	{
		BITBOARD pin_mask = GetPinMask(s, xs);
		return QuietEvasion(alpha, beta, pin_mask);
	}

	if (piece_mat[s] == 0 && BlockedPawns(s) == 1)
	{
		if (SafeKingMoves(s, xs) == 0)
			return 0;
	}

	int eval = Eval(alpha, beta);

	if (eval >= beta)
	{
		return beta;
	}
	return CaptureSearch(s, xs, alpha, beta, eval);
}

int CaptureSearch(const int s, const int xs, int alpha, const int beta, const int eval)
{
	int score = eval;

	BITBOARD pinners[2];
	BITBOARD pin_mask = 0;
	BITBOARD xpin_mask = 0;

	pinners[s] = PinnersPossible(s, xs);
	pinners[xs] = PinnersPossible(xs, s);

	if (pinners[s])
	{
		pin_mask = GetPinMask(s, xs);
	}
	if (pinners[xs])
	{
		xpin_mask = GetPinMask(xs, s);
	}
	/*
	if (pin_mask)
	{
		PrintBitBoard(pinners[s]);
		PrintBitBoard(pin_mask);
		printf(" pin ");
		z();
	}
	/*
	if (xpin_mask)
	{
		PrintBitBoard(pinners[xs]);
		PrintBitBoard(xpin_mask);
		printf(" xpin ");
		z();
	}
	//*/
	int diff;
	if (alpha < 10000)
		diff = alpha - score;
	else
		diff = 0;
	GenQuietCaptures(s, xs, diff, pin_mask, xpin_mask);//

	if (score > alpha)
		alpha = score;

	if (first_move[ply] == first_move[ply + 1])
	{
		return alpha;
	}
	
	int from, to, piece, flags, lowest;
	int capture_score = 0;
	nodes++;
	qnodes++;

	const int k = kingloc[s];
	
	BITBOARD disco_mask = GetDiscoCaptures(s, xs);

	int fail = 0;

	RemoveBlunders(s, xs, first_move[ply], disco_mask);

	for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
	{
		SelectCapture(i, first_move[ply + 1]);
		from = move_list[i].from;
		to = move_list[i].to;
		flags = move_list[i].flags;
		/*
		if (fail > 0 && p_value[b[to]] < fail && !(flags & CHECK))
		{
			printf(" pass ");
			Alg(from, to);
			ShowAll(ply);
			continue;
		}
		//*/

		lowest = GetLowestAttacker(xs, to);

		flags = move_list[i].flags;

		if (lowest == -1)
		{
			MakeCapture(from, to, flags);
			score = -Eval(-beta, -alpha);
			UnMakeCapture();
		}
		else
		{
			score = SEEQuiet(s, from, to, pin_mask, xpin_mask, eval, alpha, beta, flags);
			/*
			if (score > eval)
			{
				printf(" eval %d ", eval);
				printf(" ev %d ", score);
				Alg(from, to);
				z();
			}
			//*/
		}

		if (ply > deep)
		{
			deep = ply;
		}

		if (score > alpha)
		{
			if (score >= beta)
			{
				return beta;
			}
			alpha = score;
		}
		if (lowest == -1)
		{
			//printf(" fail ");
			//Alg(from, to);
			//z();	
			//fail = p_value[b[to]];
		}
	}
	return alpha;
}

int BlockedPawns(const int s)
{
	if (s == 0)
	{
		if (bit_pieces[0][P] & (~(bit_units[0] | bit_units[1])) >> 8)
			return 0;
	}
	else if (bit_pieces[1][P] & (~(bit_units[0] | bit_units[1])) << 8)
		return 0;
	return 1;
}

void SelectCapture(const int from, const int last)
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

BITBOARD PinnersPossible(const int s, const int xs)
{
	int	k = kingloc[s];
	BITBOARD b1 = bit_rookmoves[k] & (bit_pieces[xs][R] | bit_pieces[xs][Q]);
	b1 |= bit_bishopmoves[k] & (bit_pieces[xs][B] | bit_pieces[xs][Q]);
	return b1;
}

BITBOARD GetPinMask(const int s, const int xs)
{
	int	k = kingloc[s];
	BITBOARD b1 = bit_rookmoves[k] & (bit_pieces[xs][R] | bit_pieces[xs][Q]);
	b1 |= bit_bishopmoves[k] & (bit_pieces[xs][B] | bit_pieces[xs][Q]);
	BITBOARD pin_mask = 0;
	while (b1)
	{
		int from = NextBit(b1);
		BITBOARD b2 = bit_between[from][k] & bit_units[s];
		if (IsOneBit(b2) && !(bit_between[from][k] & bit_units[xs]))
		{
			pin_mask |= mask[from];
			pin_mask |= mask[NextBit(b2)];
		}
		b1 &= b1 - 1;
	}
	return pin_mask;
}

BITBOARD GetDiscoCaptures(const int s, const int xs)
{
	int	k = kingloc[xs];
	BITBOARD b1 = bit_rookmoves[k] & (bit_pieces[s][R] | bit_pieces[s][Q]);
	b1 |= bit_bishopmoves[k] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	BITBOARD disco_mask = 0;
	while (b1)
	{
		int to = NextBit(b1);
		BITBOARD b2 = bit_between[to][k] & bit_units[s];
		if (IsOneBit(b2) && !(bit_between[to][k] & bit_units[xs]))
		{
			disco_mask |= mask[NextBit(b2)];
			//printf(" disco ");
			//Alg(NextBit(b2), k);
			//z();
		}
		b1 &= b1 - 1;
	}
	return disco_mask;
}

void RemoveBlunders(const int s, const int xs, const int startmoves, BITBOARD disco_mask)
{
	int start = startmoves;
	int end = first_move[ply + 1];

	int write = start;

	int target = GetTarget(s, xs);
	int val = p_value[b[target]];

	if (target > -1)
	{
		int from, to, flags, piece, score;
		int attacker = GetNextAttacker(xs, s, target, bit_all);
		/*
		printf(" rm ");
		Algebraic(target);
		ShowAll(ply);
		//*/

		if (disco_mask)
		{
			//PrintBitBoard(disco_mask);
			//z();
		}
		for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			flags = move_list[i].flags;
			score = move_list[i].score;

			piece = b[from];
			if (to == attacker)
			{
				continue;
			}
			if (from == target)
			{
				continue;
			}
			if (p_value[b[to]] >= val)
			{
				continue;
			}
			if (flags & CHECK)
			{
				continue;
			}
			if (mask[from] & disco_mask)
			{
				//printf(" disco %d ", score);
				//Alg(from, to);
				//z();
				continue;
			}
			move_list[i].score = 0;
		}
		for (int read = start; read < end; ++read)
		{
			move_data m = move_list[read];

			if (move_list[read].score == 0)
			{
				continue;
			}
			move_list[write++] = m;
		}

		first_move[ply + 1] = write;

		//printf(" after ");
		//ShowAll(ply);
	}
}

int GetTarget(const int s, const int xs)
{
	for (int p = Q; p > 0; p--)
	{
		for (int x = 0; x < total[s][p]; x++)
		{
			int sq = pieces[s][p][x];
			int attacker = GetNextAttacker(xs, s, sq, bit_all);
			if (attacker > -1 && p_value[b[attacker]] < p_value[p])
			{
				return sq;
			}
		}
	}
	return -1;
}