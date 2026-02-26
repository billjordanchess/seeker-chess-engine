#define _CRT_SECURE_NO_WARNINGS

//UCI loop(single - threaded, minimal, fits your engine)
// uci_min.cpp  (single-threaded, minimal UCI)
// Think() returns a move; MoveString() outputs "e2e4"

#include <iostream>      // std::cin, std::cout
#include <string>        // std::string
#include <sstream>       // std::istringstream
#include <atomic>        // std::atomic (if using stop flag)
#include <cstdint>
#include <sstream>
#include <string>
#include <iostream>

// Adjust to taste
static const int DEFAULT_MAX_DEPTH = 64;
static const int DEFAULT_FALLBACK_MS = 100;   // if no clocks provided
static const int RESERVE_MS = 20;             // jitter/overhead safety

static void handle_go(const std::string& line);

struct GoParams
{
    int wtime = -1, btime = -1;   // ms
    int winc = 0, binc = 0;    // ms
    int movestogo = -1;

    int movetime = -1;           // ms (hard limit)
    int depth = -1;
    int nodes = -1;

    bool infinite = false;
    bool ponder = false;
};

//using namespace std;

#include "globals.h"

// Your engine globals/functions (adapt names if needed)
extern int side;       // 0 white, 1 black
extern int xside;
extern int ply;
extern int fixed_time;
extern int max_depth;
extern int fixed_depth;

extern move_data engine_move;

#ifndef MAX_PLY
#define MAX_PLY 64
#endif

void StartGame();
void GenCheck();
void DisplayResult();

static void ProcessMove(int from, int to, int promo);

std::string MoveString(int, int, int);

// --------- tokeniser (in-place) ----------
static inline int split_ws(char* line, char* tok[], int maxTok)
{
    int n = 0;
    char* p = line;
    while (*p && n < maxTok)
    {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        tok[n++] = p;

        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p) *p++ = '\0';
    }
    return n;
}

// --------- UCI square to 0..63 (a1=0) ----------
static inline int sq_from_uci2(const char* s)
{
    char f = (char)tolower((unsigned char)s[0]);
    char r = s[1];
    if (f < 'a' || f > 'h') return -1;
    if (r < '1' || r > '8') return -1;
    int file = f - 'a';
    int rank = r - '1';
    return rank * 8 + file;
}

// --------- parse "e2e4" or "e7e8q" ----------
static inline int parse_uci_move(const char* m, int& from, int& to, int& promo)
{
    if (!m || (int)std::strlen(m) < 4) return 0;
    from = sq_from_uci2(m);
    to = sq_from_uci2(m + 2);
    promo = 0;
    if (from < 0 || to < 0) return 0;

    if ((int)std::strlen(m) >= 5)
    {
        // If you later add promotion support, map this to your internal promo code.
        // For now you can ignore or store the char.
        char p = (char)tolower((unsigned char)m[4]);
        if (p == 'q' || p == 'r' || p == 'b' || p == 'n') promo = p;
    }
    return 1;
}

// --------- minimal time allocator ----------
static int compute_time_ms(int wtime, int btime, int winc, int binc, int movestogo, int movetime, int infinite)
{
    if (infinite) return 1 << 30;    // effectively "a lot"
    if (movetime >= 0) return movetime;

    int tleft = (side == 0) ? wtime : btime;
    int inc = (side == 0) ? winc : binc;
    if (tleft < 0) tleft = 1000;

    int mtg = (movestogo > 0) ? movestogo : 30;

    // safety buffer
    tleft -= 50;
    if (tleft < 0) tleft = 0;

    int t = tleft / (mtg + 2) + inc / 2;
    if (t < 10) t = 10;
    if (t > tleft) t = tleft;
    return t;
}

static void handle_position(const std::string& line)
{
    std::istringstream iss(line);
    std::string token;

    iss >> token; // "position"

    iss >> token;

    if (token == "startpos")
    {
        StartGame();
    }
    else if (token == "fen")
    {
        std::string fen, part;
        for (int i = 0; i < 6 && iss >> part; i++)
        {
            fen += part + " ";
        }
        //SetFEN(fen);
    }

    // moves
    if (iss >> token && token == "moves")
    {
        std::string move;
        while (iss >> move)
        {
            int from, to, promo;
            if (!parse_uci_move(move.c_str(), from, to, promo))
                continue;

            ProcessMove(from, to, promo);
        }
    }
}

static void ProcessMove(int from, int to, int promo)
{
    int flags = 0;
    if (b[from]==K && abs(from-to) == 2)
        flags |= CASTLE;
    if(promo > 0)
        flags |= PROMOTE;
    if (b[from] == P && b[to] == EMPTY && col[from] != col[to])
        flags |= EP;

    MakeMove(from, to, flags);
}

static int compute_time_limit_ms(const GoParams& gp, int side /*0=white,1=black*/)
{
    if (gp.infinite || gp.ponder)
        return -1; // no limit, wait for stop

    if (gp.movetime > 0)
        return gp.movetime;

    // If clocks are provided:
    const int t = (side == 0 ? gp.wtime : gp.btime);
    const int inc = (side == 0 ? gp.winc : gp.binc);

    if (t > 0)
    {
        // crude but safe:
        // assume ~30 moves remaining if not given
        int mtg = gp.movestogo > 0 ? gp.movestogo : 30;

        // allocate: time/mtg + 80% of increment
        int slice = t / mtg + (inc * 8) / 10;

        // safety clamps
        if (slice < 10) slice = 10;                 // never less than 10ms
        int maxSlice = t / 2;                       // never spend more than half remaining time
        if (slice > maxSlice) slice = maxSlice;

        // keep a small reserve (network/GUI lag etc.)
        int reserve = 20;
        if (slice > reserve) slice -= reserve;

        return slice;
    }

    // No clocks and no movetime/infinite: pick something conservative
    return 100; // 0.1s default
}

void uci()
{
    StartGame();
    fixed_time = 0;

    std::string line;

    while (std::getline(std::cin, line))
    {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci")
        {
            std::cout << "id name Seeker\n";
            std::cout << "id author Bill Jordan\n";
            std::cout << "uciok\n";
        }
        else if (cmd == "isready")
        {
            std::cout << "readyok\n";
        }
        else if (cmd == "ucinewgame")
        {
            StartGame();
        }
        else if (cmd == "position")
        {
            handle_position(line);
        }
        else if (cmd == "go")
        {
            handle_go(line);
        }
        else if (cmd == "stop")
        {
            stop_search = true;  // global atomic flag
        }
        else if (cmd == "quit")
        {
            break;
        }
    }
}

static int compute_slice_ms(int myTime, int myInc, int movestogo)
{
    if (myTime <= 0) return DEFAULT_FALLBACK_MS;

    int mtg = (movestogo > 0 ? movestogo : 30);

    // basic safe allocation: time/mtg + 80% increment
    int slice = myTime / mtg + (myInc * 8) / 10;

    // clamps
    if (slice < 10) slice = 10;
    int maxSlice = myTime / 2;
    if (slice > maxSlice) slice = maxSlice;

    // reserve
    if (slice > RESERVE_MS) slice -= RESERVE_MS;

    return slice;
}

static void handle_go(const std::string& line)
{
    // Parse
    std::istringstream iss(line);
    std::string tok;

    int wtime = -1, btime = -1, winc = 0, binc = 0, movestogo = -1;
    int movetime = -1;
    int depth = -1;
    long long nodesLimit = -1;   // optional if you implement it
    bool infinite = false;
    bool ponder = false;

    iss >> tok; // "go"
    while (iss >> tok)
    {
        if (tok == "wtime") iss >> wtime;
        else if (tok == "btime") iss >> btime;
        else if (tok == "winc") iss >> winc;
        else if (tok == "binc") iss >> binc;
        else if (tok == "movestogo") iss >> movestogo;
        else if (tok == "movetime") iss >> movetime;
        else if (tok == "depth") iss >> depth;
        else if (tok == "nodes") iss >> nodesLimit;
        else if (tok == "infinite") infinite = true;
        else if (tok == "ponder") ponder = true;
        // ignore other tokens safely
    }

    // Decide limits
    int fixed_time = 0;              // ms for this move; 0 means "no time limit" (SetTime should handle)
    int max_depth = DEFAULT_MAX_DEPTH;

    // Optional: nodes-based stop (only if you support it)
    // max_nodes = nodesLimit; // store globally if you implement checks in Search()

    if (infinite || ponder)
    {
        fixed_time = 0; // SetTime(0) should set stop_time far away
        max_depth = DEFAULT_MAX_DEPTH;
    }
    else if (movetime > 0)
    {
        fixed_time = movetime;
        max_depth = DEFAULT_MAX_DEPTH;
    }
    else if (depth > 0)
    {
        max_depth = depth;
        fixed_time = 0; // let depth control; SetTime(0) => far away
    }
    else
    {
        // Clock mode
        int myTime = (side == 0 ? wtime : btime);
        int myInc = (side == 0 ? winc : binc);

        fixed_time = compute_slice_ms(myTime, myInc, movestogo);
        max_depth = DEFAULT_MAX_DEPTH;
    }

    // Start search
    stop_search = false;
    move_data best = Think(fixed_time, max_depth);

    engine_move = Think(0, max_depth);

    int prom = 0;
    if (engine_move.flags & PROMOTE)
        prom = Q;

    std::cout << "bestmove "
        << MoveString(engine_move.from, engine_move.to, prom)
        << "\n";
}
