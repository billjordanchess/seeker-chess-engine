// gen_magics.cpp — Generate rook/bishop magic numbers (VS2019 friendly, no C++20).
// Build: x64, /O2 recommended. Prints rookMagics[64], bishopMagics[64], RBits[64], BBits[64].
// Square mapping assumed: a1=0, b1=1, ..., h8=63 (little-endian ranks).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <limits>
#include <algorithm>

#include "globals.h"

//typedef unsigned long long U64;

void TestQueenAttacks();

const U64 RMagic[64] = {
0x0080009125400181ULL, 0x60c0400020005000ULL, 0x0880100208200080ULL, 0x0100040820100102ULL,
0x0200080410200200ULL, 0x0300110008060400ULL, 0x0200020008c31804ULL, 0x0600140100402082ULL,
0x1005002048800104ULL, 0x0001402010004004ULL, 0x1003001301200040ULL, 0x0301002010010008ULL,
0x6003000801023014ULL, 0x0122000410090200ULL, 0x0001000100020004ULL, 0x10020004449a0904ULL,
0x0040008000804020ULL, 0x0490084020004001ULL, 0x0001010010200043ULL, 0x20020200100c4020ULL,
0x0004008004800800ULL, 0x407e008002800400ULL, 0x0402040010186142ULL, 0x0000020014005081ULL,
0x0080004240002000ULL, 0x1000210200420080ULL, 0x00082a0200104080ULL, 0x0040090100201000ULL,
0x4004080100100501ULL, 0x2001000900040002ULL, 0x0000020400017008ULL, 0x20c0008200104c09ULL,
0x0018400020801080ULL, 0x0090401000402004ULL, 0x0c00200041001100ULL, 0x0180200842001200ULL,
0x0204028004800800ULL, 0x0202008002802400ULL, 0x0008023004000128ULL, 0x0000408922000054ULL,
0x0280002000434000ULL, 0x000040201000400aULL, 0x0010040028002001ULL, 0x0005005001890021ULL,
0x0642001020060008ULL, 0x000200501c0a0008ULL, 0x5000040200010100ULL, 0x2800208041020004ULL,
0x0080024000200240ULL, 0x0198244002810100ULL, 0x0000200300421100ULL, 0x0990000800440040ULL,
0x0031001208000500ULL, 0x2004010002004040ULL, 0x0010111022580400ULL, 0x0400042048810200ULL,
0x1a08208212014502ULL, 0x000b004014a20082ULL, 0x000842001080200aULL, 0x2242002004100842ULL,
0x2802001020080402ULL, 0x0081000400480241ULL, 0x2848100208014084ULL, 0x03050c0c21814902ULL,
};

const U64 BMagic[64] = {
0x0010200208424906ULL, 0x2002100208890012ULL, 0x0010010041090001ULL, 0x100820444490280cULL,
0x0004042020644c59ULL, 0x0822080208022023ULL, 0x0004525010080218ULL, 0x0012082401080800ULL,
0x2021202002120045ULL, 0x0521150c24820208ULL, 0x000604011a060c00ULL, 0x4029082a00210000ULL,
0x04c0040421402000ULL, 0x00044510221000c0ULL, 0x00800a0104600441ULL, 0x0022009409051010ULL,
0x0042009010510106ULL, 0x2208092208081481ULL, 0x4010410200820202ULL, 0x51c2200812004010ULL,
0x4d1200c422010c04ULL, 0x0080400808021080ULL, 0x0981080848080400ULL, 0x4021081200620210ULL,
0x040804000a600821ULL, 0x0004120030424824ULL, 0x1102011008080020ULL, 0x0044080000220040ULL,
0x0001001001004008ULL, 0x0210088001004118ULL, 0x0624008010721001ULL, 0x0004048004422080ULL,
0x3822022300102042ULL, 0x0004210827241000ULL, 0x0e12008200100020ULL, 0x0000400a00002200ULL,
0x0008010010040204ULL, 0x0048010810010800ULL, 0x00186081000c0100ULL, 0x0081085100220102ULL,
0x41a8011028007000ULL, 0x0204062803100400ULL, 0x0201004030100200ULL, 0x2400044010410201ULL,
0x2000080100401c04ULL, 0x0001200800404084ULL, 0x0048181800500280ULL, 0x0030008100400108ULL,
0x2204040405042000ULL, 0x1001104104201040ULL, 0x0101010041100005ULL, 0x0001100304880125ULL,
0x0880404048320042ULL, 0x0808216202020285ULL, 0x0006100208010000ULL, 0x0020044100590025ULL,
0x000d010050020800ULL, 0x0104088401011051ULL, 0x20000d2044044400ULL, 0x4000020022050402ULL,
0x142308ac20242425ULL, 0x0306425002904508ULL, 0x0000111510240040ULL, 0x0802201808810040ULL,
};

const int RBits[64] = {
12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11,
11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12,
};

const int BBits[64] = {
6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6,
};

#include <vector>
#include <cstdio>
//#include "magics_data.h"

#pragma once
using U64 = unsigned long long;

extern const U64 RMagic[64];
extern const U64 BMagic[64];
extern const int RBits[64];
extern const int BBits[64];

using U64 = unsigned long long;

struct MagicEntry {
    U64 mask;
    U64 magic;
    int shift;     // 64 - usedBits
    int offset;    // base index into attack table
};

MagicEntry rookMag[64];
MagicEntry bishopMag[64];

// Fixed-size worst-case tables (match your earlier sizes)
U64 rookAttTable[64 * 4096];
U64 bishopAttTable[64 * 512];

// Your helper functions (must exist in engine):
static inline int file_of(int sq) { return sq & 7; }
static inline int rank_of(int sq) { return sq >> 3; }
static inline int sq_of(int r, int f) { return (r << 3) | f; }

// Masks excluding edges (same as generator)
U64 rook_mask(int sq) {
    U64 m = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1; rr <= 6; ++rr) m |= 1ULL << sq_of(rr, f);
    for (int rr = r - 1; rr >= 1; --rr) m |= 1ULL << sq_of(rr, f);
    for (int ff = f + 1; ff <= 6; ++ff) m |= 1ULL << sq_of(r, ff);
    for (int ff = f - 1; ff >= 1; --ff) m |= 1ULL << sq_of(r, ff);
    return m;
}
U64 bishop_mask(int sq) {
    U64 m = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1, ff = f + 1; rr <= 6 && ff <= 6; ++rr, ++ff) m |= 1ULL << sq_of(rr, ff);
    for (int rr = r + 1, ff = f - 1; rr <= 6 && ff >= 1; ++rr, --ff) m |= 1ULL << sq_of(rr, ff);
    for (int rr = r - 1, ff = f + 1; rr >= 1 && ff <= 6; --rr, ++ff) m |= 1ULL << sq_of(rr, ff);
    for (int rr = r - 1, ff = f - 1; rr >= 1 && ff >= 1; --rr, --ff) m |= 1ULL << sq_of(rr, ff);
    return m;
}

// OTF attacks (truth) — used only to build table
U64 rook_attacks_otf(int sq, U64 occ) {
    U64 a = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1; rr <= 7; ++rr) { U64 b = 1ULL << sq_of(rr, f); a |= b; if (occ & b) break; }
    for (int rr = r - 1; rr >= 0; --rr) { U64 b = 1ULL << sq_of(rr, f); a |= b; if (occ & b) break; }
    for (int ff = f + 1; ff <= 7; ++ff) { U64 b = 1ULL << sq_of(r, ff); a |= b; if (occ & b) break; }
    for (int ff = f - 1; ff >= 0; --ff) { U64 b = 1ULL << sq_of(r, ff); a |= b; if (occ & b) break; }
    return a;
}
U64 bishop_attacks_otf(int sq, U64 occ) {
    U64 a = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1, ff = f + 1; rr <= 7 && ff <= 7; ++rr, ++ff) { U64 b = 1ULL << sq_of(rr, ff); a |= b; if (occ & b) break; }
    for (int rr = r + 1, ff = f - 1; rr <= 7 && ff >= 0; ++rr, --ff) { U64 b = 1ULL << sq_of(rr, ff); a |= b; if (occ & b) break; }
    for (int rr = r - 1, ff = f + 1; rr >= 0 && ff <= 7; --rr, ++ff) { U64 b = 1ULL << sq_of(rr, ff); a |= b; if (occ & b) break; }
    for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff) { U64 b = 1ULL << sq_of(rr, ff); a |= b; if (occ & b) break; }
    return a;
}

static void list_mask_bits(U64 mask, std::vector<int>& bitsPos) {
    bitsPos.clear();
    for (int b = 0; b < 64; ++b) if (mask & (1ULL << b)) bitsPos.push_back(b);
}

static U64 index_to_occ(int index, const std::vector<int>& bitsPos) {
    U64 occ = 0ULL;
    for (int i = 0; i < (int)bitsPos.size(); ++i)
        if (index & (1 << i)) occ |= (1ULL << bitsPos[i]);
    return occ;
}

void InitMagicAttacks()
{
    int rookOffset = 0;
    int bishopOffset = 0;

    for (int sq = 0; sq < 64; ++sq)
    {
        // ---- Rook ----
        {
            U64 mask = rook_mask(sq);
            int used = RBits[sq];
            int size = 1 << used;

            rookMag[sq].mask = mask;
            rookMag[sq].magic = RMagic[sq];
            rookMag[sq].shift = 64 - used;     // CRITICAL
            rookMag[sq].offset = rookOffset;

            std::vector<int> bitsPos;
            list_mask_bits(mask, bitsPos);
            int perms = 1 << (int)bitsPos.size();

            for (int i = 0; i < perms; ++i) {
                U64 occ = index_to_occ(i, bitsPos);
                U64 idx = (occ * rookMag[sq].magic) >> rookMag[sq].shift;
                rookAttTable[rookOffset + (int)idx] = rook_attacks_otf(sq, occ);
            }

            rookOffset += size;
        }

        // ---- Bishop ----
        {
            U64 mask = bishop_mask(sq);
            int used = BBits[sq];
            int size = 1 << used;

            bishopMag[sq].mask = mask;
            bishopMag[sq].magic = BMagic[sq];
            bishopMag[sq].shift = 64 - used;   // CRITICAL
            bishopMag[sq].offset = bishopOffset;

            std::vector<int> bitsPos;
            list_mask_bits(mask, bitsPos);
            int perms = 1 << (int)bitsPos.size();

            for (int i = 0; i < perms; ++i) {
                U64 occ = index_to_occ(i, bitsPos);
                U64 idx = (occ * bishopMag[sq].magic) >> bishopMag[sq].shift;
                bishopAttTable[bishopOffset + (int)idx] = bishop_attacks_otf(sq, occ);
            }

            bishopOffset += size;
        }
    }

    // Optional safety checks
    if (rookOffset > 64 * 4096)  std::printf("ERROR: rookAttTable too small (%d)\n", rookOffset);
    if (bishopOffset > 64 * 512) std::printf("ERROR: bishopAttTable too small (%d)\n", bishopOffset);
}

U64 RookAttacks(int sq, U64 occ)
{
    const MagicEntry& m = rookMag[sq];
    occ &= m.mask;
    U64 idx = (occ * m.magic) >> m.shift;
    return rookAttTable[m.offset + (int)idx];
}

U64 BishopAttacks(int sq, U64 occ)
{
    const MagicEntry& m = bishopMag[sq];
    occ &= m.mask;
    U64 idx = (occ * m.magic) >> m.shift;
    return bishopAttTable[m.offset + (int)idx];
}

U64 QueenAttacks(int sq, U64 occ)
{
    return RookAttacks(sq, occ) | BishopAttacks(sq, occ);
}

void TestQueenAttacks()
{
    BITBOARD b2, b3, b4;
    int sq2;
    for (int sq = 0; sq < 64; ++sq)
    {
        for (int t = 0; t < 2000; ++t)
        {
            U64 occ = ((U64)rand() << 32) ^ (U64)rand();
            // Optional: often you want occ to include all pieces, including the queen square doesn't matter.
            U64 q1 = QueenAttacks(sq, occ);       // your fast one
            U64 q2 = 0;
            //
            bit_all = occ;
            b2 = bit_queenmoves[sq];
            b3 = b2 & bit_all;
            while (b3)
            {
                sq2 = NextBit(b3);
                b4 = ~bit_after[sq][sq2];
                b3 &= b4;
                b2 &= b4 | mask[sq2];
            }
            q2 = b2;
            //
            //PrintBitBoard(occ);
            //PrintBitBoard(q1);
            //Algebraic(sq);
            //_getch();
            //*
            if (q1 != q2)
            {
                printf("Mismatch sq=%d occ=%llx fast=%llx otf=%llx\n",
                    sq, (unsigned long long)occ,
                    (unsigned long long)q1,
                    (unsigned long long)q2);
                return;
            }
            //*/
        }
    }
    printf("QueenAttacks OK\n");
}





