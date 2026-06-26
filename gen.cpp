#include "globals.h"

bool IsOneBit(BITBOARD x);

void z();

extern BITBOARD bit_attacked[2][6];
extern BITBOARD bit_kq_defends;

BITBOARD bit_king_defends;
BITBOARD bit_queen_defends;

void GenDisco(const int x, const BITBOARD mask_line);
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

void AddRecapture(const int from, const int to);

void AddCastle(const int from, const int to);

void AddPawnMove(const int s, const int xs, const int from, const int to);
void AddKnightMove(const int s, const int xs, const int from, const int to, const int);
void AddBishopMove(const int s, const int xs, const int from, const int to, const int);
void AddRookMove(const int s, const int xs, const int from, const int to, const int);
void AddQueenMove(const int s, const int xs, const int from, const int to, const int);
void AddKingMove(const int s, const int xs, const int from, const int to);

void AddCastle(const int from, const int to);
void AddCheck(const int from, const int to, const int);
void AddEP(const int from, const int to);
void AddCapture(const int from, const int to, const unsigned int flags, const int score);

void AddPawnCapture(const int from, const int to, const int score);
void AddKnightCapture(const int from, const int to, const int score);
void AddBishopCapture(const int from, const int to, const int score);
void AddRookCapture(const int from, const int to, const int score);
void AddQueenCapture(const int from, const int to, const int score);

void GenQuietMoves(const int s, const int xs, BITBOARD pin_mask, const BITBOARD(&bit_check)[6]);

move_data* g;

void GenCaptures(const int s, const int xs, BITBOARD pin_mask)
{
	first_move[ply + 1] = first_move[ply];
	move_count = first_move[ply];

	BITBOARD b1, b2;
	BITBOARD bit_targets = bit_units[xs];

	int lastmove = -1;
	if (hply > 1)
	{
		lastmove = game_list[hply - 1].to;
		PieceScore[xs][b[lastmove]][lastmove] += 50;
	}

	const int king = kingloc[s];
	const int xking = kingloc[xs];

	GenEP(0);

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
				if (col[from] != col[king])
				{
					continue;
				}
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
		b2 = bit_knightmoves[from] & bit_targets;
		while (b2)
		{
			const int to = NextBit(b2);
			const int piece = b[to];
			unsigned int flags = 0;
			if (bit_knightmoves[to] & bit_pieces[xs][K])
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
			if (bit_rookmoves[king] & mask[from])
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
			b2 = MagicBishopAttacks(from, bit_all) & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_bishopmoves[to] & bit_pieces[xs][K] &&
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
			if (bit_bishopmoves[king] & mask[from])
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
			b2 = MagicRookAttacks(from, bit_all) & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_rookmoves[to] & bit_pieces[xs][K] &&
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
			b2 = MagicQueenAttacks(from, bit_all) & bit_targets;
			while (b2)
			{
				const int to = NextBit(b2);
				const int piece = b[to];
				unsigned int flags = 0;
				if (bit_queenmoves[to] & bit_pieces[xs][K] &&
					!(bit_between[to][xking] & bit_all))
					flags = CHECK;
				AddCapture(from, to, flags, qx[piece] + PieceScore[xs][piece][to]);
				b2 &= b2 - 1;
			}
		}
	}
	b1 = bit_kingmoves[king] & bit_targets;

	while (b1)
	{
		const int to = NextBit(b1);
		const int piece = b[to];
		if (!Attack(xs, to))
			AddCapture(king, to, 0, kx[piece] + PieceScore[xside][piece][to]);
		b1 &= b1 - 1;
	}
	if (lastmove > -1)
	{
		PieceScore[xs][b[lastmove]][lastmove] -= 50;
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
	if (!(LineAttack2(xside, kingloc[side], bit_all & not_mask[ep] & not_mask[from])))
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

void AddRecapture(const int from, const int to)
{
	g = &move_list[first_move[ply + 1]++];
	g->from = from;
	g->to = to;
}

void AddPawnMove(const int s, const int xs, const int from, const int to)//, const int bonus
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
	if (mask[from] & bit_total_attacks[xs])
	{
		if (mask[from] & (bit_undefended[s] | bit_kq_defends))//
		{
			g->score += ESCAPE_SCORE;
		}
	}
	if (bit_pawncaptures[s][to] & bit_units[xs] & ~bit_pieces[xs][P])
	{
		g->score += 500 + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
}

void AddKingMove(const int s, const int xs, const int from, const int to)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][K][from] + hist_to[s][K][to];
	//*
	if (bit_kingmoves[to] & bit_units[xs] & bit_undefended_squares[xs])
	{
		g->score += 40 + ATTACK_SCORE;
		g->flags |= ATTACK;
	}
	//*/
}

void AddKnightMove(const int s, const int xs, const int from, const int to, const int bonus)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][N][from] + hist_to[s][N][to];

	if (mask[to] & bit_weaker[xs][N] ||
		(mask[to] & bit_total_attacks[xs] && !(mask[to] & bit_defend_to[s][N]) && !(bit_knightmoves[to] & bit_pieces[s][N] & ~mask[from])))
	{
		g->score = -3 * EN_PRISE_SCORE;
	}
	else
	{
		if (bit_knightmoves[to] & (bit_attackable[N] | bit_kq_defends))
		{
			g->score += 40 + ATTACK_SCORE;
			g->flags |= ATTACK;
			if (bit_knightmoves[to] & bit_kq_defends)
			{
				//printf("knight ");
				//Alg(from, to);
				//z();
			}
		}
		g->score += bonus;
	}

	int plus = PieceScore[s][N][to] - PieceScore[s][N][from];
	if (plus > 0)
		g->score += plus;
}

void AddBishopMove(const int s, const int xs, const int from, const int to, const int bonus)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][B][from] + hist_to[s][B][to];

	if (mask[to] & bit_weaker[xs][B] ||
		(bit_total_attacks[xs] & mask[to] && !(mask[to] & bit_defend_to[s][B])))
	{
		g->score = -3 * EN_PRISE_SCORE;
	}
	else
	{
		int plus = PieceScore[s][B][to] - PieceScore[s][B][from];
		if (plus > 0)
			g->score += plus;
		BITBOARD b1 = MagicBishopAttacks(to, bit_all) & (bit_attackable[B] | bit_kq_defends) & ~bit_bishopmoves[from];
		if (b1)
		{
			int sq = NextBit(b1);
			g->score += piece_value[b[sq]] + ATTACK_SCORE;
			g->flags |= ATTACK;
		}
		g->score += bonus;
	}
}

void AddRookMove(const int s, const int xs, const int from, const int to, const int bonus)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][R][from] + hist_to[s][R][to];

	if (mask[to] & bit_weaker[xs][R] ||
		(bit_total_attacks[xs] & mask[to] && !(mask[to] & bit_defend_to[s][R]) && !(RookAttack(s, from, to))))
	{
		g->score = -2 * EN_PRISE_SCORE;
	}
	else
	{
		BITBOARD b1 = MagicRookAttacks(to, bit_all) & (bit_attackable[R] | bit_kq_defends);
		if (b1)
		{
			int sq = NextBit(b1);
			g->score += piece_value[b[sq]] + ATTACK_SCORE;
			g->flags |= ATTACK;
			//PrintBitBoard(b1);
			//Alg(from, to);
			//z();
		}
		g->score += bonus;
	}
}

void AddQueenMove(const int s, const int xs, const int from, const int to, const int bonus)
{
	g = &move_list[move_count++];
	g->flags = 0;
	g->from = from;
	g->to = to;
	g->score = hist_from[s][Q][from] + hist_to[s][Q][to];

	if (mask[to] & bit_weaker[xs][Q] ||
		(bit_weaker[xs][K] & mask[to] && !(mask[to] & bit_defend_to[s][Q])))
	{
		g->score = -9 * EN_PRISE_SCORE;
	}
	else
	{
		int plus = PieceScore[s][Q][to] - PieceScore[s][Q][from];
		if (plus > 0)
			g->score += plus;
		BITBOARD b1 = MagicQueenAttacks(to, bit_all) & (bit_attackable[Q] | bit_kq_defends);
		if (b1)
		{
			int sq = NextBit(b1);
			g->score += piece_value[b[sq]] + ATTACK_SCORE;
			g->flags |= ATTACK;
		}
		g->score += bonus;
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
				if (bit_queenmoves[to] & bit_pieces[xs][K] &&
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
		if (bit_queenmoves[to] & bit_pieces[xs][K] &&
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
		if (castle & 1 && !(bit_e1h1 & bit_all) && Attack(1u, F1) == 0)
			AddCastle(E1, G1);
		if (castle & 2 && !(bit_e1a1 & bit_all) && Attack(1u, D1) == 0)
			AddCastle(E1, C1);
	}
	else {
		if (castle & 4 && !(bit_e8h8 & bit_all) && Attack(0, F8) == 0)
			AddCastle(E8, G8);
		if (castle & 8 && !(bit_e8a8 & bit_all) && Attack(0, D8) == 0)
			AddCastle(E8, C8);
	}

	for (int x = 0; x < total[s][N]; x++)
	{
		int bonus = 0;
		int from = pieces[s][N][x];
		if (mask[from] & pin_mask)
		{
			continue;
		}
		if (mask[from] & bit_total_attacks[xs])
		{
			if (mask[from] & bit_undefended[s])
			{
				bonus += ESCAPE_SCORE + B_VALUE;
			}
			else if (mask[from] & bit_weaker[xs][N])
			{
				bonus += ESCAPE_SCORE + 200;
			}
		}

		BITBOARD b1 = bit_knightmoves[from] & ~bit_all & ~bit_check[N];
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (mask[to] & bit_weaker[xs][N])
			{
				bonus = -bonus;
			}
			AddKnightMove(s, xs, from, to, bonus);
		}
	}

	if (pin_mask != 0)
	{
		for (int x = 0; x < total[s][Q]; x++)
		{
			int from = pieces[s][Q][x];
			if (mask[from] & pin_mask)
			{
				BITBOARD bit_pin = bit_after[king][from] & pin_mask;
				assert(bit_pin != 0);
				int pinner = NextBit(bit_pin);
				BITBOARD b1 = bit_between[king][pinner] & not_mask[from] & ~bit_check[Q];
				while (b1)
				{
					int to = NextBit(b1);
					b1 &= b1 - 1;
					AddQueenMove(s, xs, from, to, 0);
				}
			}
			else
				QueenMoves(s, xs, pieces[s][Q][x], x, bit_check[Q]);
		}
		for (int x = 0; x < total[s][R]; x++)
		{
			int from = pieces[s][R][x];
			if (mask[from] & pin_mask)
			{
				if (bit_bishopmoves[king] & mask[from])
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
						AddRookMove(s, xs, from, to, 0);
					}
				}
			}
			else
			{
				RookMoves(s, xs, pieces[s][R][x], x, bit_check[R]);
			}
		}
		for (int x = 0; x < total[s][B]; x++)
		{
			int from = pieces[s][B][x];
			if (mask[from] & pin_mask)
			{
				if (bit_rookmoves[king] & mask[from])
				{
					continue;
				}
				else
				{
					int pinner = NextBit(bit_after[king][from] & pin_mask);
					BITBOARD b1 = bit_between[king][pinner] & not_mask[from] & ~bit_check[B];
					while (b1)
					{
						int to = NextBit(b1);
						b1 &= b1 - 1;
						AddBishopMove(s, xs, from, to, 0);
					}
				}
			}
			else
				BishopMoves(s, xs, pieces[s][B][x], bit_check[B]);
		}
	}
	else
	{
		for (int x = 0; x < total[s][Q]; x++)
		{
			QueenMoves(s, xs, pieces[s][Q][x], x, bit_check[Q]);
		}
		for (int x = 0; x < total[s][R]; x++)
		{
			RookMoves(s, xs, pieces[s][R][x], x, bit_check[R]);
		}
		for (int x = 0; x < total[s][B]; x++)
		{
			BishopMoves(s, xs, pieces[s][B][x], bit_check[B]);
		}
	}

	BITBOARD b1 = bit_kingmoves[king] & ~bit_all & ~bit_weaker[xs][K];
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddKingMove(s, xs, king, to);
	}
	first_move[ply + 1] = move_count;
}

void RookMoves(const int s, const int xs, const int from, const int n, const BITBOARD bit_check_squares)
{
	int bonus = 0;
	if (mask[from] & bit_total_attacks[xs])
	{
		if (mask[from] & bit_undefended[s])
		{
			bonus = ESCAPE_SCORE + 500;
		}
		else if (mask[from] & bit_weaker[xs][R])
		{
			bonus = ESCAPE_SCORE + B_VALUE;
		}
	}
	BITBOARD b1 = bit_rookattacks[s][n] & ~bit_all & ~bit_check_squares;
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddRookMove(s, xs, from, to, bonus);
	}
}

void BishopMoves(const int s, const int xs, const int from, const BITBOARD bit_check_squares)
{
	int bonus = 0;

	if (mask[from] & bit_total_attacks[xs])
	{
		if (mask[from] & bit_undefended[s])
		{
			bonus = ESCAPE_SCORE + B_VALUE;
		}
		else if (mask[from] & bit_weaker[xs][B])
		{
			bonus = ESCAPE_SCORE + 200;
		}
	}

	BITBOARD b1 = bit_attacked[s][B] & bit_bishopmoves[from] & ~bit_all & ~bit_check_squares;
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddBishopMove(s, xs, from, to, bonus);
	}
}

void QueenMoves(const int s, const int xs, const int x, const int n, const BITBOARD bit_check_squares)
{
	int bonus = 0;
	if (mask[x] & bit_undefended[s] || mask[x] & bit_weaker[xs][Q])
	{
		bonus = ESCAPE_SCORE + 900;
	}
	BITBOARD b1 = bit_queenattacks[s][n] & ~bit_all & ~bit_check_squares;
	while (b1)
	{
		int to = NextBit(b1);
		b1 &= b1 - 1;
		AddQueenMove(s, xs, x, to, bonus);
	}
}

BITBOARD DiscoPossible(const int s, const int xs)
{
	const int king = kingloc[xs];
	BITBOARD b1 = bit_rookmoves[king] & (bit_pieces[s][R] | bit_pieces[s][Q]);
	b1 |= bit_bishopmoves[king] & (bit_pieces[s][B] | bit_pieces[s][Q]);
	return b1;
}

BITBOARD GenChecks(const int s, const int xs, BITBOARD pin_mask)
{
	move_count = first_move[ply + 1];

	int to, from;
	const int king = kingloc[xs];

	memset(bit_disco_squares, 0, sizeof(bit_disco_squares));

	BITBOARD b1, b2, b3;

	BITBOARD bit_disco_pieces = 0;

	BITBOARD bit_disco = DiscoPossible(s, xs);

	if (bit_disco)
	{
		for (int x = 0; x < total[s][B]; x++)
		{
			from = pieces[s][B][x];
			if (bit_bishopmoves[from] & mask[king])
				if (!(bit_between[from][king] & bit_units[xs]) && IsOneBit(bit_between[from][king] & bit_units[s]))
				{
					int sq2 = NextBit(bit_between[from][king] & bit_units[s]);
					GenDisco(sq2, bit_between[from][king]);
					bit_disco_pieces |= mask[sq2];
				}
		}
		for (int x = 0; x < total[s][R]; x++)
		{
			from = pieces[s][R][x];
			if (bit_rookmoves[from] & mask[king])
				if (!(bit_between[from][king] & bit_units[xs]) && IsOneBit(bit_between[from][king] & bit_units[s]))
				{
					int sq2 = NextBit(bit_between[from][king] & bit_units[s]);
					GenDisco(sq2, bit_between[from][king]);
					bit_disco_pieces |= mask[sq2];
				}
		}
		for (int x = 0; x < total[s][Q]; x++)
		{
			from = pieces[s][Q][x];
			if (bit_queenmoves[from] & mask[king])
			{
				if (!(bit_between[from][king] & bit_units[xs]) && IsOneBit(bit_between[from][king] & bit_units[s]))
				{
					if (!(col[from] == col[king] && bit_between[from][king] & bit_pieces[s][0]))
					{
						int sq2 = NextBit(bit_between[from][king] & bit_units[s]);
						GenDisco(sq2, bit_between[from][king]);
						bit_disco_pieces |= mask[sq2];
					}
				}
			}
		}
	}

	const int k = kingloc[s];

	if (s == 0)
		b1 = (bit_pieces[0][0] << 8) & ~bit_all;
	else
		b1 = (bit_pieces[1][0] >> 8) & ~bit_all;

	b2 = b1 & bit_pawncaptures[xs][king];
	while (b2)
	{
		int sq2 = NextBit(b2);
		b2 &= b2 - 1;
		to = pawnplus[xs][sq2];
		if (mask[to] & bit_disco_pieces)
			continue;
		AddCheck(to, sq2, P);
	}
	if (row2[xs][king] == 3)
	{
		if (s == 0)
		{
			b1 = ((bit_pieces[0][0] & mask_ranks[0][1]) << 8) & ~bit_all;
			b2 = (b1 << 8) & ~bit_all;
			b3 = b2 & bit_pawncaptures[xs][king];
		}
		else
		{
			b1 = ((bit_pieces[1][0] & mask_ranks[1][1]) >> 8) & ~bit_all;
			b2 = (b1 >> 8) & ~bit_all;
			b3 = b2 & bit_pawncaptures[xs][king];
		}
		while (b3)
		{
			int sq2 = NextBit(b3);
			b3 &= b3 - 1;
			to = pawndouble[xs][sq2];
			if (mask[to] & bit_disco_pieces)
				continue;
			int score = check_history[P][to];
			score += CHECK_SCORE;
			AddCheck(to, sq2, P);
		}
	}

	for (int x = 0; x < total[s][N]; x++)
	{
		from = pieces[s][N][x];
		if (mask[from] & (pin_mask | bit_disco_pieces))
		{
			continue;
		}
		b2 = bit_knightmoves[from] & bit_knightmoves[king] & ~bit_all;
		while (b2)
		{
			to = NextBit(b2);
			b2 &= b2 - 1;
			int score = check_history[N][to];
			score += CHECK_SCORE;
			if (bit_knightmoves[to] & (bit_pieces[xs][R] | bit_pieces[xs][Q]))
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
			if (bit_rookmoves[k] & mask[from])
			{
				continue;
			}
		if (!(bit_bishopmoves[from] & mask[king]))
		{
			b1 = bit_bishopmoves[from] & bit_bishopmoves[king];
			while (b1)
			{
				to = NextBit(b1);
				if (!(mask[to] & bit_all))
				{
					if (!((bit_between[from][to] | bit_between[to][king]) & bit_all))
					{
						int score = check_history[B][to];
						score += CHECK_SCORE;
						AddCheck(from, to, score);
					}
				}
				b1 &= b1 - 1;
			}
		}
	}

	for (int x = 0; x < total[s][R]; x++)
	{
		from = pieces[s][R][x];
		if (mask[from] & (pin_mask | bit_disco_pieces))
			if (bit_bishopmoves[k] & mask[from])
			{
				continue;
			}
		if (!(bit_rookmoves[from] & mask[king]))
		{
			to = h_check[from][king];
			if (to > -1)
			{
				if (!((bit_between[from][to] | mask[to] | bit_between[to][king]) & bit_all))
				{
					int score = check_history[R][to];
					score += CHECK_SCORE;
					AddCheck(from, to, score);
				}
			}
			to = v_check[from][king];
			if (to > -1)
			{
				if (!((bit_between[from][to] | mask[to] | bit_between[to][king]) & bit_all))
				{
					int score = check_history[R][to];
					score += CHECK_SCORE;
					AddCheck(from, to, score);
				}
			}
		}
	}

	for (int x = 0; x < total[s][Q]; x++)
	{
		from = pieces[s][Q][x];
		int z1 = 1;
		to = q_check[from][king][0];
		while (to > -1)
		{
			if (!((bit_between[from][to] | mask[to] | bit_between[to][king]) & bit_all))
			{
				int score = check_history[Q][to];
				score += CHECK_SCORE;
				if (difference[king][to] == 2 && bit_rookmoves[to] & mask[king])
					score += 50;
				else if (bit_kingmoves[king] & mask[to])
				{
					if (colors[king] == colors[to])
						score += 550;
					else
						score += 650;
				}
				AddCheck(from, to, score);
			}
			to = q_check[from][king][z1++];
		}
	}
	first_move[ply + 1] = move_count;

	return bit_disco_pieces;
}

void GenDisco(const int from, const BITBOARD mask_line)
{
	const int piece = b[from];
	if (piece == N)
	{
		BITBOARD b1 = bit_knightmoves[from] & ~bit_all;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			AddDisco(from, to, N);
		}
		return;
	}
	if (piece == B)
	{
		BITBOARD b1 = bit_bishopmoves[from] & ~bit_all;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
				AddDisco(from, to, B);
			//else
			//	b1 &= ~bit_after[from][to];
		}
		return;
	}
	if (piece == R)
	{
		BITBOARD b1 = bit_rookmoves[from] & ~bit_all;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(bit_between[from][to] & bit_all))
				AddDisco(from, to, R);
			//else
			//	b1 &= ~bit_after[from][to];
		}
		return;
	}
	if (piece == K)
	{
		BITBOARD b1 = bit_kingmoves[from] & ~bit_all & ~mask_line;
		while (b1)
		{
			int to = NextBit(b1);
			b1 &= b1 - 1;
			if (!(Attack(xside, to)))
				AddDisco(from, to, K);
		}
		return;
	}
	if (piece == P)
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
	assert(b[from] < 6);
	assert(b[to] == 6);
	assert(piece < 6);

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
