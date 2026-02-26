
#include <memory.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dos.h>
#include <time.h>
#include <assert.h>
#include <string>
#include <fstream>
#include <string>

constexpr int CASTLE_WK = 1;
constexpr int CASTLE_WQ = 2;
constexpr int CASTLE_BK = 4;
constexpr int CASTLE_BQ = 8;

extern unsigned int CAPTURE;
extern unsigned int INCHECK;
extern unsigned int ATTACK;
extern unsigned int CASTLE;
extern unsigned int QUIET;
extern unsigned int KILLER;
extern unsigned int PASSED6;
extern unsigned int PASSED7;
extern unsigned int PROMOTE;
extern unsigned int CHECK;
extern unsigned int DEFEND;
extern unsigned int PAWNMOVE;
extern unsigned int COUNTER;
extern unsigned int EP;
extern unsigned int MATETHREAT;
extern unsigned int DISCO;

enum {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8
};

constexpr int DEBUG = 1;
constexpr int NULLMOVE = 1;

using BITBOARD = unsigned __int64;
using U64 = unsigned __int64;

constexpr int BISHOP_PAIR = 25;

constexpr int KINGSIDE = 0;
constexpr int QUEENSIDE = 1;
constexpr int KINGOPPOSITE = 3;
constexpr int QUEENOPPOSITE = 4;

constexpr int PAWNLESS = 1;
constexpr int PAWN_ENDING = 2;
constexpr int QUEEN_ENDING = 3;
constexpr int ONE_PAWN = 3;
constexpr int OPPOSITE = 5;
constexpr int NORMAL = 6;

constexpr int HASH_SCORE = 2500000;
constexpr int MATES_SCORE = 1500000;
constexpr int PROMOTE_SCORE = 1000350;
constexpr int CAPTURE_SCORE = 1000000;
constexpr int CHECK_SCORE = 900000;

constexpr int ESCAPE_SCORE = 120000;
constexpr int DEFEND_SCORE = 100000;
constexpr int ATTACK_SCORE = 90000;//80000
constexpr int COUNTER_SCORE = 60000;
constexpr int CONT_SCORE = 80000;
constexpr int KILLER1_SCORE = 80000;
constexpr int KILLER2_SCORE = 40000;
constexpr int EN_PRISE_SCORE = 10000;

constexpr int BLUNDER_SCORE = 10000;

constexpr int HISTORY_LIMIT = 800000;

// Quiet-only bonuses (all << BLUNDER_BUCKET)
constexpr int QUIET_ESCAPE_BONUS = 120000;   // highest quiet bonus
constexpr int QUIET_KILLER1_BONUS = 80000;
constexpr int QUIET_KILLER2_BONUS = 60000;
constexpr int QUIET_COUNTER_BONUS = 40000;

constexpr int MATE_BOUND = 9900;

constexpr int UPPER = 3;
constexpr int LOWER = 1;

constexpr int DOUBLE_CHECK = 100;

constexpr int NOTHING = 0;
constexpr int BETA = 1;
constexpr int EXACT = 2;
constexpr int ALPHA = 3;

constexpr int GEN_STACK = 2400;//2000//1120 16/11/12
constexpr int MAX_PLY = 64;//48//10/4/14 was 40
constexpr int HIST_STACK = 400;

constexpr int LIGHT = 0;
constexpr int DARK = 1;

constexpr int PAWN = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK = 3;
constexpr int QUEEN = 4;
constexpr int KING = 5;

//constexpr int ALL			5

constexpr int P = 0;
constexpr int N = 1;
constexpr int B = 2;
constexpr int R = 3;
constexpr int Q = 4;
constexpr int K = 5;

constexpr int EMPTY = 6;

constexpr int NORTH = 0;
constexpr int NE = 1;
constexpr int EAST = 2;
constexpr int SE = 3;
constexpr int SOUTH = 4;
constexpr int SW = 5;
constexpr int WEST = 6;
constexpr int NW = 7;

constexpr int DRAWN = 77;

constexpr int  WON_ENDGAME = 9900;
constexpr int  LOST_ENDGAME = -9900;

extern int fixed_depth;

extern int list[2][8];
extern int table_score[2];
extern int kingside[2];
extern int queenside[2];
extern int kingattack[2];
extern int queenattack[2];
extern int KingSide[2][64];
extern int QueenSide[2][64];
extern int KingSide2[2][64];
extern int QueenSide2[2][64];
extern int king_side2[64];
extern int queen_side2[64];

extern int index[64];

extern BITBOARD bit_e1h1;
extern BITBOARD bit_e1a1;
extern BITBOARD bit_e8h8;
extern BITBOARD bit_e8a8;

extern BITBOARD bit_attacked[2][6];
extern BITBOARD bit_weaker[2][6];
extern BITBOARD bit_line_attackers[MAX_PLY];
extern BITBOARD bit_undefended[2];
extern BITBOARD bit_undefended_squares[2];
extern BITBOARD bit_total_attacks[2];

extern int king_attackers[MAX_PLY][2];
extern BITBOARD bit_adjacent_king[MAX_PLY][2];

typedef  struct {
	BITBOARD bit_attacked[2][6];
	BITBOARD bit_weaker[2][6];
	BITBOARD bit_line_attackers;
	BITBOARD bit_undefended[2];
	BITBOARD bit_undefended_squares;
}am;

typedef struct
{
	int piece;
	int from;
	int to;
	int score;
	unsigned int flags;
} move_data;

typedef struct
{
	int piece;
	int from;
	int to;
	unsigned int flags;
	int capture;
	int castle;
	int fifty;
	int streak;
	BITBOARD hash;
	BITBOARD lock;
} game;

typedef struct
{
	int from;
	int to;
	unsigned int flags;
}lookup_move;

extern move_data hash_move;

extern move_data move_list[GEN_STACK];

typedef struct
{
	int first;
	int last;
	int next;
} link;

typedef struct
{
	int sq;
	int next;
}list1;

struct hashpawn
{
	BITBOARD hashlock;
	int score[2];
	int defence[2][2];
	BITBOARD passed_pawns[2];
};

/* Gen.cpp */
void GenPromote(const int, const int, const int from, const int to);
bool MakeMove(const int from, const int to, const int flags);
void UnMakeMove();
void GenCaptures(const int, const int, BITBOARD);

/* Search.cpp */
move_data Think(int, int);
int QuietSearch(int alpha, int beta);
int Reps();
int Reps2();

/* eval.cpp */
int Eval(const int s, const int xs, const int alpha, const int beta);

/* main.cpp */
U64 GetTime();
void DisplayBoard();

extern int adjfile[64][64];
extern int kingqueen[64][64];
extern int kingknight[64][64];
extern int kingking[64][64];

extern BITBOARD bishop_a7[2];
extern BITBOARD bishop_h7[2];
extern BITBOARD knight_a7[2];
extern BITBOARD knight_h7[2];

extern BITBOARD bit_adjacent[64];

extern BITBOARD passed_list[2];

extern int side;
extern int xside;
extern int castle;
extern int fifty;
extern int ply;
extern int hply;
extern move_data move_list[GEN_STACK];
extern int first_move[MAX_PLY];

extern game game_list[HIST_STACK];
extern U64 max_time;
extern U64 start_time;
extern U64 stop_time;
extern U64 nodes;

extern int max_depth;
extern BITBOARD qnodes;
extern BITBOARD all_nodes;
extern BITBOARD cut_nodes;
extern BITBOARD cut_tt_nodes;
extern BITBOARD first_nodes;
extern BITBOARD first_tt_nodes;
extern int av_nodes;
extern int a_nodes;
extern int b_nodes;

extern int hash_piece[2][6][64];
extern int hash_side;
extern int hash_ep[64];
extern int castle_mask[64];
//extern char piece_char[7];
extern int startmat[2];

extern BITBOARD currentkey;
extern BITBOARD currentpawnkey, currentpawnlock;

//bitboard.cpp
extern BITBOARD mask_squarepawn[2][64];
extern BITBOARD mask_squareking[2][64];
extern BITBOARD mask_edge;
extern BITBOARD mask_corner;

extern int difference[64][64];
extern int pawn_difference[64][64];

extern BITBOARD mask_squarepawn[2][64];
extern BITBOARD bit_between[64][64];

//legal moves from each square
extern BITBOARD bit_pawncaptures[2][64];
extern BITBOARD bit_pawndefends[2][64];
extern BITBOARD bit_left[2][64];
extern BITBOARD bit_right[2][64];
extern BITBOARD bit_knightmoves[64];
extern BITBOARD bit_bishopmoves[64];
extern BITBOARD bit_rookmoves[64];
extern BITBOARD bit_queenmoves[64];
extern BITBOARD bit_kingmoves[64];

extern BITBOARD bit_moves[6][64];
//current position

extern BITBOARD bit_pieces[2][7];
extern BITBOARD bit_units[2];
extern BITBOARD bit_all;

extern BITBOARD bit_color[2];
extern BITBOARD mask_centre;

//current Attacks
extern BITBOARD bit_leftcaptures[2];
extern BITBOARD bit_rightcaptures[2];
extern BITBOARD bit_pawnattacks[2];
extern BITBOARD bit_kingattacks[2];
extern BITBOARD bit_attacks[2];

extern BITBOARD mask_passed[2][64];
extern BITBOARD mask_path[2][64];
extern BITBOARD mask_backward[2][64];
extern BITBOARD mask_ranks[2][8];
extern BITBOARD mask_files[8];
extern BITBOARD mask_rookfiles;
extern BITBOARD mask_cols[64];

extern BITBOARD mask_isolated[64];
extern BITBOARD mask_left_col[64];
extern BITBOARD mask_right_col[64];
extern BITBOARD mask_nwdiag[64];
extern BITBOARD mask_nediag[64];

extern BITBOARD bit_colors;

extern BITBOARD mask_abc;
extern BITBOARD mask_def;

extern BITBOARD mask_kingpawns[2];
extern BITBOARD mask_queenpawns[2];

extern BITBOARD mask[64];
extern BITBOARD not_mask[64];
extern BITBOARD not_mask_rookfiles;
extern BITBOARD not_mask_edge;
extern BITBOARD not_mask_corner;
extern BITBOARD not_mask_files[8];
extern BITBOARD not_mask_rows[8];

extern BITBOARD not_a_file;
extern BITBOARD not_h_file;
extern BITBOARD not_rank6;
extern BITBOARD not_rank1;
extern BITBOARD mask_wide_centre;

extern int linemoves[64][8];
extern int knightmoves[64][8];
extern int kingmoves[64][8];
extern int kingloc[2];

extern int piece_mat[2];
extern int pawn_mat[2];

extern int captures[MAX_PLY];
extern int extend[MAX_PLY];
extern int Threat[MAX_PLY];
extern int InCheck[MAX_PLY];

extern int color[64];
extern int b[64];

extern int ply;
extern int currentmax;

extern int KingPawn[2][64];
extern int pawn_score[64];
extern int knight_score[64];
extern int bishop_score[64];
extern int rook_score[64];
extern int queen_score[64];
extern int king_score[64];
extern int king_endgame_score[64];
extern int Flip[64];

//global variables
extern const int colors[64];

extern const int col[64];
extern const int row[64];
extern const int nwdiag[64];
extern const int nediag[64];

extern int row2[2][64];
extern int lastsquare[2][64];

extern int turn;

extern int PieceScore[2][6][64];
extern int KingScore[2][64];

extern int KingPawnLess[64];

extern int pieces[2][6][10];
extern int total[2][6];

extern int table_score[2];

extern int piece_value[6];
extern int done[1000];

extern int passed[2][64];
extern int adjacent_passed[2][64];
extern int defended_passed[2][64];

extern int isolated[64];
extern int pawn_blocked[64];

extern int PawnBlocked[2][64];

extern int pawnleft[2][64];
extern int pawnright[2][64];
extern int pawnplus[2][64];
extern int pawndouble[2][64];

extern int difference[64][64];
extern int squares[2][64];
extern int start[64];
extern int end[64];

extern int castle_start[64];
extern int castle_dest[64];

extern int knight_total[64];
extern int king_total[64];

extern int check_history[6][64];
extern int hist_from[2][6][64];
extern int hist_to[2][6][64];

void SetFromTo();

//functions
void PrintBitBoard(BITBOARD bb);
int NextBit(BITBOARD bb);

//hash.cpp
void RandomizeHash();

BITBOARD GetKey();
BITBOARD GetLock();
BITBOARD GetPawnKey();
BITBOARD GetPawnLock();
void AddKey(const int, const int, const int);
void AddKeys(const int s, const int p, const int x, const int y);
void AddHash(const int, const int, const int, const int, const int, const int, const unsigned int);

void AddPawnHash(const int s1, const int s2, const BITBOARD, const BITBOARD);
hashpawn& LookUpPawn();
bool PawnHashHit(const hashpawn& e);

void AddPawnKeys(const int s, const int x, const int y);

void SetRanks();
void SetBits();
void SetScores();
void SetPassed();
void SetKingPawnTable();

void Alg(int a, int b);
void Algebraic(int a);
void Alg1(int a);
void Alg2(int a, int b);

int Debug(const int);

int NextBit(BITBOARD x);

void Alg(int a, int b);
void Algebraic(int a);

void ShowAll2();
void ShowAll(int);

void SetPassed();

//bitboard.cpp
void z();

//int BitsGreaterThanOne(BITBOARD x);

int CountBits(BITBOARD b1);

void GenQuietCaptures(const int s, const int xs, const int diff, BITBOARD pin_mask, BITBOARD bit_xpinned);
void GenQuietMoves(const int s, const int xs, const BITBOARD not_captured, BITBOARD pin_mask, const BITBOARD* bit_cs);

bool Attack(const int s, const int sq);
bool CheckAttack(const int s, const int sq);
bool LineAttack(const int s, const int sq);

void UpdatePawn(const int s, const int from, const int to);
void UpdatePiece(const int s, const int p, const int from, const int to);
void RemovePiece(const int s, const int p, const int sq);
void AddPiece(const int s, const int p, const int sq);

void NewPosition();

void SetKingPawnTable();

int LookUp2(const int s);

int LookUp(const int side, const int depth, const int alpha, const int beta);

bool MakeCapture(const int, const int, const int);
void UnMakeCapture();

void MakeRecapture(const int, const int);
void UnMakeRecapture();

int BlockedPawns(const int s, const int x);
int SafeKingMoves(const int, const int);

bool BestThreat(const int s, const int xs, const int diff);

extern int scale[200];
extern int h_check[64][64];
extern int v_check[64][64];
extern int left_check[64][64];
extern int right_check[64][64];
extern int q_check[64][64][13];

extern int stats_depth[20];
extern int stats_count[100];
extern int stats_killers[2];
extern int total_killers[2];

extern int PlyMove[MAX_PLY];
extern int PlyType[MAX_PLY];

extern int endmatrix[10][3][10][3];
extern int king_zone[2][64];

void SetUp();

extern BITBOARD ep_hash[64];

void ShowAllEval(int ply);

int EvalPawnless(const int, const int);

int Check(const int s, const int sq);

constexpr int P_VALUE = 100;
constexpr int N_VALUE = 200;
constexpr int B_VALUE = 300;
constexpr int R_VALUE = 500;
constexpr int Q_VALUE = 900;
constexpr int BB_VALUE = 600;

int GetLowestAttacker(const int s, const int sq);
int GetNextAttackerSquare(const int s, const int, const int sq, const BITBOARD);
int SEE(int s, const int a, const int sq, const BITBOARD p1, const BITBOARD p2);

void HashTest();

void EvadeQuiet(const int s, const int xs, const int checker, BITBOARD);
void EvadeCapture(const int s, const int xs, const int checker, BITBOARD);
BITBOARD GetPinMask(const int s, const int xs);

bool MakeQuietMove(const int from, const int to, const int flags);
void UnMakeQuietMove();

extern const int px[6];
extern const int nx[6];
extern const int bx[6];
extern const int rx[6];
extern const int qx[6];
extern const int kx[6];

extern BITBOARD bit_attacked[2][6];
extern BITBOARD bit_defend_to[2][6];

extern BITBOARD bit_after[64][64];
extern int target_list[MAX_PLY][15];
extern int targets[MAX_PLY];
extern BITBOARD bit_targets[MAX_PLY];

extern BITBOARD bit_defended[2][MAX_PLY];

extern int test_mode;

int GetLowestLineAttacker(const int s, const int sq);
bool Attack2(const int s, const int sq, const BITBOARD occ, const BITBOARD);
bool KingLessAttack(const int s, const int sq);

void PlayOpening(int x);

bool RookAttack(const int s, const int from, const int to);

bool SameDiag(const int a, const int b, const int c);
bool SameLine(const int a, const int b, const int c);

extern BITBOARD bit_high[64];
extern BITBOARD bit_low[64];

extern BITBOARD bit_rookattacks[2][8];
extern BITBOARD bit_queenattacks[2][8];

bool IsOneBit(BITBOARD x);
int NextHighBit(BITBOARD bb);

//magics
U64 BishopAttacks(int sq, U64 occ);
U64 RookAttacks(int sq, U64 occ);
U64 QueenAttacks(int sq, U64 occ);

bool IsCheck(const int p, const int sq, const int king);

extern bool stop_search;



















