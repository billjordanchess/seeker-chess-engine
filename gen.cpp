//11/9/26
#include "globals.h"

bool IsOneBit(BITBOARD x);

void z();

extern BITBOARD bit_attacked[2][6];
extern BITBOARD bit_kq_defends;
extern int target_bonus[64];
extern BITBOARD bit_undefendable;
extern BITBOARD bit_defendable;
extern BITBOARD bit_unblock;
extern BITBOARD bit_line;

extern int block[64], unblock[64];

void GenDisco(const int x, const BITBOARD mask_line, BITBOARD pin_mask);
void AddDisco(const int from, const int to, const int piece);
void RemoveDiscoChecks(const int startmoves);

bool SameDiag(const int a, const int b, const int c);
bool SameLine(const int a, const int b, const int c);

BITBOARD bit_disco_squares[64];

BITBOARD bit_attackable[6];

int move_count;

static int c = CAPTURE_SCORE;
const int px[6] = { 0 + c,c + 100,c + 200,c + 300,c + 400,0 + c };
const int nx[6] = { c - 30,c + 70,c + 170,c + 270,c + 370,c + 0 };
const int bx[6] = { c - 30,c + 70,c + 170,c + 270,c + 370,c + 0 };
const int rx[6] = { c - 50,c + 50,c + 150,c + 250,c + 350,c + 0 };
const int qx[6] = { c - 90,c + 10,c + 110,c + 210,c + 310,c + 0 };
const int kx[6] = { c + 00,c + 100,c + 200,c + 300,c + 400,c + 0 };

void BishopMoves(const int s, const int xs, const int x, const BITBOARD);
void RookMoves(const int s, const int xs, const int x, const int, const BITBOARD);
void QueenMoves(const int s, const int xs, const int x, const int, const BITBOARD);

void GenEP(BITBOARD);
bool isSafeEP(const int ep, const int from);
bool LineAttack2(const int s, const int sq, const BITBOARD occ);

void AddCastle(const int from, const int to);

void AddPawnMove(const int s, const int xs, const int from, const int to);
void AddKnightMove(const int s, const int xs, const int from, const int to, const int);
void AddBishopMove(const int s, const int xs, const int from, const int to, const int, const BITBOARD);
void AddRookMove(const int s, const int xs, const int from, const int to, const int, const BITBOARD);
void AddQueenMove(const int s, const int xs, const int from, const int to, const int, const BITBOARD);
void AddKingMove(const int s, const int xs, const int from, const int to);

void AddCastle(const int from, const int to);
void AddCheck(const int from, const int to, const int);
void AddEP(const int from, const int to);
void AddCapture(const int from, const int to, const unsigned int flags, const int score);

void AddPawnCapture(const int from, const int to, const int score);

void GenQuietMoves(const int s, const int xs, BITBOARD pin_mask, const BITBOARD(&bit_check)[6]);

BITBOARD bit_brq_attacks[64];

move_data* g;

void GenCaptures(const int s, const int xs, BITBOARD pin_mask)
{
	first_move[ply + 1] = first_move[ply];
	move_count = first_move[ply];

	BITBOARD b1, b2;
	BITBOARD bit_targets = bit_units[xs];

	const int king = kingloc[s];
	const int xking = kingloc[xs];

	GenEP(0);

	if (pin_mask == 0)
	{
		b1 = bit_pieces[s][P] & mask_ranks[s][6];
		while (b1)
		{
			const int from = NextBit(b1);
			b1 &= b1 - 1;
			const int left = pawnleft[s][from];
			if (bit_left[s][from] & bit_units[xs])
			{
				GenPromote(s, xs, from, left);
			}
			const int right = pawnright[s][from];
			if (bit_right[s][from] & bit_units[xs])
			{
				GenPromote(s, xs, from, right);
			}
			const int to = pawnplus[s][from];
			if (b[to] == EMPTY)
			{
				GenPromote(s, xs, from, to);
			}
		}
		/*
		b1 = bit_pieces[s][P] & mask_ranks[s][5];
		while (b1)
		{
			const int from = NextBit(b1);
			b1 &= b1 - 1;
			if (b[pawnplus[s][from] == EMPTY)
				AddPawnMove(s, xs, from, pawnplus[s][from]);
		}
		*/

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
			const int from = NextBit(b1);
			const int to = pawnleft[s][from];
			b1 &= b1 - 1;
			AddPawnCapture(from, to, px[b[to]]);
		}
		while (b2)
		{
			const int from = NextBit(b2);
			const int to = pawnright[s][from];
			b2 &= b2 - 1;
			AddPawnCapture(from, to, px[b[to]]);
		}
		for (int x = 0; x < total[s][N]; x++)
		{
			const int from = pieces[s][N][x];
			b2 = bit_moves[N][from] & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_moves[N][to] & bit_pieces[xs][K])
					flags = CHECK;
				AddCapture(from, to, flags, nx[piece] + PieceScore[xs][piece][to]);
				b2 &= b2 - 1;
			}
		}
		for (int x = 0; x < total[s][B]; x++)
		{
			const int from = pieces[s][B][x];
			b2 = slider_moves[from][ply] & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_moves[B][to] & bit_pieces[xs][K] &&
					!(bit_between[to][xking] & bit_all))
					flags = CHECK;
				AddCapture(from, to, flags, bx[piece] + PieceScore[xs][piece][to]);
				b2 &= b2 - 1;
			}
		}

		for (int x = 0; x < total[s][R]; x++)
		{
			const int from = pieces[s][R][x];
			b2 = slider_moves[from][ply] & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_moves[R][to] & bit_pieces[xs][K] &&
					!(bit_between[to][xking] & bit_all))
					flags = CHECK;
				AddCapture(from, to, flags, rx[piece] + PieceScore[xs][piece][to]);
				b2 &= b2 - 1;
			}
		}

		for (int x = 0; x < total[s][Q]; x++)
		{
			const int from = pieces[s][Q][x];
			b2 = slider_moves[from][ply] & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_moves[Q][to] & bit_pieces[xs][K] &&
					!(bit_between[to][xking] & bit_all))
					flags = CHECK;
				AddCapture(from, to, flags, qx[piece] + PieceScore[xs][piece][to]);
				b2 &= b2 - 1;
			}
		}
	}
	else
	{
		{
			b1 = bit_pieces[s][P] & mask_ranks[s][6];
			while (b1)
			{
				const int from = NextBit(b1);
				b1 &= b1 - 1;
				const int left = pawnleft[s][from];
				if (bit_left[s][from] & bit_units[xs])
				{
					if (mask[from] & pin_mask)
					{
						if (!(SameDiag(from, left, king)))
						{
							continue;
						}
					}
					GenPromote(s, xs, from, left);
				}
				const int right = pawnright[s][from];
				if (bit_right[s][from] & bit_units[xs])
				{
					if (mask[from] & pin_mask)
					{
						if (!(SameDiag(from, right, king)))
						{
							continue;
						}
					}
					GenPromote(s, xs, from, right);
				}
				const int to = pawnplus[s][from];
				if (b[to] == EMPTY)
				{
					if (mask[from] & pin_mask)
					{
						continue;
					}
					GenPromote(s, xs, from, to);
				}
			}
			/*
			b1 = bit_pieces[s][P] & mask_ranks[s][5];
			while (b1)
			{
				const int from = NextBit(b1);
				b1 &= b1 - 1;
				if (b[pawnplus[s][from] == EMPTY)
					AddPawnMove(s, xs, from, pawnplus[s][from]);
			}
			*/

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
				const int from = NextBit(b1);
				const int to = pawnleft[s][from];
				b1 &= b1 - 1;
				if (mask[from] & pin_mask)
				{
					if (!(SameDiag(from, to, king)))
					{
						continue;
					}
				}
				AddPawnCapture(from, to, px[b[to]]);
			}
			while (b2)
			{
				const int from = NextBit(b2);
				const int to = pawnright[s][from];
				b2 &= b2 - 1;
				if (mask[from] & pin_mask)
				{
					if (!(SameDiag(from, king, to)))
					{
						continue;
					}
				}
				AddPawnCapture(from, to, px[b[to]]);
			}
			for (int x = 0; x < total[s][N]; x++)
			{
				const int from = pieces[s][N][x];
				if (mask[from] & pin_mask)
				{
					continue;
				}
				b2 = bit_moves[N][from] & bit_targets;
				while (b2)
				{
					const int to = NextBit(b2);
					const int piece = b[to];
					unsigned int flags = 0;
					if (bit_moves[N][to] & bit_pieces[xs][K])
						flags = CHECK;
					AddCapture(from, to, flags, nx[piece] + PieceScore[xs][piece][to]);
					b2 &= b2 - 1;
				}
			}
			for (int x = 0; x < total[s][B]; x++)
			{
				const int from = pieces[s][B][x];
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
							const int to = NextBit(b2);
							int piece = b[to];
							AddCapture(from, to, 0, bx[piece] + PieceScore[xs][piece][to]);
						}
					}
				}
				else
				{
					b2 = slider_moves[from][ply] & bit_targets;
					while (b2)
					{
						const int to = NextBit(b2);
						const int piece = b[to];
						unsigned int flags = 0;
						if (bit_moves[B][to] & bit_pieces[xs][K] &&
							!(bit_between[to][xking] & bit_all))
							flags = CHECK;
						AddCapture(from, to, flags, bx[piece] + PieceScore[xs][piece][to]);
						b2 &= b2 - 1;
					}
				}
			}

			for (int x = 0; x < total[s][R]; x++)
			{
				const int from = pieces[s][R][x];
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
							const int to = NextBit(b2);
							const int piece = b[to];
							AddCapture(from, to, 0, rx[piece] + PieceScore[xs][piece][to]);
						}
					}
				}
				else
				{
					b2 = slider_moves[from][ply] & bit_targets;
					while (b2)
					{
						const int to = NextBit(b2);
						const int piece = b[to];
						unsigned int flags = 0;
						if (bit_moves[R][to] & bit_pieces[xs][K] &&
							!(bit_between[to][xking] & bit_all))
							flags = CHECK;
						AddCapture(from, to, flags, rx[piece] + PieceScore[xs][piece][to]);
						b2 &= b2 - 1;
					}
				}
			}

			for (int x = 0; x < total[s][Q]; x++)
			{
				const int from = pieces[s][Q][x];
				if (mask[from] & pin_mask)
				{
					b2 = (bit_after[king][from] & pin_mask & bit_units[xs]);
					if (b2)
					{
						const int to = NextBit(b2);
						const int piece = b[to];
						AddCapture(from, to, 0, qx[piece] + PieceScore[xs][piece][to]);
					}
				}
				else
				{
					b2 = slider_moves[from][ply] & bit_targets;
					while (b2)
					{
						const int to = NextBit(b2);
						const int piece = b[to];
						unsigned int flags = 0;
						if (bit_moves[Q][to] & bit_pieces[xs][K] &&
							!(bit_between[to][xking] & bit_all))
							flags = CHECK;
						AddCapture(from, to, flags, qx[piece] + PieceScore[xs][piece][to]);
						b2 &= b2 - 1;
					}
				}
			}
		}
	}
	b1 = bit_moves[K][king] & bit_targets;

	while (b1)
	{
		const int to = NextBit(b1);
		const int piece = b[to];
		if (!Attack(xs, to, bit_all))
			AddCapture(king, to, 0, kx[piece] + PieceScore[xside][piece][to]);
		b1 &= b1 - 1;
	}
	first_move[ply + 1] = move_count;
}

void GenEP(BITBOARD pin_mask)
{
	const int ep = game_list[hply - 1].to;

	if (bit_pieces[xside][P] & mask[ep] && abs(game_list[hply - 1].from - ep) == 16 && row2[side][ep] == 4)
	{
		if (col[ep] > 0 && mask[ep - 1] & bit_pieces[side][P])
		{
			if (!(mask[ep - 1] & pin_mask))
				if (isSafeEP(ep, ep - 1))
				{
					AddEP(ep - 1, pawnplus[side][ep]);
				}
		}
		if (col[ep] < 7 && mask[ep + 1] & bit_pieces[side][P])
		{
			if (!(mask[ep + 1] & pin_mask))
				if (isSafeEP(ep, ep + 1))
				{
					AddEP(ep + 1, pawnplus[side][ep]);
				}
		}
	}
}

bool isSafeEP(const int ep, const int from)
{
	if (!(LineAttack(xside, kingloc[side], bit_all & ~mask[ep] & ~mask[from])))
	{
		return true;
	}
	return false;
}

void AddEP(const int from, const int to)
{
	g = &move_list[move_count++];
	g->flags = EP;
	g->from = from;
	g->to = to;
	g->score = px[P];
}

void AddCastle(const int from, const int to)
{
	g = &move_list[move_count++];
	g->flags = CASTLE;
	g->from = from;
	g->to = to;
	g->score = 1000;//
}

void AddCapture(const int from, const int to, const unsigned int flags, const int score)
{
	g = &move_list[move_count++];
	g->from = from;
	g->to = to;
	g->flags = CAPTURE | flags;
	g->score = score;
}

void AddPawnCapture(const int from, const int to, const int score)
{
	g = &move_list[move_count++];
	g->flags = CAPTURE;
	g->from = from;
	g->to = to;
	g->score = score;
}

void AddPawnMove(const int s, const int xs, const int from, const int to)
{
	if (bit_pawncaptures[s][to] & bit_pieces[xs][K])
	{
		return;
	}
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][P][from] + hist_to[s][P][to];

	if (mask[to] & mask_ranks[s][6])
	{
		g->score += ATTACK_SCORE + 10000;
		g->flags |= PASSED7;
	}
	if (target_bonus[from])
	{
		g->score = ESCAPE_SCORE;
	}
	BITBOARD b1 = bit_pawncaptures[s][from] & bit_units[s] & ~bit_defend_to[s][P] & bit_total_attacked[xs] & ~bit_total_weaker[s];
	while (b1)
	{
		int square = NextBit(b1);
		b1 &= b1 - 1;
		if (!(bit_pawncaptures[xs][square] & bit_pieces[s][P] & ~mask[from]))
		{
			g->score = -EN_PRISE_SCORE;
			break;
		}
	}
	b1 = bit_pawncaptures[side][to] & bit_defendable;
	if (b1)
	{
		int square = NextBit(b1);
		g->score += DEFEND_SCORE + piece_value[b[square]];
	}
	//*
	if (bit_defendable && mask[from] & bit_unblock && (mask[to] & bit_unblock) == 0)
	{
		int sq = unblock[from];
		g->score += DEFEND_SCORE;// +piece_value[b[sq]];
	}
	//*/
	if (mask[to] & bit_line)
	{
		g->score += DEFEND_SCORE;// +piece_value[b[block[attacker]]];
	}
	if (bit_pawncaptures[s][to] & bit_units[xs] & (~bit_pieces[xs][P] | bit_total_attacked[s]))//
	{
		g->score += 500 + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
}

void AddKnightMove(const int s, const int xs, const int from, const int to, const int bonus)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][N][from] + hist_to[s][N][to];

	if (mask[to] & bit_weaker[xs][N] ||
		(mask[to] & bit_total_attacked[xs] && !(mask[to] & bit_defend_to[s][N]) &&
			!(bit_moves[N][to] & bit_pieces[s][N] & ~mask[from])))
	{
		g->score = -EN_PRISE_MINOR;
	}
	else
	{
		BITBOARD b1 = bit_defendable;
		while (b1)
		{
			int square = NextBit(b1);
			b1 &= b1 - 1;
			if ((bit_moves[N][to] & mask[square]) && square != from)
			{
				g->score += DEFEND_SCORE + piece_value[b[square]];
			}
		}
		if (bit_defendable && mask[from] & bit_unblock)
		{
			int sq = unblock[from];
			g->score += DEFEND_SCORE;// +piece_value[b[sq]];
		}
		if (mask[to] & bit_line)
		{
			g->score += DEFEND_SCORE;// +piece_value[b[block[attacker]]];
		}
	}
	BITBOARD b1 = bit_kq_defends & bit_total_attacked[s] & bit_units[xs];
	if (bit_moves[N][to] & (bit_attackable[N] | b1))
	{
		g->score += 40 + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
	g->score += bonus;
}

void AddBishopMove(const int s, const int xs, const int from, const int to, const int bonus, const BITBOARD tied)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][B][from] + hist_to[s][B][to];

	if (mask[to] & bit_weaker[xs][B] ||
		(bit_total_attacked[xs] & mask[to] && !(mask[to] & bit_defend_to[s][B])))
	{
		g->score = -EN_PRISE_MINOR;
	}
	else
	{
		if (tied & ~bit_total_weaker[s] && (!(bit_moves[B][to] & tied & ~bit_total_weaker[s])))
		{
			g->score = -EN_PRISE_SCORE;
		}
		BITBOARD b1 = bit_defendable;
		while (b1)
		{
			int square = NextBit(b1);
			b1 &= b1 - 1;
			if (bit_moves[B][to] & mask[square] &&
				!(bit_between[to][square] & bit_all) &&
				!(bit_moves[B][from] & mask[square] &&
					!(bit_between[from][square] & bit_all)))
			{
				g->score += DEFEND_SCORE + piece_value[b[square]];
			}
		}
		if (bit_defendable && mask[from] & bit_unblock && !(mask[to] & bit_unblock))
		{
			int sq = unblock[from];
			g->score += DEFEND_SCORE;// +piece_value[b[sq]];
		}
		if (mask[to] & bit_line)
		{
			g->score += DEFEND_SCORE;// +piece_value[b[block[attacker]]];
		}
	}
	BITBOARD b1 = MagicBishopAttacks(to, bit_all) &
		(bit_attackable[B] | (bit_kq_defends & bit_total_attacked[s] & bit_units[xs])) & ~bit_moves[B][from];
	if (b1)
	{
		/*
		if (b1 & ~bit_attackable[B])
		{
			PrintBitBoard(bit_kq_defends & bit_units[xs]);
			PrintBitBoard(bit_total_attacked[s]);
			printf("kq ");
			Alg(from, to);
			z();
		}
		*/
		int sq = NextBit(b1);
		g->score += piece_value[b[sq]] + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
	g->score += bonus;
}

void AddRookMove(const int s, const int xs, const int from, const int to, const int bonus, const BITBOARD tied)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][R][from] + hist_to[s][R][to];

	if (mask[to] & bit_weaker[xs][R] ||
		(bit_total_attacked[xs] & mask[to] &&
			!(mask[to] & bit_defend_to[s][R]) &&
			!RookQueenAttack(s, from, to)))
	{
		g->score = -EN_PRISE_ROOK;
	}
	else
	{
		BITBOARD b1 = bit_defendable;
		while (b1)
		{
			int square = NextBit(b1);
			b1 &= b1 - 1;
			if (bit_moves[R][to] & mask[square] &&
				!(bit_between[to][square] & bit_all) &&
				!(bit_moves[R][from] & mask[square] &&
					!(bit_between[from][square] & bit_all)))
			{
				g->score += DEFEND_SCORE + piece_value[b[square]];
			}
		}
		if (bit_defendable && mask[from] & bit_unblock && !(mask[to] & bit_unblock))
		{
			int sq = unblock[from];
			g->score += DEFEND_SCORE;// +piece_value[b[sq]];
		}
		b1 = tied & ~bit_total_weaker[s] & ~bit_moves[R][to];
		while (b1)
		{
			int square = NextBit(b1);
			b1 &= b1 - 1;
			if (!(MagicRookAttacks(square, bit_all) & bit_pieces[s][R] & ~mask[from]))
			{
				g->score += -EN_PRISE_SCORE - piece_value[b[square]];
			}
		}
	}

	BITBOARD b1 = MagicRookAttacks(to, bit_all) &
		(bit_attackable[R] | (bit_kq_defends & bit_total_attacked[s] & bit_units[xs])) & ~bit_moves[R][from];
	if (b1)
	{
		int sq = NextBit(b1);
		g->score += piece_value[b[sq]] + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
	g->score += bonus;
}

void AddQueenMove(const int s, const int xs, const int from, const int to, const int bonus, const BITBOARD tied)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][Q][from] + hist_to[s][Q][to];

	if (mask[to] & bit_weaker[xs][Q] ||
		(bit_total_attacked[xs] & mask[to] && !(mask[to] & bit_defend_to[s][Q])))
	{
		g->score = -EN_PRISE_QUEEN;
	}
	else
	{
		if (tied & ~bit_total_weaker[s] && !(MagicQueenAttacks(to, bit_all & ~mask[from]) & tied))
		{
			g->score = -EN_PRISE_SCORE;
		}
		BITBOARD b1 = bit_defendable & ~mask[from];
		while (b1)
		{
			int square = NextBit(b1);
			b1 &= b1 - 1;
			if (bit_moves[Q][to] & mask[square] &&
				!(bit_between[to][square] & bit_all) &&
				!(bit_moves[Q][from] & mask[square] &&
					!(bit_between[from][square] & bit_all)))
			{
				g->score += DEFEND_SCORE + piece_value[b[square]];
			}
		}
		b1 = MagicQueenAttacks(to, bit_all) &
			(bit_attackable[Q] | (bit_kq_defends & bit_total_attacked[s] & bit_units[xs])) & ~bit_moves[Q][from];
		if (b1)
		{
			int sq = NextBit(b1);
			g->score += piece_value[b[sq]] + ATTACK_SCORE;
			g->flags |= ATTACK;
		}
		g->score += kingqueen[to][kingloc[xs]] - kingqueen[from][kingloc[xs]];//
		g->score += bonus;
	}
}

void AddKingMove(const int s, const int xs, const int from, const int to)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][K][from] + hist_to[s][K][to];

	BITBOARD b1 = bit_moves[K][from] & bit_units[s] & ~bit_defend_to[s][K] & bit_total_attacked[xs] & ~bit_total_weaker[s];
	b1 &= ~bit_moves[K][to];
	if (b1)
	{
		g->score = -EN_PRISE_SCORE;
	}
	b1 = bit_defendable;
	while (b1)
	{
		int square = NextBit(b1);
		b1 &= b1 - 1;
		if ((bit_moves[K][to] & mask[square]) &&
			!(bit_moves[K][from] & mask[square]))
		{
			g->score += DEFEND_SCORE + piece_value[b[square]];
			continue;
		}
	}
	if (bit_moves[K][to] & bit_units[xs] & bit_undefended_squares[xs])
	{
		g->score += 40 + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
}

void GenPromote(const int s, const int xs, const int from, const int to)
{
	if (ply < 1 && currentmax < 2)
	{
		for (int i = KNIGHT; i <= QUEEN; i++)
		{
			g = &move_list[move_count++];

			g->flags = PROMOTE;
			g->from = from;
			g->to = to;
			g->score = CAPTURE_SCORE + (i * 10);
			if (b[to] != EMPTY)
			{
				g->flags |= CAPTURE;
			}
			if (i == Q)//other types could be added
				if (bit_moves[Q][to] & bit_pieces[xs][K] &&
					!(bit_between[to][kingloc[xs]] & (bit_all ^ mask[from])))
				{
					g->flags |= CHECK;
					g->score += 50;
				}
		}
	}
	else
	{
		g = &move_list[move_count++];
		g->flags = PROMOTE;
		g->score = PROMOTE_SCORE;
		if (b[to] != EMPTY)
		{
			g->flags |= CAPTURE;
			g->score += piece_value[b[to]];
		}
		g->from = from;
		g->to = to;
		if (bit_moves[Q][to] & bit_pieces[xs][K] &&
			!(bit_between[to][kingloc[xs]] & (bit_all ^ mask[from])))
		{
			g->flags |= CHECK;
			g->score += 50;
		}
	}
}

void GenQuietMoves(const int s, const int xs, BITBOARD pin_mask, const BITBOARD(&bit_check)[6])
{
	move_count = first_move[ply + 1];

	bit_attackable[N] = ~bit_pieces[xs][N] & (bit_undefended_squares[xs] | bit_pieces[xs][R] | bit_pieces[xs][Q]);
	bit_attackable[B] = ~bit_pieces[xs][B] & (bit_undefended_squares[xs] | bit_pieces[xs][R] | bit_pieces[xs][Q]);
	bit_attackable[R] = ~bit_pieces[xs][R] & (bit_undefended_squares[xs] | bit_pieces[xs][Q]);
	bit_attackable[Q] = bit_undefended_squares[xs];

	const int king = kingloc[s];

	BITBOARD bit_pawns;

	if (s == 0)
	{
		bit_pawns = bit_pieces[0][P] & not_rank6 & ~(bit_all >> 8) & ~bit_check[P];
	}
	else
	{
		bit_pawns = bit_pieces[1][P] & not_rank1 & ~(bit_all << 8) & ~bit_check[P];
	}

	while (bit_pawns)
	{
		int from = NextBit(bit_pawns);
		bit_pawns &= bit_pawns - 1;

		if (!(mask[from] & pin_mask) || col[from] == col[king])
		{
			AddPawnMove(s, xs, from, pawnplus[s][from]);
			if (row2[s][from] == 1 && b[pawndouble[s][from]] == EMPTY)
			{
				AddPawnMove(s, xs, from, pawndouble[s][from]);
			}
		}
		else
		{
			if (col[from] != col[king])
			{
				continue;
			}
		}
	}
	
	if (s == 0) {
		if (castle & CASTLE_WK && !(bit_e1h1 & bit_all) && Attack(1, F1, bit_all) == 0 && Attack(1, G1, bit_all) == 0)
			AddCastle(E1, G1);
		if (castle & CASTLE_WQ && !(bit_e1a1 & bit_all) && Attack(1, D1, bit_all) == 0 && Attack(1, C1, bit_all) == 0)
			AddCastle(E1, C1);
	}
	else {
		if (castle & CASTLE_BK && !(bit_e8h8 & bit_all) && Attack(0, F8, bit_all) == 0 && Attack(0, G8, bit_all) == 0)
			AddCastle(E8, G8);
		if (castle & CASTLE_BQ && !(bit_e8a8 & bit_all) && Attack(0, D8, bit_all) == 0 && Attack(0, C8, bit_all) == 0)
			AddCastle(E8, C8);
	}

	for (int x = 0; x < total[s][N]; x++)
	{
		int from = pieces[s][N][x];
		if (mask[from] & pin_mask)
		{
			continue;
		}
		int bonus = 0;
		if (target_bonus[from])
		{
			bonus = ESCAPE_SCORE + target_bonus[from];
		}

		BITBOARD b1 = bit_moves[N][from] & bit_units[s] & ~bit_defend_to[s][N] & bit_total_attacked[xs] & ~bit_total_weaker[s];
		while (b1)
		{
			int square = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_moves[N][square] & bit_pieces[s][N] & ~mask[from]))
			{
				bonus += -EN_PRISE_SCORE - piece_value[b[square]];
			}
		}

		b1 = bit_moves[N][from] & ~bit_all & ~bit_check[N];
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			AddKnightMove(s, xs, from, to, bonus);
		}
	}

	if (pin_mask == 0)
	{
		for (int x = 0; x < total[s][B]; x++)
		{
			BishopMoves(s, xs, pieces[s][B][x], bit_check[B]);
		}
		for (int x = 0; x < total[s][R]; x++)
		{
			RookMoves(s, xs, pieces[s][R][x], x, bit_check[R]);
		}
		for (int x = 0; x < total[s][Q]; x++)
		{
			QueenMoves(s, xs, pieces[s][Q][x], x, bit_check[Q]);
		}
	}
	else
	{
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
					int pinner = NextBit(bit_after[king][from] & pin_mask & bit_units[xs]);
					BITBOARD b1 = bit_between[king][pinner] & ~mask[from] & ~bit_check[B];
					while (b1)
					{
						int to = NextBit(b1);
						b1 &= b1 - 1;
						AddBishopMove(s, xs, from, to, 0, 0);
					}
				}
			}
			else
				BishopMoves(s, xs, pieces[s][B][x], bit_check[B]);
		}
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
					int pinner = NextBit(bit_after[king][from] & pin_mask & bit_units[xs]);
					BITBOARD b1 = bit_between[king][pinner] & ~mask[from] & ~bit_check[R];
					while (b1)
					{
						int to = NextBit(b1);
						b1 &= b1 - 1;
						AddRookMove(s, xs, from, to, 0, 0);
					}
				}
			}
			else
			{
				RookMoves(s, xs, pieces[s][R][x], x, bit_check[R]);
			}
		}
		for (int x = 0; x < total[s][Q]; x++)
		{
			int from = pieces[s][Q][x];
			if (mask[from] & pin_mask)
			{
				int pinner = NextBit(bit_after[king][from] & pin_mask & bit_units[xs]);
				BITBOARD b1 = bit_between[king][pinner] & ~mask[from] & ~bit_check[Q];
				while (b1)
				{
					int to = NextBit(b1);
					b1 &= b1 - 1;
					AddQueenMove(s, xs, from, to, 0, 0);
				}
			}
			else
				QueenMoves(s, xs, pieces[s][Q][x], x, bit_check[Q]);
		}		
	}

	BITBOARD b1 = bit_moves[K][king] & ~bit_all & ~bit_total_attacked[xs];
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddKingMove(s, xs, king, to);
	}
	first_move[ply + 1] = move_count;

}

void BishopMoves(const int s, const int xs, const int from, const BITBOARD bit_check_squares)
{
	int bonus = 0;
	if (target_bonus[from])
	{
		bonus = ESCAPE_SCORE + target_bonus[from];
	}
	BITBOARD b1 = slider_moves[from][ply] & ~bit_all & ~bit_check_squares;
	BITBOARD tied = bit_moves[B][from] & bit_attacked[s][B] & bit_units[s] & ~bit_defend_to[s][B] & bit_total_attacked[xs];

	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddBishopMove(s, xs, from, to, bonus, tied);
	}
}

void RookMoves(const int s, const int xs, const int from, const int n, const BITBOARD bit_check_squares)
{
	int bonus = 0;
	if (target_bonus[from])
	{
		bonus = ESCAPE_SCORE + target_bonus[from];
	}
	BITBOARD b1 = slider_moves[from][ply] & ~bit_all & ~bit_check_squares;
	BITBOARD tied = bit_attacked[s][R] & bit_units[s] & ~bit_defend_to[s][R] & bit_total_attacked[xs];
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddRookMove(s, xs, from, to, bonus, tied);
	}
}

void QueenMoves(const int s, const int xs, const int from, const int n, const BITBOARD bit_check_squares)
{
	int bonus = 0;
	if (target_bonus[from])
	{
		bonus = ESCAPE_SCORE + target_bonus[from];
	}
	BITBOARD b1 = slider_moves[from][ply] & ~bit_all & ~bit_check_squares;
	BITBOARD tied = bit_attacked[s][Q] & bit_units[s] & ~bit_defend_to[s][Q] & bit_total_attacked[xs];
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddQueenMove(s, xs, from, to, bonus, tied);
	}
}

BITBOARD DiscoPossible(const int s, const int xs)
{
	const int king = kingloc[xs];
	BITBOARD b1 = bit_moves[R][king] & (bit_pieces[s][R] | bit_pieces[s][Q]);
	b1 |= bit_moves[B][king] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	return b1;
}

BITBOARD GenChecks(const int s, const int xs, BITBOARD pin_mask)
{
	move_count = first_move[ply + 1];

	int to, from;
	const int xking = kingloc[xs];

	memset(bit_disco_squares, 0, sizeof(bit_disco_squares));

	BITBOARD b1, b2, b3;

	BITBOARD bit_disco_pieces = 0;

	BITBOARD bit_disco = DiscoPossible(s, xs);

	BITBOARD bit_bishop_checks = MagicBishopAttacks(xking, bit_all) & ~bit_all;
	BITBOARD bit_rook_checks = MagicRookAttacks(xking, bit_all) & ~bit_all;
	BITBOARD bit_queen_checks = bit_bishop_checks | bit_rook_checks & ~bit_all;

	while (bit_disco)
	{
		from = NextBit(bit_disco);
		bit_disco &= bit_disco - 1;
		BITBOARD between = bit_between[from][xking];
		if (!(between & bit_units[xs]) && IsOneBit(between & bit_units[s]))
		{
			int sq2 = NextBit(between & bit_units[s]);
			GenDisco(sq2, between, pin_mask);
			bit_disco_pieces |= mask[sq2];
		}
	}

	const int king = kingloc[s];

	if (s == 0)
		b1 = (bit_pieces[0][0] << 8) & ~bit_all;
	else
		b1 = (bit_pieces[1][0] >> 8) & ~bit_all;

	b2 = b1 & bit_pawncaptures[xs][xking];
	while (b2)
	{
		int to = NextBit(b2);
		b2 &= b2 - 1;
		from = pawnplus[xs][to];
		if (mask[from] & pin_mask)
			continue;
		AddCheck(from, to, P);
	}
	if (row2[xs][xking] == 3)
	{
		if (s == 0)
		{
			b1 = ((bit_pieces[0][0] & mask_ranks[0][1]) << 8) & ~bit_all;
			b2 = (b1 << 8) & ~bit_all;
			b3 = b2 & bit_pawncaptures[xs][xking];
		}
		else
		{
			b1 = ((bit_pieces[1][0] & mask_ranks[1][1]) >> 8) & ~bit_all;
			b2 = (b1 >> 8) & ~bit_all;
			b3 = b2 & bit_pawncaptures[xs][xking];
		}
		while (b3)
		{
			int to = NextBit(b3);
			b3 &= b3 - 1;
			from = pawndouble[xs][to];
			if (mask[from] & pin_mask)
				continue;
			int score = check_history[P][to];
			score += CHECK_SCORE;
			AddCheck(from, to, P);
		}
	}

	b1 = bit_pieces[s][N] & ~pin_mask & ~bit_disco_pieces;
	while (b1)
	{
		from = NextBit(b1);
		b1 &= b1 - 1;
		b2 = bit_moves[N][from] & bit_moves[N][xking] & ~bit_all;
		while (b2)
		{
			to = NextBit(b2);
			b2 &= b2 - 1;
			int score = check_history[N][to];
			score += CHECK_SCORE;
			if (bit_moves[N][to] & (bit_pieces[xs][R] | bit_pieces[xs][Q]))
			{
				score += 25;
			}
			AddCheck(from, to, score);
		}
	}

	for (int x = 0; x < total[s][B]; x++)
	{
		from = pieces[s][B][x];
		if (mask[from] & (pin_mask | bit_disco_pieces))
		{
			if (bit_moves[R][king] & mask[from])
			{
				continue;
			}
			if (mask[from] & bit_disco_pieces)
			{
				continue;
			}
		}
		if (!(bit_moves[B][from] & mask[xking]))
		{
			b1 = slider_moves[from][ply] & bit_bishop_checks;
			while (b1)
			{
				to = NextBit(b1);
				if (!(mask[from] & pin_mask) || !(LineAttack(xs, king, bit_all & ~mask[from])))
				{
					int score = check_history[B][to];
					score += CHECK_SCORE;
					AddCheck(from, to, score);
				}
				b1 &= b1 - 1;
			}
		}
	}
	for (int x = 0; x < total[s][R]; x++)
	{
		from = pieces[s][R][x];
		if (mask[from] & (pin_mask | bit_disco_pieces))
		{		
			if (bit_moves[B][king] & mask[from])
			{
				continue;
			}
			if (mask[from] & bit_disco_pieces)
			{
				continue;
			}
		}
		if (!(bit_moves[R][from] & mask[xking]))
		{
			b1 = slider_moves[from][ply] & bit_rook_checks;
			while (b1)
			{
				to = NextBit(b1);
				if (!(mask[from] & pin_mask) || !(LineAttack(xs, king, bit_all & ~mask[from])))
				{
					int score = check_history[R][to];
					score += CHECK_SCORE;
					AddCheck(from, to, score);
				}
				b1 &= b1 - 1;
			}
		}
	}
	for (int x = 0; x < total[s][Q]; x++)
	{
		from = pieces[s][Q][x];
		BITBOARD b1 = slider_moves[from][ply] & bit_queen_checks;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(mask[from] & pin_mask) || !(LineAttack(xs, king, bit_all & ~mask[from] | mask[to])))
			{
				int score = check_history[Q][to];
				score += CHECK_SCORE;
				if (difference[xking][to] == 2 && bit_moves[R][to] & mask[xking])
					score += 50;
				else if (bit_moves[K][xking] & mask[to])
				{
					if (colors[xking] == colors[to])
						score += 550;
					else
						score += 650;
				}
			}
		}
	}
	first_move[ply + 1] = move_count;

	return bit_disco_pieces;
}

void GenDisco(const int from, const BITBOARD mask_line, BITBOARD pin_mask)
{
	const int piece = b[from];
	if (piece == N)
	{
		if (!(mask[from] & pin_mask))
		{
			BITBOARD b1 = bit_moves[N][from] & ~bit_all;
			while (b1)
			{
				int to = NextBit(b1);
				b1 &= b1 - 1;
				AddDisco(from, to, N);
			}
		}
		return;
	}
	if (piece == B)
	{
		BITBOARD b1 = bit_moves[B][from] & ~bit_all;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (!(mask[from] & pin_mask) || !(LineAttack(xside, kingloc[side], bit_all & ~mask[from])))
				{
					AddDisco(from, to, R);
				}
			}
			else
				b1 &= ~bit_after[from][to];
		}
		return;
	}
	if (piece == R)
	{
		BITBOARD b1 = bit_moves[R][from] & ~bit_all;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
			{
				if (!(mask[from] & pin_mask) || !(LineAttack(xside, kingloc[side], bit_all & ~mask[from])))
				{
					AddDisco(from, to, R);
				}
			}
			else
				b1 &= ~bit_after[from][to];
		}
		return;
	}
	if (piece == K)
	{
		BITBOARD b1 = bit_moves[K][from] & ~bit_all & ~mask_line;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(Attack(xside, to, bit_all)))
				AddDisco(from, to, K);
		}
		return;
	}
	if (piece == P)
	{
		if (!(mask[from] & pin_mask))
		{
			int to = pawnplus[side][from];
			if (row2[side][to] != 7)
			{
				if (!(mask_line & mask[to]))
				{
					if (b[to] == EMPTY)
					{
						AddDisco(from, to, P);
						if (row2[side][from] == 1)
						{
							int sq2 = pawnplus[side][to];
							if (b[sq2] == EMPTY)
							{
								AddDisco(from, to, P);
							}
						}
					}
				}
			}
		}
		return;
	}
}

void AddCheck(const int from, const int to, const int score)
{
	g = &move_list[move_count++];
	g->flags = CHECK;
	g->from = from;
	g->to = to;
	g->score = score;
}

void AddDisco(const int from, const int to, const int piece)
{
	g = &move_list[move_count++];
	g->flags = CHECK | DISCO;
	g->from = from;
	g->to = to;
	g->score = check_history[piece][to] + 1000;
	bit_disco_squares[from] |= mask[to];
}

void RemoveDiscoChecks(const int startmoves)
{
	int start = startmoves;
	int end = first_move[ply + 1];

	int write = start;

	for (int read = start; read < end; ++read)
	{
		move_data m = move_list[read];
		int from = m.from;
		int to = m.to;

		if (bit_disco_squares[from] & mask[to])
		{
			continue;
		}

		move_list[write++] = m;
	}

	first_move[ply + 1] = write;
}
