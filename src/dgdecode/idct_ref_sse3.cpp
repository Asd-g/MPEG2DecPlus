/* 
idct_reference_sse3.cpp

rewite to single precision sse3 intrinsic code.
OKA Motofumi - August 21, 2016

*/


#include <pmmintrin.h>
#include "idct.h"

/*  Perform IEEE 1180 reference (64-bit floating point, separable 8x1
 *  direct matrix multiply) Inverse Discrete Cosine Transform
*/


/* cosine transform matrix for 8x1 IDCT */
alignas(64) static const float ref_dct_matrix_t[] = {
     0.353553f,  0.490393f,  0.461940f,  0.415735f,
     0.353553f,  0.277785f,  0.191342f,  0.097545f,
     0.353553f,  0.415735f,  0.191342f, -0.097545f,
    -0.353553f, -0.490393f, -0.461940f, -0.277785f,
     0.353553f,  0.277785f, -0.191342f, -0.490393f,
    -0.353553f,  0.097545f,  0.461940f,  0.415735f,
     0.353553f,  0.097545f, -0.461940f, -0.277785f,
     0.353553f,  0.415735f, -0.191342f, -0.490393f,
     0.353553f, -0.097545f, -0.461940f,  0.277785f,
     0.353553f, -0.415735f, -0.191342f,  0.490393f,
     0.353553f, -0.277785f, -0.191342f,  0.490393f,
    -0.353553f, -0.097545f,  0.461940f, -0.415735f,
     0.353553f, -0.415735f,  0.191342f,  0.097545f,
    -0.353553f,  0.490393f, -0.461940f,  0.277785f,
     0.353553f, -0.490393f,  0.461940f, -0.415735f,
     0.353553f, -0.277785f,  0.191342f, -0.097545f,
};


#if 0
static inline void idct_ref_8x8_c(const float* srcp, float*dstp) noexcept
{
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            float t = 0;
            for (int z = 0; z < 8; ++z) {
                t += ref_dct_matrix_t[8 * x + z] * srcp[8 * y + z];
            }
            dstp[8 * y + x] = t;
        }
    }
}

static inline void transpose_8x8_c(const float* srcp, float* dstp) noexcept
{
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            dstp[x] = src[8 * x + y];
        }
        dstp += 8;
    }
}
#endif


static inline void short_to_float(const short* srcp, float*dstp) noexcept
{
    const __m128i zero = _mm_setzero_si128();
    for (int i = 0; i < 64; i += 8) {
        __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + i));
        __m128i mask = _mm_cmpgt_epi16(zero, s);
        __m128 d0 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(s, mask));
        __m128 d1 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(s, mask));
        _mm_store_ps(dstp + i, d0);
        _mm_store_ps(dstp + i + 4, d1);
    }
}


static inline void transpose_8x8(const float* srcp, float* dstp) noexcept
{
    __m128 s0 = _mm_load_ps(srcp +  0);
    __m128 s1 = _mm_load_ps(srcp +  8);
    __m128 s2 = _mm_load_ps(srcp + 16);
    __m128 s3 = _mm_load_ps(srcp + 24);
    _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
    _mm_store_ps(dstp +  0, s0);
    _mm_store_ps(dstp +  8, s1);
    _mm_store_ps(dstp + 16, s2);
    _mm_store_ps(dstp + 24, s3);

    s0 = _mm_load_ps(srcp + 32);
    s1 = _mm_load_ps(srcp + 40);
    s2 = _mm_load_ps(srcp + 48);
    s3 = _mm_load_ps(srcp + 56);
    _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
    _mm_store_ps(dstp +  4, s0);
    _mm_store_ps(dstp + 12, s1);
    _mm_store_ps(dstp + 20, s2);
    _mm_store_ps(dstp + 28, s3);

    s0 = _mm_load_ps(srcp +  4);
    s1 = _mm_load_ps(srcp + 12);
    s2 = _mm_load_ps(srcp + 20);
    s3 = _mm_load_ps(srcp + 28);
    _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
    _mm_store_ps(dstp + 32, s0);
    _mm_store_ps(dstp + 40, s1);
    _mm_store_ps(dstp + 48, s2);
    _mm_store_ps(dstp + 56, s3);

    s0 = _mm_load_ps(srcp + 36);
    s1 = _mm_load_ps(srcp + 44);
    s2 = _mm_load_ps(srcp + 52);
    s3 = _mm_load_ps(srcp + 60);
    _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
    _mm_store_ps(dstp + 36, s0);
    _mm_store_ps(dstp + 44, s1);
    _mm_store_ps(dstp + 52, s2);
    _mm_store_ps(dstp + 60, s3);
}


static inline void idct_ref_8x8_sse3(const float* srcp, float* dstp) noexcept
{
    for (int i = 0; i < 8; ++i) {
        __m128 s0 = _mm_load_ps(srcp + 8 * i);
        __m128 s1 = _mm_load_ps(srcp + 8 * i + 4);

        for (int j = 0; j < 8; j += 4) {
            const float* mpos = ref_dct_matrix_t + 8 * j;

            __m128 m0 = _mm_load_ps(mpos);
            __m128 m1 = _mm_load_ps(mpos + 4);
            __m128 m2 = _mm_load_ps(mpos + 8);
            __m128 m3 = _mm_load_ps(mpos + 12);
            m0 = _mm_mul_ps(m0, s0);
            m1 = _mm_mul_ps(m1, s1);
            m2 = _mm_mul_ps(m2, s0);
            m3 = _mm_mul_ps(m3, s1);
            __m128 d0 = _mm_hadd_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, m3));

            m0 = _mm_load_ps(mpos + 16);
            m1 = _mm_load_ps(mpos + 20);
            m2 = _mm_load_ps(mpos + 24);
            m3 = _mm_load_ps(mpos + 28);
            m0 = _mm_mul_ps(m0, s0);
            m1 = _mm_mul_ps(m1, s1);
            m2 = _mm_mul_ps(m2, s0);
            m3 = _mm_mul_ps(m3, s1);
            __m128 d1 = _mm_hadd_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, m3));

            _mm_store_ps(dstp + 8 * i + j, _mm_hadd_ps(d0, d1));
        }
    }
}


static inline void float_to_dst(const float* srcp, int16_t* dst) noexcept
{
    static const __m128i minimum = _mm_set1_epi16(-256);
    static const __m128i maximum = _mm_set1_epi16(255);

    for (int i = 0; i < 64; i += 8) {
        __m128 s0 = _mm_load_ps(srcp + i);
        __m128 s1 = _mm_load_ps(srcp + i + 4);
        __m128i si = _mm_packs_epi32(_mm_cvtps_epi32(s0), _mm_cvtps_epi32(s1));
        si = _mm_min_epi16(_mm_max_epi16(si, minimum), maximum);
        _mm_store_si128(reinterpret_cast<__m128i*>(dst + i), si);
    }
}


void __fastcall idct_ref_sse3(int16_t* block) noexcept
{
    alignas(64) float blockf[64];
    alignas(64) float tmp[64];

    short_to_float(block, blockf);

    idct_ref_8x8_sse3(blockf, tmp);

    transpose_8x8(tmp, blockf);

    idct_ref_8x8_sse3(blockf, tmp);

    transpose_8x8(tmp, blockf);

    float_to_dst(blockf, block);
}


void __fastcall prefetch_ref() noexcept
void __fastcall prefetch_tables_ref() noexcept
{
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 16), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 32), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 48), _MM_HINT_NTA);
}
