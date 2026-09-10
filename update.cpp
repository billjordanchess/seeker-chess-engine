//11/9/26
#pragma once
#include "globals.h"

static int index[64];

void UpdatePawn(const int s, const int from, const int to);
void RemovePawn(const int s, const int sq);
void AddPawn(const int s, const int sq);

void UnMakeCapture();
void AfterCastle(const int);
void BeforeCastle(const int);
void AddPawnKey(const int s, const int x);
void MakeEvasion(const int from, const int to);
void UnMakeEvasion();
void UnMakeNull();

void MakeCheck(const int from, const int to, const int flags);
void UnMakeCheck();

void UpdatePawn(const int s, const int from, const int to)
{
	bit_units[s] &= ~mask[from];
	bit_units[s] |= mask[to];
	bit_all = bit_units[0] | bit_units[1];
	AddKeys(s, P, from, to);
	b[to] = P;
	b[from] = EMPTY;
	bit_pieces[s][P] &= ~mask[from];
	bit_pieces[s][P] |= mask[to];
	AddPawnKeys(s, from, to);
}

void RemovePawn(const int s, const int sq)
{
	AddKey(s, P, sq);
	AddPawnKey(s, sq);
	b[sq] = EMPTY;
	const BITBOARD m = ~mask[sq];
	bit_units[s] &= m;
	bit_all &= m;
	bit_pieces[s][P] &= m;
	pawn_mat[s] -= P_VALUE;
}

void AddPawn(const int s, const int sq)
{
	AddKey(s, P, sq);
	AddPawnKey(s, sq);
	b[sq] = P;
	const BITBOARD m = mask[sq];
	bit_units[s] |= m;
	bit_all |= m;
	bit_pieces[s][P] |= m;
	pawn_mat[s] += P_VALUE;
}

void UpdatePiece(const int s, const int piece, const int from, const int to)
{
	bit_units[s] &= ~mask[from];
	bit_units[s] |= mask[to];
	bit_all = bit_units[0] | bit_units[1];
	AddKeys(s, piece, from, to);

	b[to] = piece;
	b[from] = EMPTY;
	bit_pieces[s][piece] &= ~mask[from];
	bit_pieces[s][piece] |= mask[to];

	table_score[s] -= PieceScore[s][piece][from];
	table_score[s] += PieceScore[s][piece][to];

	index[to] = index[from];
	pieces[s][piece][index[to]] = to;
	kingloc[s] = (piece == K) ? to : kingloc[s];
}

void RemovePiece(const int s, const int piece, const int sq)
{
	const BITBOARD m = ~mask[sq];
	bit_units[s] &= m;
	bit_all &= m;

	assert(piece < 6);
	bit_pieces[s][piece] &= m;
	AddKey(s, piece, sq);
	b[sq] = EMPTY;

	table_score[s] -= PieceScore[s][piece][sq];
	piece_mat[s] -= piece_value[piece];

	assert(total[s][piece] > 0);

	const int idx = index[sq];
	const int last = total[s][piece] - 1;

	if (pieces[s][piece][idx] != sq)
	{
		//Algebraic(sq);
		//z();
	}

	assert(idx >= 0);
	assert(idx <= last);
	assert(pieces[s][piece][idx] == sq);

	if (idx != last)
	{
		const int last_sq = pieces[s][piece][last];

		pieces[s][piece][idx] = last_sq;
		index[last_sq] = idx;
	}

	total[s][piece]--;
	index[sq] = -1;
}

void AddPiece(const int s, const int piece, const int sq)
{
	b[sq] = piece;
	AddKey(s, piece, sq);
	const BITBOARD m = mask[sq];
	bit_units[s] |= m;
	bit_all |= m;
	bit_pieces[s][piece] |= m;

	table_score[s] += PieceScore[s][piece][sq];
	index[sq] = total[s][piece];
	pieces[s][piece][total[s][piece]] = sq;
	total[s][piece]++;
	piece_mat[s] += piece_value[piece];
}

void MakeMove(const int from, const int to, const int flags)
{
	game* m = &game_list[hply];
	m->piece = b[from];
	m->flags = flags;
	m->from = from;
	m->to = to;
	m->capture = b[to];
	m->castle = castle;
	m->fifty = fifty;
	m->hash = currentkey;

	if (b[from] == K)
	{
		if (flags & CASTLE)
		{
			UpdatePiece(side, R, castle_start[to], castle_dest[to]);
			KingScore[side][squares[side][E1]] = -40;
			if (col[to] == 6)
			{
				AfterCastle(side);
			}
		}
	}

	castle &= castle_mask[from] & castle_mask[to];

	fifty++;

	if (b[from] == P)
	{
		fifty = 0;

		if (flags & EP)
		{
			RemovePawn(xside, pawnplus[xside][to]);
		}
		if (m->capture != EMPTY)
		{
			if (m->capture > P)
				RemovePiece(xside, b[to], to);
			else
				RemovePawn(xside, to);
		}
		if (row2[side][to] == 7)
		{
			RemovePawn(side, from);
			AddPiece(side, Q, to);
			m->flags |= PROMOTE;
		}
		else
		{
			UpdatePawn(side, from, to);
		}
	}
	else
	{
		if (m->capture != EMPTY)
		{
			fifty = 0;
			if (m->capture > P)
				RemovePiece(xside, b[to], to);
			else
				RemovePawn(xside, to);
		}
		UpdatePiece(side, b[from], from, to);
	}

	ply++;
	hply++;
	side ^= 1;
	xside ^= 1;
}

void UnMakeMove()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	castle = m->castle;
	fifty = m->fifty;

	const int from = m->from;
	const int to = m->to;

	if (m->piece == P)
	{
		if (m->flags & PROMOTE)
		{
			RemovePiece(side, b[to], to);
			AddPawn(side, from);

			if (m->capture != EMPTY)
				AddPiece(xside, m->capture, to);

			return;
		}

		UpdatePawn(side, to, from);

		if (m->flags & EP)
		{
			AddPawn(xside, pawnplus[xside][to]);
		}
		else if (m->capture != EMPTY)
		{
			if (m->capture > P)
				AddPiece(xside, m->capture, to);
			else
				AddPawn(xside, to);
		}
	}
	else
	{
		UpdatePiece(side, b[to], to, from);
		if (m->capture != EMPTY)
		{
			if (m->capture > P)
				AddPiece(xside, m->capture, to);
			else
				AddPawn(xside, to);
		}
	}
	if (m->flags & CASTLE)
	{
		const int to2 = castle_start[to];
		const int from2 = castle_dest[to];
		UpdatePiece(side, R, from2, to2);
		KingScore[side][squares[side][E1]] = 10;
		if (col[to] == 6)
		{
			BeforeCastle(side);
		}
	}
}

void MakeCapture(const int from, const int to, const int flags)
{
	const int mover = b[from];

	game* m = &game_list[hply];
	m->flags = flags;
	m->from = from;
	m->to = to;
	m->piece = mover;
	m->capture = b[to];
	m->fifty = 0;
	m->hash = currentkey;
	m->castle = castle;

	fifty = 0;
	castle &= castle_mask[from] & castle_mask[to];

	if (m->capture != EMPTY)
	{
		if (m->capture > P)
			RemovePiece(xside, m->capture, to);
		else
			RemovePawn(xside, to);
	}
	if (mover == P)
	{
		if (flags & EP)
		{
			RemovePawn(xside, pawnplus[xside][to]);
		}
		if (row2[side][to] == 7)
		{
			RemovePawn(side, from);
			AddPiece(side, Q, to);
			m->flags |= PROMOTE;
		}
		else
			UpdatePawn(side, from, to);
	}
	else
	{
		UpdatePiece(side, mover, from, to);
	}
	++ply;
	++hply;
	side ^= 1;
	xside ^= 1;
}
//10 148 23 131979 c8c3 g3h4 d8d2 e2d2 b7e4 d2g2 e4g2 h1g2 c3c2 g2f3
//10 148 24 131979 c8c3 g3h4 d8d2 e2d2 b7e4 d2g2 e4g2 h1g2 c3c2 g2f3
//10 148 24 131867 c8c3 g3h4 d8d2 e2d2 b7e4 d2g2 e4g2 h1g2 c3c2 g2f3

void UnMakeCapture()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* h = &game_list[hply];
	fifty = h->fifty;
	castle = h->castle;

	const int to = h->to;
	const int from = h->from;
	const int captured = h->capture;
	const int mover = h->piece;

	if (h->flags & PROMOTE)
	{
		RemovePiece(side, b[to], to);
		AddPawn(side, from);
		if (captured != EMPTY)
		{
			AddPiece(xside, captured, to);
		}
		return;
	}
	if (mover > P)
		UpdatePiece(side, mover, to, from);
	else
		UpdatePawn(side, to, from);
	if (captured != EMPTY)
	{
		if (captured > P)
			AddPiece(xside, captured, to);
		else
			AddPawn(xside, to);
	}
	else if (h->flags & EP)
	{
		AddPawn(xside, pawnplus[xside][to]);
	}
}

void MakeEvasion(const int from, const int to)
{
	game* m = &game_list[hply];
	m->flags = 0;
	m->from = from;
	m->to = to;
	m->capture = EMPTY;
	m->castle = castle;
	m->fifty = fifty;
	m->hash = currentkey;

	castle &= castle_mask[from] & castle_mask[to];

	fifty++;

	++ply;
	++hply;

	if (b[from] > P)
		UpdatePiece(side, b[from], from, to);
	else
	{
		if (row2[side][to] != 7)
		{
			UpdatePawn(side, from, to);
		}
		else
		{
			RemovePawn(side, from);
			AddPiece(side, Q, to);
			m->flags |= PROMOTE;
		}			
		fifty = 0;
	}
	side ^= 1;
	xside ^= 1;
}

void UnMakeEvasion()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	castle = m->castle;
	fifty = m->fifty;

	const int from = m->from;
	const int to = m->to;
	
	if (m->flags & PROMOTE)
	{
		RemovePiece(side, b[to], to);
		AddPawn(side, from);
		return;
	}
	if (b[to] > P)
		UpdatePiece(side, b[to], to, from);
	else
		UpdatePawn(side, to, from);
}

void MakeQuietMove(const int from, const int to, const int flags)
{
	if (b[from] == K)
	{
		if (flags & CASTLE)
		{
			UpdatePiece(side, R, castle_start[to], castle_dest[to]);
			KingScore[side][squares[side][E1]] = -40;
			if (col[to] == 6)
			{
				AfterCastle(side);
			}
		}
	}
	game* m = &game_list[hply];
	m->piece = b[from];
	m->flags = flags;
	m->from = from;
	m->to = to;
	m->capture = b[to];
	m->castle = castle;
	m->fifty = fifty;
	m->hash = currentkey;

	castle &= castle_mask[from] & castle_mask[to];

	fifty++;

	if (b[from] == P)
	{
		fifty = 0;
		UpdatePawn(side, from, to);
	}
	else
	{
		UpdatePiece(side, b[from], from, to);
	}
	ply++;
	hply++;
	side ^= 1;
	xside ^= 1;
}

void UnMakeQuietMove()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	castle = m->castle;
	fifty = m->fifty;

	const int from = m->from;
	const int to = m->to;

	if (b[to] > P)
		UpdatePiece(side, b[to], to, from);
	else
		UpdatePawn(side, to, from);

	if (m->flags & CASTLE)
	{
		const int to2 = castle_start[to];
		const int from2 = castle_dest[to];
		UpdatePiece(side, R, from2, to2);
		KingScore[side][squares[side][E1]] = 10;
		if (col[to] == 7)
		{
			BeforeCastle(side);
		}
	}
}

void MakeCheck(const int from, const int to, const int flags)
{
	game* m = &game_list[hply];
	m->piece = b[from];
	m->flags = flags;
	m->from = from;
	m->to = to;
	m->capture = EMPTY;
	m->castle = castle;
	m->fifty = fifty;
	m->hash = currentkey;

	castle &= castle_mask[from] & castle_mask[to];

	fifty++;

	if (b[from] == P)
	{
		fifty = 0;
		UpdatePawn(side, from, to);
	}
	else
	{
		UpdatePiece(side, b[from], from, to);
	}
	ply++;
	hply++;
	side ^= 1;
	xside ^= 1;
}

void UnMakeCheck()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	castle = m->castle;
	fifty = m->fifty;

	const int from = m->from;
	const int to = m->to;

	if (b[to] > P)
		UpdatePiece(side, b[to], to, from);
	else
		UpdatePawn(side, to, from);
}

void UnMakeNull()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	castle = m->castle;
	fifty = m->fifty;
}

void AfterCastle(const int s)
{
	KingScore[s][squares[s][F1]] = -20;
	KingScore[s][squares[s][F2]] = -25;
	KingScore[s][squares[s][G1]] = 20;
	PieceScore[s][P][squares[s][E2]] = 0;
	PieceScore[s][B][squares[s][G2]] = 8;
	PieceScore[s][B][squares[s][F1]] = -10;
	PieceScore[s][N][squares[s][G1]] = -16;
	PieceScore[s][P][squares[s][F2]] = 4;
	PieceScore[s][P][squares[s][F3]] = 4;
}

void BeforeCastle(const int s)
{
	KingScore[s][squares[s][F1]] = -60;
	KingScore[s][squares[s][F2]] = -45;
	KingScore[s][squares[s][G1]] = -20;
	PieceScore[s][P][squares[s][E2]] = -8;
	PieceScore[s][B][squares[s][G2]] = 4;
	PieceScore[s][B][squares[s][F1]] = -12;
	PieceScore[s][N][squares[s][G1]] = -20;
	PieceScore[s][P][squares[s][F2]] = 5;
	PieceScore[s][P][squares[s][F3]] = 3;
}
