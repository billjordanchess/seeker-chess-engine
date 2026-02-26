#include "globals.h"

static int capture_count;

void AddPawnCaptureQ(const int from, const int to, const int score);
void AddKnightCaptureQ(const int from, const int to, const int score);
void AddBishopCaptureQ(const int from, const int to, const int score);
void AddRookCaptureQ(const int from, const int to, const int score);
void AddQueenCaptureQ(const int from, const int to, const int score);
void AddCaptureQ(const int from, const int to, const int score);
void GenPromoteQ(const int s, const int xs, const int from, const int to);

BITBOARD GetKnightDefences(const int s);
BITBOARD GetBishopDefences(const int s);
BITBOARD GetRookDefences(const int s);
BITBOARD GetKnightDefencesPins(const int s, BITBOARD pin_mask);
BITBOARD GetBishopDefencesPins(const int s, BITBOARD pin_mask);
BITBOARD GetRookDefencesPins(const int s, BITBOARD pin_mask);

move_data* q;

void GenQuietCaptures(const int s, const int xs, const int diff, BITBOARD pin_mask, BITBOARD bit_xpinned)
{
	first_move[ply + 1] = first_move[ply];
	capture_count = first_move[ply];

	BITBOARD bit_targets = 0;

	int from, to;
	BITBOARD b1, b2;

	int k = kingloc[s];

	b1 = bit_pieces[s][P] & mask_ranks[s][6];
	while (b1)
	{
		from = NextBit(b1);
		b1 &= b1 - 1;
		to = pawnplus[s][from];
		if (bit_left[s][from] & bit_units[xs])
		{
			if (mask[from] & pin_mask)
			{
				if (!(SameDiag(from, k, pawnleft[s][from])))
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
				if (!(SameDiag(from, k, pawnright[s][from])))
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
				if (col[from] != col[k])
				{
					continue;
				}
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
			if (diff < B_VALUE)
			{
				bit_targets |= bit_pieces[xs][N] | bit_pieces[xs][B];
				if (diff < P_VALUE + 50)
				{
					bit_targets |= bit_pieces[xs][P];
				}
			}
		}
	}

	//BITBOARD b1 = bit_rookmoves[k] & 

	if (bit_targets == 0)
	{
		first_move[ply + 1] = capture_count;
		return;
	}

	BITBOARD bit_pawndefences = 0;
	BITBOARD bit_knightdefences = 0;
	BITBOARD bit_bishopdefences = 0;
	BITBOARD bit_rookdefences = 0;

	if (bit_xpinned == 0)
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
	}
	else
	{
		if (xs == 0)
		{
			bit_pawndefences = (bit_pieces[0][P] & not_a_file & ~bit_xpinned) << 7;
			bit_pawndefences |= (bit_pieces[0][P] & not_h_file & ~bit_xpinned) << 9;
		}
		else
		{
			bit_pawndefences = (bit_pieces[1][P] & not_a_file & ~bit_xpinned) >> 9;
			bit_pawndefences |= (bit_pieces[1][P] & not_h_file & ~bit_xpinned) >> 7;
		}

		bit_knightdefences = GetKnightDefencesPins(xs, bit_xpinned);
		bit_bishopdefences = GetBishopDefencesPins(xs, bit_xpinned);
		bit_rookdefences = GetRookDefencesPins(xs, bit_xpinned);
	}

	int lastmove = -1;
	if (hply > 1)
	{
		lastmove = game_list[hply - 1].to;
		PieceScore[xs][b[lastmove]][lastmove] += 50;//11/25
	}

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
		from = NextBit(b1);
		b1 &= b1 - 1;
		if (mask[from] & pin_mask)
		{
			if (!(SameDiag(from, k, pawnleft[s][from])))
			{
				continue;
			}
		}
		AddPawnCaptureQ(from, pawnleft[s][from], px[b[pawnleft[s][from]]]);
	}
	while (b2)
	{
		from = NextBit(b2);
		b2 &= b2 - 1;
		if (mask[from] & pin_mask)
		{
			if (!(SameDiag(from, k, pawnright[s][from])))
			{
				continue;
			}
		}
		AddPawnCaptureQ(from, pawnright[s][from], px[b[pawnright[s][from]]]);
	}

	bit_targets &= ~(bit_pieces[xs][P] & bit_pawndefences);

	for (int x = 0; x < total[s][N]; x++)
	{
		from = pieces[s][N][x];
		if (mask[from] & pin_mask)
		{
			continue;
		}
		b2 = bit_knightmoves[from] & bit_targets;
		while (b2)
		{
			to = NextBit(b2);
			b2 &= b2 - 1;
			AddKnightCaptureQ(from, to, b[to]);
		}
	}

	for (int x = 0; x < total[s][B]; x++)
	{
		from = pieces[s][B][x];
		if (mask[from] & pin_mask)
		{
			if (bit_rookmoves[k] & mask[from])
			{
				continue;
			}
			else
			{
				b2 = (bit_after[k][from] & pin_mask & bit_units[xs]);
				if (b2)
				{
					to = NextBit(b2);
					AddBishopCaptureQ(from, to, b[to]);
				}
			}
		}
		else
		{
			b2 = BishopAttacks(from, bit_all) & bit_targets;
			while (b2)
			{
				to = NextBit(b2);
				b2 &= b2 - 1;
				AddBishopCaptureQ(from, to, b[to]);
			}
			b2 = bit_bishopmoves[from] & ~bit_targets & bit_units[xs] & bit_bishopmoves[kingloc[xs]];
			while (b2)
			{
				to = NextBit(b2);
				b2 &= b2 - 1;
				if (!(bit_between[from][to] & bit_all))
				{
					AddBishopCaptureQ(from, to, b[to]);
				}
			}
		}
	}

	bit_targets &= ~((bit_pieces[xs][N] | bit_pieces[xs][B]) & bit_pawndefences);
	bit_targets &= ~(bit_pieces[xs][P] & (bit_knightdefences | bit_bishopdefences));

	for (int x = 0; x < total[s][R]; x++)
	{
		from = pieces[s][R][x];
		if (mask[from] & pin_mask)
		{
			if (bit_bishopmoves[k] & mask[from])
			{
				continue;
			}
			else
			{
				b2 = (bit_after[k][from] & pin_mask & bit_units[xs]);
				if (b2)
				{
					to = NextBit(b2);
					AddRookCaptureQ(from, to, b[to]);
				}
			}
		}
		else
		{
			b2 = RookAttacks(from, bit_all) & bit_targets;				
			while (b2)
			{
				to = NextBit(b2);
				b2 &= b2 - 1;
				AddRookCaptureQ(from, to, b[to]);
			}
		}
	}

	if (bit_pieces[s][Q])
	{
		bit_targets &= ~(bit_pieces[xs][P] & bit_rookdefences);
		bit_targets &= ~((bit_pieces[xs][N] | bit_pieces[xs][B] | bit_pieces[xs][R]) & (bit_knightdefences | bit_bishopdefences));
	}

	for (int x = 0; x < total[s][Q]; x++)
	{
		from = pieces[s][Q][x];
		if (mask[from] & pin_mask)
		{
			b2 = (bit_after[k][from] & pin_mask & bit_units[xs]);
			if (b2)
			{
				to = NextBit(b2);
				AddQueenCaptureQ(from, to, b[to]);
			}
		}
		else
		{
			b2 = QueenAttacks(from, bit_all) & bit_targets;
			while (b2)
			{
				to = NextBit(b2);
				b2 &= b2 - 1;
				AddQueenCaptureQ(from, to, b[to]);
			}
		}
	}

	b1 = bit_kingmoves[kingloc[s]] & bit_targets;

	while (b1)
	{
		to = NextBit(b1);
		b1 &= b1 - 1;
		if (!Attack(xs, to))
			AddCaptureQ(kingloc[s], to, kx[b[to]]);
	}
	if (lastmove > -1)
	{
		PieceScore[xs][b[lastmove]][lastmove] -= 50;//11/25
	}

	first_move[ply + 1] = capture_count;
	if (bit_xpinned && nodes > 10000)
	{
		//printf(" diff %d ", diff);
		//ShowAll(ply);
	}
}

BITBOARD GetKnightDefences(const int s)
{
	BITBOARD b2 = 0;
	for (int x = 0; x < total[s][N]; x++)
	{
		int from = pieces[s][N][x];
		BITBOARD b1 = bit_knightmoves[from] & bit_units[s];
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			b2 |= mask[to];
		}
	}
	return b2;
}

BITBOARD GetBishopDefences(int s)
{
	BITBOARD b2 = 0;
	BITBOARD friends = bit_units[s];
	BITBOARD occ = bit_all;

	for (int x = 0; x < total[s][B]; x++)
	{
		int from = pieces[s][B][x];
		b2 |= BishopAttacks(from, occ) & friends;
	}
	return b2;
}

BITBOARD GetRookDefences(int s)
{
	BITBOARD b2 = 0;
	BITBOARD friends = bit_units[s];
	BITBOARD occ = bit_all;

	for (int x = 0; x < total[s][R]; x++)
	{
		int from = pieces[s][R][x];
		b2 |= RookAttacks(from, occ) & friends;
	}
	return b2;
}

BITBOARD GetKnightDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b2 = 0;
	for (int x = 0; x < total[s][N]; x++)
	{
		int from = pieces[s][N][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = bit_knightmoves[from] & bit_units[s];
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			b2 |= mask[to];
		}
	}
	return b2;
}

BITBOARD GetBishopDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b2 = 0;
	for (int x = 0; x < total[s][B]; x++)
	{
		int from = pieces[s][B][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = bit_bishopmoves[from] & bit_units[s];
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
				b2 |= mask[to];
		}
	}
	return b2;
}

BITBOARD GetRookDefencesPins(const int s, BITBOARD pin_mask)
{
	BITBOARD b2 = 0;
	for (int x = 0; x < total[s][R]; x++)
	{
		int from = pieces[s][R][x];
		if (mask[from] & pin_mask)
			continue;
		BITBOARD b1 = bit_rookmoves[from] & bit_units[s];
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
				b2 |= mask[to];
		}
	}
	return b2;
}

void AddCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score + PieceScore[xside][b[to]][to];//2025
}

void AddPawnCaptureQ(const int from, const int to, const int score)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = score;
}

void AddKnightCaptureQ(const int from, const int to, const int p)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = nx[p] + PieceScore[xside][p][to];//2025
	if (bit_knightmoves[to] & bit_pieces[xside][K])
	{
		q->flags |= CHECK;
	}
}

void AddBishopCaptureQ(const int from, const int to, const int p)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = bx[p] + PieceScore[xside][p][to];//2025
	if (bit_bishopmoves[to] & bit_pieces[xside][K] &&
		!(bit_between[to][kingloc[xside]] & bit_all))
	{
		q->flags |= CHECK;
	}
}

void AddRookCaptureQ(const int from, const int to, const int p)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = rx[p] + PieceScore[xside][p][to];//2025
	if (bit_rookmoves[to] & bit_pieces[xside][K] &&
		!(bit_between[to][kingloc[xside]] & bit_all))
	{
		q->flags |= CHECK;
	}
}

void AddQueenCaptureQ(const int from, const int to, const int p)
{
	q = &move_list[capture_count++];
	q->flags = CAPTURE;
	q->from = from;
	q->to = to;
	q->score = qx[p] + PieceScore[xside][p][to];//2025
	if (bit_queenmoves[to] & bit_pieces[xside][K] &&
		!(bit_between[to][kingloc[xside]] & bit_all))
		q->flags |= CHECK;
}

void GenPromoteQ(const int s, const int xs, const int from, const int to)
{
	if (ply < 1 && currentmax < 2)
	{
		for (int i = KNIGHT; i <= QUEEN; i++)
		{
			q = &move_list[capture_count++];

			q->flags = PROMOTE;
			q->from = from;
			q->to = to;
			q->score = CAPTURE_SCORE + (i * 10);
			if (b[to] != EMPTY)
			{
				q->flags |= CAPTURE;
			}
			if (i == Q)//other types could be added
				if (bit_queenmoves[to] & bit_pieces[xs][K] &&
					!(bit_between[to][kingloc[xs]] & (bit_all ^ mask[from])))
				{
					q->flags |= CHECK;
					q->score += 50;
				}
		}
	}
	else
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
		if (bit_queenmoves[to] & bit_pieces[xs][K] &&
			!(bit_between[to][kingloc[xs]] & (bit_all ^ mask[from])))
		{
			q->flags |= CHECK;
			q->score += 50;
		}
	}
}
