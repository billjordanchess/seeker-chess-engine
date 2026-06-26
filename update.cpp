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
bool MakeEvasion(const int from, const int to);
void UnMakeEvasion();
void UnMakeNull();

void UpdatePawn(const int s, const int from, const int to)
{
	bit_units[s] &= not_mask[from];
	bit_units[s] |= mask[to];
	bit_all = bit_units[0] | bit_units[1];
	AddKeys(s, P, from, to);
	b[to] = P;
	b[from] = EMPTY;
	bit_pieces[s][P] &= not_mask[from];
	bit_pieces[s][P] |= mask[to];
	AddPawnKeys(s, from, to);
}

void RemovePawn(const int s, const int sq)
{
	AddKey(s, P, sq);
	AddPawnKey(s, sq);
	b[sq] = EMPTY;
	const BITBOARD m = not_mask[sq];	
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
	bit_units[s] &= not_mask[from];
	bit_units[s] |= mask[to];
	bit_all = bit_units[0] | bit_units[1];
	AddKeys(s, piece, from, to);

	b[to] = piece;
	b[from] = EMPTY;
	bit_pieces[s][piece] &= not_mask[from];
	bit_pieces[s][piece] |= mask[to];

	if (piece == P)
	{
		AddPawnKeys(s, from, to);
		return;
	}
	table_score[s] -= PieceScore[s][piece][from];
	table_score[s] += PieceScore[s][piece][to];

	index[to] = index[from];
	pieces[s][piece][index[to]] = to;
	kingloc[s] = (piece == K) ? to : kingloc[s];
}

void RemovePiece(const int s, const int piece, const int sq)
{
	const BITBOARD m = not_mask[sq];
	bit_units[s] &= m;
	bit_all &= m;

	assert(piece < 6);
	bit_pieces[s][piece] &= m;
	AddKey(s, piece, sq);
	b[sq] = EMPTY;

	if (piece == P) {
		pawn_mat[s] -= P_VALUE;
		AddPawnKey(s, sq);
		return;
	}

	table_score[s] -= PieceScore[s][piece][sq];
	piece_mat[s] -= piece_value[piece];

	assert(total[s][piece] > 0);

	const int idx = index[sq];
	if (pieces[s][piece][idx] != sq)
	{
		Alg(sq, sq);
		z();
	}
	const int last = total[s][piece] - 1;

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

	if (piece == P)
	{
		pawn_mat[s] += P_VALUE;
		AddPawnKey(s, sq);
		return;
	}
	table_score[s] += PieceScore[s][piece][sq];
	index[sq] = total[s][piece];
	pieces[s][piece][total[s][piece]] = sq;
	total[s][piece]++;
	piece_mat[s] += piece_value[piece];
}

bool MakeMove(const int from, const int to, const int flags)
{
	if (b[from] == K)
	{
		if (Attack(xside, to))
		{
			return false;
		}
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

		if (flags & EP)
		{
			RemovePawn(xside, pawnplus[xside][to]);
		}
		if (b[to] != EMPTY)
		{
			RemovePiece(xside, b[to], to);
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
		if (b[to] != EMPTY)
		{
			fifty = 0;
			RemovePiece(xside, b[to], to);
		}
		UpdatePiece(side, b[from], from, to);
	}
	if (fifty == 0 || flags & (CHECK | INCHECK))
	{
		m->streak = 0;
	}

	ply++;
	hply++;
	side ^= 1;
	xside ^= 1;

	if (Attack(side, kingloc[xside]))
	{
		UnMakeMove();
		return false;
	}
	return true;
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

	if (m->flags & PROMOTE)
	{
		RemovePiece(side, b[to], to);
		AddPawn(side, from);
	}
	else
	{
		UpdatePiece(side, b[to], to, from);
	}
	if (m->capture != EMPTY)
	{
		AddPiece(xside, m->capture, to);
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
	if (m->flags & EP)
	{
		AddPawn(xside, pawnplus[xside][m->to]);
	}
}

bool MakeCapture(const int from, const int to, const int flags)
{
	const int mover = b[from];

	game* m = &game_list[hply];
	m->flags = flags;
	m->from = from;
	m->to = to;
	m->piece = mover;
	m->capture = b[to];
	m->fifty = 0;
	m->streak = 0;
	m->hash = currentkey;
	m->castle = castle;

	castle &= castle_mask[from] & castle_mask[to];

	if (b[to] != EMPTY)
	{
		RemovePiece(xside, b[to], to);
	}
	else if (mover == P && col[from] != col[to])
	{
		m->flags |= EP;
		RemovePawn(xside, pawnplus[xside][to]);
	}

	if (mover == P)
	{
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
	if (Attack(side, kingloc[xside]))
	{
		UnMakeCapture();
		return false;
	}
	return true;
}

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
		AddPawn(side, from);
		RemovePiece(side, b[to], to);
		if (captured != EMPTY)
		{
			AddPiece(xside, captured, to);
		}
		return;
	}
	UpdatePiece(side, mover, to, from);
	if (captured != EMPTY)
	{
		AddPiece(xside, captured, to);
	}
	else if (h->flags & EP)
	{
		AddPawn(xside, pawnplus[xside][to]);
	}
}

void MakeRecapture(const int from, const int to)
{
	game* m = &game_list[hply];
	m->from = from;
	m->to = to;
	m->capture = b[to];

	++ply;
	++hply;

	RemovePiece(xside, b[to], to);
	UpdatePiece(side, b[from], from, to);
	side ^= 1;
	xside ^= 1;
}

void UnMakeRecapture()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	const int from = m->from;
	const int to = m->to;
	const int capture = m->capture;
	UpdatePiece(side, b[to], to, from);
	AddPiece(xside, capture, to);
}

bool MakeEvasion(const int from, const int to)
{
	game* m = &game_list[hply];
	m->from = from;
	m->to = to;
	m->capture = EMPTY;

	++ply;
	++hply;

	UpdatePiece(side, b[from], from, to);
	side ^= 1;
	xside ^= 1;
	if (Attack(side, kingloc[xside]))
	{
		UnMakeEvasion();
		return false;
	}
	return true;
}

void UnMakeEvasion()
{
	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	game* m = &game_list[hply];
	const int from = m->from;
	const int to = m->to;
	UpdatePiece(side, b[to], to, from);
}

bool MakeQuietMove(const int from, const int to, const int flags)
{
	if (b[from] == K)
	{
		if (Attack(xside, to))
		{
			return false;
		}
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
		m->streak = 0;
		fifty = 0;
		UpdatePawn(side, from, to);
	}
	else
	{
		UpdatePiece(side, b[from], from, to);
	}
	if (fifty == 0)
	{
		m->streak = 0;
	}
	else
	{
		game_list[hply + 1].streak = m->streak + 1;
	}
	ply++;
	hply++;
	side ^= 1;
	xside ^= 1;

#if DEBUG
	if (LineAttack(side, kingloc[xside]))
	{
		printf(" line ");
		Alg(from, to);
		z();
		LineAttack(side, kingloc[xside]);
		UnMakeQuietMove();
		return false;
	}
	if (Attack(side, kingloc[xside]))
	{
		Alg(from, to);
		z();
		UnMakeQuietMove();
		return false;
	}
#endif
	return true;
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

	UpdatePiece(side, b[to], to, from);

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
