#include "globals.h"

int p_value[6] = { 1,3,3,5,9,100 };//100? 1/12/25

int SEEQuiet(int s, int attacker, const int sq, const BITBOARD p1, const BITBOARD p2, const int eval, const int, const int, const int);
int GetNextAttackerPins(const int s, const int, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask);
bool SEEAttackKing(const int s, const int sq, const BITBOARD bit_occ);

int QuietEvasion(int alpha, int beta, BITBOARD);

bool IsMate(const int checker);

int SEE(int s, const int attacker, const int sq, const BITBOARD p1, const BITBOARD p2)
{
	int attack_sq = attacker;
	int count = 0;
	int score[12];
	int list[12];

	int total_score = 0;

	score[0] = p_value[b[sq]];
	score[1] = p_value[b[attacker]];

	BITBOARD bit_occ = bit_all;
	BITBOARD pins[2];
	pins[0] = p1;
	pins[1] = p2;
	list[0] = attacker;

	while (count < 10)
	{
		bit_occ &= not_mask[attack_sq];
		s ^= 1;

		count++;
		if (pins[s])
			attack_sq = GetNextAttackerPins(s, s ^ 1, sq, bit_occ, pins[s]);
		else
			attack_sq = GetNextAttacker(s, s ^ 1, sq, bit_occ);

		if (attack_sq > -1)
		{
			score[count + 1] = p_value[b[attack_sq]];
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

int SEEQuiet(int s, const int attacker, const int sq, const BITBOARD p1, const BITBOARD p2, const int eval, const int alpha, const int beta, const int flags)
{
	int attack_sq = attacker;
	int count = 0;
	int score[12];
	int list[12];

	int total_score = 0;

	score[0] = p_value[b[sq]];
	score[1] = p_value[b[attacker]];

	BITBOARD bit_occ = bit_all;
	BITBOARD pins[2];
	pins[0] = p1;
	pins[1] = p2;
	list[0] = attacker;

	

	while (count < 10)
	{
		bit_occ &= not_mask[attack_sq];
		s ^= 1;

		count++;
		if (pins[s])
		{
			if (SEEAttackKing(s ^ 1, kingloc[s], bit_occ))
			{
				/*
				PrintBitBoard(bit_occ);
				printf(" disco ");
				Alg(attacker, sq);
				z();
				//*/
			}
			attack_sq = GetNextAttackerPins(s, s ^ 1, sq, bit_occ, pins[s]);
		}
		else
			attack_sq = GetNextAttacker(s, s ^ 1, sq, bit_occ);

		if (attack_sq > -1)
		{
			score[count + 1] = p_value[b[attack_sq]];
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
		if (score[count - 1] >= score[count - 2])
			count -= 2;
		else
			break;
	}
	/*
	for (int x = 0; x < count; x++)
	{
		if (x % 2 == 0)
			total_score += score[x];
		else
			total_score -= score[x];
	}
	//if (total_score == 0 && count > 0)
	if (count < 0)
	{
		Alg(attacker, sq);
		printf(" ts %d ", total_score);
		z();
		//return eval;
	}
	//*/

	int ev = eval;

	if (list[0] == sq)
	{
		printf(" equal ");
		Alg(sq, sq);
		z();
	}

	if (count > 0)
	{
		MakeCapture(list[0], sq, flags);
		for (int x = 1; x < count; x++)
		{
			MakeRecapture(list[x], sq);
		}
		//*
		if (Attack(xside, kingloc[side]))
		{
			int check = Check(xside, kingloc[side]);
			if(IsMate(check))
			{
			//	printf(" ismate ");
			//Algebraic(sq);
			//z();
			for (int x = 1; x < count; x++)
			{
				UnMakeRecapture();
			}
			UnMakeCapture();
			return ply - 10000;
			}			
			//ev = QuietEvasion(alpha, beta);
		}
		//else
		//*/
		if (count % 2 == 0)
		{		
			ev = Eval(alpha, beta);
		}
		else
		{
			ev = -Eval(beta, alpha);
		}

		for (int x = 1; x < count; x++)
		{
			UnMakeRecapture();
		}
		UnMakeCapture();
	}
	return ev;
}

int GetNextAttacker(const int s, const int xs, const int sq, const BITBOARD bit_occ)
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

int GetNextAttackerPins(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask)
{
	const int k = kingloc[s];

	if (pin_mask)
	{
		//PrintBitBoard(pin_mask);
		//Algebraic(sq);
		//z();
	}
	
	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		if (!isPinnedLinePiece(xs, k, pawnleft[xs][sq], sq, pin_mask))
			return pawnleft[xs][sq];
	}
	else if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		if (!isPinnedLinePiece(xs, k, pawnleft[xs][sq], sq, pin_mask))
			return pawnright[xs][sq];
	}
	BITBOARD b1 = bit_knightmoves[sq] & bit_pieces[s][N] & bit_occ & ~pin_mask;
	if (b1)
	{
		int sq2 = NextBit(b1);
		return NextBit(b1);
	}
	b1 = bit_bishopmoves[sq] & bit_pieces[s][B] & bit_occ;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			if (mask[sq2] & pin_mask)
			{
				//printf(" b ");
				//Alg(sq2, sq);
				//z();
			}
			if (!isPinnedLinePiece(xs, k, sq2, sq, pin_mask))
			return sq2;
		}
		b1 &= b1 - 1;
	}
	b1 = bit_rookmoves[sq] & bit_pieces[s][R] & bit_occ;
	while (b1)
	{
		int sq2 = NextBit(b1);
		if (mask[sq2] & pin_mask)
		{
			//printf(" r ");
			//Alg(sq2, sq);
			//z();
		}
		if (!(bit_between[sq2][sq] & bit_occ))
		{
			if (!isPinnedLinePiece(xs, k, sq2, sq, pin_mask))
			return sq2;
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
				if (mask[i] & pin_mask)
				{
					//printf(" q ");
					//Alg(i, sq);
					//z();
				}
				if (!isPinnedLinePiece(xs, k, i, sq, pin_mask))
					return i;
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

int GetNextAttackerPins2(const int s, const int xs, const int sq, const BITBOARD bit_occ, const BITBOARD pin_mask)
{
	const int k = kingloc[s];

	if (bit_left[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		if (!isPinnedLinePiece(xs, k, pawnleft[xs][sq], sq, pin_mask))
			return pawnleft[xs][sq];
	}
	else if (bit_right[xs][sq] & bit_pieces[s][P] & bit_occ)
	{
		if (!isPinnedLinePiece(xs, k, pawnleft[xs][sq], sq, pin_mask))
			return pawnright[xs][sq];
	}

	for (int x = 0; x < total[s][N]; x++)
	{
		int i = pieces[s][N][x];
		if (mask[i] & pin_mask)
		{
			printf(" n ");
			Alg(i,sq);
			z();
		}
		if (bit_knightmoves[sq] & mask[i] & bit_occ)
		{
			if (!isPinned(xs, k, i, pin_mask))
				return i;
		}
	}
	for (int x = 0; x < total[s][B]; x++)
	{
		int i = pieces[s][B][x];
		if (bit_bishopmoves[sq] & mask[i] & bit_occ)
		{
			if (!(bit_between[i][sq] & bit_occ))
			{
				if (!isPinnedLinePiece(xs, k, i, sq, pin_mask))
					return i;
			}
		}
	}
	for (int x = 0; x < total[s][R]; x++)
	{
		int i = pieces[s][R][x];
		if (bit_rookmoves[sq] & mask[i] & bit_occ)
		{
			if (!(bit_between[i][sq] & bit_occ))
			{
				if (!isPinnedLinePiece(xs, k, i, sq, pin_mask))
					return i;
			}
		}
	}
	for (int x = 0; x < total[s][Q]; x++)
	{
		int i = pieces[s][Q][x];
		if (bit_queenmoves[sq] & mask[i] & bit_occ)
		{
			if (!(bit_between[i][sq] & bit_occ))
			{
				if (!isPinnedLinePiece(xs, k, i, sq, pin_mask))
					return i;
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
