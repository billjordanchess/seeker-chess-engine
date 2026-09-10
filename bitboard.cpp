//11/9/26
using namespace std;//

#include "globals.h"

#if !defined(_M_X64) && !defined(_M_AMD64)
#error "This chess engine must be built as x64"
#endif

#include <intrin.h>
#include <cstdint>

bool IsOneBit(BITBOARD x);
int CountBits(BITBOARD b1);

int NextHighBit(BITBOARD bb);

int kingqueen[64][64];
int kingknight[64][64];
int kingking[64][64];
constexpr int taxi[] = { 0,16,12,8,4,0,0,0 };
constexpr int kingtaxi[] = { 0,24,20,16,12,8,4,0 };

const int col[64] =
{
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7,
0,1,2,3,4,5,6,7
};

const int row[64] =
{
0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,
2,2,2,2,2,2,2,2,
3,3,3,3,3,3,3,3,
4,4,4,4,4,4,4,4,
5,5,5,5,5,5,5,5,
6,6,6,6,6,6,6,6,
7,7,7,7,7,7,7,7
};

const int colors[64] =
{
	 1,0,1,0,1,0,1,0,
	 0,1,0,1,0,1,0,1,
	 1,0,1,0,1,0,1,0,
	 0,1,0,1,0,1,0,1,
	 1,0,1,0,1,0,1,0,
	 0,1,0,1,0,1,0,1,
	 1,0,1,0,1,0,1,0,
	 0,1,0,1,0,1,0,1
};

const int nwdiag[64] =
{
	 14,13,12,11,10, 9, 8, 7,
	 13,12,11,10, 9, 8, 7, 6,
	 12,11,10, 9, 8, 7, 6, 5,
	 11,10, 9, 8, 7, 6, 5, 4,
	 10, 9, 8, 7, 6, 5, 4, 3,
	  9, 8, 7, 6, 5, 4, 3, 2,
	  8, 7, 6, 5, 4, 3, 2, 1,
	  7, 6, 5, 4, 3, 2, 1, 0
};

const int nediag[64] =
{
	 7, 8,9,10,11,12,13,14,
	 6, 7,8, 9,10,11,12,13,
	 5, 6,7, 8, 9,10,11,12,
	 4, 5,6, 7, 8, 9,10,11,
	 3, 4,5, 6, 7, 8, 9,10,
	 2, 3,4, 5, 6, 7, 8, 9,
	 1, 2,3, 4, 5, 6, 7, 8,
	 0, 1,2, 3, 4, 5, 6, 7
};

int adjfile[64][64];

BITBOARD bishop_a7[2];
BITBOARD bishop_h7[2];
BITBOARD knight_a7[2];
BITBOARD knight_h7[2];

BITBOARD mask_left_col[64];
BITBOARD mask_right_col[64];

BITBOARD bit_adjacent[64];

BITBOARD mask_abc;
BITBOARD mask_ghi;
BITBOARD mask_abc2;
BITBOARD mask_abc3;
BITBOARD mask_abc4;
BITBOARD mask_fgh2;
BITBOARD mask_fgh3;
BITBOARD mask_fgh4;

BITBOARD bit_between[64][64];
BITBOARD bit_after[64][64];

//legal moves from each square
BITBOARD bit_pawncaptures[2][64];
BITBOARD bit_pawndefends[2][64];
BITBOARD bit_left[2][64];
BITBOARD bit_right[2][64];

BITBOARD bit_moves[6][64];

//current position
BITBOARD bit_pieces[2][7];
BITBOARD bit_units[2];//pieces+pawns
BITBOARD bit_all;

//current attacks
BITBOARD bit_leftcaptures[2];
BITBOARD bit_rightcaptures[2];
BITBOARD bit_pawnattacks[2];

BITBOARD bit_colors;
BITBOARD bit_color[2];

BITBOARD mask_isolated[64];
BITBOARD mask_backward[2][64];

BITBOARD passed_list[2];
BITBOARD mask_passed[2][64];
BITBOARD mask_path[2][64];
BITBOARD mask_squarepawn[2][2][64];

BITBOARD mask_ranks[2][8];
BITBOARD mask_files[8];
BITBOARD mask_cols[64];

BITBOARD mask_rookfiles;
BITBOARD mask_edge;
BITBOARD mask_corner;
BITBOARD mask_centre;
BITBOARD mask_wide_centre;

BITBOARD not_mask_rookfiles;
BITBOARD not_a_file;
BITBOARD not_h_file;
BITBOARD not_mask_edge;
BITBOARD not_mask_corner;
BITBOARD not_rank6;
BITBOARD not_rank1;

BITBOARD mask[64];

BITBOARD bit_e1h1;
BITBOARD bit_e1a1;
BITBOARD bit_e8h8;
BITBOARD bit_e8a8;

bool SameDiag(const int a, const int b, const int c);
bool SameLine(const int a, const int b, const int c);

void SetBit(BITBOARD& bb, int square);
void SetBitFalse(BITBOARD& bb, int square);
void PrintBitBoard(BITBOARD bb);
void PrintCell(int x, BITBOARD bb);

void SetColors();
void SetPawnBits();
void SetPawnMoves();
void SetRanksFiles();
void SetSquares();
void SetMoves();
void SetRanks();
void SetMaskPawns();
void SetDifference();
void SetBetweenVector();

void SetKingDistance();

void SetBitAfter();
int GetEdge(int sq, int plus);

void SetBit(BITBOARD& bb, int square)
{
	bb |= (1ui64 << square);
}

void SetBitFalse(BITBOARD& bb, int square)
{
	bb &= ~mask[square];
}

void PrintBitBoard(BITBOARD bb)
{
	printf("\n");
	for (int x = 56; x < 64; x++)
		PrintCell(x, bb);
	for (int x = 48; x < 56; x++)
		PrintCell(x, bb);
	for (int x = 40; x < 48; x++)
		PrintCell(x, bb);
	for (int x = 32; x < 40; x++)
		PrintCell(x, bb);
	for (int x = 24; x < 32; x++)
		PrintCell(x, bb);
	for (int x = 16; x < 24; x++)
		PrintCell(x, bb);
	for (int x = 8; x < 16; x++)
		PrintCell(x, bb);
	for (int x = 0; x < 8; x++)
		PrintCell(x, bb);
}

void PrintCell(int x, BITBOARD bb)
{
	if (mask[x] & bb)
		printf(" X");
	else
		printf(" -");
	if ((x + 1) % 8 == 0)
		printf("\n");
}

void SetBits()
{
	SetColors();
	SetRanks();
	SetPawnMoves();
	SetPawnBits();
	SetRanksFiles();
	SetSquares();
	SetDifference();
	SetMaskPawns();
	SetBetweenVector();
	SetMoves();
	SetKingDistance();
	SetBitAfter();
}

void SetColors()
{
	bit_colors = 0;
	for (int x = 0; x < 64; x++)
	{
		if (colors[x] == 1)
			SetBit(bit_colors, x);
	}
	for (int x = 0; x < 64; x++)
	{
		if (colors[x] == 0)
			SetBit(bit_color[0], x);
		if (colors[x] == 1)
			SetBit(bit_color[1], x);
	}
}

void SetRanks()
{
	for (int x = 0; x < 64; x++)
	{
		squares[0][x] = x;
		squares[1][x] = (7 - row[x]) * 8 + col[x];
		row2[0][x] = row[x];
		row2[1][x] = 7 - row[x];
	}
	for (int s = 0; s < 2; s++)
		for (int x = 0; x < 64; x++)
		{
			lastsquare[0][x] = col[x] + A8;
			lastsquare[1][x] = col[x];
		}
	memset(adjfile, 0, sizeof(adjfile));
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 64; y++)
		{
			if (abs(col[x] - col[y]) < 2)
				adjfile[x][y] = 1;
		}
	}
}

void SetPawnMoves()
{
	for (int x = 0; x < 64; x++)
	{
		pawnleft[0][x] = -1;
		pawnleft[1][x] = -1;
		pawnright[0][x] = -1;
		pawnright[1][x] = -1;
		if (col[x] > 0)
		{
			if (row[x] < 7) { pawnleft[0][x] = x + 7; }
			if (row[x] > 0) { pawnleft[1][x] = x - 9; }
		}
		if (col[x] < 7)
		{
			if (row[x] < 7) { pawnright[0][x] = x + 9; }
			if (row[x] > 0) { pawnright[1][x] = x - 7; }
		}
	}
	for (int x = 0; x < 64; x++)
	{
		if (row[x] < 7)
		{
			pawnplus[0][x] = x + 8;
		}
		if (row[x] < 6)
		{
			pawndouble[0][x] = x + 16;
		}
		if (row[x] > 0)
		{
			pawnplus[1][x] = x - 8;
		}
		if (row[x] > 1)
		{
			pawndouble[1][x] = x - 16;
		}
	}
}

void SetPawnBits()
{
	memset(bit_pawncaptures, 0, sizeof(bit_pawncaptures));

	for (int x = 0; x < 64; x++)
	{
		if (col[x] > 0)
		{
			if (row[x] < 7)
			{
				SetBit(bit_pawncaptures[0][x], pawnleft[0][x]);
				SetBit(bit_left[0][x], pawnleft[0][x]);
			}
			if (row[x] > 0)
			{
				SetBit(bit_pawncaptures[1][x], pawnleft[1][x]);
				SetBit(bit_left[1][x], pawnleft[1][x]);
			}
		}
		if (col[x] < 7)
		{
			if (row[x] < 7)
			{
				SetBit(bit_pawncaptures[0][x], pawnright[0][x]);
				SetBit(bit_right[0][x], pawnright[0][x]);
			}
			if (row[x] > 0)
			{
				SetBit(bit_pawncaptures[1][x], pawnright[1][x]);
				SetBit(bit_right[1][x], pawnright[1][x]);
			}
		}
	}
	for (int x = 0; x < 64; x++)
	{
		bit_pawndefends[0][x] = bit_pawncaptures[1][x];
		bit_pawndefends[1][x] = bit_pawncaptures[0][x];
	}
}

void SetRanksFiles()
{
	for (int y = 0; y < 8; y++)
		for (int x = 0; x < 64; x++)
		{
			if (col[x] == y)
				SetBit(mask_files[y], x);
			if (row[x] == y)
			{
				SetBit(mask_ranks[0][y], x);
			}
			if (row[x] == 7 - y)
			{
				SetBit(mask_ranks[1][y], x);
			}
		}
	for (int x = 0; x < 64; x++)
		for (int y = 0; y < 64; y++)
		{
			if (col[x] == col[y])
				SetBit(mask_cols[x], y);
		}
	not_a_file = ~mask_files[0];
	not_h_file = ~mask_files[7];
	not_rank6 = ~mask_ranks[0][6];
	not_rank1 = ~mask_ranks[0][1];
	mask_rookfiles = mask_files[0] | mask_files[7];
}

void SetSquares()
{
	for (int x = 0; x < 64; x++)
	{
		SetBit(mask[x], x);
	}
	for (int x = 0; x < 64; x++)
	{
		if (col[x] > 0)
			SetBit(bit_adjacent[x], x - 1);
		if (col[x] < 7)
			SetBit(bit_adjacent[x], x + 1);
	}
	for (int x = 0; x < 64; x++)
	{
		if (row[x] == 0 || row[x] == 7 || col[x] == 0 || col[x] == 7)
			SetBit(mask_edge, x);
	}
	for (int x = 0; x < 64; x++)
	{
		if (col[x] < 3)
			SetBit(mask_abc, x);
		if (col[x] > 4)
			SetBit(mask_ghi, x);
	}
	SetBit(mask_centre, D4);
	SetBit(mask_centre, E4);
	SetBit(mask_centre, D5);
	SetBit(mask_centre, E5);

	SetBit(mask_corner, A1);
	SetBit(mask_corner, A8);
	SetBit(mask_corner, H1);
	SetBit(mask_corner, H8);

	not_mask_corner = ~mask_corner;
	not_mask_rookfiles = ~mask_rookfiles;
	not_mask_edge = ~mask_edge;

	bit_e1h1 = mask[F1] | mask[G1];
	bit_e1a1 = mask[D1] | mask[C1] | mask[B1];
	bit_e8h8 = mask[F8] | mask[G8];
	bit_e8a8 = mask[D8] | mask[C8] | mask[B8];
}

void SetDifference()
{
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 64; y++)
		{
			int col_diff = abs(col[x] - col[y]);
			int row_diff = abs(row[x] - row[y]);
			if (col_diff > row_diff)
			{
				difference[x][y] = col_diff;
				pawn_difference[x][y] = col_diff * 20;
			}
			else
			{
				difference[x][y] = row_diff;
				pawn_difference[x][y] = row_diff * 20;
			}
		}
	}
}

void SetMoves()
{
	memset(bit_moves[N], 0, sizeof(bit_moves[N]));
	memset(bit_moves[B], 0, sizeof(bit_moves[B]));
	memset(bit_moves[R], 0, sizeof(bit_moves[R]));
	memset(bit_moves[Q], 0, sizeof(bit_moves[Q]));
	memset(bit_moves[K], 0, sizeof(bit_moves[K]));

	for (int x = 0; x < 64; x++)
	{
		bit_moves[N][x] = 0;
		if (row[x] < 6 && col[x] < 7)
			bit_moves[N][x] |= mask[x + 17];
		if (row[x] < 7 && col[x] < 6)
			bit_moves[N][x] |= mask[x + 10];
		if (row[x] < 6 && col[x]>0)
			bit_moves[N][x] |= mask[x + 15];
		if (row[x] < 7 && col[x]>1)
			bit_moves[N][x] |= mask[x + 6];
		if (row[x] > 1 && col[x] < 7)
			bit_moves[N][x] |= mask[x - 15];
		if (row[x] > 0 && col[x] < 6)
			bit_moves[N][x] |= mask[x - 6];
		if (row[x] > 1 && col[x] > 0)
			bit_moves[N][x] |= mask[x - 17];
		if (row[x] > 0 && col[x] > 1)
			bit_moves[N][x] |= mask[x - 10];
	}
	for (int x = 0; x < 64; x++)
	{
		if (col[x] > 0)
			bit_moves[K][x] |= mask[x - 1];
		if (col[x] < 7)
			bit_moves[K][x] |= mask[x + 1];
		if (row[x] > 0)
			bit_moves[K][x] |= mask[x - 8];
		if (row[x] < 7)
			bit_moves[K][x] |= mask[x + 8];
		if (col[x] < 7 && row[x] < 7)
			bit_moves[K][x] |= mask[x + 9];
		if (col[x] > 0 && row[x] < 7)
			bit_moves[K][x] |= mask[x + 7];
		if (col[x] > 0 && row[x] > 0)
			bit_moves[K][x] |= mask[x - 9];
		if (col[x] < 7 && row[x]>0)
			bit_moves[K][x] |= mask[x - 7];
	}
	for (int x = 0; x < 64; x++)
	{
		bit_moves[B][x] = 0;
		bit_moves[R][x] = 0;
		bit_moves[Q][x] = 0;

		for (int y = 0; y < 64; y++)
		{
			if (x == y)
				continue;
			if (nwdiag[x] == nwdiag[y] || nediag[x] == nediag[y])
				bit_moves[B][x] |= mask[y];
			if (row[x] == row[y] || col[x] == col[y])
				bit_moves[R][x] |= mask[y];
			if (nwdiag[x] == nwdiag[y] || nediag[x] == nediag[y] || row[x] == row[y] || col[x] == col[y])
				bit_moves[Q][x] |= mask[y];
		}
	}
}

void SetBetweenVector()
{
	for (int x = 0; x < 64; x++)
		for (int y = 0; y < 64; y++)
		{
			if (row[x] == row[y])
			{
				if (y > x)
					for (int z = x + 1; z < y; z++)
						SetBit(bit_between[x][y], z);
				else
					for (int z = y + 1; z < x; z++)
						SetBit(bit_between[x][y], z);
			}
			if (col[x] == col[y])
			{
				if (y > x)
					for (int z = x + 8; z < y; z += 8)
						SetBit(bit_between[x][y], z);
				else
					for (int z = y + 8; z < x; z += 8)
						SetBit(bit_between[x][y], z);
			}
			if (nwdiag[x] == nwdiag[y])
			{
				if (y > x)
					for (int z = x + 7; z < y; z += 7)
						SetBit(bit_between[x][y], z);
				else
					for (int z = y + 7; z < x; z += 7)
						SetBit(bit_between[x][y], z);
			}
			if (nediag[x] == nediag[y])
			{
				if (y > x)
					for (int z = x + 9; z < y; z += 9)
						SetBit(bit_between[x][y], z);
				else
					for (int z = y + 9; z < x; z += 9)
						SetBit(bit_between[x][y], z);
			}
		}
}

void SetMaskPawns()
{
	for (int x = 0; x < 64; x++)
	{
		for (int y = A2; y < A8; y++)
		{
			int dp = row[y];
			int dk = difference[x][col[y]];
			if (dp == 6)
				dp = 5;
			if(dk <= dp)
				SetBit(mask_squarepawn[0][0][x], y);
			if (dk <= dp + 1)
				SetBit(mask_squarepawn[1][0][x], y);
			dp = 7 - row[y];
			if (dp == 6)
				dp = 5;
			dk = difference[x][col[y] + A8];
			if (dk <= dp)
				SetBit(mask_squarepawn[0][1][x], y);
			if (dk <= dp + 1)
				SetBit(mask_squarepawn[1][1][x], y);
		}
	}
	for (int x = 0; x < 64; ++x)
	{
		//Algebraic(x);
		//PrintBitBoard(mask_squarepawn[1][0][x]);
		//_getch();
	}
	for (int x = 0; x < 64; ++x)
	{
		for (int y = 0; y < 64; y++)
		{
			if (abs(col[x] - col[y]) < 2)
			{
				if (row[x] < row[y] && row[y] < 7)
					SetBit(mask_passed[0][x], y);
				if (row[x] > row[y] && row[y] > 0)
					SetBit(mask_passed[1][x], y);
			}
			if (abs(col[x] - col[y]) == 1 && row[x] != 0 && row[x] != 7)
			{
				if (row[x] >= row[y] || (row[x] == 1 && row[y] == 2))
					SetBit(mask_backward[0][x], y);
				if (row[x] <= row[y] || (row[x] == 6 && row[y] == 5))
					SetBit(mask_backward[1][x], y);
			}
			if (abs(col[x] - col[y]) == 1)
			{
				SetBit(mask_isolated[x], y);
			}
			if (col[x] - col[y] == 1)
			{
				SetBit(mask_left_col[x], y);
			}
			if (col[y] - col[x] == 1)
			{
				SetBit(mask_right_col[x], y);
			}
			if (col[x] == col[y])
			{
				if (row[x] < row[y])
					SetBit(mask_path[0][x], y);
				if (row[x] > row[y])
					SetBit(mask_path[1][x], y);
			}
		}
	}
	mask_wide_centre = mask[C4] | mask[D4] | mask[E4] | mask[C5] | mask[D5] | mask[E5];
}

void SetTrappedMinorPiece()
{
	memset(bishop_a7, 0, sizeof(bishop_a7));
	memset(bishop_h7, 0, sizeof(bishop_h7));
	memset(knight_a7, 0, sizeof(knight_a7));
	memset(knight_h7, 0, sizeof(knight_h7));

	SetBit(bishop_a7[0], C7);
	SetBit(bishop_a7[0], B6);
	SetBit(bishop_a7[1], C2);
	SetBit(bishop_a7[1], B3);
	SetBit(bishop_h7[0], F7);
	SetBit(bishop_h7[0], G6);
	SetBit(bishop_h7[1], F2);
	SetBit(bishop_h7[1], G3);

	SetBit(knight_a7[0], B7);
	SetBit(knight_a7[0], C6);
	SetBit(knight_a7[1], B2);
	SetBit(knight_a7[1], C3);
	SetBit(knight_h7[0], G7);
	SetBit(knight_h7[0], F6);
	SetBit(knight_h7[1], F3);
	SetBit(knight_h7[1], G2);
}

void SetKingDistance()
{
	memset(kingqueen, 0, sizeof(kingqueen));
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 64; y++)
		{
			kingqueen[x][y] = taxi[difference[x][y]];
		}
	}

	memset(kingknight, 0, sizeof(kingknight));
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 64; y++)
		{
			if (bit_moves[N][x] & bit_moves[K][y])
				kingknight[x][y] = 5;
		}
	}
	memset(kingking, 0, sizeof(kingking));
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 64; y++)
		{
			kingking[x][y] = kingtaxi[difference[x][y]];
		}
	}
}

int GetEdge(int sq, int plus)
{
	do
	{
		sq += plus;
	} while (col[sq] > 0 && col[sq] < 7 && row[sq] > 0 && row[sq] < 7);
	return sq;
}

bool SameDiag(const int a, const int b, const int c)
{
	if (nwdiag[a] == nwdiag[b] && nwdiag[b] == nwdiag[c])
		return true;
	if (nediag[a] == nediag[b] && nediag[b] == nediag[c])
		return true;
	return false;
}

bool SameLine(const int a, const int b, const int c)
{
	if (col[a] == col[b] && col[b] == col[c])
		return true;
	if (row[a] == row[b] && row[b] == row[c])
		return true;
	if (nwdiag[a] == nwdiag[b] && nwdiag[b] == nwdiag[c])
		return true;
	if (nediag[a] == nediag[b] && nediag[b] == nediag[c])
		return true;
	return false;
}

void SetBitAfter()
{
	for (int x = 0; x < 64; x++)
		for (int y = 0; y < 64; y++)
		{
			if (x == y)
				continue;
			if (row[x] == row[y])
			{
				if (y > x)
					for (int z = y; z <= row[y] * 8 + 7; z++)
						SetBit(bit_after[x][y], z);
				else
					for (int z = y; z >= row[y] * 8; z--)
						SetBit(bit_after[x][y], z);
			}

			if (col[x] == col[y])
			{
				if (y > x)
					for (int z = y; z <= 56 + col[y]; z += 8)
						SetBit(bit_after[x][y], z);
				else
					for (int z = y; z >= col[y]; z -= 8)
						SetBit(bit_after[x][y], z);
			}
			if (nwdiag[x] == nwdiag[y])
			{
				if (y > x)
					for (int z = y; z <= GetEdge(x, 7); z += 7)
						SetBit(bit_after[x][y], z);
				else
					for (int z = y; z >= GetEdge(x, -7); z -= 7)
						SetBit(bit_after[x][y], z);
			}

			if (nediag[x] == nediag[y])
			{
				if (y > x)
					for (int z = y; z <= GetEdge(x, 9); z += 9)
						SetBit(bit_after[x][y], z);
				else
					for (int z = y; z >= GetEdge(x, -9); z -= 9)
						SetBit(bit_after[x][y], z);
			}
		}
}

bool IsOneBit(BITBOARD x)
{
	return x && !(x & (x - 1));
}

int CountBits(BITBOARD b1)
{
	return __popcnt64(b1);
}

int NextBit(BITBOARD bb)
{
	unsigned long sq;
	_BitScanForward64(&sq, bb);   // bb must be non-zero
	return sq;
}

int NextHighBit(BITBOARD bb)
{
	unsigned long sq;
	_BitScanReverse64(&sq, bb);   // bb must be non-zero
	return sq;
}

int msb_index(uint64_t bb)
{
	unsigned long idx;
	_BitScanReverse64(&idx, bb);   // bb must be non-zero
	return (int)idx;
}











