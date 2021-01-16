#include <emmintrin.h>
#include "idct.h"


alignas(64) static const float llm_coefs[] = {
     1.175876f,  1.175876f,  1.175876f,  1.175876f,
    -1.961571f, -1.961571f, -1.961571f, -1.961571f,
    -0.390181f, -0.390181f, -0.390181f, -0.390181f,
    -0.899976f, -0.899976f, -0.899976f, -0.899976f,
    -2.562915f, -2.562915f, -2.562915f, -2.562915f,
     0.298631f,  0.298631f,  0.298631f,  0.298631f,
     2.053120f,  2.053120f,  2.053120f,  2.053120f,
     3.072711f,  3.072711f,  3.072711f,  3.072711f,
     1.501321f,  1.501321f,  1.501321f,  1.501321f,
     0.541196f,  0.541196f,  0.541196f,  0.541196f,
    -1.847759f, -1.847759f, -1.847759f, -1.847759f,
     0.765367f,  0.765367f,  0.765367f,  0.765367f,
};


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


static inline void idct_8x4_with_transpose(const float* srcp, float* dstp) noexcept
{
    __m128 s0 = _mm_load_ps(srcp);
    __m128 s1 = _mm_load_ps(srcp + 8);
    __m128 s2 = _mm_load_ps(srcp + 16);
    __m128 s3 = _mm_load_ps(srcp + 24);
    _MM_TRANSPOSE4_PS(s0, s1, s2, s3);
    __m128 s4 = _mm_load_ps(srcp + 4);
    __m128 s5 = _mm_load_ps(srcp + 12);
    __m128 s6 = _mm_load_ps(srcp + 20);
    __m128 s7 = _mm_load_ps(srcp + 28);
    _MM_TRANSPOSE4_PS(s4, s5, s6, s7);

    __m128 z0 = _mm_add_ps(s1, s7);
    __m128 z1 = _mm_add_ps(s3, s5);
    __m128 z2 = _mm_add_ps(s3, s7);
    __m128 z3 = _mm_add_ps(s1, s5);
    __m128 z4 = _mm_mul_ps(_mm_add_ps(z0, z1), _mm_load_ps(llm_coefs));

    z2 =_mm_add_ps(_mm_mul_ps(z2, _mm_load_ps(llm_coefs + 4)), z4);
    z3 =_mm_add_ps(_mm_mul_ps(z3, _mm_load_ps(llm_coefs + 8)), z4);
    z0 =_mm_mul_ps(z0, _mm_load_ps(llm_coefs + 12));
    z1 =_mm_mul_ps(z1, _mm_load_ps(llm_coefs + 16));

    __m128 b3 =_mm_add_ps(_mm_add_ps(_mm_mul_ps(s7, _mm_load_ps(llm_coefs + 20)), z0), z2);
    __m128 b2 =_mm_add_ps(_mm_add_ps(_mm_mul_ps(s5, _mm_load_ps(llm_coefs + 24)), z1), z3);
    __m128 b1 =_mm_add_ps(_mm_add_ps(_mm_mul_ps(s3, _mm_load_ps(llm_coefs + 28)), z1), z2);
    __m128 b0 =_mm_add_ps(_mm_add_ps(_mm_mul_ps(s1, _mm_load_ps(llm_coefs + 32)), z0), z3);

    z4 = _mm_mul_ps(_mm_add_ps(s2, s6), _mm_load_ps(llm_coefs + 36));
    z0=_mm_add_ps(s0, s4);
    z1=_mm_sub_ps(s0, s4);

    z2=_mm_add_ps(z4, _mm_mul_ps(s6, _mm_load_ps(llm_coefs + 40)));
    z3=_mm_add_ps(z4, _mm_mul_ps(s2, _mm_load_ps(llm_coefs + 44)));

    s0 = _mm_add_ps(z0, z3);
    s3 = _mm_sub_ps(z0, z3);
    s1 = _mm_add_ps(z1, z2);
    s2 = _mm_sub_ps(z1, z2);

    _mm_store_ps(dstp     , _mm_add_ps(s0, b0));
    _mm_store_ps(dstp + 56, _mm_sub_ps(s0, b0));
    _mm_store_ps(dstp +  8, _mm_add_ps(s1, b1));
    _mm_store_ps(dstp + 48, _mm_sub_ps(s1, b1));
    _mm_store_ps(dstp + 16, _mm_add_ps(s2, b2));
    _mm_store_ps(dstp + 40, _mm_sub_ps(s2, b2));
    _mm_store_ps(dstp + 24, _mm_add_ps(s3, b3));
    _mm_store_ps(dstp + 32, _mm_sub_ps(s3, b3));
}


static inline void float_to_dst_llm(const float* srcp, int16_t* dstp) noexcept
{
    static const __m128 one_eighth = _mm_set1_ps(0.1250f);
    static const __m128i minimum = _mm_set1_epi16(-256);
    static const __m128i maximum = _mm_set1_epi16(255);

    for (int i = 0; i < 64; i += 8) {
        __m128 s0 = _mm_load_ps(srcp + i);
        __m128 s1 = _mm_load_ps(srcp + i + 4);
        s0 = _mm_mul_ps(s0, one_eighth);
        s1 = _mm_mul_ps(s1, one_eighth);
        __m128i d = _mm_packs_epi32(_mm_cvtps_epi32(s0), _mm_cvtps_epi32(s1));
        d = _mm_min_epi16(_mm_max_epi16(d, minimum), maximum);
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp + i), d);
    }
}


void __fastcall idct_llm_float_sse2(int16_t* block) noexcept
{
    alignas(64) float blockf[64];
    alignas(64) float tmp[64];

    short_to_float(block, blockf);

    idct_8x4_with_transpose(blockf, tmp);
    idct_8x4_with_transpose(blockf + 32, tmp + 4);

    idct_8x4_with_transpose(tmp, blockf);
    idct_8x4_with_transpose(tmp + 32, blockf + 4);

    float_to_dst_llm(blockf, block);
}


void __fastcall prefetch_llm_float_sse2() noexcept
{
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 16), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 32), _MM_HINT_NTA);
}

