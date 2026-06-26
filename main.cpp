#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

using namespace std;

#include "globals.h"

void InitMagicAttacks();
void TestQueenAttacks();

BITBOARD PinnersPossible(const int s, const int xs);
BITBOARD GetPinMask(const int s, const int xs);
void BuildAttackMap();

void GenRoot(const int, const int);

void uci();

const int White = 0;
const int  Black = 1;

void ClearContHistory();

int Train();

string MoveString(int, int, int);

void GenCheck();
void GenCaptures(const int s, const int xs, BITBOARD pin_mask);
void GenQuietMoves(const int, BITBOARD, const BITBOARD(&bit_check)[6]);
BITBOARD GenChecks(const int, const int, BITBOARD);

void ShowHelp();
void SetUp();
void xboard();

void FreeAllHash();

void AfterCastle(const int);

void LoadBook();

static char piece_char[2][6] =
{
  {'P', 'N', 'B', 'R', 'Q', 'K'},
  {'p', 'n', 'b', 'r', 'q', 'k'}
};

static int board_color[64] =
{
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1
};

int LoadDiagram(string file);
int ParseMove(string s);

int flip = 0;

int computer_side;
int player[2];

int fixed_time;
int fixed_depth;
int turn = 0;

extern BITBOARD collisions;

extern unsigned long total_percent;
extern unsigned long total_moves;

void DisplayResult();
void StartGame();
void SetMaterial();
void SetBits();
void Free();

move_data engine_move;

double av = av_nodes;
double cut = cut_nodes;

int main()
{
	cout << "seeker counters no target" << endl;
	cout << "Bilbo Chess Engine 1.0" << endl;
	cout << "Version new reduce , 19/2/26" << endl;
	cout << "Bill Jordan 2026" << endl;
	cout << "FIDE Master and multiple state champion." << endl;
	cout << "I have published a number of chess books" << endl;
	cout << "including books on chess programming." << endl;
	cout << "My author page at Amazon is at billjordanchess" << endl;
	cout << "https://www.amazon.com/-/e/B07F5WSFHZ " << endl;
	cout << "" << endl;
	cout << "\"help\" displays a list of commands." << endl;
	cout << "" << endl;

	string s;
	string sFen;
	string sText;

	int m;
	int turns = 0;
	U64 t;

	int from, to, flags;

	BITBOARD nps;

	fixed_time = 0;

	RandomizeHash();
	SetBits();
	SetUp();
	StartGame();
	SetMaterial();
	LoadBook();

	InitMagicAttacks();
	//TestQueenAttacks();  // should print "MagicQueenAttacks OK"

	srand(time(NULL));

	GenCheck();
	computer_side = EMPTY;
	player[0] = 0;
	player[1] = 0;
	max_time = 1 << 25;
	max_depth = 8;

	while (true)
	{
		if (side == computer_side)
		{
			player[side] = 1;
			start_time = GetTime();//
			engine_move = Think(fixed_time, max_depth);
			turns++;

			currentkey = GetKey();

			if (engine_move.from != 0 || engine_move.to != 0)
			{
				Alg(engine_move.from, engine_move.to); cout << endl;
			}
			else
			{
				cout << "(no legal moves)" << endl;
				computer_side = EMPTY;
				DisplayBoard();

				GenCheck();
				continue;
			}
			cout << endl << "collisions " << collisions << endl;
			collisions = 0;

			cout << "Computer's move: "; Alg(engine_move.from, engine_move.to);
			MakeMove(engine_move.from, engine_move.to, engine_move.flags);

			cout << endl << "nodes " << nodes;
			cout << endl << "qnodes " << qnodes;
			cout << endl << "cut nodes " << cut_tt_nodes + cut_nodes;
			cout << endl << "all nodes " << all_nodes;
			cout << endl << "first nodes " << first_nodes;
			if (cut_tt_nodes > 0)
				cout << endl << "tt percent " << (first_tt_nodes * 100 / cut_tt_nodes);
			if (cut_nodes > 0)
				cout << endl << "percent " << (first_nodes * 100 / cut_nodes);
			av = av_nodes;
			cut = cut_nodes;
			if (cut_nodes > 0)
				cout << endl << "av cut " << (av_nodes) / cut_nodes << " " << endl;
			if (a_nodes > 0)
				cout << endl << "av sort " << (100 - b_nodes * 100 / a_nodes) << " " << endl;
			if (total_moves > 0)
				cout << endl << "av depth of break " << (total_percent / total_moves) << " " << endl;

			SetMaterial();

			t = GetTime() - start_time;
			cout << endl << "Time: ms " << t << endl;
			if (t > 0)
				nps = nodes / t;
			else
				nps = 0;
			nps *= 1000;
			cout << "Nodes per second: " << nps << endl;
			ply = 0;

			first_move[0] = 0;

			GenCheck();
			DisplayResult();
			cout << "turn " << turn++ << endl;
			DisplayBoard();
			continue;
		}
		cout << "Enter move or command> ";
		cin >> s;

		if (s == "d")
		{
			DisplayBoard();
			continue;
		}
		if (s == "f")
		{
			flip = 1 - flip;
			DisplayBoard();
			continue;
		}
		if (s == "go")
		{
			computer_side = side;
			continue;
		}
		if (s == "help")
		{
			ShowHelp();
			continue;
		}
		if (s == "moves")
		{
			cout << "Moves " << endl;

			GenCheck();
			for (int i = 0; i < first_move[1]; i++)
			{
				Alg(move_list[i].from, move_list[i].to); cout << endl;
			}
			continue;
		}
		if (s == "new")
		{
			StartGame();
			computer_side = EMPTY;
			continue;
		}
		if (s == "on" || s == "p")
		{
			computer_side = side;
			continue;
		}
		if (s == "off")
		{
			computer_side = EMPTY;
			continue;
		}
		if (s == "quit")
		{
			cout << "Program exiting" << endl;
			break;
		}
		if (s == "sb")
		{
			sFen = "c:\\users\\bill\\desktop\\bscp\\";
			cin >> sText;
			sFen += sText + ".fen";
			LoadDiagram(sFen);
			continue;
		}
		if (s == "sd")
		{
			scanf_s("%d", &max_depth);
			max_time = 1 << 25;
			fixed_depth = 1;
			continue;
		}
		if (s == "st")
		{
			scanf_s("%ld", &max_time);
			max_time *= 1000;
			max_depth = MAX_PLY;
			fixed_time = 1;
			continue;
		}
		if (s == "sw")
		{
			side ^= 1;
			xside ^= 1;
			continue;
		}
		if (s == "t")
		{
			Train();
			continue;
		}
		if (s == "undo")
		{
			if (!hply)
				continue;
			computer_side = EMPTY;
			UnMakeMove();
			ply = 0;
			if (first_move[0] != 0)
				first_move[0] = 0;

			GenCheck();
			continue;
		}
		if (s == "xboard")
		{
			xboard();
			break;
		}
		if (s == "uci")
		{
			uci();
			break;
		}

		ply = 0;
		first_move[0] = 0;
		GenRoot(side, xside);
		//GenCheck();
		//ShowAll(ply);
		m = ParseMove(s);
		from = move_list[m].from;
		to = move_list[m].to;
		flags = move_list[m].flags;
		//Attack 26/4/21
		if (m == -1 || !MakeMove(from, to, flags))
		{
			cout << "Illegal move. " << endl;
			cout << s << " " << endl;
			MoveString(from, to, 0);
			if (m == -1)
				cout << " m = -1 " << endl;
		}
		if (game_list[hply - 1].flags & PROMOTE && (row[to] == 0 || row[to] == 7))
		{
			RemovePiece(xside, Q, to);
			if (s[4] == 'n' || s[4] == 'N')
				AddPiece(xside, N, to);
			else if (s[4] == 'b' || s[4] == 'B')
				AddPiece(xside, B, to);
			else if (s[4] == 'r' || s[4] == 'R')
				AddPiece(xside, R, to);
			else AddPiece(xside, Q, to);
		}
	}
	Free();
	return 0;
}

int ParseMove(string s)
{
	int from, to, i;

	if (s[0] < 'a' || s[0] > 'h' ||
		s[1] < '1' || s[1] > '8' ||
		s[2] < 'a' || s[2] > 'h' ||
		s[3] < '1' || s[3] > '8')
		return -1;

	from = s[0] - 'a';
	from += ((s[1] - '0') - 1) * 8;
	to = s[2] - 'a';
	to += ((s[3] - '0') - 1) * 8;

	for (i = 0; i < first_move[1]; i++)
		if (move_list[i].from == from && move_list[i].to == to)
		{
			return i;
		}
	return -1;
}

void DisplayBoard()
{
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	int text = 15;

	int i;
	int x = 0;
	int c;

	if (flip == 0)
		cout << "\n8";
	else
		cout << "\n1";

	for (int j = 0; j < 64; ++j)
	{
		if (flip == 0)
			i = Flip[j];
		else
			i = 63 - Flip[j];
		c = 6;
		if (bit_units[White] & mask[i]) c = 0;
		if (bit_units[Black] & mask[i]) c = 1;
		switch (c)
		{
		case EMPTY:
			if (board_color[i] == 0)
				text = 127;
			else
				text = 34;
			SetConsoleTextAttribute(hConsole, text);

			cout << "  ";
			SetConsoleTextAttribute(hConsole, 15);
			break;
		case 0:
			if (board_color[i] == 0)
				text = 126;
			else
				text = 46;
			SetConsoleTextAttribute(hConsole, text);
			cout << " " << piece_char[c][b[i]];
			SetConsoleTextAttribute(hConsole, 15);
			break;

		case 1:
			if (board_color[i] == 0)
				text = 112;
			else
				text = 32;
			SetConsoleTextAttribute(hConsole, text);
			cout << " " << piece_char[c][b[i]];
			SetConsoleTextAttribute(hConsole, 15);
			break;

		default:
			cout << " ." << c;
			break;

		}
		if ((bit_all & mask[i]) && b[i] == EMPTY)
			if (x == 0)
				cout << " " << c;
			else
				cout << " ";
		if (b[i] < 0 || b[i] > 6)
			if (x == 0)
				cout << " ." << b[i];
			else
				cout << "  " << b[i];
		if (flip == 0)
		{
			if ((j + 1) % 8 == 0 && j != 63)
				cout << endl << row[i];
		}
		else
		{
			if ((j + 1) % 8 == 0 && row[i] != 7)
				cout << endl << row[j] + 2;
		}
	}
	if (flip == 0)
		cout << "\n\n   a b c d e f g h\n\n";
	else
		cout << "\n\n   h g f e d c b a\n\n";
}

void DisplayResult()
{
	int i = 0;
	int flag = 0;

	SetMaterial();

	GenCheck();
	for (i = 0; i < first_move[1]; i++)
		if (MakeMove(move_list[i].from, move_list[i].to, move_list[i].flags))
		{
			UnMakeMove();
			flag = 1;
			break;
		}
	if (pawn_mat[0] == 0 && pawn_mat[1] == 0 && piece_mat[0] <= B_VALUE && piece_mat[1] <= B_VALUE)
	{
		cout << "1/2-1/2 {Stalemate}" << endl;

		StartGame();
		computer_side = EMPTY;
		return;
	}
	if (i == first_move[1] && flag == 0)
	{
		DisplayBoard();
		cout << " end of game ";

		if (Attack(xside, kingloc[side]))
		{
			if (side == 0)
			{
				cout << "0-1 {Black mates}" << endl;
			}
			else
			{
				cout << "1-0 {White mates}" << endl;
			}
		}
		else
		{
			cout << "1/2-1/2 {Stalemate}" << endl;
		}
		StartGame();
		computer_side = EMPTY;
	}
	else if (Reps() >= 3)
	{
		cout << "1/2-1/2 {Draw by repetition}" << endl;
		StartGame();
		computer_side = EMPTY;
	}
	else if (fifty >= 100)
	{
		cout << "1/2-1/2 {Draw by fifty move rule}" << endl;
		StartGame();
		computer_side = EMPTY;
	}
}

int Reps()
{
	int repeats = 0;

	for (int i = hply; i >= hply - fifty; i -= 2)
		if (game_list[i].hash == currentkey)
			repeats++;
	return repeats;
}

void xboard()
{
	int computer_side;
	char line[256], command[256];
	int m;
	int post = 0;
	int analyze = 0;

	signal(SIGINT, SIG_IGN);
	printf("\n");
	StartGame();
	fixed_time = 0;
	computer_side = EMPTY;

	while (true)
	{
		fflush(stdout);
		if (side == computer_side)
		{
			engine_move = Think(fixed_time, max_depth);

			if (engine_move.from == 0 && engine_move.to == 0)
			{
				printf(" no engine move!\n");
				return;
			}

			SetMaterial();
			currentkey = GetKey();

			move_list[0].from = engine_move.from;
			move_list[0].to = engine_move.to;
			cout << "move " << MoveString(engine_move.from, engine_move.to, 0) << endl;
			cout.flush();

			MakeMove(engine_move.from, engine_move.to, engine_move.flags);

			ply = 0;
			DisplayResult();
			continue;
		}
		if (!fgets(line, 256, stdin))
			return;
		if (line[0] == '\n')
			continue;
		sscanf_s(line, "%s", command);
		if (!strcmp(command, "xboard"))
			continue;
		if (!strcmp(command, "new"))
		{
			StartGame();
			computer_side = 1;
			continue;
		}
		if (!strcmp(command, "quit"))
			return;
		if (!strcmp(command, "force"))
		{
			computer_side = EMPTY;
			continue;
		}
		if (!strcmp(command, "white"))
		{
			side = 0;
			xside = 1;
			GenCheck();
			computer_side = 1;
			continue;
		}
		if (!strcmp(command, "black"))
		{
			side = 1;
			xside = 0;
			GenCheck();
			computer_side = 0;
			continue;
		}
		if (!strcmp(command, "sd"))
		{
			sscanf_s(line, "sd %d", &max_depth);
			max_time = 1 << 25;
			fixed_depth = 1;
			continue;
		}
		if (!strcmp(command, "st"))
		{
			sscanf_s(line, "st %lld", &max_time);
			max_time *= 1000;
			max_depth = MAX_PLY;
			fixed_time = 1;
			fixed_depth = 0;
			continue;
		}
		if (!strcmp(command, "time"))
		{
			sscanf_s(line, "time %lld", &max_time); // centiseconds
			max_depth = MAX_PLY;
			fixed_time = 1;
			continue;
		}
		if (!strcmp(command, "otim"))
		{
			continue;
		}
		if (!strcmp(command, "go"))
		{
			computer_side = side;
			continue;
		}
		if (!strcmp(command, "random"))
			continue;
		if (!strcmp(command, "level"))
			continue;
		if (!strcmp(command, "hard"))
			continue;
		if (!strcmp(command, "easy"))
			continue;
		if (!strcmp(command, "post"))
		{
			post = 2;
			continue;
		}
		if (!strcmp(command, "nopost"))
		{
			post = 0;
			continue;
		}

		first_move[0] = 0;
		ply = 0;
		GenRoot(side, xside);

		m = ParseMove(line);
		if (m == -1 || !MakeMove(move_list[m].from, move_list[m].to, move_list[m].flags))
			printf("Error (unknown command): %s\n", command);
		else
		{
			ply = 0;
			DisplayResult();
		}
	}
}

static int LoadDiagram(string file_name)
{
	ifstream file(file_name);
	if (!file) {
		cerr << "File not found!" << endl;
		return 1;
	}

	string fen_text;

	getline(file, fen_text);

	int n = 0;

	int c = 0, i = 0, j;

	memset(pawn_mat, 0, sizeof(pawn_mat));
	memset(piece_mat, 0, sizeof(piece_mat));
	memset(bit_pieces, 0, sizeof(bit_pieces));
	memset(bit_units, 0, sizeof(bit_units));
	bit_all = 0;

	for (int x = 0; x < 64; x++)
	{
		b[x] = EMPTY;
	}
	NewPosition();

	while (c < fen_text.size())
	{
		if (fen_text[c] >= '0' && fen_text[c] <= '8')
			i += fen_text[c] - 48;
		if (fen_text[c] == '\\')
			continue;
		j = Flip[i];

		switch (fen_text[c])
		{
		case 'K': AddPiece(0, K, j); kingloc[0] = j; i++; break;
		case 'Q': AddPiece(0, Q, j); i++; break;
		case 'R': AddPiece(0, R, j); i++; break;
		case 'B': AddPiece(0, B, j); i++; break;
		case 'N': AddPiece(0, N, j); i++; break;
		case 'P': AddPiece(0, P, j); i++; break;
		case 'k': AddPiece(1, K, j); kingloc[1] = j; i++; break;
		case 'q': AddPiece(1, Q, j); i++; break;
		case 'r': AddPiece(1, R, j); i++; break;
		case 'b': AddPiece(1, B, j); i++; break;
		case 'n': AddPiece(1, N, j); i++; break;
		case 'p': AddPiece(1, P, j); i++; break;
		}
		c++;
		if (fen_text[c] == ' ')
			break;
		if (i > 63)
			break;
	}
	if (fen_text[c] == ' ' && fen_text[c + 2] == ' ')
	{
		if (fen_text[c + 1] == 'w')
		{
			side = 0; xside = 1;
		}
		if (fen_text[c + 1] == 'b')
		{
			side = 1; xside = 0;
		}
	}

	castle = 0;

	while (c < fen_text.size())
	{
		switch (fen_text[c])
		{
		case '-': break;
		case 'K':if (bit_pieces[White][K] & mask[E1]) castle |= CASTLE_WK;
			break;
		case 'Q':if (bit_pieces[White][K] & mask[E1]) castle |= CASTLE_WQ;
			break;
		case 'k':if (bit_pieces[Black][K] & mask[E8]) castle |= CASTLE_BK;
			break;
		case 'q':if (bit_pieces[Black][K] & mask[E8]) castle |= CASTLE_BQ;
			break;
		default:break;
		}
		c++;
	}

	currentkey = GetKey();
	hply = 8;

	DisplayBoard();
	if (side == 0)
		cout << "White to move" << endl;
	else
		cout << "Black to move" << endl;
	cout << fen_text << endl;
	return 0;
}

void ShowHelp()
{
	cout << "d - Displays the b." << endl;
	cout << "f - Flips the b." << endl;
	cout << "go - Starts the engine." << endl;
	cout << "help - Displays help on the commands." << endl;
	cout << "moves - Displays of list of possible moves." << endl;
	cout << "new - Starts a new game ." << endl;
	cout << "off - Turns the computer player off." << endl;
	cout << "on or p - The computer plays a move." << endl;
	cout << "sb - Loads a fen diagram." << endl;
	cout << "sd - Sets the Search depth." << endl;
	cout << "st - Sets the time limit per move in seconds." << endl;
	cout << "sw - Switches sides." << endl;
	cout << "t - test." << endl;
	cout << "quit - Quits the program." << endl;
	cout << "undo - Takes back the last move." << endl;
	cout << "xboard - Starts xboard." << endl;
}

void SetMaterial()
{
	int c;
	pawn_mat[0] = 0;
	pawn_mat[1] = 0;
	piece_mat[0] = 0;
	piece_mat[1] = 0;
	for (int x = 0; x < 64; x++)
	{
		if (b[x] != EMPTY)
		{
			if (bit_units[0] & mask[x])
				c = 0;
			else
				c = 1;
			if (b[x] == P)
				pawn_mat[c]++;
			else
				piece_mat[c] += piece_value[b[x]];
		}
	}
}

U64 GetTime()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>
		(
			steady_clock::now().time_since_epoch()).count();
}


string MoveString(int from, int to, int promote)
{
	string str = "";

	str += col[from] + 'a';
	str += row[from] + '1';
	str += col[to] + 'a';
	str += row[to] + '1';

	string promote_piece[] = { "-","n","b","r","q","q" };

	if (promote > 0)
	{
		str += promote_piece[promote];
	}
	return str;
}

void GenCheck()
{
	BITBOARD bit_disco_pieces;
	BITBOARD pins[2];

	pins[0] = PinnersPossible(0, 1);
	pins[1] = PinnersPossible(1, 0);
	BITBOARD pin_mask = GetPinMask(side, xside);
	BITBOARD bit_check_squares[6];
	memset(bit_check_squares, 0, sizeof(bit_check_squares));

	int check = Check(xside, kingloc[side]);
	if (check > -1)
	{
		EvadeCapture(side, xside, check, pin_mask);
		EvadeQuiet(side, xside, check, pin_mask);
	}
	else
	{
		GenCaptures(side, xside, pin_mask);
		bit_disco_pieces = GenChecks(side, xside, pin_mask);
		BuildAttackMap();
		GenQuietMoves(side, xside, pin_mask, bit_check_squares);
	}
}

