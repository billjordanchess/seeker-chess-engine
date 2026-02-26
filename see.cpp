#include "globals.h"

int RecaptureSearch(int s, int attacker, const int sq, const BITBOARD p1, const BITBOARD p2, const int eval, const int, const int, const int);
int GetNextAttackerSquarePins(const int s, const int, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);
bool SEEAttackKing(const int s, const int sq, const BITBOARD bit_occ);
bool LineAttack2(const int s, const int sq, const BITBOARD occ);
BITBOARD GetPinBetween(const int s, const int xs);

int QuietEvasion(int alpha, int beta, BITBOARD);

bool IsMate(const int checker);

int SEE(int s, const int attacker, const int sq, const BITBOARD p1, const BITBOARD p2)
{
	int attack_sq = attacker;
	int count = 0;
	int score[12];
	int list[12];

	int total_score = 0;
	score[0] = piece_value[b[sq]];
	score[1] = piece_value[b[attacker]];

	BITBOARD bit_occ = bit_all;
	BITBOARD pins[2];
	pins[0] = p1;
	pins[1] = p2;
	list[0] = attacker;

	pins[0] = GetPinBetween(0, 1);
	pins[1] = GetPinBetween(1, 0);

	while (count < 10)
	{
		bit_occ &= not_mask[attack_sq];
		s ^= 1;

		count++;
		if (pins[s] & bit_units[s])
			attack_sq = GetNextAttackerSquarePins(s, s ^ 1, sq, bit_occ, pins[s]);
		else
			attack_sq = GetNextAttackerSquare(s, s ^ 1, sq, bit_occ);

		if (attack_sq > -1)
		{
			score[count + 1] = piece_value[b[attack_sq]];
			if (score[count] > score[count - 1] + score[count + 1])
			{
				count--;
				break;
			}
		}
		else
		{
			break;
		}
		list[count] = attack_sq;
	}

	while (count > 2)
	{
		if (score[count - 1] >= score[count - 2])
			count -= 2;
		else
			break;
	}

	for (int x = 0; x < count; x++)
	{
		if (x % 2 == 0)
			total_score += score[x];
		else
			total_score -= score[x];
	}

	return total_score;
}

int RecaptureSearch(int s, const int attacker, const int sq, const BITBOARD p1, const BITBOARD p2, const int eval, const int alpha, const int beta, const int flags)
{
	const int start_side = side;
	int attack_sq = attacker;
	int count = 0;
	int score[12];
	int list[12];

	int total_score = 0;

	score[0] = piece_value[b[sq]];
	score[1] = piece_value[b[attacker]];

	BITBOARD bit_occ = bit_all;
	BITBOARD pins[2];
	pins[0] = p1;
	pins[1] = p2;
	list[0] = attacker;

	pins[0] = GetPinBetween(0, 1);
	pins[1] = GetPinBetween(1, 0);

	/*/
	if (pins[0])
		PrintBitBoard(pins[0]);
	if (pins[1])
	{
		PrintBitBoard(pins[1]);
	}

	if (pins[0] | pins[1])
		z();
	/*/

	while (count < 10)
	{
		bit_occ &= not_mask[attack_sq];

		if (LineAttack2(s, kingloc[!s], bit_occ | mask[sq]))
		{
			if (bit_kingmoves[kingloc[!s]] & mask[sq])
			{
				if (GetNextAttackerSquare(s, s ^ 1, sq, bit_occ) == -1)
				{
					count++;
					list[count] = kingloc[!s];
				}
			}
			break;
		}

		s ^= 1;

		count++;
		if (pins[s] & bit_units[s])
		{/*
			printf("-");
			PrintBitBoard(pins[s]);
			Alg(sq,kingloc[s]);
			printf(" b ");
			z();
			*/
			attack_sq = GetNextAttackerSquarePins(s, s ^ 1, sq, bit_occ, pins[s]);
		}
		else
			attack_sq = GetNextAttackerSquare(s, s ^ 1, sq, bit_occ);

		if (attack_sq > -1)
		{
			score[count + 1] = piece_value[b[attack_sq]];
			if (score[count] > score[count - 1] + score[count + 1])
			{
				count--;
				break;
			}
		}
		else
		{
			break;
		}
		list[count] = attack_sq;
	}

	while (count > 1)
	{
		if (score[count - 1] > score[count - 2])
			count -= 2;
		else
			break;
	}

	int ev = eval;

	if (count > 0)
	{
		MakeCapture(list[0], sq, flags);
		for (int x = 1; x < count; x++)
		{
			MakeRecapture(list[x], sq);
		}
		if (Attack(xside, kingloc[side]))
		{
			int check = Check(xside, kingloc[side]);
			if (IsMate(check))
			{
				for (int x = 1; x < count; x++)
				{
					UnMakeRecapture();
				}
				UnMakeCapture();
				if (side = start_side)
					return ply - 10000;
				else
					return 10000 - ply;
			}
			//BITBOARD pin_mask = GetPinMask(side, xside);
			//ev = QuietEvasion(alpha, beta, pin_mask);
		}
		if (side == start_side)
			ev = Eval(side, xside, alpha, beta);
		else
			ev = -Eval(side, xside, -beta, -alpha);

		for (int x = 1; x < count; x++)
		{
			UnMakeRecapture();
		}
		UnMakeCapture();
	}
	return ev;
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

	b1 = BishopAttacks(sq, bit_occ);
	U64 a = b1 & bit_pieces[s][B] & bit_occ;
	if (a)
		return NextBit(a);

	U64 b2 = RookAttacks(sq, bit_occ);
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
	const int k = kingloc[s];

	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		int sq2 = pawnleft[xs][sq];
		BITBOARD b2 = mask[sq2] & pin_between;
		if (!(b2 && IsOneBit(pin_between & bit_all & ~mask[sq2]) && !(mask[sq] & pin_between)))
		{
			return sq2;
		}
	}
	else if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		int sq2 = pawnright[xs][sq];
		BITBOARD b2 = mask[sq2] & pin_between;
		if (!(b2 && IsOneBit(pin_between & bit_all & ~mask[sq2]) && !(mask[sq] & pin_between)))
		{
			return sq2;
		}
	}
	BITBOARD b1 = bit_knightmoves[sq] & bit_pieces[s][N] & bit_occ;
	if (b1)
	{
		int sq2 = NextBit(b1);
		BITBOARD b2 = mask[sq2] & pin_between;
		if (!(b2 && IsOneBit(pin_between & bit_all & ~mask[sq2])))
		{
			return NextBit(b1);
		}
	}
	b1 = bit_bishopmoves[sq] & bit_pieces[s][B] & bit_occ;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			BITBOARD b2 = mask[sq2] & pin_between;
			if (!(b2 && IsOneBit(pin_between & bit_all & ~mask[sq2]) && !(mask[sq] & pin_between)))
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
			if (!(b2 && IsOneBit(pin_between & bit_all & ~mask[sq2]) && !(mask[sq] & pin_between)))
			{
				return sq2;
			}
		}
		b1 &= b1 - 1;
	}
	for (int x = 0; x < total[s][Q]; x++)
	{
		int i = pieces[s][Q][x];
		if (bit_queenmoves[sq] & mask[i] & bit_occ)
		{
			if (!(bit_between[i][sq] & bit_occ))
			{
				BITBOARD b2 = mask[i] & pin_between;
				if (!(b2 && IsOneBit(pin_between & bit_all & ~mask[i]) && !(mask[sq] & pin_between)))
				{
					return i;
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
