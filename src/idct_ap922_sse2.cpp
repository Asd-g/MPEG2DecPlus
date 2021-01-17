/*
idct_ap922_sse2.cpp

Originally provided by Intel at AP-922
http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
(See more app notes at http://developer.intel.com/vtune/cbts/strmsimd/appnotes.htm)
but in a limited edition.
New macro implements a column part for precise iDCT
The routine precision now satisfies IEEE standard 1180-1990.

Copyright (c) 2000-2001 Peter Gubanov <peter@elecard.net.ru>
Rounding trick Copyright (c) 2000 Michel Lespinasse <walken@zoy.org>

http://www.elecard.com/peter/idct.html
http://www.linuxvideo.org/mpeg2dec/

SSE2 code by Dmitry Rozhdestvensky

rewite to intrinsic by OKA Motofumi

============================================================================

These examples contain code fragments for first stage iDCT 8x8
(for rows) and first stage DCT 8x8 (for columns)

============================================================================

 The first stage iDCT 8x8 - inverse DCTs of rows

-----------------------------------------------------------------------------
 The 8-point inverse DCT direct algorithm
-----------------------------------------------------------------------------

 static const short w[32] = {
   FIX(cos_4_16),  FIX(cos_2_16),  FIX(cos_4_16),  FIX(cos_6_16),
   FIX(cos_4_16),  FIX(cos_6_16), -FIX(cos_4_16), -FIX(cos_2_16),
   FIX(cos_4_16), -FIX(cos_6_16), -FIX(cos_4_16),  FIX(cos_2_16),
   FIX(cos_4_16), -FIX(cos_2_16),  FIX(cos_4_16), -FIX(cos_6_16),
   FIX(cos_1_16),  FIX(cos_3_16),  FIX(cos_5_16),  FIX(cos_7_16),
   FIX(cos_3_16), -FIX(cos_7_16), -FIX(cos_1_16), -FIX(cos_5_16),
   FIX(cos_5_16), -FIX(cos_1_16),  FIX(cos_7_16),  FIX(cos_3_16),
   FIX(cos_7_16), -FIX(cos_5_16),  FIX(cos_3_16), -FIX(cos_1_16) };

 #define DCT_8_INV_ROW(x, y)
 {
   int a0, a1, a2, a3, b0, b1, b2, b3;

   a0 =x[0]*w[0]+x[2]*w[1]+x[4]*w[2]+x[6]*w[3];
   a1 =x[0]*w[4]+x[2]*w[5]+x[4]*w[6]+x[6]*w[7];
   a2 = x[0] * w[ 8] + x[2] * w[ 9] + x[4] * w[10] + x[6] * w[11];
   a3 = x[0] * w[12] + x[2] * w[13] + x[4] * w[14] + x[6] * w[15];
   b0 = x[1] * w[16] + x[3] * w[17] + x[5] * w[18] + x[7] * w[19];
   b1 = x[1] * w[20] + x[3] * w[21] + x[5] * w[22] + x[7] * w[23];
   b2 = x[1] * w[24] + x[3] * w[25] + x[5] * w[26] + x[7] * w[27];
   b3 = x[1] * w[28] + x[3] * w[29] + x[5] * w[30] + x[7] * w[31];

   y[0] = SHIFT_ROUND ( a0 + b0 );
   y[1] = SHIFT_ROUND ( a1 + b1 );
   y[2] = SHIFT_ROUND ( a2 + b2 );
   y[3] = SHIFT_ROUND ( a3 + b3 );
   y[4] = SHIFT_ROUND ( a3 - b3 );
   y[5] = SHIFT_ROUND ( a2 - b2 );
   y[6] = SHIFT_ROUND ( a1 - b1 );
   y[7] = SHIFT_ROUND ( a0 - b0 );
 }

-----------------------------------------------------------------------------

 In this implementation the outputs of the iDCT-1D are multiplied
   for rows 0,4 - by cos_4_16,
   for rows 1,7 - by cos_1_16,
   for rows 2,6 - by cos_2_16,
   for rows 3,5 - by cos_3_16
 and are shifted to the left for better accuracy

 For the constants used,
   FIX(float_const) = (short) (float_const * (1<<15) + 0.5)

=============================================================================
*/


#include <cstdint>
#include <emmintrin.h>

#ifndef _WIN32
#include "win_import_min.h"
#endif


alignas(64) static constexpr int16_t table04[] = {
    16384, 21407,  16384,   8867,  16384,  -8867, 16384, -21407, // w0, w1, w4, w5, w8, w9,w12,w13
    16384,  8867, -16384, -21407, -16384,  21407, 16384,  -8867, // w2, w3, w6, w7,w10,w11,w14,w15
    22725, 19266,  19266,  -4520,  12873, -22725,  4520, -12873, //w16,w17,w20,w21,w24,w25,w28,w29
    12873,  4520, -22725, -12873,   4520,  19266, 19266, -22725, //w18,w19,w22,w23,w26,w27,w30,w31
};

alignas(64) static constexpr int16_t table17[] = {
    22725, 29692,  22725,  12299,  22725, -12299, 22725, -29692, // w0, w1, w4, w5, w8, w9,w12,w13
    22725, 12299, -22725, -29692, -22725,  29692, 22725, -12299, // w2, w3, w6, w7,w10,w11,w14,w15
    31521, 26722,  26722,  -6270,  17855, -31521,  6270, -17855, //w16,w17,w20,w21,w24,w25,w28,w29
    17855,  6270, -31521, -17855,   6270,  26722, 26722, -31521, //w18,w19,w22,w23,w26,w27,w30,w31
};

alignas(64) static constexpr int16_t table26[] = {
    21407, 27969,  21407,  11585,  21407, -11585, 21407, -27969, // w0, w1, w4, w5, w8, w9,w12,w13
    21407, 11585, -21407, -27969, -21407,  27969, 21407, -11585, // w2, w3, w6, w7,w10,w11,w14,w15
    29692, 25172,  25172,  -5906,  16819, -29692,  5906, -16819, //w16,w17,w20,w21,w24,w25,w28,w29
    16819,  5906, -29692, -16819,   5906,  25172, 25172, -29692, //w18,w19,w22,w23,w26,w27,w30,w31
};

alignas(64) static constexpr int16_t table35[] = {
    19266, 25172,  19266,  10426,  19266, -10426, 19266, -25172, // w0, w1, w4, w5, w8, w9,w12,w13
    19266, 10426, -19266, -25172, -19266,  25172, 19266, -10426, // w2, w3, w6, w7,w10,w11,w14,w15
    26722, 22654,  22654,  -5315,  15137, -26722,  5315, -15137, //w16,w17,w20,w21,w24,w25,w28,w29
    15137,  5315, -26722, -15137,   5315,  22654, 22654, -26722, //w18,w19,w22,w23,w26,w27,w30,w31
};

alignas(64) static constexpr int32_t rounders[8][4] = {
    { 65536, 65536, 65536, 65536 },
    { 3597, 3597, 3597, 3597 },
    { 2260, 2260, 2260, 2260 },
    { 1203, 1203, 1203, 1203 },
    { 0, 0, 0, 0 },
    { 120, 120, 120, 120 },
    { 512, 512, 512, 512 },
    { 512, 512, 512, 512 },
};

alignas(64) static constexpr int16_t tg[4][8] = {
    { 13036, 13036, 13036, 13036, 13036, 13036, 13036, 13036 },
    { 27146,  27146,  27146,  27146,  27146,  27146,  27146,  27146},
    {-21746, -21746, -21746, -21746, -21746, -21746, -21746, -21746},
    { 23170,  23170,  23170,  23170,  23170,  23170,  23170,  23170},
};


static __forceinline void
idct_row_sse2(int16_t* block, const int16_t* table, const int32_t* rounder) noexcept
{
    __m128i* blk = reinterpret_cast<__m128i*>(block);
    const __m128i* tbl = reinterpret_cast<const __m128i*>(table);
    const __m128i* rnd = reinterpret_cast<const __m128i*>(rounder);

    __m128i row = _mm_load_si128(blk);
    row = _mm_shufflehi_epi16(row, _MM_SHUFFLE(3, 1, 2, 0));
    row = _mm_shufflelo_epi16(row, _MM_SHUFFLE(3, 1, 2, 0));

    __m128i t0 = _mm_shuffle_epi32(row, _MM_SHUFFLE(0, 0, 0, 0));
    t0 = _mm_madd_epi16(t0, _mm_load_si128(tbl));

    __m128i t1 = _mm_shuffle_epi32(row, _MM_SHUFFLE(2, 2, 2, 2));
    t1 = _mm_madd_epi16(t1, _mm_load_si128(++tbl));

    t0 = _mm_add_epi32(_mm_add_epi32(t0, t1), _mm_load_si128(rnd));

    __m128i t2 = _mm_shuffle_epi32(row, _MM_SHUFFLE(1, 1, 1, 1));
    t2 = _mm_madd_epi16(t2, _mm_load_si128(++tbl));

    __m128i t3 = _mm_shuffle_epi32(row, _MM_SHUFFLE(3, 3, 3, 3));
    t3 = _mm_madd_epi16(t3, _mm_load_si128(++tbl));

    t3 = _mm_add_epi32(t2, t3);

    t1 = _mm_add_epi32(t0, t3);
    t2 = _mm_sub_epi32(t0, t3);

    t0 = _mm_packs_epi32(_mm_srai_epi32(t1, 11), _mm_srai_epi32(t2, 11));
    t0 = _mm_shufflehi_epi16(t0, _MM_SHUFFLE(0, 1, 2, 3));

    _mm_store_si128(blk, t0);
}


static __forceinline void
idct_colx8_sse2(int16_t* block) noexcept
{
    const __m128i* tg1 = reinterpret_cast<const __m128i*>(tg[0]);
    const __m128i* tg2 = reinterpret_cast<const __m128i*>(tg[1]);
    const __m128i* tg3 = reinterpret_cast<const __m128i*>(tg[2]);
    const __m128i* ocos4 = reinterpret_cast<const __m128i*>(tg[3]);

    __m128i* blk = reinterpret_cast<__m128i*>(block);

    __m128i x0 = _mm_load_si128(blk + 0);
    __m128i x4 = _mm_load_si128(blk + 4);
    __m128i x2 = _mm_load_si128(blk + 2);
    __m128i x6 = _mm_load_si128(blk + 6);
    __m128i tgx = _mm_load_si128(tg2);

    __m128i u04 = _mm_adds_epi16(x0, x4);
    __m128i v04 = _mm_subs_epi16(x0, x4);

    __m128i t0 = _mm_mulhi_epi16(x2, tgx);
    __m128i t1 = _mm_mulhi_epi16(x6, tgx);
    __m128i v26 = _mm_subs_epi16(t0, x6);
    __m128i u26 = _mm_adds_epi16(t1, x2);

    __m128i a0 = _mm_adds_epi16(u04, u26);
    __m128i a1 = _mm_adds_epi16(v04, v26);
    __m128i a2 = _mm_subs_epi16(v04, v26);
    __m128i a3 = _mm_subs_epi16(u04, u26);

    __m128i x1 = _mm_load_si128(blk + 1);
    __m128i x7 = _mm_load_si128(blk + 7);
    __m128i x3 = _mm_load_si128(blk + 3);
    __m128i x5 = _mm_load_si128(blk + 5);
    tgx = _mm_load_si128(tg1);

    t0 = _mm_mulhi_epi16(x1, tgx);
    t1 = _mm_mulhi_epi16(x7, tgx);
    __m128i u17 = _mm_adds_epi16(t1, x1);
    __m128i v17 = _mm_subs_epi16(t0, x7);

    tgx = _mm_load_si128(tg3);

    t0 = _mm_mulhi_epi16(x3, tgx);
    t1 = _mm_mulhi_epi16(x5, tgx);
    t0 = _mm_adds_epi16(t0, x3);
    t1 = _mm_adds_epi16(t1, x5);
    __m128i v35 = _mm_subs_epi16(t0, x5);
    __m128i u35 = _mm_adds_epi16(t1, x3);

    __m128i b0 = _mm_adds_epi16(u17, u35);
    __m128i b3 = _mm_subs_epi16(v17, v35);
    __m128i u12 = _mm_subs_epi16(u17, u35);
    __m128i v12 = _mm_adds_epi16(v17, v35);

    tgx = _mm_load_si128(ocos4);
    t0 = _mm_adds_epi16(u12, v12);
    t1 = _mm_subs_epi16(u12, v12);
    t0 = _mm_mulhi_epi16(t0, tgx);
    t1 = _mm_mulhi_epi16(t1, tgx);
    __m128i b1 = _mm_adds_epi16(t0, t0);
    __m128i b2 = _mm_adds_epi16(t1, t1);

    _mm_store_si128(blk + 0, _mm_srai_epi16(_mm_adds_epi16(a0, b0), 6));
    _mm_store_si128(blk + 7, _mm_srai_epi16(_mm_subs_epi16(a0, b0), 6));

    _mm_store_si128(blk + 3, _mm_srai_epi16(_mm_adds_epi16(a3, b3), 6));
    _mm_store_si128(blk + 4, _mm_srai_epi16(_mm_subs_epi16(a3, b3), 6));

    _mm_store_si128(blk + 1, _mm_srai_epi16(_mm_adds_epi16(a1, b1), 6));
    _mm_store_si128(blk + 6, _mm_srai_epi16(_mm_subs_epi16(a1, b1), 6));

    _mm_store_si128(blk + 2, _mm_srai_epi16(_mm_adds_epi16(a2, b2), 6));
    _mm_store_si128(blk + 5, _mm_srai_epi16(_mm_subs_epi16(a2, b2), 6));
}


void idct_ap922_sse2(int16_t* block)
{
    idct_row_sse2(block +  0, table04, rounders[0]);
    idct_row_sse2(block +  8, table17, rounders[1]);
    idct_row_sse2(block + 16, table26, rounders[2]);
    idct_row_sse2(block + 24, table35, rounders[3]);
    idct_row_sse2(block + 32, table04, rounders[4]);
    idct_row_sse2(block + 40, table35, rounders[5]);
    idct_row_sse2(block + 48, table26, rounders[6]);
    idct_row_sse2(block + 56, table17, rounders[7]);

    idct_colx8_sse2(block);
}


void prefetch_ap922()
{
    _mm_prefetch(reinterpret_cast<const char*>(table04), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(table17), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(table26), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(table35), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(rounders[0]), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(tg[0]), _MM_HINT_NTA);
}
