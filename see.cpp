#include "globals.h"

int GetNextAttackerSquarePins(const int s, const int, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);
bool SEEAttackKing(const int s, const int sq, const BITBOARD bit_occ);
bool LineAttack2(const int s, const int sq, const BITBOARD occ);
BITBOARD GetPinBetween(const int s, const int xs);

int QuietEvasion(int alpha, int beta, BITBOARD);

bool IsMate(const int checker);

int Max(const int a, const int b);

int SEE(int s, const int attacker, const int sq, const BITBOARD p1, const BITBOARD p2)
{
	int gain[16];
	int depth = 0;

	BITBOARD bit_occ = bit_all;
	BITBOARD pins[2];
	pins[0] = p1;
	pins[1] = p2;

	memset(gain, 0, sizeof(gain));

	gain[0] = piece_value[b[sq]];

	int attack_sq = attacker;
	int captured_value = piece_value[b[attack_sq]];   // piece that moves onto sq and may be recaptured

	bit_occ &= not_mask[attack_sq];
	s ^= 1;   // side to move now becomes the reply side

	while (1)
	{
		gain[++depth] = captured_value - gain[depth - 1];

		if (depth >= 15)
			break;

		if (captured_value == piece_value[K])
			break;

		if (pins[s] & bit_occ & bit_units[s])
			attack_sq = GetNextAttackerSquarePins(s, s ^ 1, sq, bit_occ, pins[s]);
		else
			attack_sq = GetNextAttackerSquare(s, s ^ 1, sq, bit_occ);

		if (attack_sq == -1)
			break;

		captured_value = piece_value[b[attack_sq]];
		bit_occ &= not_mask[attack_sq];
		s ^= 1;
	}

	while (depth > 0)
	{
		gain[depth - 1] = -Max(-gain[depth - 1], gain[depth]);
		depth--;
	}
	return gain[0];
}

int Max(const int a, const int b)
{
	if (a > b)
		return a;
	else
		return b;
}

int GetNextAttackerSquare(const int s, const int xs, const int sq, const BITBOARD bit_occ)
{
	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		return pawnleft[xs][sq];
	}
	else if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		return pawnright[xs][sq];
	}

	BITBOARD b1 = bit_knightmoves[sq] & bit_pieces[s][N] & bit_occ;
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

	if (bit_kingmoves[sq] & bit_pieces[s][K])
	{
		if (SEEAttackKing(xs, sq, bit_occ) == 0)
			return kingloc[s];
	}
	return -1;
}

int GetNextAttackerSquarePins(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_between)
{
	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		int sq2 = pawnleft[xs][sq];
		BITBOARD b2 = mask[sq2] & pin_between;
		if (!(b2 && IsOneBit(pin_between & bit_occ & ~mask[sq2]) && !(mask[sq] & pin_between)))
		{
			return sq2;
		}
	}
	if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		int sq2 = pawnright[xs][sq];
		BITBOARD b2 = mask[sq2] & pin_between;
		if (!(b2 && IsOneBit(pin_between & bit_occ & ~mask[sq2]) && !(mask[sq] & pin_between)))
		{
			return sq2;
		}
	}
	BITBOARD b1 = bit_knightmoves[sq] & bit_pieces[s][N] & bit_occ;
	while (b1)
	{
		int sq2 = NextBit(b1);
		BITBOARD b2 = mask[sq2] & pin_between;
		if (!(b2 && IsOneBit(pin_between & bit_occ & ~mask[sq2])))
			return sq2;
		b1 &= b1 - 1;
	}
	b1 = bit_bishopmoves[sq] & bit_pieces[s][B] & bit_occ;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			BITBOARD b2 = mask[sq2] & pin_between;
			if (!(b2 && IsOneBit(pin_between & bit_occ & ~mask[sq2]) && !(mask[sq] & pin_between)))
			{
				return sq2;
			}
		}
		b1 &= b1 - 1;
	}
	b1 = bit_rookmoves[sq] & bit_pieces[s][R] & bit_occ;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			BITBOARD b2 = mask[sq2] & pin_between;
			if (!(b2 && IsOneBit(pin_between & bit_occ & ~mask[sq2]) && !(mask[sq] & pin_between)))
			{
				return sq2;
			}
		}
		b1 &= b1 - 1;
	}
	for (int x = 0; x < total[s][Q]; x++)
	{
		int sq2 = pieces[s][Q][x];
		if (bit_queenmoves[sq] & mask[sq2] & bit_occ)
		{
			if (!(bit_between[sq2][sq] & bit_occ))
			{
				BITBOARD b2 = mask[sq2] & pin_between;
				if (!(b2 && IsOneBit(pin_between & bit_occ & ~mask[sq2]) && !(mask[sq] & pin_between)))
				{
					return sq2;
				}
			}
		}
	}
	if (bit_kingmoves[sq] & bit_pieces[s][K])
	{
		if (SEEAttackKing(xs, sq, bit_occ) == 0)
			return kingloc[s];
	}
	return -1;
}

bool SEEAttackKing(const int s, const int king, const BITBOARD bit_occ)
{
	if (bit_pawndefends[s][king] & bit_pieces[s][P] & bit_occ)
		return true;
	if (bit_knightmoves[king] & bit_pieces[s][N] & bit_occ)
		return true;

	BITBOARD b1 = bit_rookmoves[king] & bit_occ & (bit_pieces[s][R] | bit_pieces[s][Q]);
	b1 |= (bit_bishopmoves[king] & bit_occ & (bit_pieces[s][B] | bit_pieces[s][Q]));

	while (b1)
	{
		int i = NextBit(b1);
		if (!(bit_between[i][king] & bit_occ))
		{
			return true;
		}
		b1 &= b1 - 1;
	}

	if (bit_kingmoves[king] & bit_pieces[s][K])
		return true;
	return false;
}
