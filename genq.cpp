//11/9/26
#include "globals.h"

static int capture_count;

void AddPawnCaptureQ(const int from, const int to, const int score);
void AddKnightCaptureQ(const int from, const int to, const int score);
void AddBishopCaptureQ(const int from, const int to, const int score);
void AddRookCaptureQ(const int from, const int to, const int score);
void AddQueenCaptureQ(const int from, const int to, const int score, const BITBOARD defended);
void AddCaptureQ(const int from, const int to, const int score);
void GenPromoteQ(const int s, const int xs, const int from, const int to);

BITBOARD GetKnightDefences(const int s);
BITBOARD GetBishopDefences(const int s);
BITBOARD GetRookDefences(const int s);
BITBOARD GetQueenDefences(const int s);
BITBOARD GetKnightDefencesPins(const int s, BITBOARD pin_mask);
BITBOARD GetBishopDefencesPins(const int s, BITBOARD pin_mask);
BITBOARD GetRookDefencesPins(const int s, BITBOARD pin_mask);
BITBOARD GetQueenDefencesPins(const int s, BITBOARD pin_mask);

move_data* q;

void GenQuietCaptures(const int s, const int xs, const int diff, BITBOARD pin_mask, BITBOARD xpin_mask)
{
	first_move[ply + 1] = first_move[ply];
	capture_count = first_move[ply];

	BITBOARD bit_targets = 0;
	BITBOARD b1, b2;

	const int king = kingloc[s];
	const int xking = kingloc[xs];

	b1 = bit_pieces[s][P] & mask_ranks[s][6];
	while (b1)
	{
		int from = NextBit(b1);
		b1 &= b1 - 1;
		int to = pawnplus[s][from];
		if (bit_left[s][from] & bit_units[xs])
		{
			if (mask[from] & pin_mask)
			{
				if (!(SameDiag(from, king, pawnleft[s][from])))
				{
					continue;
				}
			}
			GenPromoteQ(s, xs, from, pawnleft[s][from]);
		}
		if (bit_right[s][from] & bit_units[xs])
		{
			if (mask[from] & pin_mask)
			{
				if (!(SameDiag(from, king, pawnright[s][from])))
				{
					continue;
				}
			}
			GenPromoteQ(s, xs, from, pawnright[s][from]);
		}
		if (b[to] == EMPTY)
		{
			if (mask[from] & pin_mask)
			{
				continue;
			}
			GenPromoteQ(s, xs, from, to);
		}
	}

	if (diff < Q_VALUE + 50)
	{
		bit_targets = bit_pieces[xs][Q];
		if (diff < R_VALUE + 50)
		{
			bit_targets |= bit_pieces[xs][R];
			if (diff < B_VALUE + 50)
			{
				bit_targets |= bit_pieces[xs][N] | bit_pieces[xs][B];
				if (diff < P_VALUE + 50)
				{
					bit_targets |= bit_pieces[xs][P];
				}
			}
		}
	}

	BITBOARD bit_rookchecks = MagicRookAttacks(xking, bit_all) & bit_units[xs];
	BITBOARD bit_bishopchecks = MagicBishopAttacks(xking, bit_all) & bit_units[xs];
	BITBOARD bit_knightchecks = bit_moves[N][xking] & bit_units[xs];

	if ((bit_targets | bit_rookchecks | bit_bishopchecks | bit_knightchecks) == 0)
	{
		first_move[ply + 1] = capture_count;
		return;
	}

	BITBOARD bit_pawndefences = 0;
	BITBOARD bit_knightdefences = 0;
	BITBOARD bit_bishopdefences = 0;
	BITBOARD bit_rookdefences = 0;
	BITBOARD bit_queendefences = 0;
	BITBOARD bit_kingdefences = 0;
	BITBOARD bit_minordefences = 0;

	if (xpin_mask == 0)
	{
		if (xs == 0)
		{
			bit_pawndefences = (bit_pieces[0][P] & not_a_file) << 7;
			bit_pawndefences |= (bit_pieces[0][P] & not_h_file) << 9;
		}
		else
		{
			bit_pawndefences = (bit_pieces[1][P] & not_a_file) >> 9;
			bit_pawndefences |= (bit_pieces[1][P] & not_h_file) >> 7;
		}

		bit_knightdefences = GetKnightDefences(xs);
		bit_bishopdefences = GetBishopDefences(xs);
		bit_rookdefences = GetRookDefences(xs);
		bit_queendefences = GetQueenDefences(xs);
	}
	else
	{
		if (xs == 0)
		{
			bit_pawndefences = (bit_pieces[0][P] & not_a_file & ~xpin_mask) << 7;
			bit_pawndefences |= (bit_pieces[0][P] & not_h_file & ~xpin_mask) << 9;
		}
		else
		{
			bit_pawndefences = (bit_pieces[1][P] & not_a_file & ~xpin_mask) >> 9;
			bit_pawndefences |= (bit_pieces[1][P] & not_h_file & ~xpin_mask) >> 7;
		}

		bit_knightdefences = GetKnightDefencesPins(xs, xpin_mask);
		bit_bishopdefences = GetBishopDefencesPins(xs, xpin_mask);
		bit_rookdefences = GetRookDefencesPins(xs, xpin_mask);
		bit_queendefences = GetQueenDefencesPins(xs, xpin_mask);
	}
	bit_minordefences = bit_knightdefences | bit_bishopdefences;
	bit_kingdefences = bit_moves[K][xking];
	BITBOARD bit_defences = bit_pawndefences | bit_knightdefences
		| bit_bishopdefences | bit_rookdefences | bit_queendefences;

	bit_knightchecks &= ~bit_defences;
	bit_bishopchecks &= ~bit_defences;
	bit_rookchecks &= ~bit_defences;

	bit_defences |= bit_kingdefences;

	if (pin_mask == 0)
	{
		if (s == 0)
		{
			b1 = bit_pieces[0][P] & ((bit_targets & not_h_file) >> 7) & not_rank6;
			b2 = bit_pieces[0][P] & ((bit_targets & not_a_file) >> 9) & not_rank6;
		}
		else
		{
			b1 = bit_pieces[1][P] & ((bit_targets & not_h_file) << 9) & not_rank1;
			b2 = bit_pieces[1][P] & ((bit_targets & not_a_file) << 7) & not_rank1;
		}
		while (b1)
		{
			int from = NextBit(b1);
			b1 &= b1 - 1;
			int to = pawnleft[s][from];
			AddPawnCaptureQ(from, to, px[b[to]]);
		}
		while (b2)
		{
			int from = NextBit(b2);
			b2 &= b2 - 1;
			int to = pawnright[s][from];
			AddPawnCaptureQ(from, to, px[b[to]]);
		}
		BITBOARD bit_not_pawndefended = ~(bit_pieces[xs][P] & bit_pawndefences);
		bit_targets &= bit_not_pawndefended;

		for (int x = 0; x < total[s][N]; x++)
		{
			int from = pieces[s][N][x];
			b2 = bit_moves[N][from] & (bit_targets | bit_knightchecks);
			while (b2)
			{
				int to = NextBit(b2);
				b2 &= b2 - 1;
				AddKnightCaptureQ(from, to, nx[b[to]]);
			}
		}

		for (int x = 0; x < total[s][B]; x++)
		{
			int from = pieces[s][B][x];
			b2 = MagicBishopAttacks(from, bit_all) & (bit_targets | bit_bishopchecks);
			while (b2)
			{
				int to = NextBit(b2);
				b2 &= b2 - 1;
				AddBishopCaptureQ(from, to, bx[b[to]]);
			}
		}
		BITBOARD bit_not_minordefended = ~((bit_pieces[xs][N] | bit_pieces[xs][B]) & bit_pawndefences);
		bit_not_minordefended &= ~(bit_pieces[xs][P] & bit_minordefences);

		bit_targets &= bit_not_minordefended;

		for (int x = 0; x < total[s][R]; x++)
		{
			int from = pieces[s][R][x];
			b2 = MagicRookAttacks(from, bit_all) & (bit_targets | bit_rookchecks);
			while (b2)
			{
				int to = NextBit(b2);
				b2 &= b2 - 1;
				AddRookCaptureQ(from, to, rx[b[to]]);
			}
		}
		if (bit_pieces[s][Q])
		{
			BITBOARD bit_queenchecks = bit_bishopchecks | bit_rookchecks;
			BITBOARD bit_not_rookdefended = ~((bit_pieces[xs][N] | bit_pieces[xs][B] | bit_pieces[xs][R]) & bit_minordefences);
			bit_not_rookdefended &= ~(bit_pieces[xs][P] & bit_rookdefences);
			bit_targets &= bit_not_rookdefended;

			for (int x = 0; x < total[s][Q]; x++)
			{
				int from = pieces[s][Q][x];
				b2 = MagicQueenAttacks(from, bit_all) & (bit_targets | bit_queenchecks);
				while (b2)
				{
					int to = NextBit(b2);
					b2 &= b2 - 1;
					AddQueenCaptureQ(from, to, b[to], bit_defences);
				}
			}
		}
	}
	else
	{
		if (s == 0)
		{
			b1 = bit_pieces[0][P] & ((bit_targets & not_h_file) >> 7) & not_rank6;
			b2 = bit_pieces[0][P] & ((bit_targets & not_a_file) >> 9) & not_rank6;
		}
		else
		{
			b1 = bit_pieces[1][P] & ((bit_targets & not_h_file) << 9) & not_rank1;
			b2 = bit_pieces[1][P] & ((bit_targets & not_a_file) << 7) & not_rank1;
		}
		while (b1)
		{
			int from = NextBit(b1);
			b1 &= b1 - 1;
			int to = pawnleft[s][from];
			if (mask[from] & pin_mask)
			{
				if (!(SameDiag(from, king, to)))
				{
					continue;
				}
			}
			AddPawnCaptureQ(from, to, px[b[to]]);
		}
		while (b2)
		{
			int from = NextBit(b2);
			b2 &= b2 - 1;
			int to = pawnright[s][from];
			if (mask[from] & pin_mask)
			{
				if (!(SameDiag(from, king, to)))
				{
					continue;
				}
			}
			AddPawnCaptureQ(from, to, px[b[to]]);
		}

		bit_targets &= ~(bit_pieces[xs][P] & bit_pawndefences);
		bit_knightchecks &= ~(bit_pieces[xs][P] & bit_pawndefences);

		for (int x = 0; x < total[s][N]; x++)
		{
			int from = pieces[s][N][x];
			if (mask[from] & pin_mask)
			{
				continue;
			}
			b2 = bit_moves[N][from] & (bit_targets | bit_knightchecks);
			while (b2)
			{
				int to = NextBit(b2);
				b2 &= b2 - 1;
				AddKnightCaptureQ(from, to, nx[b[to]]);
			}
		}
		bit_bishopchecks &= ~(bit_pieces[xs][P] & bit_pawndefences);

		for (int x = 0; x < total[s][B]; x++)
		{
			int from = pieces[s][B][x];
			if (mask[from] & pin_mask)
			{
				if (bit_moves[R][king] & mask[from])
				{
					continue;
				}
				else
				{
					b2 = (bit_after[king][from] & pin_mask & bit_units[xs]);
					if (b2)
					{
						int to = NextBit(b2);
						AddBishopCaptureQ(from, to, bx[b[to]]);
					}
				}
			}
			else
			{
				b2 = MagicBishopAttacks(from, bit_all) & (bit_targets | bit_bishopchecks);
				while (b2)
				{
					int to = NextBit(b2);
					b2 &= b2 - 1;
					AddBishopCaptureQ(from, to, bx[b[to]]);
				}
			}
		}

		bit_targets &= ~((bit_pieces[xs][N] | bit_pieces[xs][B]) & bit_pawndefences);
		bit_targets &= ~(bit_pieces[xs][P] & bit_minordefences);

		for (int x = 0; x < total[s][R]; x++)
		{
			int from = pieces[s][R][x];
			if (mask[from] & pin_mask)
			{
				if (bit_moves[B][king] & mask[from])
				{
					continue;
				}
				else
				{
					b2 = (bit_after[king][from] & pin_mask & bit_units[xs]);
					if (b2)
					{
						int to = NextBit(b2);
						AddRookCaptureQ(from, to, rx[b[to]]);
					}
				}
			}
			else
			{
				b2 = MagicRookAttacks(from, bit_all) & (bit_targets | bit_rookchecks);
				while (b2)
				{
					int to = NextBit(b2);
					b2 &= b2 - 1;
					AddRookCaptureQ(from, to, rx[b[to]]);
				}
			}
		}
		if (bit_pieces[s][Q])
		{
			bit_targets &= ~(bit_pieces[xs][P] & bit_rookdefences);
			bit_targets &= ~((bit_pieces[xs][N] | bit_pieces[xs][B] | bit_pieces[xs][R]) & bit_minordefences);

			for (int x = 0; x < total[s][Q]; x++)
			{
				int from = pieces[s][Q][x];
				if (mask[from] & pin_mask)
				{
					b2 = (bit_after[king][from] & pin_mask & bit_units[xs]);
					if (b2)
					{
						int to = NextBit(b2);
						AddQueenCaptureQ(from, to, b[to], bit_defences);
					}
				}
				else
				{
					b2 = MagicQueenAttacks(from, bit_all) & (bit_targets | bit_bishopchecks | bit_rookchecks);
					while (b2)
					{
						int to = NextBit(b2);
						b2 &= b2 - 1;
						AddQueenCaptureQ(from, to, b[to], bit_defences);
					}
				}
			}
		}
	}

	b1 = bit_moves[K][king] & bit_targets & ~bit_defences;
	if (b1)
	{
		if (xpin_mask == 0)
		{
			while (b1)
			{
				int to = NextBit(b1);
				b1 &= b1 - 1;
				AddCaptureQ(king, to, kx[b[to]]);
			}
		}
		else
		{
			while (b1)
			{
				int to = NextBit(b1);
				b1 &= b1 - 1;
				if (!Attack(xs, to, bit_all))
					AddCaptureQ(king, to, kx[b[to]]);
			}
		}
	}
	first_move[ply + 1] = capture_count;
}

BITBOARD GetKnightDefences(const int s)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][N]; x++)
	{
		int from = pieces[s][N][x];
		b1 |= bit_moves[N][from];
	}
	return b1;
}

BITBOARD GetBishopDefences(int s)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][B]; x++)
	{
		int from = pieces[s][B][x];
		b1 |= MagicBishopAttacks(from, bit_all);
	}
	return b1;
}

BITBOARD GetRookDefences(int s)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][R]; x++)
	{
		int from = pieces[s][R][x];
		b1 |= MagicRookAttacks(from, bit_all);
	}
	return b1;
}

BITBOARD GetQueenDefences(int s)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][Q]; x++)
	{
		int from = pieces[s][Q][x];
		b1 |= MagicQueenAttacks(from, bit_all);
	}
	return b1;
}

BITBOARD GetKnightDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][N]; x++)
	{
		int from = pieces[s][N][x];
		if (!(mask[from] & pin_mask))
			b1 |= bit_moves[N][from];
	}
	return b1;
}

BITBOARD GetBishopDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][B]; x++)
	{
		int from = pieces[s][B][x];
		if (!(mask[from] & pin_mask))
			b1 |= MagicBishopAttacks(from, bit_all);
	}
	return b1;
}

BITBOARD GetRookDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][R]; x++)
	{
		int from = pieces[s][R][x];
		if (!(mask[from] & pin_mask))
			b1 |= MagicRookAttacks(from, bit_all);
	}
	return b1;
}

BITBOARD GetQueenDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b1 = 0;
	for (int x = 0; x < total[s][Q]; x++)
	{
		int from = pieces[s][Q][x];
		if (!(mask[from] & pin_mask))
			b1 |= MagicQueenAttacks(from, bit_all);
	}
	return b1;
}

void AddCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score;
}

void AddPawnCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score;
}

void AddKnightCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score;
	if (bit_moves[N][to] & bit_pieces[xside][K])
	{
		q->flags |= CHECK;
	}
}

void AddBishopCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score;
	if (bit_moves[B][to] & bit_pieces[xside][K] &&
		!(bit_between[to][kingloc[xside]] & bit_all))
	{
		q->flags |= CHECK;
	}
}

void AddRookCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score;
	if (bit_moves[R][to] & bit_pieces[xside][K] &&
		!(bit_between[to][kingloc[xside]] & bit_all))
	{
		q->flags |= CHECK;
	}
}

void AddQueenCaptureQ(const int from, const int to, const int p, const BITBOARD defended)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	if (defended)
		q->score = qx[p];
	else
		q->score = piece_value[p];
	if (bit_moves[Q][to] & bit_pieces[xside][K] &&
		!(bit_between[to][kingloc[xside]] & bit_all))
		q->flags |= CHECK;
}

void GenPromoteQ(const int s, const int xs, const int from, const int to)
{
	q = &move_list[capture_count++];
	q->flags = PROMOTE;
	q->score = PROMOTE_SCORE;
	if (b[to] != EMPTY)
	{
		q->flags |= CAPTURE;
		q->score += piece_value[b[to]];
	}
	q->from = from;
	q->to = to;
	if (bit_moves[Q][to] & bit_pieces[xs][K] &&
		!(bit_between[to][kingloc[xs]] & (bit_all ^ mask[from])))
	{
		q->flags |= CHECK;
		q->score += 50;
	}
}

