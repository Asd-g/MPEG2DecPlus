#ifndef __AVX2__
#error arch:avx2 is not set.
#endif

#include <immintrin.h>
#ifndef _WIN32
#include "win_import_min.h"
#endif
#include "idct.h"

alignas(64) static const float llm_coefs[] = {
     1.175876f,  1.175876f,  1.175876f,  1.175876f,  1.175876f,  1.175876f,  1.175876f,  1.175876f,
    -1.961571f, -1.961571f, -1.961571f, -1.961571f, -1.961571f, -1.961571f, -1.961571f, -1.961571f,
    -0.390181f, -0.390181f, -0.390181f, -0.390181f, -0.390181f, -0.390181f, -0.390181f, -0.390181f,
    -0.899976f, -0.899976f, -0.899976f, -0.899976f, -0.899976f, -0.899976f, -0.899976f, -0.899976f,
    -2.562915f, -2.562915f, -2.562915f, -2.562915f, -2.562915f, -2.562915f, -2.562915f, -2.562915f,
     0.298631f,  0.298631f,  0.298631f,  0.298631f,  0.298631f,  0.298631f,  0.298631f,  0.298631f,
     2.053120f,  2.053120f,  2.053120f,  2.053120f,  2.053120f,  2.053120f,  2.053120f,  2.053120f,
     3.072711f,  3.072711f,  3.072711f,  3.072711f,  3.072711f,  3.072711f,  3.072711f,  3.072711f,
     1.501321f,  1.501321f,  1.501321f,  1.501321f,  1.501321f,  1.501321f,  1.501321f,  1.501321f,
     0.541196f,  0.541196f,  0.541196f,  0.541196f,  0.541196f,  0.541196f,  0.541196f,  0.541196f,
    -1.847759f, -1.847759f, -1.847759f, -1.847759f, -1.847759f, -1.847759f, -1.847759f, -1.847759f,
     0.765367f,  0.765367f,  0.765367f,  0.765367f,  0.765367f,  0.765367f,  0.765367f,  0.765367f,
};


static __forceinline __m256
load_and_convert_to_float_x8_avx2(const int16_t* srcp) noexcept
{
    __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp));
    return _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(s));
}


static __forceinline void
transpose_8x8_avx2(__m256& a, __m256& b, __m256& c, __m256& d, __m256& e, __m256& f, __m256& g, __m256& h) noexcept
{
    __m256 ac0145 = _mm256_unpacklo_ps(a, c); // a0 c0 a1 c1 a4 c4 a5 c5
    __m256 ac2367 = _mm256_unpackhi_ps(a, c); // a2 c2 a3 c3 a6 c6 a7 c7
    __m256 bd0145 = _mm256_unpacklo_ps(b, d); // b0 d0 b1 d1 b4 d4 b5 d5
    __m256 bd2367 = _mm256_unpackhi_ps(b, d); // b2 d2 b3 d3 b6 d6 b7 d7
    __m256 eg0145 = _mm256_unpacklo_ps(e, g); // e0 g0 e1 g1 e4 g4 e5 g5
    __m256 eg2367 = _mm256_unpackhi_ps(e, g); // e2 g2 e3 g3 e6 g6 e7 g7
    __m256 fh0145 = _mm256_unpacklo_ps(f, h); // f0 h0 f1 h1 f4 h4 f5 h5
    __m256 fh2367 = _mm256_unpackhi_ps(f, h); // f2 h2 f3 h3 f6 h6 f7 h7

    __m256 abcd04 = _mm256_unpacklo_ps(ac0145, bd0145); // a0 b0 c0 d0 a4 b4 c4 d4
    __m256 abcd15 = _mm256_unpackhi_ps(ac0145, bd0145); // a1 b1 c1 d1 a5 b5 c5 d5
    __m256 abcd26 = _mm256_unpacklo_ps(ac2367, bd2367); // a2 b2 c2 d2 a6 b6 c6 d6
    __m256 abcd37 = _mm256_unpackhi_ps(ac2367, bd2367); // a3 b3 c3 d3 a7 b7 c7 d7
    __m256 efgh04 = _mm256_unpacklo_ps(eg0145, fh0145); // e0 f0 g0 h0 e4 f4 g4 h4
    __m256 efgh15 = _mm256_unpackhi_ps(eg0145, fh0145); // e1 f1 g1 h1 e5 f5 g5 h5
    __m256 efgh26 = _mm256_unpacklo_ps(eg2367, fh2367); // e2 f2 g2 h2 e6 f6 g6 h6
    __m256 efgh37 = _mm256_unpackhi_ps(eg2367, fh2367); // e3 f3 g3 h3 e7 f7 g7 h7

    a = _mm256_permute2f128_ps(abcd04, efgh04, (2 << 4) | 0); //a0 b0 c0 d0 e0 f0 g0 h0
    e = _mm256_permute2f128_ps(abcd04, efgh04, (3 << 4) | 1); //a4 b4 c4 d4 e4 f4 g4 h4
    b = _mm256_permute2f128_ps(abcd15, efgh15, (2 << 4) | 0); //a1 b1 c1 d1 e1 f1 g1 h1
    f = _mm256_permute2f128_ps(abcd15, efgh15, (3 << 4) | 1); //a5 b5 c5 d5 e5 f5 g5 h5
    c = _mm256_permute2f128_ps(abcd26, efgh26, (2 << 4) | 0); //a2 b2 c2 d2 e2 f2 g2 h2
    g = _mm256_permute2f128_ps(abcd26, efgh26, (3 << 4) | 1); //a6 b6 c6 d6 e6 f6 g6 h6
    d = _mm256_permute2f128_ps(abcd37, efgh37, (2 << 4) | 0); //a3 b3 c3 d3 e3 f3 g3 h3
    h = _mm256_permute2f128_ps(abcd37, efgh37, (3 << 4) | 1); //a7 b7 c7 d7 e7 f7 g7 h7
}


static __forceinline void
idct_8x8_fma3(__m256& s0, __m256& s1, __m256& s2, __m256& s3, __m256& s4, __m256& s5, __m256& s6, __m256& s7) noexcept
{
    __m256 z0 = _mm256_add_ps(s1, s7);
    __m256 z1 = _mm256_add_ps(s3, s5);
    __m256 z2 = _mm256_add_ps(s3, s7);
    __m256 z3 = _mm256_add_ps(s1, s5);
    __m256 z4 = _mm256_mul_ps(_mm256_add_ps(z0, z1), _mm256_load_ps(llm_coefs));

    z2 = _mm256_fmadd_ps(z2, _mm256_load_ps(llm_coefs + 8), z4);
    z3 = _mm256_fmadd_ps(z3, _mm256_load_ps(llm_coefs + 16), z4);
    z0 = _mm256_mul_ps(z0, _mm256_load_ps(llm_coefs + 24));
    z1 = _mm256_mul_ps(z1, _mm256_load_ps(llm_coefs + 32));

    __m256 b3 = _mm256_fmadd_ps(_mm256_load_ps(llm_coefs + 40), s7, _mm256_add_ps(z0, z2));
    __m256 b2 = _mm256_fmadd_ps(_mm256_load_ps(llm_coefs + 48), s5, _mm256_add_ps(z1, z3));
    __m256 b1 = _mm256_fmadd_ps(_mm256_load_ps(llm_coefs + 56), s3, _mm256_add_ps(z1, z2));
    __m256 b0 = _mm256_fmadd_ps(_mm256_load_ps(llm_coefs + 64), s1, _mm256_add_ps(z0, z3));

    z4 = _mm256_mul_ps(_mm256_add_ps(s2, s6), _mm256_load_ps(llm_coefs + 72));
    z0 = _mm256_add_ps(s0, s4);
    z1 = _mm256_sub_ps(s0, s4);

    z2=_mm256_fmadd_ps(s6, _mm256_load_ps(llm_coefs + 80), z4);
    z3=_mm256_fmadd_ps(s2, _mm256_load_ps(llm_coefs + 88), z4);

    __m256 a0 = _mm256_add_ps(z0, z3);
    __m256 a3 = _mm256_sub_ps(z0, z3);
    __m256 a1 = _mm256_add_ps(z1, z2);
    __m256 a2 = _mm256_sub_ps(z1, z2);

    s0 = _mm256_add_ps(a0, b0);
    s7 = _mm256_sub_ps(a0, b0);
    s1 = _mm256_add_ps(a1, b1);
    s6 = _mm256_sub_ps(a1, b1);
    s2 = _mm256_add_ps(a2, b2);
    s5 = _mm256_sub_ps(a2, b2);
    s3 = _mm256_add_ps(a3, b3);
    s4 = _mm256_sub_ps(a3, b3);
}


static __forceinline void
float_to_dst_avx2(const __m256& s0, const __m256& s1, int16_t* dst) noexcept
{
    static const __m256 one_eighth = _mm256_set1_ps(0.1250f);
    static const __m256i minimum = _mm256_set1_epi16(-256);
    static const __m256i maximum = _mm256_set1_epi16(255);

    __m256 t0 = _mm256_mul_ps(s0, one_eighth);
    __m256 t1 = _mm256_mul_ps(s1, one_eighth);
    __m256i d0 = _mm256_packs_epi32(_mm256_cvtps_epi32(t0), _mm256_cvtps_epi32(t1));
    d0 = _mm256_permute4x64_epi64(d0, _MM_SHUFFLE(3, 1, 2, 0));
    d0 = _mm256_max_epi16(_mm256_min_epi16(d0, maximum), minimum);
    _mm256_store_si256(reinterpret_cast<__m256i*>(dst), d0);
}


void __fastcall idct_llm_float_avx2(int16_t* block) noexcept
{
    __m256 s0 = load_and_convert_to_float_x8_avx2(block);
    __m256 s1 = load_and_convert_to_float_x8_avx2(block + 8);
    __m256 s2 = load_and_convert_to_float_x8_avx2(block + 16);
    __m256 s3 = load_and_convert_to_float_x8_avx2(block + 24);
    __m256 s4 = load_and_convert_to_float_x8_avx2(block + 32);
    __m256 s5 = load_and_convert_to_float_x8_avx2(block + 40);
    __m256 s6 = load_and_convert_to_float_x8_avx2(block + 48);
    __m256 s7 = load_and_convert_to_float_x8_avx2(block + 56);

    transpose_8x8_avx2(s0, s1, s2, s3, s4, s5, s6, s7);

    idct_8x8_fma3(s0, s1, s2, s3, s4, s5, s6, s7);

    transpose_8x8_avx2(s0, s1, s2, s3, s4, s5, s6, s7);

    idct_8x8_fma3(s0, s1, s2, s3, s4, s5, s6, s7);

    float_to_dst_avx2(s0, s1, block +  0);
    float_to_dst_avx2(s2, s3, block + 16);
    float_to_dst_avx2(s4, s5, block + 32);
    float_to_dst_avx2(s6, s7, block + 48);
}


void __fastcall prefetch_llm_float_avx2() noexcept
{
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 16), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 32), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 48), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 64), _MM_HINT_NTA);
    _mm_prefetch(reinterpret_cast<const char*>(llm_coefs + 80), _MM_HINT_NTA);
}
