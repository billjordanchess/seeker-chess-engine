
#include <iostream>

#include "globals.h"

using namespace std;

int GetCurrentDepth();
int GetBest(int ply);
int GetBest2(int ply);

void Alg(int a, int b)
{
	Algebraic(a);
	Algebraic(b);
}

void Alg1(int a)
{
	Algebraic(a);
}

void Alg2(int a, int b)
{
	Algebraic(a);
	printf("X");
	Algebraic(b);
}

void Algebraic(int sq)
{
	char file = 'a' + (sq & 7);
	char rank = '1' + (sq >> 3);
	printf("%c%c", file, rank);
	//std::cout << file << row[rank] + 1;
}

void ShowAll2()
{
	if (nodes < 1)
		return;

	DisplayBoard();
	memset(done, 0, sizeof(done));

	printf(" ply %d", ply);
	printf(" current max %d", currentmax);
	printf(" currentdepth %d ", GetCurrentDepth());
	printf(" nodes %lld ", nodes);
	printf(" side %d", side);
	printf(" xside %d\n", xside);

	cout << " currentkey " << currentkey << endl;

	for (int z = ply; z > 0; z--)
	{
		//if (z>1 && first_move[z-1] == first_move[z])
		//	printf(" NULL ");
		//else
		{
			if (game_list[hply - z].capture == EMPTY)
				Alg(game_list[hply - z].from, game_list[hply - z].to);
			else
				Alg2(game_list[hply - z].from, game_list[hply - z].to);
			printf(" ");
		}
	}
	printf("\n  order ");
	for (int z = 1; z <= ply; z++)
		printf(" %d ", PlyMove[z]);

	printf("\n");
	printf(" extend ");
	for (int x = 1; x <= ply; x++)
	{
		printf(" %d ", extend[x]);
	}
	printf("\n");
	printf(" check ");
	for (int x = 1; x <= ply; x++)
	{
		printf(" %d ", (game_list[x].flags & INCHECK));
	}
	printf("\n");
	printf(" threat ");
	for (int x = 1; x <= ply; x++)
	{
		printf(" %d ", Threat[x]);
	}
	printf("\n");
	printf(" prom ");
	for (int x = 1; x <= ply; x++)
	{
		;//   printf(" %d ",Prom[x]);
	}
	printf("\n");
	_getch();
}

void z()
{
	ShowAll2();
}

void z2()
{
	ShowAll(ply);
}

void p(BITBOARD bb)
{
	PrintBitBoard(bb);
}

void ShowAll(int ply)
{
	move_data* g;
	DisplayBoard();
	memset(done, 0, sizeof(done));

	printf(" ply ");
	printf("%d", ply);
	printf(" current max ");
	printf("%d", currentmax);
	printf(" currentdepth %d", GetCurrentDepth());
	printf(" nodes ");
	printf("%lld", nodes);
	printf(" side ");
	printf("%d", side);
	printf(" xside ");
	printf("%d", xside);
	printf("\n");
	for (int z = ply; z > 0; z--)
	{
		Alg(game_list[hply - z].from, game_list[hply - z].to);
	}
	printf("\n");

	int j;
	int c = 0;

	printf(" non-caps \n");
	for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
		//     for(int i=first_move[ply+1];i<first_move[ply + 2];i++)
	{
		j = GetBest(ply);
		{
			//how to display current line?
			g = &move_list[j];
			Alg(move_list[j].from, move_list[j].to);
			printf(" ");
			printf(" score ");
			printf("%d", g->score);
			printf("\n");
			c++;
		}
	}
	printf("\n moves %d\n", c);
	printf("\n");
	/*
	   for(int i=first_move[ply];i<first_move[ply + 1];i++)//realone
	 //     for(int i=first_move[ply+1];i<first_move[ply + 2];i++)
	   {
		   j = GetBest(ply);
		   {
		   //how to display current line?
		   g = &move_list[j];
		   printf("%s",MoveString(move_list[j]));
		   printf(" ");
		   printf(" score ");
		   printf("%d",g->score);
		   printf("\n");
		   }
	   }
	printf("\n");

	   for(int i=first_move[ply+1];i<first_move[ply + 2];i++)
	 //     for(int i=first_move[ply+1];i<first_move[ply + 2];i++)
	   {
		   //j = GetBest(ply);
		   {
		   //how to display current line?
		   g = &move_list[i];
		   printf("%s",move_str(move_list[i]));
		   printf(" ");
		   printf(" score ");
		   printf("%d",g->score);
		   printf("\n");
		   }
	   }
	 */
	_getch();
}

void ShowMoves(int p)
{
	move_data* g;
	printf("\n");

	int j;
	for (int i = first_move[p]; i < first_move[p + 1]; i++)
	{
		j = GetBest(p);
		{
			//how to display current line?
			g = &move_list[j];
			Alg(g->from, g->to);
			printf(" ");
			printf(" score ");
			printf("%d", g->score);
			printf("\n");
		}
	}
	for (int i = first_move[p]; i < first_move[p + 1]; i++)
	{
		j = GetBest(p);
		{
			//how to display current line?
			g = &move_list[j];
			Alg(g->from, g->to);
			printf(" ");
			printf(" score ");
			printf("%d", g->score);
			printf("\n");
		}
	}
	_getch();
}

int Debug(const int p)
{
	for (int x = 0; x < 64; x++)
	{
		if (!(mask[x] & bit_all) && b[x] < EMPTY)
		{
			//Alg(x,x);
			//z();
			//_getch();
		}
	}

	for (int x = 0; x < 64; x++)
	{
		if (b[x] == EMPTY && mask[x] & bit_all)
		{
			Alg1(x);
			printf(" units place %d ", p);
			z();
			return 1;
		}
	}
	//return 0;

	/*
	if(color[kingloc[0]]==1 || color[pieces[1][5][0]]==0)
	{
	  printf(" king_zone bug ");
	  ShowAll2();
	  return 0;
	}
	*/
	int wr = 0, wb = 0, wn = 0;
	for (int x = 0; x < 64; x++)
	{
		if (mask[x] & bit_pieces[0][1])
			wn++;
		if (mask[x] & bit_pieces[0][B])
			wb++;
		if (mask[x] & bit_pieces[0][R])
			wr++;

		if (wn > 2)
		{
			printf("\n 3 knights");
			printf(" place %d ", p);
			z();
		}
		if (wb > 2)
		{
			printf("\n 3 bishops");
			printf(" place %d ", p);
			z();
		}
		if (wr > 2)
		{
			printf("\n 3 rooks");
			printf(" place %d ", p);
			z();
		}
	}
	int knights[2];
	knights[0] = 0;
	knights[1] = 0;
	if (knights[0] > 2 || knights[1] > 2)
	{
		//printf(" bug piecess %d ",pieces[0]);
		//printf(" piecess %d ",pieces[1]);
		printf(" pieces[0][1][0] %d ", pieces[0][1][0]);
		printf(" pieces[0][1][1] %d ", pieces[0][1][1]);
		printf(" pieces[1][1][0] %d ", pieces[1][1][0]);
		printf(" pieces[1][1][1] %d ", pieces[1][1][1]);
		PrintBitBoard(bit_pieces[0][1]);
		PrintBitBoard(bit_pieces[1][1]);
		printf(" place %d ", p);
		ShowAll2();
	}

	if (ply > 1)
		for (int s = 0; s < 2; s++)
		{
			for (int x = 0; x < 2; x++)
			{
				if (total[s][N] > x && b[pieces[s][N][x]] != KNIGHT)
				{
					printf(" bug place %d ", p);
					printf(" pieces[s][N][x] %d ", pieces[s][N][x]);
					printf("debug 4");
					PrintBitBoard(bit_pieces[0][1]);
					ShowAll2();
				}
				if (total[s][B] > x && b[pieces[s][B][x]] != BISHOP)
				{
					printf(" bug bishop ");
					printf(" place %d ", p);
					printf(" x %d ", x);
					printf(" bishop[s][0] %d ", pieces[s][B][0]);
					printf(" bishop[s][1] %d ", pieces[s][B][1]);
					printf(" total %d ", total[s][B]);
					ShowAll2();
				}
				if (total[s][R] > x && b[pieces[s][R][x]] != ROOK)
				{
					printf(" bug rook ");
					printf(" rook[s][x] %d ", pieces[s][R][x]);
					printf(" place %d ", p);
					ShowAll2();
					printf("debug 6");
				}
			}
			if (b[kingloc[s]] != KING)
			{
				printf("bug no king");
				Alg(kingloc[0], kingloc[1]);
				printf(" place %d ", p);
				ShowAll2();
				return 1;
			}
			if (kingloc[s] == -1)
			{
				printf(" bug place %d ", p);
				printf("no king_zone ");
				ShowAll2();
			}
		}
	return 0;
}

int GetBest(int ply)
{
	move_data* g;
	int bestscore = -100000000;
	int best = 0;
	for (int i = 0; i < first_move[ply + 1] - first_move[ply]; i++)
	{
		if (done[i] == 1) continue;
		g = &move_list[first_move[ply] + i];
		if (g->score > bestscore)
		{
			bestscore = g->score;
			best = i;
		}
	}
	if (best < 1000) done[best] = 1;//1000?
	return first_move[ply] + best;
}

int GetBest2(int ply)
{
	move_data* g;
	int bestscore = -100000000;
	int best = 0;
	for (int i = 0; i < first_move[ply + 1] - first_move[ply]; i++)
	{
		g = &move_list[first_move[ply] + i];
		if (g->score > bestscore)
		{
			bestscore = g->score;
			best = i;
		}
	}
	return first_move[ply] + best;
	//*/
}

void ShowAllEval(int ply)
{
	move_data* g;
	DisplayBoard();
	memset(done, 0, sizeof(done));

	printf(" ply ");
	printf("%d", ply);
	printf(" current max ");
	printf("%d", currentmax);
	printf(" currentdepth %d", GetCurrentDepth());
	printf(" nodes ");
	printf("%lld", nodes);
	printf(" side ");
	printf("%d", side);
	printf(" xside ");
	printf("%d", xside);
	printf("\n");

	printf("\n");

	int j;

	printf(" non-caps \n");

	for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
	{
		MakeMove(move_list[i].from, move_list[i].to, move_list[i].flags);
			move_list[i].score = -Eval(-10000, 10000);
			UnMakeMove();
	}
	for (int i = first_move[ply]; i < first_move[ply + 1]; i++)
	{
		j = GetBest(ply);
		{
			g = &move_list[j];
			Alg(move_list[j].from, move_list[j].to);
			printf(" ");
			printf(" eval ");
			printf("%d", g->score);
			printf("\n");
		}
	}
	printf("\n");
	_getch();
}

