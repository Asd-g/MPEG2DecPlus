/*
idct_reference_sse3.cpp

rewite to double precision sse3 intrinsic code.
OKA Motofumi - August 29, 2016

*/


#include <pmmintrin.h>
#include "idct.h"

/*  Perform IEEE 1180 reference (64-bit floating point, separable 8x1
 *  direct matrix multiply) Inverse Discrete Cosine Transform
*/


/* cosine transform matrix for 8x1 IDCT */
alignas(64) static const double ref_dct_matrix_t[] = {
     3.5355339059327379e-001,  4.9039264020161522e-001,
     4.6193976625564337e-001,  4.1573480615127262e-001,
     3.5355339059327379e-001,  2.7778511650980114e-001,
     1.9134171618254492e-001,  9.7545161008064166e-002,
     3.5355339059327379e-001,  4.1573480615127262e-001,
     1.9134171618254492e-001, -9.7545161008064096e-002,
    -3.5355339059327373e-001, -4.9039264020161522e-001,
    -4.6193976625564342e-001, -2.7778511650980109e-001,
     3.5355339059327379e-001,  2.7778511650980114e-001,
    -1.9134171618254486e-001, -4.9039264020161522e-001,
    -3.5355339059327384e-001,  9.7545161008064152e-002,
     4.6193976625564326e-001,  4.1573480615127273e-001,
     3.5355339059327379e-001,  9.7545161008064166e-002,
    -4.6193976625564337e-001, -2.7778511650980109e-001,
     3.5355339059327368e-001,  4.1573480615127273e-001,
    -1.9134171618254495e-001, -4.9039264020161533e-001,
     3.5355339059327379e-001, -9.7545161008064096e-002,
    -4.6193976625564342e-001,  2.7778511650980092e-001,
     3.5355339059327384e-001, -4.1573480615127256e-001,
    -1.9134171618254528e-001,  4.9039264020161522e-001,
     3.5355339059327379e-001, -2.7778511650980098e-001,
    -1.9134171618254517e-001,  4.9039264020161522e-001,
    -3.5355339059327334e-001, -9.7545161008064013e-002,
     4.6193976625564337e-001, -4.1573480615127251e-001,
     3.5355339059327379e-001, -4.1573480615127267e-001,
     1.9134171618254500e-001,  9.7545161008064388e-002,
    -3.5355339059327356e-001,  4.9039264020161533e-001,
    -4.6193976625564320e-001,  2.7778511650980076e-001,
     3.5355339059327379e-001, -4.9039264020161522e-001,
     4.6193976625564326e-001, -4.1573480615127256e-001,
     3.5355339059327329e-001, -2.7778511650980076e-001,
     1.9134171618254478e-001, -9.7545161008064291e-002,
};


#if 0
static inline void transpose_8x8_c(const double* srcp, double* dstp) noexcept
{
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            dstp[x] = srcp[8 * x + y];
        }
        dstp += 8;
    }
}


static inline void idct_ref_8x8_c(const double* srcp, double* dstp) noexcept
{
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            double t = 0;
            for (int z = 0; z < 8; ++z) {
                t += ref_dct_matrix_t[8 * x + z] * srcp[8 * y + z];
            }
            dstp[8 * y + x] = t;
        }
    }
}

#endif


static inline void short_to_double_sse2(const short* srcp, double* dstp) noexcept
{
    const __m128i zero = _mm_setzero_si128();
    for (int i = 0; i < 64; i += 8) {
        __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + i));
        __m128i mask = _mm_cmpgt_epi16(zero, s);
        __m128i s0 = _mm_unpacklo_epi16(s, mask);
        __m128i s1 = _mm_unpackhi_epi16(s, mask);
        __m128d d0 = _mm_cvtepi32_pd(s0);
        __m128d d1 = _mm_cvtepi32_pd(_mm_srli_si128(s0, 8));
        __m128d d2 = _mm_cvtepi32_pd(s1);
        __m128d d3 = _mm_cvtepi32_pd(_mm_srli_si128(s1, 8));
        _mm_store_pd(dstp + i, d0);
        _mm_store_pd(dstp + i + 2, d1);
        _mm_store_pd(dstp + i + 4, d2);
        _mm_store_pd(dstp + i + 6, d3);
    }
}


static inline void transpose_8x8_sse2(const double* srcp, double* dstp) noexcept
{
    for (int y = 0; y < 8; y += 2) {
        double* d = dstp + y;
        for (int x = 0; x < 8; x += 2) {
            __m128d s0 = _mm_load_pd(srcp + x);
            __m128d s1 = _mm_load_pd(srcp + x + 8);
            _mm_store_pd(d, _mm_unpacklo_pd(s0, s1));
            _mm_store_pd(d + 8, _mm_unpackhi_pd(s0, s1));
            d += 16;
        }
        srcp += 16;
    }
}


static inline void idct_ref_8x8_sse3(const double* srcp, double* dstp) noexcept
{
    for (int i = 0; i < 8; ++i) {
        __m128d s0 = _mm_load_pd(srcp + 8 * static_cast<int64_t>(i));
        __m128d s1 = _mm_load_pd(srcp + 8 * static_cast<int64_t>(i) + 2);
        __m128d s2 = _mm_load_pd(srcp + 8 * static_cast<int64_t>(i) + 4);
        __m128d s3 = _mm_load_pd(srcp + 8 * static_cast<int64_t>(i) + 6);

        for (int j = 0; j < 8; j += 2) {
            const double* mpos = ref_dct_matrix_t + 8 * static_cast<int64_t>(j);

            __m128d m0 = _mm_mul_pd(_mm_load_pd(mpos), s0);
            __m128d m1 = _mm_mul_pd(_mm_load_pd(mpos + 2), s1);
            __m128d m2 = _mm_mul_pd(_mm_load_pd(mpos + 4), s2);
            __m128d m3 = _mm_mul_pd(_mm_load_pd(mpos + 6), s3);
            __m128d d0 = _mm_add_pd(_mm_add_pd(m0, m1), _mm_add_pd(m2, m3));

            m0 = _mm_mul_pd(_mm_load_pd(mpos + 8), s0);
            m1 = _mm_mul_pd(_mm_load_pd(mpos + 10), s1);
            m2 = _mm_mul_pd(_mm_load_pd(mpos + 12), s2);
            m3 = _mm_mul_pd(_mm_load_pd(mpos + 14), s3);
            __m128d d1 = _mm_add_pd(_mm_add_pd(m0, m1), _mm_add_pd(m2, m3));

            _mm_store_pd(dstp + 8 * static_cast<int64_t>(i) + j, _mm_hadd_pd(d0, d1));
        }
    }
}


static inline void double_to_dst_sse2(const double* srcp, int16_t* dst) noexcept
{
    static const __m128i minimum = _mm_set1_epi16(-256);
    static const __m128i maximum = _mm_set1_epi16(255);

    for (int i = 0; i < 64; i += 8) {
        __m128d s0 = _mm_load_pd(srcp + i);
        __m128d s1 = _mm_load_pd(srcp + i + 2);
        __m128d s2 = _mm_load_pd(srcp + i + 4);
        __m128d s3 = _mm_load_pd(srcp + i + 6);
        __m128i d0 = _mm_unpacklo_epi64(_mm_cvtpd_epi32(s0), _mm_cvtpd_epi32(s1));
        __m128i d1 = _mm_unpacklo_epi64(_mm_cvtpd_epi32(s2), _mm_cvtpd_epi32(s3));
        d0 = _mm_min_epi16(_mm_max_epi16(_mm_packs_epi32(d0, d1), minimum), maximum);
        _mm_store_si128(reinterpret_cast<__m128i*>(dst + i), d0);
    }
}


void idct_ref_sse3(int16_t* block)
{
    alignas(64) double blockf[64];
    alignas(64) double tmp[64];

    short_to_double_sse2(block, blockf);

    idct_ref_8x8_sse3(blockf, tmp);

    transpose_8x8_sse2(tmp, blockf);

    idct_ref_8x8_sse3(blockf, tmp);

    transpose_8x8_sse2(tmp, blockf);

    double_to_dst_sse2(blockf, block);
}


void prefetch_ref()
{
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t +  0), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t +  8), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 16), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 24), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 32), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 40), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 48), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(ref_dct_matrix_t + 56), _MM_HINT_NTA);
}

