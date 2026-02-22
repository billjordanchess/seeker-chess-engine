#include "globals.h"

int linemoves[64][8];
int knightmoves[64][8];
int kingmoves[64][8];

void SetKnightMoves();
void SetKingMoves();
void SetLineMoves();

void SetKnightMoves()
{
	int count = 0;

	for (int x = 0; x < 64; x++)
	{
		count = 0;
		if (row[x] < 6 && col[x] < 7)
			knightmoves[x][count++] = x + 17;
		if (row[x] < 7 && col[x] < 6)
			knightmoves[x][count++] = x + 10;
		if (row[x] < 6 && col[x]>0)
			knightmoves[x][count++] = x + 15;
		if (row[x] < 7 && col[x]>1)
			knightmoves[x][count++] = x + 6;
		if (row[x] > 1 && col[x] < 7)
			knightmoves[x][count++] = x - 15;
		if (row[x] > 0 && col[x] < 6)
			knightmoves[x][count++] = x - 6;
		if (row[x] > 1 && col[x] > 0)
			knightmoves[x][count++] = x - 17;
		if (row[x] > 0 && col[x] > 1)
			knightmoves[x][count++] = x - 10;
	}
}

void SetKingMoves()
{
	int count = 0;

	for (int x = 0; x < 64; x++)
	{
		count = 0;
		if (col[x] > 0)
			kingmoves[x][count++] = x - 1;
		if (col[x] < 7)
			kingmoves[x][count++] = x + 1;
		if (row[x] > 0)
			kingmoves[x][count++] = x - 8;
		if (row[x] < 7)
			kingmoves[x][count++] = x + 8;
		if (col[x] < 7 && row[x] < 7)
			kingmoves[x][count++] = x + 9;
		if (col[x] > 0 && row[x] < 7)
			kingmoves[x][count++] = x + 7;
		if (col[x] > 0 && row[x] > 0)
			kingmoves[x][count++] = x - 9;
		if (col[x] < 7 && row[x] > 0)
			kingmoves[x][count++] = x - 7;
	}
}

void SetLineMoves()
{
	int count = 0;
		for (int x = 0; x < 64; x++)
	{
		count = 0;
		for (int dir = 0; dir < 8; dir++)
		{
			linemoves[x][dir] = -1;
		}

		if (col[x] > 0) linemoves[x][WEST] = x - 1;
		if (col[x] < 7) linemoves[x][EAST] = x + 1;
		if (row[x] > 0) linemoves[x][SOUTH] = x - 8;
		if (row[x] < 7) linemoves[x][NORTH] = x + 8;
		if (col[x] < 7 && row[x] < 7) linemoves[x][NE] = x + 9;
		if (col[x] > 0 && row[x] < 7) linemoves[x][NW] = x + 7;
		if (col[x] > 0 && row[x] > 0) linemoves[x][SW] = x - 9;
		if (col[x] < 7 && row[x] > 0) linemoves[x][SE] = x - 7;
	}
}
