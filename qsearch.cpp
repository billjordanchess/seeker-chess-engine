#include <stdlib.h>
#include "globals.h"

constexpr int  STALEMATE = 0;

const int MAX1 = MAX_PLY - 1;
const int MAX2 = MAX_PLY - 2;

extern int deep;
extern int currentdepth;
extern BITBOARD bit_weaker[2][6];
//extern BITBOARD bit_line_attackers[MAX_PLY];

bool LineAttack2(const int s, const int sq, const BITBOARD occ);

BITBOARD GetKnightAttacks(const int s);

void RemoveBlunders(const int s, const int xs, const int startmoves, BITBOARD);
int GetTarget(const int s, const int xs);

int Sort(const int from, const int, const int);
void SelectCapture(const int from, const int last);
int RecaptureSearch(int s, const int attacker, const int sq, BITBOARD p1, BITBOARD p2, const int, const int, const int, const int);

int check_history[6][64];

void z();//

void AddRecapture(const int from, const int to);

BITBOARD GetPinMask(const int s, const int xs);
BITBOARD GetPinBetween(const int s, const int xs);

void ShowMoves(int);

bool MakeCapture(const int, const int, const int);
void UnMakeCapture();
void MakeRecapture(const int, const int);
void UnMakeRecapture();
bool MakeEvasion(const int from, const int to);
void UnMakeEvasion();
int QuietEvasion(int alpha, int beta, BITBOARD);
bool IsMate(const int checker);

int CaptureSearch(int alpha, const int beta);

int BlockedPawns(const int s);
int SafeKingMoves(const int, const int);

BITBOARD PinnersPossible(const int s, const int xs);
BITBOARD GetDiscoCaptures(const int s, const int xs);

int GetNextAttackerSquarePins(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);

extern int pv_len[MAX_PLY];

int QuietSearch(int alpha, int beta)
{
	pv_len[ply] = 0;
	if (ply > MAX2)
		return Eval(side, xside, alpha, beta);

	if (Attack(xside, kingloc[side]))
	{
		BITBOARD pin_mask = GetPinMask(side, xside);
		return QuietEvasion(alpha, beta, pin_mask);
	}

	if (piece_mat[side] == 0 && BlockedPawns(side) == 1)
	{
		if (SafeKingMoves(side, xside) == 0)
			return 0;
	}
	return CaptureSearch(alpha, beta);
}

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
		score = -CaptureSearch(-beta, -alpha);
		UnMakeCapture();

		if (score > alpha)
		{
			if (score >= beta)
			{
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
	
		score = -CaptureSearch(-beta, -alpha);

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

int CaptureSearch(int alpha, const int beta)
{	
	if (Attack(xside, kingloc[side]))
	{
		BITBOARD pin_mask = GetPinMask(side, xside);
		return QuietEvasion(alpha, beta, pin_mask);
	}

	int eval = Eval(side, xside, alpha, beta);//23/2/26 seekerx

	if (eval >= beta)
	{
		return beta;
	}

	BITBOARD pinners[2];
	BITBOARD pin_mask = 0;
	BITBOARD xpin_mask = 0;

	pinners[side] = PinnersPossible(side, xside);
	pinners[xside] = PinnersPossible(xside, side);

	if (pinners[side])
	{
		pin_mask = GetPinMask(side, xside);
	}
	if (pinners[xside])
	{
		xpin_mask = GetPinMask(xside, side);
	}
	/*
	if (pin_mask)
	{
		PrintBitBoard(pinners[side]);
		PrintBitBoard(pin_mask);
		printf(" pin ");
		//z();
	}
	/*
	if (xpin_mask)
	{
		PrintBitBoard(pinners[xside]);
		PrintBitBoard(xpin_mask);
		printf(" xpin ");
		//z();
	}
	//*/
	int diff;
	if (alpha > -10000)//
		diff = alpha - eval;
	else
		diff = 0;
	
	GenQuietCaptures(side, xside, diff, pin_mask, xpin_mask);//

	if (eval > alpha)
		alpha = eval;

	if (first_move[ply] == first_move[ply + 1])
	{
		return alpha;
		//return -alpha;
	}
	
	int from, to, flags, lowest;
	int capture_score = 0;
	nodes++;
	qnodes++;

	const int k = kingloc[side];
	
	BITBOARD disco_mask = GetDiscoCaptures(side, xside);

	int fail = 0;

	RemoveBlunders(side, xside, first_move[ply], disco_mask);

	int score = 0;

	for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
	{
		SelectCapture(i, first_move[ply + 1]);
		from = move_list[i].from;
		to = move_list[i].to;
		flags = move_list[i].flags;

		lowest = GetLowestAttacker(xside, to);

		flags = move_list[i].flags;

		if (lowest == -1)
		{
			MakeCapture(from, to, flags);
			if (Attack(xside, kingloc[side]))
			{
				int check = Check(xside, kingloc[side]);
				if (IsMate(check))
				{
					UnMakeCapture();
					return 10000 - ply;
				}
				score = -Eval(side, xside, -beta, -alpha);
				UnMakeCapture();
			}
			else
			{
				score = -Eval(side, xside, -beta, -alpha);
				UnMakeCapture();
			}
		}
		else
		{
			int attack_sq;
			if ((pin_mask & bit_units[side])== 0)
			{
				attack_sq = GetNextAttackerSquare(side, xside, to, bit_all & ~mask[from]);
			}
			else
			{
				//attack_sq = GetNextAttackerSquarePins(side, xside, to, bit_all & ~mask[from], pin_mask);
				attack_sq = GetNextAttackerSquare(side, xside, to, bit_all & ~mask[from]);//??
			}

			if (attack_sq == -1)
			{
				if (piece_value[b[from]] > piece_value[b[to]])
				{
					continue;
				}
				//else
				{
					//printf("+");
					/*
					attack_sq = GetNextAttackerSquare(xside, side, to, bit_all & ~mask[from]);
					MakeRecapture(from, to);
					MakeRecapture(attack_sq, to);
					if (LineAttack(side, kingloc[xside]))
					{
						printf(" line ");
						Alg(from, to);
						z();
						LineAttack(side, kingloc[xside]);
					}
					score = Eval(side, xside, alpha, beta);
					//printf(" ev %d ", score);
					//Alg(from, to);
					//z();
					UnMakeRecapture();
					UnMakeRecapture();
					//*/
				}
			}
			//else
			{
				score = RecaptureSearch(side, from, to, pin_mask, xpin_mask, eval, alpha, beta, flags);
					/*
					if (score > eval)
					{
						printf(" eval %d ", eval);
						printf(" ev %d ", score);
						Alg(from, to);
						//z();
					}
					//*/
			}
		}

		if (ply > deep)
		{
			deep = ply;
		}

		if (score > alpha)
		{
			if (score >= beta)
			{
				//Alg(from, to);
				//z();
				return beta;
			}
			alpha = score;
		}
	}
	//ShowAll(ply);
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

BITBOARD GetPinBetween(const int s, const int xs)
{
	int	k = kingloc[s];
	BITBOARD b1 = bit_rookmoves[k] & (bit_pieces[xs][R] | bit_pieces[xs][Q]);
	b1 |= bit_bishopmoves[k] & (bit_pieces[xs][B] | bit_pieces[xs][Q]);
	BITBOARD pin_between = 0;
	while (b1)
	{
		int from = NextBit(b1);
		pin_between |= mask[from];
		pin_between |= bit_between[from][k];
		b1 &= b1 - 1;
	}
	return pin_between;
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
	int val = piece_value[b[target]];

	if (target > -1)
	{
		int from, to, flags, piece, score;
		int attacker = GetNextAttackerSquare(xs, s, target, bit_all);
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
			if (piece_value[b[to]] >= val)
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
			int attacker = GetNextAttackerSquare(xs, s, sq, bit_all);
			if (attacker > -1 && piece_value[b[attacker]] < piece_value[p])
			{
				return sq;
			}
		}
	}
	return -1;
}

int SafeKingMoves(const int s, const int xs)
{
	int sq;
	int	i = kingloc[s];
	BITBOARD b1 = bit_kingmoves[i] & ~bit_units[s];
	while (b1)
	{
		sq = NextBit(b1);
		b1 &= b1 - 1;
		if (!(Attack(xs, sq)))
			return 1;
	}
	return 0;
}
