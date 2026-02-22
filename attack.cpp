#include "globals.h"

#define MAGIC 1

int king_attackers[MAX_PLY][2];
int target_list[MAX_PLY][15];
int targets[MAX_PLY];

BITBOARD bit_rookattacks[2][8];
BITBOARD bit_queenattacks[2][8];

BITBOARD bit_kq_defends;

BITBOARD bit_targets[MAX_PLY];

BITBOARD bit_line_attackers[MAX_PLY];
BITBOARD bit_defended[2][MAX_PLY];

BITBOARD bit_attacked[2][6];
BITBOARD bit_weaker[2][6];
BITBOARD bit_total_attacks[2];
BITBOARD bit_undefended[2];
BITBOARD bit_undefended_squares[2];
BITBOARD bit_defend_to[2][6];

BITBOARD bit_adjacent_king[MAX_PLY][2];

BITBOARD GetKnightAttacks(const int s);
BITBOARD GetBishopAttacks(const int s);
BITBOARD GetRookAttacks(const int s);
BITBOARD GetQueenAttacks(const int s);
BITBOARD GetKingAttacks(const int s);

bool RookAttack(const int s, const int from, const int to);

int GetLowestQuietAttacker(const int s, const int to);
int GetLowestLineAttacker(const int s, const int sq);

int GetAttackingSquare(const int s, const int sq);

bool KingLessAttack(const int s, const int sq); 
void BuildAttackMap();

bool Attack2(const int s, const int sq, const BITBOARD occ, const BITBOARD);
bool LineAttack2(const int s, const int sq, const BITBOARD occ, const BITBOARD not_mover);

bool Attack(const int s, const int sq)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P])
		return true;
	if (bit_knightmoves[sq] & bit_pieces[s][N])
		return true;
	if (RookAttacks(sq, bit_all) & (bit_pieces[s][R] | bit_pieces[s][Q]))
		return true;
	if (BishopAttacks(sq, bit_all) & (bit_pieces[s][B] | bit_pieces[s][Q]))
		return true;
	if (bit_kingmoves[sq] & bit_pieces[s][K])
		return true;
	return false;
}

bool Attack2(const int s, const int sq, const BITBOARD occ, const BITBOARD not_mover)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P] & not_mover)
		return true;
	if (bit_knightmoves[sq] & bit_pieces[s][N] & not_mover)
		return true;

	if (RookAttacks(sq, occ) & (bit_pieces[s][R] | bit_pieces[s][Q]) & not_mover)
		return true;
	if (BishopAttacks(sq, occ) & (bit_pieces[s][B] | bit_pieces[s][Q]) & not_mover)
		return true;

	if (bit_kingmoves[sq] & bit_pieces[s][K])
		return true;
	return false;
}

bool KingLessAttack(const int s, const int sq)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P])
		return true;
	if (bit_knightmoves[sq] & bit_pieces[s][N])
		return true;
	if (RookAttacks(sq, bit_all) & (bit_pieces[s][R] | bit_pieces[s][Q]))
		return true;
	if (BishopAttacks(sq, bit_all) & (bit_pieces[s][B] | bit_pieces[s][Q]))
		return true;
	return false;
}

int GetAttackingSquare(const int s, const int sq)//magic
{
	BITBOARD b1 = bit_pawndefends[s][sq] & bit_pieces[s][P];
	if (b1)
		return NextBit(b1);
	b1 = bit_knightmoves[sq] & bit_pieces[s][N];
	if (b1)
		return NextBit(b1);

	b1 = BishopAttacks(sq, bit_all);
	if (b1 & bit_pieces[s][B])
		return NextBit(b1);

	BITBOARD b2 = RookAttacks(sq, bit_all);
	if (b2 & bit_pieces[s][R])
		return NextBit(b2);

	BITBOARD b3 = (b1 | b2) & bit_pieces[s][Q];
	if (b3)
		return NextBit(b3);
	
	if (bit_kingmoves[sq] & bit_pieces[s][K])
		return kingloc[s];
	return -1;
}

bool IsCheck(const int p, const int sq, const int king)
{
	if (b[p] > 0)
	{
		if (bit_moves[b[p]][sq] & mask[king])
		{
			if (!(bit_between[sq][king] & bit_all))
			{
				if (b[p] > 1)
				{
					return true;
				}
			}
		}
	}
	else
	{
		if (bit_pawncaptures[xside][sq] & mask[king])
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
	/*
	if (bit_knightmoves[sq] & bit_pieces[s][N])
	{
		for (int x = 0; x < total[s][N]; x++)
		{
			i = pieces[s][N][x];
			if (bit_knightmoves[sq] & mask[i])
			{
				checker_square = i;
				count++;
			}
		} 
	}
	*/
	BITBOARD b1 = bit_knightmoves[sq] & bit_pieces[s][N];
	if (b1)
	{
		checker_square = NextBit(b1);
		count++;
	}
	//*/

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
	b1 = bit_bishopmoves[sq] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	b1 |= (bit_rookmoves[sq] & (bit_pieces[s][R] | bit_pieces[s][Q]));

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
	if(count > 1)
	{
		return DOUBLE_CHECK;
	}
	return checker_square;
}

bool LineAttack(const int s, const int sq)
{
	if (RookAttacks(sq, bit_all) & (bit_pieces[s][R] | bit_pieces[s][Q]))
		return true;
	if (BishopAttacks(sq, bit_all) & (bit_pieces[s][B] | bit_pieces[s][Q]))
		return true;
	return false;
}

bool LineAttack2(const int s, const int sq, const BITBOARD occ, const BITBOARD not_mover)
{
	BITBOARD b1 = bit_bishopmoves[sq] & (bit_pieces[s][B] | bit_pieces[s][Q]) & not_mover;
	b1 |= (bit_rookmoves[sq] & (bit_pieces[s][R] | bit_pieces[s][Q])) & not_mover;

	while (b1)  
	{
		int i = NextBit(b1);
		if (!(bit_between[i][sq] & bit_all & occ))
			return true;
		b1 &= b1 - 1;
	}
	return false;
}

bool RookAttack(const int s, const int from, const int to)
{
	BITBOARD b1 = bit_rookmoves[to] & bit_pieces[s][R] & not_mask[from];
	if (b1 && !(bit_between[to][NextBit(b1)] & bit_all))
	{
		return true;
	}
	return false;
}

bool isPinned(const int s, const int k, const int pinned, BITBOARD b1)
{
	while (b1)
	{
		int sq = NextBit(b1);
		if (!(bit_between[sq][k] & bit_all & not_mask[pinned]))
			return true;
		b1 &= b1 - 1;
	}
	return false;
}

bool isPinnedLinePiece(const int s, const int k, const int pinned, const int to, BITBOARD b1)
{	
	b1 &= not_mask[to];

	while (b1)
	{
		int sq = NextBit(b1);
		if (!(bit_between[sq][k] & bit_all & not_mask[pinned]))
			return true;
		b1 &= b1 - 1;
	}
	return false;
}

int GetLowestAttacker(const int s, const int sq)
{
	if (bit_pawndefends[s][sq] & bit_pieces[s][P])
		return P;
	if (bit_knightmoves[sq] & bit_pieces[s][N])
		return N;

	BITBOARD b1, b2;
	b1 = BishopAttacks(sq, bit_all);
	if (b1 & bit_pieces[s][B])
		return B;

	b2 = RookAttacks(sq, bit_all);
	if (b2 & bit_pieces[s][R])
		return R;

	if ((b1 | b2) & bit_pieces[s][Q])
		return Q;

	if (bit_kingmoves[sq] & bit_pieces[s][K])
		return K;
	return -1;
}

int GetLowestQuietAttacker(const int s, const int to)
{		
	if (bit_attacked[s][P] & mask[to])
		return P;
	if (bit_attacked[s][N] & mask[to])
		return N;
	if (bit_attacked[s][B] & mask[to])
		return N;
	if (bit_attacked[s][R] & mask[to])
		return R;
	if (bit_attacked[s][Q] & mask[to])
		return Q;
	if (bit_kingmoves[to] & bit_pieces[s][K])
		return K;
	return -1;
}

BITBOARD GetKnightAttacks(const int s)
{
	BITBOARD b1 = 0;

	for (int x = 0; x < total[s][N]; x++)
	{
		b1 |= bit_knightmoves[pieces[s][N][x]];
	}
	return b1;
}

BITBOARD GetBishopAttacks(const int s)
{
	BITBOARD b1 = 0, b2;

	for (int x = 0; x < total[s][B]; x++)
	{
		int sq = pieces[s][B][x];
		b2 = BishopAttacks(sq, bit_all);
		b1 |= b2;
	}
	return b1;
}

BITBOARD GetRookAttacks(const int s)
{
	BITBOARD b1 = 0, b2;

	for (int x = 0; x < total[s][R]; x++)
	{
		int sq = pieces[s][R][x];
		b2 = RookAttacks(sq, bit_all);
		bit_rookattacks[s][x] = b2;
		b1 |= b2;
	}
	return b1;
}

BITBOARD GetQueenAttacks(const int s)
{
	BITBOARD b1 = 0, b2;
	for (int x = 0; x < total[s][Q]; x++)
	{
		int sq = pieces[s][Q][x];
		b2 = QueenAttacks(sq, bit_all);
		bit_queenattacks[s][x] = b2;
		b1 |= b2;
	}
	return b1;
}

BITBOARD GetKingAttacks(const int s)
{
	return bit_kingmoves[kingloc[s]];
}

//could be incremental make/unmake, or by ply 
//update all pieces of same type
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
		bit_weaker[s][B] = (bit_attacked[s][P]);
		bit_weaker[s][N] = (bit_attacked[s][P]);
		bit_weaker[s][R] = (bit_attacked[s][P] | bit_attacked[s][N] | bit_attacked[s][B]);
		bit_weaker[s][Q] = (bit_weaker[s][R] | bit_attacked[s][R]); 
		bit_total_attacks[s] = (bit_weaker[s][Q] | bit_attacked[s][Q] | bit_attacked[s][K]);
	}

	for (int s = 0; s < 2; s++)//what attacks them
	{
		BITBOARD bit_every = 
			bit_attacked[s][P] | bit_attacked[s][N] | bit_attacked[s][B] | bit_attacked[s][R] | bit_attacked[s][Q] | bit_attacked[s][K];
		bit_defend_to[s][N] = bit_every & ~bit_attacked[s][N];
		bit_defend_to[s][B] = bit_every & ~bit_attacked[s][B];
		bit_defend_to[s][R] = bit_every & ~bit_attacked[s][R];
		bit_defend_to[s][Q] = bit_every & ~bit_attacked[s][Q];
		bit_defend_to[s][K] = bit_total_attacks[s];
	}

	bit_undefended[0] = bit_units[0] & bit_total_attacks[1] & ~bit_total_attacks[0];
	bit_undefended[1] = bit_units[1] & bit_total_attacks[0] & ~bit_total_attacks[1];

	bit_undefended_squares[0] = bit_units[0] & ~bit_total_attacks[0];
	bit_undefended_squares[1] = bit_units[1] & ~bit_total_attacks[1];

	bit_kq_defends = 0;

	if (!(bit_attacked[side][P] | bit_attacked[side][N] | bit_attacked[side][B] | bit_attacked[side][R])
		&& (bit_attacked[side][Q] | bit_attacked[side][K]))
		bit_kq_defends = bit_attacked[side][Q] | bit_attacked[side][K];

	bit_line_attackers[ply] = bit_attacked[xside][B] | bit_attacked[xside][R] | bit_attacked[xside][Q];

	///BITBOARD kingless_attacks = bit_weaker[xside][Q] | bit_attacked[xside][Q];

	bit_defended[side][ply] = bit_units[side] & bit_total_attacks[xside] & bit_total_attacks[side];
}



