#include "globals.h"

#include <cstdint>

using UCHAR = unsigned char;
using USHORT = unsigned short;

static BITBOARD Random64();

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
BITBOARD clashes, bestmatches, collisions, pcollisions;

BITBOARD pawnhash[2][64];
BITBOARD pawnlock[2][64];

BITBOARD Random2(int size);

const int HASH_SIZE = 25;
const BITBOARD MAXHASH = 1ULL << HASH_SIZE;          
const BITBOARD HASHMASK = MAXHASH - 1;

const int PAWNHASH_SIZE = 18;                 
const uint32_t MAXPAWNHASH = (1u << PAWNHASH_SIZE) - 1;

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

void RandomizeHash()
{
	for (int piece = 0; piece < 6; piece++)
		for (int x = 0; x < 64; x++)
		{
			//hash[0][piece][x] = Random2(25); //64
			//hash[1][piece][x] = Random2(25); //64
			hash[0][piece][x] = Random64(); //64
			hash[1][piece][x] = Random64(); //64
		}

	for (int x = 0; x < 64; x++)
	{
		pawnhash[0][x] = Random2(PAWNHASH_SIZE);
		pawnhash[1][x] = Random2(PAWNHASH_SIZE);
		pawnlock[0][x] = Random2(PAWNHASH_SIZE);
		pawnlock[1][x] = Random2(PAWNHASH_SIZE);
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

void AddKey(const int s, const int piece, const int sq)
{
	currentkey ^= hash[s][piece][sq];
}

void AddKeys(const int s, const int piece, const int from, const int to)
{
	currentkey ^= hash[s][piece][from];
	currentkey ^= hash[s][piece][to];
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

static inline size_t HashIndex(uint64_t key)
{
	key ^= key >> 32;
	key ^= key >> 16;

	return (size_t)(key & HASHMASK);
}

void AddHash(const int s, int depth, int score, const int type,
	const int from, const int to, const unsigned int flags)
{
	if (score >= MATE_BOUND) 
		score += ply;
	else 
		if (score <= -MATE_BOUND) 
			score -= ply;

	if (depth < 0) 
		depth = 0;

	const size_t index = HashIndex(currentkey);

	hashp* ptr = &hashpos[s][index];

	const U64 old_lock = ptr->hashlock;
	const bool occupied = (old_lock != 0);

	if (occupied && old_lock != (uint64_t)currentkey)
		++collisions;

	const int oldDepth = (int)ptr->depth;

	if (occupied && depth < oldDepth && type != EXACT)
		return;

	ptr->hashlock = (U64)currentkey;   

	ptr->score = (short)score;
	ptr->depth = (unsigned char)depth;
	ptr->type = (unsigned char)type;
	ptr->from = (unsigned char)from;
	ptr->to = (unsigned char)to;
	ptr->flags = (unsigned short)flags;
}

int LookUp(const int s, const int depth, const int alpha, const int beta)
{
	const size_t index = HashIndex(currentkey);
	const hashp* ptr = &hashpos[s][index];

	const U64 lock = (U64)ptr->hashlock;
	if (!lock) return -1;

	if (lock != (U64)currentkey) return -1;

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

bool LookUp2(const int s)
{
	const size_t index = HashIndex((uint64_t)currentkey);
	const hashp* ptr = &hashpos[s][index];

	const uint64_t lock = (uint64_t)ptr->hashlock;
	if (!lock || lock != (uint64_t)currentkey) return false;

	hash_move.from = ptr->from;
	hash_move.to = ptr->to;
	hash_move.flags = ptr->flags;
	return true;
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

