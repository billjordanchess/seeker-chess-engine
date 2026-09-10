//11/9/26
#include "globals.h"

#define MAGIC 1

int targets[MAX_PLY];

BITBOARD slider_moves[64][MAX_PLY];

BITBOARD bit_kq_defends;

BITBOARD bit_targets[MAX_PLY];

BITBOARD bit_attacked[2][6];
BITBOARD bit_weaker[2][6];
BITBOARD bit_total_attacked[2];
BITBOARD bit_total_weaker[2];
BITBOARD bit_undefended[2];
BITBOARD bit_undefended_squares[2];
BITBOARD bit_defend_to[2][6];

void SetSliderMoves();

BITBOARD GetKnightAttacks(const int s);
BITBOARD GetBishopAttacks(const int s);
BITBOARD GetRookAttacks(const int s);
BITBOARD GetQueenAttacks(const int s);
BITBOARD GetKingAttacks(const int s);

bool RookQueenAttack(const int s, const int from, const int to);

int GetLowestLineAttacker(const int s, const int sq);
int GetLowestAttackerPins(const int s, const int sq, const BITBOARD pin_between);

int GetAttackingSquare(const int s, const int sq);

bool KingLessAttack(const int s, const int sq);
void BuildAttackMap();

bool Attack2(const int s, const int sq, const BITBOARD occ, const BITBOARD);
bool LineAttack2(const int s, const int sq, const BITBOARD occ);

bool IsCheck(const int piece, const int sq, const int king);

bool Attack(const int s, const int sq, const BITBOARD occ)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P])
		return true;
	if (bit_moves[N][sq] & bit_pieces[s][N])
		return true;
	if (MagicRookAttacks(sq, occ) & (bit_pieces[s][R] | bit_pieces[s][Q]))
		return true;
	if (MagicBishopAttacks(sq, occ) & (bit_pieces[s][B] | bit_pieces[s][Q]))
		return true;
	if (bit_moves[K][sq] & bit_pieces[s][K])
		return true;
	return false;
}

bool Attack2(const int s, const int sq, const BITBOARD occ, const BITBOARD not_mover)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P] & not_mover)
		return true;
	if (bit_moves[N][sq] & bit_pieces[s][N] & not_mover)
		return true;

	if (MagicRookAttacks(sq, occ) & (bit_pieces[s][R] | bit_pieces[s][Q]) & not_mover)
		return true;
	if (MagicBishopAttacks(sq, occ) & (bit_pieces[s][B] | bit_pieces[s][Q]) & not_mover)
		return true;

	if (bit_moves[K][sq] & bit_pieces[s][K])
		return true;
	return false;
}

int GetAttackingSquare(const int s, const int sq)
{
	BITBOARD b1 = bit_pawndefends[s][sq] & bit_pieces[s][P];
	if (b1)
		return NextBit(b1);
	b1 = bit_moves[N][sq] & bit_pieces[s][N];
	if (b1)
		return NextBit(b1);

	b1 = MagicBishopAttacks(sq, bit_all);
	if (b1 & bit_pieces[s][B])
		return NextBit(b1 & bit_pieces[s][B]);

	BITBOARD b2 = MagicRookAttacks(sq, bit_all);
	if (b2 & bit_pieces[s][R])
		return NextBit(b2 & bit_pieces[s][R]);

	BITBOARD b3 = (b1 | b2) & bit_pieces[s][Q];
	if (b3)
		return NextBit(b3);

	if (bit_moves[K][sq] & bit_pieces[s][K])
		return kingloc[s];
	return -1;
}

bool IsLineCheck(const int from, const int to, const int king)
{
	if (bit_moves[b[from]][to] & mask[king])
	{
		if (!(bit_between[to][king] & bit_all))
		{
			return true;
		}
	}
	return false;
}

int Check(const int s, const int sq)
{
	int i, checker_square = -1;
	int count = 0;

	BITBOARD b1 = bit_moves[N][sq] & bit_pieces[s][N];
	if (b1)
	{
		checker_square = NextBit(b1);
		count++;
	}

	if (bit_left[!s][sq] & bit_pieces[s][P])
	{
		checker_square = pawnleft[!s][sq];
		count++;
	}
	else if (bit_right[!s][sq] & bit_pieces[s][P])
	{
		checker_square = pawnright[!s][sq];
		count++;
	}
	b1 = bit_moves[B][sq] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	b1 |= (bit_moves[R][sq] & (bit_pieces[s][R] | bit_pieces[s][Q]));

	while (b1)
	{
		i = NextBit(b1);
		if (!(bit_between[i][sq] & bit_all))
		{
			checker_square = i;
			count++;
		}
		b1 &= b1 - 1;
	}
	if (count > 1)
	{
		return DOUBLE_CHECK;
	}
	return checker_square;
}

bool LineAttack(const int s, const int sq, const BITBOARD occ)
{
	if (MagicRookAttacks(sq, occ) & (bit_pieces[s][R] | bit_pieces[s][Q]))
		return true;
	if (MagicBishopAttacks(sq, occ) & (bit_pieces[s][B] | bit_pieces[s][Q]))
		return true;
	return false;
}

bool LineAttack2(const int s, const int sq, const BITBOARD occ)
{
	BITBOARD b1 = bit_moves[B][sq] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	b1 |= (bit_moves[R][sq] & (bit_pieces[s][R] | bit_pieces[s][Q]));

	while (b1)
	{
		int i = NextBit(b1);
		if (!(bit_between[i][sq] & occ))
			return true;
		b1 &= b1 - 1;
	}
	return false;
}

bool RookQueenAttack(const int s, const int from, const int to)
{
	return MagicRookAttacks(to, bit_all & ~mask[from]) &
		(bit_pieces[s][R] | bit_pieces[s][Q]) &
		~mask[from];
}

int GetLowestAttacker(const int s, const int sq)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P])
		return P;
	if (bit_moves[N][sq] & bit_pieces[s][N])
		return N;

	BITBOARD b1, b2;
	b1 = MagicBishopAttacks(sq, bit_all);
	if (b1 & bit_pieces[s][B])
		return B;

	b2 = MagicRookAttacks(sq, bit_all);
	if (b2 & bit_pieces[s][R])
		return R;

	if ((b1 | b2) & bit_pieces[s][Q])
		return Q;

	if (bit_moves[K][sq] & bit_pieces[s][K])
		return K;
	return -1;
}

int GetLowestAttacker2(const int s, const int sq, const BITBOARD occ)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P] & occ)
		return P;
	if (bit_moves[N][sq] & bit_pieces[s][N] & occ)
		return N;

	BITBOARD b1, b2;
	b1 = MagicBishopAttacks(sq, occ);
	if (b1 & bit_pieces[s][B] & occ)
		return B;

	b2 = MagicRookAttacks(sq, occ);
	if (b2 & bit_pieces[s][R] & occ)
		return R;

	if ((b1 | b2) & bit_pieces[s][Q] & occ)
		return Q;

	if (bit_moves[K][sq] & bit_pieces[s][K])
		return K;
	return -1;
}

int GetLowestAttackerPins(const int s, const int sq, const BITBOARD pin_mask)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P] & ~pin_mask)
		return P;
	if (bit_moves[N][sq] & bit_pieces[s][N] & ~pin_mask)
		return N;

	BITBOARD b1, b2;
	b1 = MagicBishopAttacks(sq, bit_all);
	if (b1 & bit_pieces[s][B] & ~pin_mask)
		return B;

	b2 = MagicRookAttacks(sq, bit_all);
	if (b2 & bit_pieces[s][R] & ~pin_mask)
		return R;

	if ((b1 | b2) & bit_pieces[s][Q] & ~pin_mask)
		return Q;

	if (bit_moves[K][sq] & bit_pieces[s][K])
		return K;
	return -1;
}

BITBOARD GetKnightAttacks(const int s)
{
	BITBOARD b1 = 0;

	for (int x = 0; x < total[s][N]; x++)
	{
		b1 |= bit_moves[N][pieces[s][N][x]];
	}
	return b1;
}

BITBOARD GetBishopAttacks(const int s)
{
	BITBOARD b1 = 0;

	for (int x = 0; x < total[s][B]; x++)
	{
		int from = pieces[s][B][x];
		b1 |= slider_moves[from][ply];
	}
	return b1;
}

BITBOARD GetRookAttacks(const int s)
{
	BITBOARD b1 = 0;

	for (int x = 0; x < total[s][R]; x++)
	{
		int from = pieces[s][R][x];
		b1 |= slider_moves[from][ply];
	}
	return b1;
}

BITBOARD GetQueenAttacks(const int s)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][Q]; x++)
	{
		int from = pieces[s][Q][x];
		b1 |= slider_moves[from][ply];
	}
	return b1;
}

BITBOARD GetKingAttacks(const int s)
{
	return bit_moves[K][kingloc[s]];
}

void BuildAttackMap()
{
	bit_attacked[0][P] = (bit_pieces[0][P] & not_a_file) << 7;
	bit_attacked[0][P] |= (bit_pieces[0][P] & not_h_file) << 9;

	bit_attacked[1][P] = (bit_pieces[1][P] & not_a_file) >> 9;
	bit_attacked[1][P] |= (bit_pieces[1][P] & not_h_file) >> 7;

	for (int s = 0; s < 2; s++)//what they attack
	{
		bit_attacked[s][N] = GetKnightAttacks(s);
		bit_attacked[s][B] = GetBishopAttacks(s);
		bit_attacked[s][R] = GetRookAttacks(s);
		bit_attacked[s][Q] = GetQueenAttacks(s);
		bit_attacked[s][K] = GetKingAttacks(s);
	}

	for (int s = 0; s < 2; s++)//what attacks them
	{
		bit_weaker[s][B] = bit_attacked[s][P];
		bit_weaker[s][N] = bit_attacked[s][P];
		bit_weaker[s][R] = (bit_attacked[s][P] | bit_attacked[s][N] | bit_attacked[s][B]);
		bit_weaker[s][Q] = (bit_weaker[s][R] | bit_attacked[s][R]);
		bit_total_attacked[s] = (bit_weaker[s][Q] | bit_attacked[s][Q] | bit_attacked[s][K]);
	}

		bit_total_weaker[0] = (bit_weaker[1][N] & bit_pieces[0][N]) | (bit_weaker[1][B] & bit_pieces[0][B]) |
			(bit_weaker[1][R] & bit_pieces[0][R]) | (bit_weaker[1][Q] & bit_pieces[0][Q]);
		bit_total_weaker[1] = (bit_weaker[0][N] & bit_pieces[1][N]) | (bit_weaker[0][B] & bit_pieces[1][B]) |
			(bit_weaker[0][R] & bit_pieces[1][R]) | (bit_weaker[0][Q] & bit_pieces[1][Q]);
		//PrintBitBoard(bit_total_weaker[0]);
		//z();

	for (int s = 0; s < 2; s++)//what defends a move
	{
		bit_defend_to[s][P] = bit_attacked[s][N] | bit_attacked[s][B] | bit_attacked[s][R] | bit_attacked[s][Q] | bit_attacked[s][K];
		bit_defend_to[s][N] = bit_attacked[s][P] | bit_attacked[s][B] | bit_attacked[s][R] | bit_attacked[s][Q] | bit_attacked[s][K];
		bit_defend_to[s][B] = bit_attacked[s][P] | bit_attacked[s][N] | bit_attacked[s][R] | bit_attacked[s][Q] | bit_attacked[s][K];
		bit_defend_to[s][R] = bit_attacked[s][P] | bit_attacked[s][N] | bit_attacked[s][B] | bit_attacked[s][Q] | bit_attacked[s][K];
		bit_defend_to[s][Q] = bit_attacked[s][P] | bit_attacked[s][N] | bit_attacked[s][B] | bit_attacked[s][R] | bit_attacked[s][K];
		bit_defend_to[s][K] = bit_weaker[s][Q] | bit_attacked[s][Q];
	}

	bit_undefended[0] = bit_units[0] & bit_total_attacked[1] & ~bit_total_attacked[0];
	bit_undefended[1] = bit_units[1] & bit_total_attacked[0] & ~bit_total_attacked[1];

	bit_undefended_squares[0] = bit_units[0] & ~bit_total_attacked[0];
	bit_undefended_squares[1] = bit_units[1] & ~bit_total_attacked[1];

	bit_kq_defends =
		(bit_attacked[xside][Q] | bit_attacked[xside][K]) &
		~(bit_attacked[xside][P] | bit_attacked[xside][N] |
			bit_attacked[xside][B] | bit_attacked[xside][R]);
}

void SetSliderMoves()
{
	for (int s = 0; s < 2; s++)
	{
		for (int x = 0; x < total[s][B]; x++)
		{
			const int from = pieces[s][B][x];
			slider_moves[from][ply] = MagicBishopAttacks(from, bit_all);
		}
		for (int x = 0; x < total[s][R]; x++)
		{
			const int from = pieces[s][R][x];
			slider_moves[from][ply] = MagicRookAttacks(from, bit_all);
		}
		for (int x = 0; x < total[s][Q]; x++)
		{
			const int from = pieces[s][Q][x];
			slider_moves[from][ply] = MagicQueenAttacks(from, bit_all);
		}
	}
}

