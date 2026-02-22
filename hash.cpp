#include "globals.h"

#include <cstdint>

using UCHAR = unsigned char;
using USHORT = unsigned short;

// 16-bit move: bits [0..5]=from, [6..11]=to, [12..15]=flags
typedef USHORT move16;

move16 pack_move(UCHAR from, UCHAR to, UCHAR flags);

static inline UCHAR mv_from(move16 m);
static inline UCHAR mv_to(move16 m);
static inline UCHAR mv_flags(move16 m);

static BITBOARD Random64();

void HashTest();

move_data hash_move;

void Free();
void FreeAllHash();
void Alg(int a, int b);
void SetBit(BITBOARD& bb, int square);

//Hash table with 2^16 =64000
BITBOARD hash[2][6][64];
BITBOARD lock[2][6][64];
BITBOARD ep_hash[64];

BITBOARD currentkey;
BITBOARD currentpawnkey, currentpawnlock;

BITBOARD pawnhash[2][64];
BITBOARD pawnlock[2][64];

BITBOARD clashes, bestmatches, collisions, pcollisions;

BITBOARD Random64(int x);
BITBOARD Random2(int size);

void AddBookHash(const int s, const int score, const int type, const move_data move);
int LookUpBook();

const int HASH_SIZE = 25;//23;
const BITBOARD MAXHASH = 1ULL << HASH_SIZE;          // GOOD
const BITBOARD HASHMASK = MAXHASH - 1;

const int PAWNHASH_SIZE = 18;                 // pick 18 or 19, your choice
const uint32_t MAXPAWNHASH = (1u << PAWNHASH_SIZE) - 1;

//const BITBOARD MAXPAWNHASH = 1ULL << PAWNHASH_SIZE;
//const BITBOARD PAWNMASK = MAXPAWNHASH - 1;

struct hashp
{
	BITBOARD hashlock;
	short score;
	unsigned char depth;
	unsigned char type;
	unsigned char from;
	unsigned char to;
	unsigned short flags;
};

hashp* hashpos[2];
BITBOARD hashpositions[2];

hashpawn hashpawns[MAXPAWNHASH];

void Store(hashp* ptr, int score, int depth, int type,
                  int from, int to, unsigned int flags);

BITBOARD castle_key[16], castle_lock[16];
BITBOARD ep_file_key[8], ep_file_lock[8];   // file 0..7

void InitKeys() {
	/*
    for (int r = 0; r < 16; ++r) {
        castle_key[r]  = Random64(HASH_SIZE);
        castle_lock[r] = Random64(64);
    }
    for (int f = 0; f < 8; ++f) {
        ep_file_key[f]  = Random64(HASH_SIZE);
        ep_file_lock[f] = Random64(64);
    }
	*/
}

void RandomizeHash()
{
	int p, x;
	for (p = 0; p < 6; p++)
		for (x = 0; x < 64; x++)
		{
			hash[0][p][x] = Random64(); 
			hash[1][p][x] = Random64(); 
			//lock[0][p][x] = Random2(HASH_SIZE);
			//lock[1][p][x] = Random2(HASH_SIZE);
		}

	for (x = 0; x < 64; x++)
	{
		pawnhash[0][x] = Random2(PAWNHASH_SIZE);
		pawnhash[1][x] = Random2(PAWNHASH_SIZE);
		pawnlock[0][x] = Random2(PAWNHASH_SIZE);
		pawnlock[1][x] = Random2(PAWNHASH_SIZE);
	}
	for (x = 0; x < 64; x++)
	{
		ep_hash[x] = Random64();
	}
	hashpos[0] = new hashp[MAXHASH];
	hashpos[1] = new hashp[MAXHASH];
	memset(hashpos[0], 0, MAXHASH * sizeof(hashp));
	memset(hashpos[1], 0, MAXHASH * sizeof(hashp));
}

void FreeAllHash()
{
memset(hashpos[0], 0, MAXHASH * sizeof(hashp));
memset(hashpos[1], 0, MAXHASH * sizeof(hashp));
memset(hashpawns,0,sizeof(hashpawns));
}
//*
BITBOARD Random2(int size)
{
	BITBOARD r = 0;
	for (int y = 0; y < size; y++)
	{
		if (rand() % 128 < 64)
			SetBit(r, y);
	}
	return r;
}
//*/

static BITBOARD Random64() {
	static uint64_t x = 0x9e3779b97f4a7c15ull;
	x += 0x9e3779b97f4a7c15ull;
	uint64_t z = x;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
	return z ^ (z >> 31);
}

void Free()
{
	delete[] hashpos[0];
	delete[] hashpos[1];
    hashpos[0] = hashpos[1] = 0;
}

int GetHashScore(const int s)
{
	return hashpos[s][currentkey].score;
}

void AddKey(const int s, const int p, const int sq)
{
	currentkey ^= hash[s][p][sq];
}

void AddKeys(const int s, const int p, const int x, const int y)
{
	currentkey ^= hash[s][p][x];
	currentkey ^= hash[s][p][y];
}

BITBOARD GetLock()
{
	BITBOARD loc = 0;
	for (int x = 0; x < 64; x++)
	{
		if (b[x] != EMPTY)
		{
			if (bit_units[0] & mask[x])
				loc ^= lock[0][b[x]][x];
			if (bit_units[1] & mask[x])
				loc ^= lock[1][b[x]][x];
		}
	}
	return loc;
}

BITBOARD GetKey()
{
	BITBOARD key = 0;
	for (int x = 0; x < 64; x++)
	{
		if (b[x] != EMPTY)
		{
			if (bit_units[0] & mask[x])
				key ^= hash[0][b[x]][x];
			if (bit_units[1] & mask[x])
				key ^= hash[1][b[x]][x];
		}
	}
	return key;
}

static inline uint32_t TT_Tag32(uint64_t key) {
	return (uint32_t)(key >> 32);               // upper 32 bits of the 64-bit key
}
static inline uint32_t TT_Read32(uint64_t lock) {
	return (uint32_t)(lock >> 32);              // tag stored in HIGH 32 bits
}
static inline uint64_t TT_Write32(uint32_t tag) {
	return (uint64_t)tag << 32;                 // pack into HIGH 32 bits
}

static inline size_t TTIndex(uint64_t key)
{
	// Mix bits so low bits are not structurally weak
	key ^= key >> 32;
	key ^= key >> 16;

	return (size_t)(key & HASHMASK);
}

void AddHash(const int s, int depth, int score, const int type,
	const int from, const int to, const unsigned int flags)
{
	if (score >= MATE_BOUND)       score += ply;
	else if (score <= -MATE_BOUND) score -= ply;

	if (depth < 0) depth = 0;

	const size_t index = TTIndex(currentkey);
	hashp* ptr = &hashpos[s][index];

	const U64 old_lock = ptr->hashlock;
	const bool occupied = (old_lock != 0);

	// Count collision only if slot already has a different full key
	if (occupied && old_lock != (uint64_t)currentkey)
		++collisions;

	const int oldDepth = (int)ptr->depth;

	if (occupied && depth < oldDepth && type != EXACT)
		return;

	ptr->hashlock = (U64)currentkey;   // FULL LOCK

	ptr->score = (short)score;
	ptr->depth = (unsigned char)depth;
	ptr->type = (unsigned char)type;
	ptr->from = (unsigned char)from;
	ptr->to = (unsigned char)to;
	ptr->flags = (unsigned short)flags;
}

int LookUp(const int s, const int depth, const int alpha, const int beta)
{
	const size_t index = TTIndex(currentkey);
	const hashp* ptr = &hashpos[s][index];

	const U64 lock = (U64)ptr->hashlock;
	if (!lock) return -1;

	if (lock != (U64)currentkey) return -1;

	// Always expose TT move for ordering
	hash_move.from = ptr->from;
	hash_move.to = ptr->to;
	hash_move.flags = ptr->flags;

	if ((int)ptr->depth < depth) return 0;

	int score = (int)ptr->score;
	if (score >= MATE_BOUND)       score -= ply;
	else if (score <= -MATE_BOUND) score += ply;

	switch (ptr->type) {
	case EXACT:
		hash_move.score = score;
		return EXACT;
	case BETA:
		return (score >= beta) ? BETA : 0;
	case ALPHA:
		return (score <= alpha) ? ALPHA : 0;
	default:
		return 0;
	}
}

int LookUp2(const int s)
{
	const size_t index = TTIndex((uint64_t)currentkey);
	const hashp* ptr = &hashpos[s][index];

	const uint64_t lock = (uint64_t)ptr->hashlock;
	if (!lock || lock != (uint64_t)currentkey) return -1;

	hash_move.from = ptr->from;
	hash_move.to = ptr->to;
	hash_move.flags = ptr->flags;
	return 0;
}

BITBOARD GetPawnKey()
{
	int colour = 0;
	BITBOARD key = 0;
	for (int x = 0; x < 64; x++)
	{
		if (b[x] == P)
		{
			if (bit_units[0] & mask[x]) 
				colour = 0;
			else 
				colour = 1;
			key ^= pawnhash[colour][x];
		}
	}
	return key;
}

BITBOARD GetPawnLock()
{
	int colour = 0;
	BITBOARD key = 0;
	for (int x = 0; x < 64; x++)
	{
		if (b[x] == P)
		{
			if (bit_units[0] & mask[x]) 
				colour = 0;
			else 
				colour = 1;
			key ^= pawnlock[colour][x];
		}
	}
	return key;
}
//*
void AddPawnHash(const int s1, const int s2, const BITBOARD p1, const BITBOARD p2)
{
	hashpawns[currentpawnkey].score[0] = s1;
	hashpawns[currentpawnkey].score[1] = s2;
	hashpawns[currentpawnkey].hashlock = currentpawnlock;

	hashpawns[currentpawnkey].passed_pawns[0] = p1;
	hashpawns[currentpawnkey].passed_pawns[1] = p2;
	return;
}
//*/
/*
void AddPawnHash(const int s1, const int s2, const BITBOARD p1, const BITBOARD p2)
{
	const size_t index = currentpawnkey & PAWNMASK;
	const unsigned short tag = currentpawnkey >> 48;
	hashpawns[index].hashlock = (BITBOARD)tag << 48;
	hashpawns[index].score[0] = s1;
	hashpawns[index].score[1] = s2;

	hashpawns[index].passed_pawns[0] = p1;
	hashpawns[index].passed_pawns[1] = p2;
	return;
}
//*/

void AddPawnKey(const int s, const int x)
{
	currentpawnkey ^= pawnhash[s][x];
	currentpawnlock ^= pawnlock[s][x];
}

void AddPawnKeys(const int s, const int x, const int y)
{
	currentpawnkey ^= pawnhash[s][x];
	currentpawnkey ^= pawnhash[s][y];
	currentpawnlock ^= pawnlock[s][x];
	currentpawnlock ^= pawnlock[s][y];
}
/*
BITBOARD FullKey() {
    BITBOARD key = 0;
    for (int x = 0; x < 64; ++x) if (b[x] != EMPTY) {
        int col = (bit_units[0] & mask[x]) ? 0 : 1;
        key ^= hash[col][b[x]][x];
    }
    key ^= castle_key[castle];
    if (ep_file >= 0) key ^= ep_file_key[ep_file];
    return key;
}

BITBOARD FullLock() {
    BITBOARD loc = 0;
    for (int x = 0; x < 64; ++x) if (b[x] != EMPTY) {
        int col = (bit_units[0] & mask[x]) ? 0 : 1;
        loc ^= lock[col][b[x]][x];
    }
    loc ^= castle_lock[castle];
    if (ep_file >= 0) loc ^= ep_file_lock[ep_file];
    return loc;
}
*/
hashpawn& LookUpPawn()
{
	return hashpawns[currentpawnkey];
}

bool PawnHashHit(const hashpawn& e)
{
	return e.hashlock == currentpawnlock;
}

move_data GetHashMove()
{
	return hash_move;
}

void HashTest()
{
	//*
if(//hashpos[1][6495712].from == D5 && hashpos[1][6495712].to==C4 ||
hashpos[1][5690949].from == E8 && hashpos[1][2610156].to==D8 ||
hashpos[1][6495712].from == D5 && hashpos[1][6495712].to==C4
)
{
	Alg(hashpos[1][396057].from,hashpos[1][1394300].to);
	z();
	//ShowAll(ply);
	nodes=nodes;
}
//*/
}

// pack
static inline move16 pack_move(UCHAR from, UCHAR to, UCHAR flags) {
	return (move16)((from & 63) | ((to & 63) << 6) | ((flags & 15) << 12));
}

// unpack
static inline UCHAR mv_from(move16 m) { return  (UCHAR)(m & 63); }
static inline UCHAR mv_to(move16 m) { return  (UCHAR)((m >> 6) & 63); }
static inline UCHAR mv_flags(move16 m) { return  (UCHAR)((m >> 12) & 15); }
