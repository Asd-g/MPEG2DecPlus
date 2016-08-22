/*
 *  Motion Compensation for MPEG2Dec3
 *
 *  Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *  This file is part of MPEG2Dec3, a free MPEG-2 decoder
 *
 *  MPEG2Dec3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  MPEG2Dec3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


// SSE2 intrinsic implementation
// OKA Motofumi - August 23, 2016


#include <emmintrin.h>
#include "mc.h"


alignas(16) static const uint8_t one_x16[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};


static __forceinline __m128i load(const uint8_t* p)
{
    return _mm_load_si128(reinterpret_cast<const __m128i*>(p));
}

static __forceinline __m128i loadl(const uint8_t* p)
{
    return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p));
}

static __forceinline __m128i loadu(const uint8_t* p)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
}

static __forceinline __m128i avgu8(const __m128i& x, const __m128i& y)
{
    return _mm_avg_epu8(x, y);
}

static __forceinline void store(uint8_t* p, const __m128i& x)
{
    _mm_store_si128(reinterpret_cast<__m128i*>(p), x);
}

static __forceinline void storel(uint8_t* p, const __m128i& x)
{
    _mm_storel_epi64(reinterpret_cast<__m128i*>(p), x);
}

static __forceinline void storeu(uint8_t* p, const __m128i& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(p), x);
}


static void MC_put_8_c(uint8_t * dest, const uint8_t * ref, int stride, int, int height)
{
    do {
        *reinterpret_cast<uint64_t*>(dest) = *reinterpret_cast<const uint64_t*>(ref);
        dest += stride; ref += stride;
    } while (--height > 0);
}


static void MC_put_16_sse2(uint8_t * dest, const uint8_t * ref, int stride, int, int height)
{
    do {
        storeu(dest, loadu(ref));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_avg_8_sse2(uint8_t * dest, const uint8_t * ref, int stride, int, int height)
{
    do {
        storel(dest, avgu8(loadl(ref), loadl(dest)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_avg_16_sse2(uint8_t * dest, const uint8_t * ref, int stride, int, int height)
{
    do {
        storeu(dest, avgu8(loadu(ref), loadu(dest)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_put_x8_sse2(uint8_t * dest, const uint8_t * ref, int stride, int, int height)
{
    do {
        storel(dest, avgu8(loadl(ref), loadl(ref + 1)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_put_y8_sse2(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
{
    do {
        storel(dest, avgu8(loadl(ref), loadl(ref + offs)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_put_x16_sse2(uint8_t * dest, const uint8_t * ref, int stride, int, int height)
{
    do {
        storeu(dest, avgu8(loadu(ref), loadu(ref + 1)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_put_y16_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    do {
        storeu(dest, avgu8(loadu(ref), loadu(ref + offs)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_avg_x8_sse2(uint8_t* dest, const uint8_t* ref, int stride, int, int height)
{
    do {
        storel(dest, avgu8(avgu8(loadl(ref), loadl(ref + 1)), loadl(dest)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_avg_y8_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    do {
        storel(dest, avgu8(avgu8(loadl(ref), loadl(ref + offs)), loadl(dest)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_avg_x16_sse2(uint8_t* dest, const uint8_t* ref, int stride, int, int height)
{
    do {
        storeu(dest, avgu8(avgu8(loadu(ref), loadu(ref + 1)), loadu(dest)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static void MC_avg_y16_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    do {
        storeu(dest, avgu8(avgu8(loadu(ref), loadu(ref + offs)), loadu(dest)));
        ref += stride; dest += stride;
    } while (--height > 0);
}


static __forceinline __m128i
get_correcter(const __m128i& r0, const __m128i& r1, const __m128i& r2, const __m128i& r3,
              const __m128i& avg0, const __m128i& avg1, const __m128i& one)
{
    __m128i t0 = _mm_or_si128(_mm_xor_si128(r0, r3), _mm_xor_si128(r1, r2));
    t0 = _mm_and_si128(t0, _mm_xor_si128(avg0, avg1));
    return _mm_and_si128(t0, one);
}


static void MC_put_xy8_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    const __m128i one = load(one_x16);
    const uint8_t* ro = ref + offs;

    do {
        __m128i r0 = loadl(ref);
        __m128i r1 = loadl(ref + 1);
        __m128i r2 = loadl(ro);
        __m128i r3 = loadl(ro + 1);

        __m128i avg0 = avgu8(r0, r3);
        __m128i avg1 = avgu8(r1, r2);

        __m128i t0 = get_correcter(r0, r1, r2, r3, avg0, avg1, one);

        storel(dest, _mm_subs_epu8(avgu8(avg0, avg1), t0));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}


static void MC_put_xy16_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    const __m128i one = load(one_x16);
    const uint8_t* ro = ref + offs;

    do {
        __m128i r0 = loadu(ref);
        __m128i r1 = loadu(ref + 1);
        __m128i r2 = loadu(ro);
        __m128i r3 = loadu(ro + 1);

        __m128i avg0 = avgu8(r0, r3);
        __m128i avg1 = avgu8(r1, r2);

        __m128i t0 = get_correcter(r0, r1, r2, r3, avg0, avg1, one);

        storeu(dest, _mm_subs_epu8(avgu8(avg0, avg1), t0));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}


static void MC_avg_xy8_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    const __m128i one = load(one_x16);
    const uint8_t* ro = ref + offs;

    do {
        __m128i r0 = loadl(ref);
        __m128i r1 = loadl(ref + 1);
        __m128i r2 = loadl(ro);
        __m128i r3 = loadl(ro + 1);

        __m128i avg0 = avgu8(r0, r3);
        __m128i avg1 = avgu8(r1, r2);

        __m128i t0 = get_correcter(r0, r1, r2, r3, avg0, avg1, one);

        storel(dest, avgu8(_mm_subs_epu8(avgu8(avg0, avg1), t0), loadl(dest)));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}


static void MC_avg_xy16_sse2(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
{
    const __m128i one = load(one_x16);
    const uint8_t* ro = ref + offs;

    do {
        __m128i r0 = loadu(ref);
        __m128i r1 = loadu(ref + 1);
        __m128i r2 = loadu(ro);
        __m128i r3 = loadu(ro + 1);

        __m128i avg0 = avgu8(r0, r3);
        __m128i avg1 = avgu8(r1, r2);

        __m128i t0 = get_correcter(r0, r1, r2, r3, avg0, avg1, one);

        storeu(dest, avgu8(_mm_subs_epu8(avgu8(avg0, avg1), t0), loadu(dest)));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}



// This project requires SSE2. MMX/MMX_EXT/3DNOW! are obsoute.
// fastMC was discontinued...who cares about that?

MCFuncPtr ppppf_motion[2][2][4];

void Choose_Prediction(bool)
{
    ppppf_motion[0][0][0] = MC_put_8_c;
    ppppf_motion[0][0][1] = MC_put_y8_sse2;
    ppppf_motion[0][0][2] = MC_put_x8_sse2;
    ppppf_motion[0][0][3] = MC_put_xy8_sse2;

    ppppf_motion[0][1][0] = MC_put_16_sse2;
    ppppf_motion[0][1][1] = MC_put_y16_sse2;
    ppppf_motion[0][1][2] = MC_put_x16_sse2;
    ppppf_motion[0][1][3] = MC_put_xy16_sse2;

    ppppf_motion[1][0][0] = MC_avg_8_sse2;
    ppppf_motion[1][0][1] = MC_avg_y8_sse2;
    ppppf_motion[1][0][2] = MC_avg_x8_sse2;
    ppppf_motion[1][0][3] = MC_avg_xy8_sse2;

    ppppf_motion[1][1][0] = MC_avg_16_sse2;
    ppppf_motion[1][1][1] = MC_avg_y16_sse2;
    ppppf_motion[1][1][2] = MC_avg_x16_sse2;
    ppppf_motion[1][1][3] = MC_avg_xy16_sse2;
}
