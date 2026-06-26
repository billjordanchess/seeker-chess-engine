#include "globals.h"

extern int move_count;

move_data* m;

constexpr int evade_pawn[5] = { 0,0,1,1,1 };
constexpr int evade_bishop[5] = { 0,0,1,0,2 };
constexpr int evade_rook[5] = { 0,0,-2,1,2 };
constexpr int evade_queen[5] = { 0,0,-6,-4, 1 };

void AddEvasion(const int from, const int to, const int score);

void GenEP(BITBOARD);

void AddEvasion(const int from, const int to, const int score);
void EvadeDouble();

bool IsMate(const int checker);

BITBOARD PinnersPossible(const int s, const int xs);

void EvadeDouble()
{
	move_count = first_move[ply];
	int king = kingloc[side];
	int to;
	BITBOARD b1 = bit_kingmoves[king] & ~bit_units[side];
	
	while (b1)
	{
		to = NextBit(b1);
		b1 &= b1 - 1;
		if (!(Attack2(xside, to, bit_all & ~mask[king], ~mask[king])))
		{
			if (b[to] == EMPTY)
				AddEvasion(king, to, 0);
			else
				AddEvasion(king, to, kx[b[to]]);
		}
	}
	first_move[ply + 1] = move_count;
}

void AddEvasion(const int from, const int to, const int score)
{
	m = &move_list[move_count++];
	m->flags = INCHECK;
	m->from = from;
	m->to = to;
	m->score = score;
}

void EvadeCapture(const int s, const int xs, const int checker, BITBOARD pin_mask)// no magics
{
	if (checker == DOUBLE_CHECK)
	{
		EvadeDouble();
		return;
	}

	first_move[ply + 1] = first_move[ply];
	move_count = first_move[ply];

	BITBOARD b1;

	const int king = kingloc[s];
	const int check_piece = b[checker];

	if (b[checker] == P)
		GenEP(pin_mask);

	if (bit_left[xs][checker] & bit_pieces[s][P] & ~pin_mask)
	{
		int from = pawnleft[xs][checker];
		AddEvasion(from, checker, px[check_piece]);
	}
	if (bit_right[xs][checker] & bit_pieces[s][P] & ~pin_mask)
	{
		int from = pawnright[xs][checker];
		AddEvasion(from, checker, px[check_piece]);
	}

	b1 = bit_pieces[s][N] & bit_knightmoves[checker] & ~pin_mask;
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		AddEvasion(from, checker, nx[check_piece]);
	}

	b1 = bit_pieces[s][B] & bit_bishopmoves[checker] & ~pin_mask;
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		if (!(bit_between[from][checker] & bit_all))
		{
			AddEvasion(from, checker, bx[check_piece]);
		}
	}

	b1 = bit_pieces[s][R] & bit_rookmoves[checker] & ~pin_mask;
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		if (!(bit_between[from][checker] & bit_all))
		{
			AddEvasion(from, checker, rx[check_piece]);
		}
	}

	b1 = bit_pieces[s][Q] & bit_queenmoves[checker] & ~pin_mask;
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		if (!(bit_between[from][checker] & bit_all))
		{
			AddEvasion(from, checker, qx[check_piece]);
		}
	}

	b1 = bit_kingmoves[king] & bit_units[xs];
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		if (!(Attack2(xs, to, bit_all & ~mask[king], ~mask[king])))
		{
			AddEvasion(king, to, kx[b[to]]);
		}
	}
	first_move[ply + 1] = move_count;
}
 
void EvadeQuiet(const int s, const int xs, const int checker, BITBOARD pin_mask)
{
	const int king = kingloc[s];
	const int check_piece = b[checker];

	move_count = first_move[ply + 1];

	BITBOARD b1 = bit_kingmoves[king] & ~bit_all;

	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		if( !(Attack2(xs, to, bit_all & ~mask[king], ~mask[king])))
			AddEvasion(king, to, 0);
	}
	
	if (checker == DOUBLE_CHECK)
	{
		return;
	}

	const BITBOARD between = bit_between[checker][king];

	if (!(between))
	{
		first_move[ply + 1] = move_count;
		return;
	}
	
	BITBOARD b2;

	if (s == 0)
	{
		b1 = bit_pieces[0][P] & (between >> 8);
		b2 = between & mask_ranks[0][3] & (bit_pieces[0][P] << 16) & ~(bit_all << 8);
	}
	else
	{
		b1 = bit_pieces[1][P] & ~pin_mask & (between << 8);
		b2 = between & ~pin_mask & mask_ranks[1][3] & (bit_pieces[1][P] >> 16) & ~(bit_all >> 8);
	}
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		if (Attack(s, from) == 0)
		{
			AddEvasion(from, pawnplus[s][from], -100);
		}
		else
			AddEvasion(from, pawnplus[s][from], evade_pawn[check_piece]);
	}

	while (b2)
	{
		int from = NextBit(b2);
		b2 &= b2 - 1;
		AddEvasion(pawndouble[xs][from], from, evade_pawn[check_piece]);
	}

	b1 = bit_pieces[s][N] & ~pin_mask;
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		b2 = bit_knightmoves[from] & between;
		while (b2)
		{
			int to = NextBit(b2);
			b2 &= b2 - 1;
			if (Attack2(s, to, bit_all & not_mask[from], not_mask[from]) == 0 ||
				bit_pawndefends[xs][to] & bit_pieces[xs][P])
			{
				AddEvasion(from, to, -300);
			}
			else
				AddEvasion(from, to, -2);
		}
	}

	b1 = bit_pieces[s][B] & ~pin_mask;//magics
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		b2 = bit_bishopmoves[from] & between;
		//b2 = MagicBishopAttacks(from, bit_all) & line;

		while (b2)
		{
			int to = NextBit(b2);
			b2 &= b2 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (Attack2(s, to, bit_all & not_mask[from], not_mask[from]) == 0 ||
					bit_pawndefends[xs][to] & bit_pieces[xs][P])
				{
					AddEvasion(from, to, -300);
				}
				else
					AddEvasion(from, to, evade_bishop[check_piece]);
			}
		}
	}
	b1 = bit_pieces[s][R] & ~pin_mask;
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		b2 = bit_rookmoves[from] & between;
		while (b2)
		{
			int to = NextBit(b2);
			b2 &= b2 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (Attack2(s, to, bit_all & not_mask[from], not_mask[from]) == 0 ||
					bit_pawndefends[xs][to] & bit_pieces[xs][P] ||
					bit_knightmoves[to] & bit_pieces[xs][N])
				{
					AddEvasion(from, to, -500);
				}
				else
					AddEvasion(from, to, evade_rook[check_piece]);
			}
		}
	}

	for (int x = 0; x < total[s][Q]; x++)
	{
		int from = pieces[s][Q][x];
		if (mask[from] & pin_mask)
		{
			continue;
		}
		b2 = bit_queenmoves[from] & between;
		while (b2)
		{
			int to = NextBit(b2);
			b2 &= b2 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (Attack2(s, to, bit_all & not_mask[from], not_mask[from]) == 0 ||
					bit_pawndefends[xs][to] & bit_pieces[xs][P] ||
					bit_knightmoves[to] & bit_pieces[xs][N])
				{
					AddEvasion(from, to, -900);
				}
				else
					AddEvasion(from, to, evade_queen[check_piece]);
			}
		}
	}
	first_move[ply + 1] = move_count;
}

bool IsMate(const int checker)
{
	int from, to;
	int king = kingloc[side];
	int check_piece = b[checker];

	const BITBOARD between = bit_between[checker][king];
	BITBOARD b1 = bit_kingmoves[king] & bit_units[xside];

	while (b1)
	{
		to = NextBit(b1);
		b1 &= b1 - 1;
		if (!(Attack2(xside, to, bit_all & ~mask[king], ~mask[king])))
			return false;
	}

	b1 = bit_kingmoves[king] & ~bit_all;

	while (b1)
	{
		to = NextBit(b1);
		b1 &= b1 - 1;
		if (!(Attack2(xside, to, bit_all & ~mask[king], ~mask[king])))
			return false;
	}

	BITBOARD b2;

	if (!(bit_between[checker][king]))
	{
		return true;
	}

	if (side == 0)
	{
		b1 = bit_pieces[0][P] & (between >> 8);
		b2 = between & mask_ranks[0][3] & (bit_pieces[0][P] << 16) & ~(bit_all << 8);
	}
	else
	{
		b1 = bit_pieces[1][P] & (between << 8);
		b2 = between & mask_ranks[1][3] & (bit_pieces[1][P] >> 16) & ~(bit_all >> 8);
	}
	while (b1)
	{
		to = NextBit(b1);
		return false;
		b1 &= b1 - 1;
	}

	while (b2)
	{
		to = NextBit(b2);
		b2 &= b2 - 1;
		return false;
	}

	for (int x = 0; x < total[side][N]; x++)
	{
		from = pieces[side][N][x];
		b1 = bit_knightmoves[from] & between;
		while (b1)
		{
			to = NextBit(b1);
			return false;
			b1 &= b1 - 1;
		}
	}

	for (int x = 0; x < total[side][B]; x++)
	{
		from = pieces[side][B][x];
		b2 = bit_bishopmoves[from] & between;
		while (b2)
		{
			to = NextBit(b2);
			if (!(bit_between[from][to] & bit_all))
			{
				return false;
			}
			b2 &= b2 - 1;
		}
	}

	for (int x = 0; x < total[side][R]; x++)
	{
		from = pieces[side][R][x];
		b2 = bit_rookmoves[from] & between;
		while (b2)
		{
			to = NextBit(b2);
			if (!(bit_between[from][to] & bit_all))
			{
				return false;
			}
			b2 &= b2 - 1;
		}
	}

	for (int x = 0; x < total[side][Q]; x++)
	{
		from = pieces[side][Q][x];
		b2 = bit_queenmoves[from] & between;
		while (b2)
		{
			to = NextBit(b2);		
			if (!(bit_between[from][to] & bit_all))
			{
				return false;
			}
			b2 &= b2 - 1;
		}
	}
	return true;
}
