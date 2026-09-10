//11/9/26
#include <stdlib.h>
#include "globals.h"

constexpr int STALEMATE = 0;

const int MAX1 = MAX_PLY - 1;
const int MAX2 = MAX_PLY - 2;

extern int deep;
extern int currentdepth;
extern BITBOARD bit_weaker[2][6];

bool LineAttack2(const int s, const int sq, const BITBOARD occ);

BITBOARD GetKnightAttacks(const int s);

void RemoveBlunders(const int s, const int xs, const int startmoves, BITBOARD);
int GetTarget(const int s, const int xs);

int SelectMove(const int from, const int, const int);
void SelectCapture(const int from, const int last);
int RecaptureSearch(int s, const int attacker, const int, const int sq, BITBOARD p1, BITBOARD p2);

void z();//

BITBOARD GetPinMask(const int s, const int xs);

void ShowMoves(int);

void MakeCapture(const int, const int, const int);
void UnMakeCapture();
void MakeEvasion(const int from, const int to);
void UnMakeEvasion();
int QuietEvasion(int alpha, int beta, BITBOARD);
bool IsMate(const int checker);

int CaptureSearch(int alpha, const int beta);

BITBOARD PinnersPossible(const int s, const int xs);
BITBOARD GetDiscoCaptures(const int s, const int xs);

int RecaptureFrom(const int s, const int, const int sq, const BITBOARD);
int RecaptureFromPins(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);
static bool AttackKing(const int s, const int king, const BITBOARD bit_occ);

int QuietSearch(int alpha, int beta)
{
	if (ply > MAX2)
		return Eval(alpha, beta);
	return CaptureSearch(alpha, beta);
}

int QuietEvasion(int alpha, int beta, BITBOARD pin_mask)
{
	first_move[ply + 1] = first_move[ply];

	int count = 0;
	int top = HASH_SCORE;
	int start = first_move[ply];
	const int check = Check(xside, kingloc[side]);

	EvadeCapture(side, xside, check, pin_mask);
	game_list[hply].flags |= INCHECK;
	int end = first_move[ply + 1];

	int score;
	int flags;

	for (int i = start; i < end; i++)
	{
		top = SelectMove(i, top, end);
		int from = move_list[i].from;
		int to = move_list[i].to;

		MakeCapture(from, to, move_list[i].flags);
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
		top = SelectMove(i, top, end);

		int from = move_list[i].from;
		int to = move_list[i].to;
		flags = move_list[i].flags;

		MakeEvasion(from, to);

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
	if (Attack(xside, kingloc[side], bit_all))
	{
		BITBOARD pin_mask = GetPinMask(side, xside);
		return QuietEvasion(alpha, beta, pin_mask);
	}

	const int eval = Eval(alpha, beta);

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

	int diff = 0;

	if (alpha > -10000)
		diff = alpha - eval;

	GenQuietCaptures(side, xside, diff, pin_mask, xpin_mask);

	if (eval > alpha)
		alpha = eval;

	if (first_move[ply] == first_move[ply + 1])
	{
		return alpha;
	}

	int capture_score = 0;
	nodes++;
	qnodes++;

	const int k = kingloc[side];

	BITBOARD disco_mask = GetDiscoCaptures(side, xside);

	int fail = 0;

	RemoveBlunders(side, xside, first_move[ply], disco_mask);

	for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
	{
		SelectCapture(i, first_move[ply + 1]);
		const int from = move_list[i].from;
		const int to = move_list[i].to;
		const unsigned int flags = move_list[i].flags;
		int score = -10000;
		int defender_sq;

		if (pin_mask == 0)
			defender_sq = RecaptureFrom(xside, side, to, bit_all);
		else
			defender_sq = RecaptureFromPins(xside, side, to, bit_all, pin_mask);

		if (defender_sq == -1)
		{
			MakeCapture(from, to, flags);

			if (Attack(xside, kingloc[side], bit_all))
			{
				int check = Check(xside, kingloc[side]);
				if (IsMate(check))
				{
					UnMakeCapture();
					return 10000 - ply;
				}
			}

			score = -Eval(-beta, -alpha);
			UnMakeCapture();
		}
		else
		{
			score = eval + RecaptureSearch(side, from, to, defender_sq, pin_mask, xpin_mask);
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
	}
	return alpha;
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
	const int king = kingloc[s];
	BITBOARD b1 = bit_moves[R][king] & (bit_pieces[xs][R] | bit_pieces[xs][Q]);
	b1 |= bit_moves[B][king] & (bit_pieces[xs][B] | bit_pieces[xs][Q]);
	return b1;
}

BITBOARD GetPinMask(const int s, const int xs)
{
	const int king = kingloc[s];
	BITBOARD b1 = bit_moves[R][king] & (bit_pieces[xs][R] | bit_pieces[xs][Q]);
	b1 |= bit_moves[B][king] & (bit_pieces[xs][B] | bit_pieces[xs][Q]);
	BITBOARD pin_mask = 0;
	while (b1)
	{
		int from = NextBit(b1);
		BITBOARD b2 = bit_between[from][king] & bit_units[s];
		if (IsOneBit(b2) && !(bit_between[from][king] & bit_units[xs]))
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
	const int king = kingloc[xs];
	BITBOARD b1 = bit_moves[R][king] & (bit_pieces[s][R] | bit_pieces[s][Q]);
	b1 |= bit_moves[B][king] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	BITBOARD disco_mask = 0;
	while (b1)
	{
		int to = NextBit(b1);
		BITBOARD b2 = bit_between[to][king] & bit_units[s];
		if (IsOneBit(b2) && !(bit_between[to][king] & bit_units[xs]))
		{
			disco_mask |= mask[NextBit(b2)];
		}
		b1 &= b1 - 1;
	}
	return disco_mask;
}

void RemoveBlunders(const int s, const int xs, const int startmoves, BITBOARD disco_mask)
{
	const int start = startmoves;
	const int end = first_move[ply + 1];

	int write = start;

	const int target = GetTarget(s, xs);

	if (target > -1)
	{
		int from, to, flags;
		const int val = piece_value[b[target]];
		const int attacker = RecaptureFrom(xs, s, target, bit_all);

		for (int i = start; i < end; i++)
		{
			from = move_list[i].from;
			to = move_list[i].to;
			flags = move_list[i].flags;

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
	}
}

int GetTarget(const int s, const int xs)
{
	for (int piece = Q; piece > P; piece--)
	{
		for (int x = 0; x < total[s][piece]; x++)
		{
			int sq = pieces[s][piece][x];
			int attacker = RecaptureFrom(xs, s, sq, bit_all);
			if (attacker > -1 && piece_value[b[attacker]] < piece_value[piece])
			{
				return sq;
			}
		}
	}
	return -1;
}

int RecaptureSearch(int s, const int attacker, const int sq, const int defender,
	const BITBOARD p1, const BITBOARD p2)
{
	const int start_side = side;

	int attack_sq = attacker;
	int value[16];
	int count = 0;

	memset(value, 0, sizeof(value));

	BITBOARD bit_occ = bit_all;
	BITBOARD pins[2];

	value[0] = piece_value[b[sq]];
	value[1] = piece_value[b[attacker]];
	value[2] = piece_value[b[defender]];

	pins[0] = p1;
	pins[1] = p2;

	if (b[attacker] == P && row2[s][sq] == 7)
	{
		value[0] = 800;
		value[1] = 900;
		value[2] = piece_value[defender];
	}

	bit_occ &= ~mask[attacker];
	attack_sq = defender;
	s ^= 1;
	count = 2;

	while (count < 12)
	{
		bit_occ &= ~mask[attack_sq];
		s ^= 1;

		if (pins[s] & bit_units[s] && !(mask[sq] & pins[s]))
		{
			if (IsOneBit(pins[s] & bit_occ & ~bit_units[s ^ 1]))
			{
				attack_sq = RecaptureFromPins(s, s ^ 1, sq, bit_occ, pins[s]);
				//attack_sq = RecaptureFrom(s, s ^ 1, sq, bit_occ, bit_occ & ~pins[s]);

				if (attack_sq > -1)
				{
					//PrintBitBoard(pins[s]);
					//Alg(attack_sq, sq);
					//z();
				}
			}
			else
				attack_sq = RecaptureFrom(s, s ^ 1, sq, bit_occ);
		}
		else
			attack_sq = RecaptureFrom(s, s ^ 1, sq, bit_occ);
		//attack_sq = RecaptureFrom(s, s ^ 1, sq, bit_occ, bit_occ & ~pins[s]);

		if (attack_sq == -1)
		{
			if (count > 2 && value[count - 1] > value[count - 2])
			{
				count -= 3;
				break;
			}
			count--;
			break;
		}
		if (b[attack_sq] == K)
		{
			if (value[count] > value[count - 1])
			{
				count -= 2;
				break;
			}
			count++;
			break;
		}
		if (value[count] > value[count - 1] + piece_value[b[attack_sq]])
		{
			count -= 2;
			break;
		}
		count++;
		value[count] = piece_value[b[attack_sq]];
	}

	int score = value[0];

	for (int x = 1; x <= count; x++)
	{
		if (x % 2 == 0)
			score += value[x];
		else
			score -= value[x];
	}
	return score;
}

int RecaptureFrom(const int s, const int xs, const int sq, const BITBOARD bit_occ)
{
	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		return pawnleft[xs][sq];
	}
	else if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		return pawnright[xs][sq];
	}

	BITBOARD b1 = bit_moves[N][sq] & bit_pieces[s][N] & bit_occ;
	if (b1)
		return NextBit(b1);

	b1 = MagicBishopAttacks(sq, bit_occ);
	U64 a = b1 & bit_pieces[s][B] & bit_occ;
	if (a)
		return NextBit(a);

	U64 b2 = MagicRookAttacks(sq, bit_occ);
	a = b2 & bit_pieces[s][R] & bit_occ;
	if (a)
		return NextBit(a);

	a = (b1 | b2) & bit_pieces[s][Q] & bit_occ;
	if (a)
		return NextBit(a);

	if (bit_moves[K][sq] & bit_pieces[s][K])
	{
		if (AttackKing(xs, sq, bit_occ) == 0)
			return kingloc[s];
	}
	return -1;
}

int RecaptureFromPins(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_between)
{
	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ & ~pin_between)
	{
		int sq2 = pawnleft[xs][sq];
		return sq2;
	}
	if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ & ~pin_between)
	{
		int sq2 = pawnright[xs][sq];
		return sq2;
	}
	BITBOARD b1 = bit_moves[N][sq] & bit_pieces[s][N] & bit_occ & ~pin_between;
	if (b1)
		return NextBit(b1);

	b1 = bit_moves[B][sq] & bit_pieces[s][B] & bit_occ & ~pin_between;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			return sq2;
		}
		b1 &= b1 - 1;
	}
	b1 = bit_moves[R][sq] & bit_pieces[s][R] & bit_occ & ~pin_between;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			return sq2;
		}
		b1 &= b1 - 1;
	}
	b1 = bit_moves[Q][sq] & bit_pieces[s][Q] & bit_occ & ~pin_between;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			return sq2;
		}
		b1 &= b1 - 1;
	}
	if (bit_moves[K][sq] & bit_pieces[s][K])
	{
		if (AttackKing(xs, sq, bit_occ) == 0)
			return kingloc[s];
	}
	return -1;
}

static bool AttackKing(const int s, const int king, const BITBOARD bit_occ)
{
	if (bit_pawndefends[s][king] & bit_pieces[s][P] & bit_occ)
		return true;
	if (bit_moves[N][king] & bit_pieces[s][N] & bit_occ)
		return true;

	BITBOARD b1 = bit_moves[R][king] & bit_occ & (bit_pieces[s][R] | bit_pieces[s][Q]);
	b1 |= (bit_moves[B][king] & bit_occ & (bit_pieces[s][B] | bit_pieces[s][Q]));

	while (b1)
	{
		int i = NextBit(b1);
		if (!(bit_between[i][king] & bit_occ))
		{
			return true;
		}
		b1 &= b1 - 1;
	}
	if (bit_moves[K][king] & bit_pieces[s][K])
		return true;
	return false;
}

/*
	if (LineAttack2(s^1, kingloc[s], bit_occ | mask[sq]))
	{
		if (bit_moves[K][kingloc[s]] & mask[sq])
		{
			//if (RecaptureFrom(s^1, s, sq, bit_occ) == -1)
			{
				count++;
				list[count] = kingloc[s];
				gain[count] = captured_value - gain[count - 1];
			}
		}
		z();
		break;
	}
	*/