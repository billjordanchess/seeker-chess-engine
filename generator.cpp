// gen_magics.cpp
// VS2019 friendly magic number generator for rook/bishop.
// Mapping: a1=0, b1=1, ..., h8=63.
// Prints: RMagic[64], BMagic[64], RBits[64], BBits[64].
//
// Build: Release x64 /O2 recommended (this is slow in Debug).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <algorithm>

using U64 = unsigned long long;

int SetMagics();

static inline int popcount64(U64 x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (int)((x * 0x0101010101010101ULL) >> 56);
}

static inline U64 random_u64() {
    // 64-bit from rand(); fine for magic search when combined
    U64 u1 = (U64)(rand() & 0xFFFF);
    U64 u2 = (U64)(rand() & 0xFFFF);
    U64 u3 = (U64)(rand() & 0xFFFF);
    U64 u4 = (U64)(rand() & 0xFFFF);
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

static inline U64 random_u64_fewbits() {
    return random_u64() & random_u64() & random_u64();
}

static inline int file_of(int sq) { return sq & 7; }
static inline int rank_of(int sq) { return sq >> 3; }
static inline int sq_of(int r, int f) { return (r << 3) | f; }

// CPW-style masks excluding the outer edge squares
static U64 rook_mask(int sq) {
    U64 m = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1; rr <= 6; ++rr) m |= 1ULL << sq_of(rr, f);
    for (int rr = r - 1; rr >= 1; --rr) m |= 1ULL << sq_of(rr, f);
    for (int ff = f + 1; ff <= 6; ++ff) m |= 1ULL << sq_of(r, ff);
    for (int ff = f - 1; ff >= 1; --ff) m |= 1ULL << sq_of(r, ff);
    return m;
}
static U64 bishop_mask(int sq) {
    U64 m = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1, ff = f + 1; rr <= 6 && ff <= 6; ++rr, ++ff) m |= 1ULL << sq_of(rr, ff);
    for (int rr = r + 1, ff = f - 1; rr <= 6 && ff >= 1; ++rr, --ff) m |= 1ULL << sq_of(rr, ff);
    for (int rr = r - 1, ff = f + 1; rr >= 1 && ff <= 6; --rr, ++ff) m |= 1ULL << sq_of(rr, ff);
    for (int rr = r - 1, ff = f - 1; rr >= 1 && ff >= 1; --rr, --ff) m |= 1ULL << sq_of(rr, ff);
    return m;
}

// On-the-fly sliding attacks (truth)
static U64 rook_attacks_otf(int sq, U64 occ) {
    U64 a = 0ULL; int r = rank_of(sq), f = file_of(sq);
    for (int rr = r + 1; rr <= 7; ++rr) { U64 b = 1ULL << sq_of(rr, f); a |= b; if (occ & b) break; }
    for (int rr = r - 1; rr >= 0; --rr) { U64 b = 1ULL << sq_of(rr, f); a |= b; if (occ & b) break; }
    for (int ff = f + 1; ff <= 7; ++ff) { U64 b = 1ULL << sq_of(r, ff); a |= b; if (occ & b) break; }
    for (int ff = f - 1; ff >= 0; --ff) { U64 b = 1ULL << sq_of(r, ff); a |= b; if (occ & b) break; }
    return a;
}
static U64 bishop_attacks_otf(int sq, U64 occ) {
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

struct MagicFindResult { U64 magic; int usedBits; };

static MagicFindResult find_magic_for_square(int sq, bool bishop)
{
    U64 mask = bishop ? bishop_mask(sq) : rook_mask(sq);
    const int bits = popcount64(mask);

    std::vector<int> bitsPos;
    list_mask_bits(mask, bitsPos);

    const int perms = 1 << bits;
    std::vector<U64> occs(perms);
    std::vector<U64> atts(perms);
    for (int i = 0; i < perms; ++i) {
        U64 occ = index_to_occ(i, bitsPos);
        occs[i] = occ;
        atts[i] = bishop ? bishop_attacks_otf(sq, occ) : rook_attacks_otf(sq, occ);
    }

    // Used bits: try bits then bits-1 (saves space sometimes)
    for (int used = bits; used >= std::max(1, bits - 1); --used)
    {
        const int size = 1 << used;
        std::vector<U64> table(size, 0ULL);
        std::vector<char> usedSlot(size, 0);

        const int MAX_TRIES = 1 << 22;
        for (int t = 0; t < MAX_TRIES; ++t)
        {
            U64 magic = random_u64_fewbits();

            // Heuristic filter
            if (popcount64((magic * mask) & 0xFF00000000000000ULL) < 6) continue;

            std::fill(usedSlot.begin(), usedSlot.end(), 0);
            bool ok = true;

            for (int i = 0; i < perms; ++i) {
                U64 idx = (occs[i] * magic) >> (64 - used);
                U64& cell = table[(size_t)idx];
                if (!usedSlot[(size_t)idx]) {
                    usedSlot[(size_t)idx] = 1;
                    cell = atts[i];
                }
                else if (cell != atts[i]) {
                    ok = false; break;
                }
            }

            if (ok) return { magic, used };
        }
    }

    return { 0ULL, bits };
}

static void print_u64_array(const char* name, const U64 a[64])
{
    std::printf("const U64 %s[64] = {\n", name);
    for (int i = 0; i < 64; ++i) {
        std::printf("0x%016llxULL,%s", (unsigned long long)a[i], (i % 4 == 3) ? "\n" : " ");
    }
    std::printf("};\n\n");
}

static void print_int_array(const char* name, const int a[64])
{
    std::printf("const int %s[64] = {\n", name);
    for (int i = 0; i < 64; ++i) {
        std::printf("%d,%s", a[i], (i % 16 == 15) ? "\n" : " ");
    }
    std::printf("};\n\n");
}

int SetMagics()
{
    std::srand((unsigned)std::time(nullptr));

    U64 RMagic[64]{};
    U64 BMagic[64]{};
    int RBits[64]{};
    int BBits[64]{};

    for (int sq = 0; sq < 64; ++sq)
    {
        auto r = find_magic_for_square(sq, false);
        auto b = find_magic_for_square(sq, true);
        if (r.magic == 0ULL || b.magic == 0ULL) {
            std::printf("Failed to find magic at sq=%d\n", sq);
            return 1;
        }
        RMagic[sq] = r.magic; RBits[sq] = r.usedBits;
        BMagic[sq] = b.magic; BBits[sq] = b.usedBits;
        std::printf("sq %2d: RBits=%2d BBits=%2d\n", sq, RBits[sq], BBits[sq]);
    }

    std::printf("\n// ---- paste into your engine ----\n\n");
    print_u64_array("RMagic", RMagic);
    print_u64_array("BMagic", BMagic);
    print_int_array("RBits", RBits);
    print_int_array("BBits", BBits);

    return 0;
}
