/*

MPEG2Dec's colorspace convertions Copyright (C) Chia-chen Kuo - April 2001

*/

// modified to be pitch != width friendly
// tritical - May 16, 2005

// lots of bug fixes and new isse 422->444 routine
// tritical - August 18, 2005

// rewite all code to sse2 intrinsic
// OKA Motofumi - August 21, 2016


#include <cstring>
#include <emmintrin.h>
#include "color_convert.h"


#if 0
// C implementation
void conv420to422I_c(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch, int width, int height)
{
    const uint8_t* s0 = src;
    const uint8_t* s1 = src + src_pitch;
    uint8_t* d0 = dst;
    uint8_t* d1 = dst + dst_pitch;

    width /= 2;
    src_pitch *= 2;
    dst_pitch *= 2;

    std::memcpy(d0, s0, width);
    std::memcpy(d1, s1, width);

    d0 += dst_pitch;
    d1 += dst_pitch;

    for (int y = 2; y < height - 2; y += 4) {
        const uint8_t* s2 = s0 + src_pitch;
        const uint8_t* s3 = s1 + src_pitch;
        uint8_t* d2 = d0 + dst_pitch;
        uint8_t* d3 = d1 + dst_pitch;

        for (int x = 0; x < width; ++x) {
            d0[x] = (s0[x] * 5 + s2[x] * 3 + 4) / 8;
            d1[x] = (s1[x] * 7 + s3[x] * 1 + 4) / 8;
            d2[x] = (s0[x] * 1 + s2[x] * 7 + 4) / 8;
            d3[x] = (s1[x] * 3 + s3[x] * 5 + 4) / 8;
        }
        s0 = s2;
        s1 = s3;
        d0 = d2 + dst_pitch;
        d1 = d3 + dst_pitch;
    }

    std::memcpy(d0, s0, width);
    std::memcpy(d1, s1, width);
}
#endif


static __forceinline __m128i
avg_weight_1_7(const __m128i& x, const __m128i& y, const __m128i& four)
{
    //(x + y * 7 + 4) / 8
    __m128i t0 = _mm_subs_epu16(_mm_slli_epi16(y, 3), y);
    t0 = _mm_adds_epu16(_mm_adds_epu16(t0, x), four);
    return _mm_srli_epi16(t0, 3);
}

static __forceinline __m128i
avg_weight_3_5(const __m128i& x, const __m128i& y, const __m128i& four)
{
    //(x * 3 + y * 5 + 4) / 8
    __m128i t0 = _mm_adds_epu16(_mm_slli_epi16(x, 1), x);
    __m128i t1 = _mm_adds_epu16(_mm_slli_epi16(y, 2), y);
    t0 = _mm_adds_epu16(_mm_adds_epu16(t0, t1), four);
    return _mm_srli_epi16(t0, 3);
}


void conv420to422I(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch, int width, int height)
{
    const uint8_t* src0 = src;
    const uint8_t* src1 = src + src_pitch;
    uint8_t* dst0 = dst;
    uint8_t* dst1 = dst + dst_pitch;

    width /= 2;
    src_pitch *= 2;
    dst_pitch *= 2;

    std::memcpy(dst0, src0, width);
    std::memcpy(dst1, src1, width);

    dst0 += dst_pitch;
    dst1 += dst_pitch;

    const __m128i zero = _mm_setzero_si128();
    const __m128i four = _mm_set1_epi16(0x0004);

    for (int y = 2; y < height - 2; y += 4) {
        const uint8_t* src2 = src0 + src_pitch;
        const uint8_t* src3 = src1 + src_pitch;
        uint8_t* dst2 = dst0 + dst_pitch;
        uint8_t* dst3 = dst1 + dst_pitch;

        for (int x = 0; x < width; x += 8) {
            __m128i s0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src0 + x));
            __m128i s1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src2 + x));
            s0 = _mm_unpacklo_epi8(s0, zero);
            s1 = _mm_unpacklo_epi8(s1, zero);
            __m128i d = _mm_packus_epi16(avg_weight_3_5(s1, s0, four), zero);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(dst0 + x), d);
            d = _mm_packus_epi16(avg_weight_1_7(s0, s1, four), zero);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(dst2 + x), d);

            s0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src1 + x));
            s1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src3 + x));
            s0 = _mm_unpacklo_epi8(s0, zero);
            s1 = _mm_unpacklo_epi8(s1, zero);
            d = _mm_packus_epi16(avg_weight_1_7(s1, s0, four), zero);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(dst1 + x), d);
            d = _mm_packus_epi16(avg_weight_3_5(s0, s1, four), zero);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(dst3 + x), d);
        }
        src0 = src2;
        src1 = src3;
        dst0 = dst2 + dst_pitch;
        dst1 = dst3 + dst_pitch;
    }

    std::memcpy(dst0, src0, width);
    std::memcpy(dst1, src1, width);
}


#if 0
// C implementation
void conv420to422P_c(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    const uint8_t* s0 = src;
    const uint8_t* s1 = s0 + src_pitch;
    uint8_t* d0 = dst;
    uint8_t* d1 = dst + dst_pitch;

    width /= 2;
    height /= 2;
    dst_pitch *= 2;

    for (int x = 0; x < width; ++x) {
        d0[x] = s0[x];
        d1[x] = (s0[x] * 3 + s1[x] + 2) / 4;
    }

    d0 += dst_pitch;
    d1 += dst_pitch;

    for (int y = 0; y < height - 2; ++y) {
        const uint8_t* s2 = s1 + src_pitch;
        for (int x = 0; x < width; ++x) {
            d0[x] = (s0[x] + s1[x] * 3 + 2) / 4;
            d1[x] = (s2[x] + s1[x] * 3 + 2) / 4;
        }
        s0 = s1;
        s1 = s2;
        d0 += dst_pitch;
        d1 += dst_pitch;
    }

    for (int x = 0; x < width; ++x) {
        d0[x] = (s0[x] + s1[x] * 3 + 2) / 4;
        d1[x] = s1[x];
    }
}
#endif


void conv420to422P(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    const uint8_t* s0 = src;
    const uint8_t* s1 = s0 + src_pitch;
    uint8_t* d0 = dst;
    uint8_t* d1 = dst + dst_pitch;

    width /= 2;
    height /= 2;
    dst_pitch *= 2;

    const __m128i one = _mm_set1_epi8(0x01);

    for (int x = 0; x < width; x += 16) {
        const __m128i sx0 = _mm_load_si128(reinterpret_cast<const __m128i*>(s0 + x));
        __m128i sx1 = _mm_load_si128(reinterpret_cast<const __m128i*>(s1 + x));

        sx1 = _mm_subs_epu8(sx1, one);
        sx1 = _mm_avg_epu8(_mm_avg_epu8(sx1, sx0), sx0);

        _mm_store_si128(reinterpret_cast<__m128i*>(d0 + x), sx0);
        _mm_store_si128(reinterpret_cast<__m128i*>(d1 + x), sx1);
    }

    d0 += dst_pitch;
    d1 += dst_pitch;

    for (int y = 0; y < height - 2; ++y) {
        const uint8_t* s2 = s1 + src_pitch;

        for (int x = 0; x < width; x += 16) {
            __m128i sx0 = _mm_load_si128(reinterpret_cast<const __m128i*>(s0 + x));
            const __m128i sx1 = _mm_load_si128(reinterpret_cast<const __m128i*>(s1 + x));
            __m128i sx2 = _mm_load_si128(reinterpret_cast<const __m128i*>(s2 + x));

            sx0 = _mm_subs_epu8(sx0, one);
            sx2 = _mm_subs_epu8(sx2, one);
            sx0 = _mm_avg_epu8(_mm_avg_epu8(sx0, sx1), sx1);
            sx2 = _mm_avg_epu8(_mm_avg_epu8(sx2, sx1), sx1);

            _mm_store_si128(reinterpret_cast<__m128i*>(d0 + x), sx0);
            _mm_store_si128(reinterpret_cast<__m128i*>(d1 + x), sx2);
        }
        s0 = s1;
        s1 = s2;
        d0 += dst_pitch;
        d1 += dst_pitch;
    }

    for (int x = 0; x < width; x += 16) {
        __m128i sx0 = _mm_load_si128(reinterpret_cast<const __m128i*>(s0 + x));
        const __m128i sx1 = _mm_load_si128(reinterpret_cast<const __m128i*>(s1 + x));

        sx0 = _mm_subs_epu8(sx0, one);
        sx0 = _mm_avg_epu8(_mm_avg_epu8(sx0, sx1), sx1);

        _mm_store_si128(reinterpret_cast<__m128i*>(d0 + x), sx0);
        _mm_store_si128(reinterpret_cast<__m128i*>(d1 + x), sx1);
    }
}

#if 0
// C implementation
void conv422to444_c(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    width /= 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            dst[2 * x]     = src[x];
            dst[2 * x + 1] = (src[x] + src[x + 1] + 1) / 2;
        }
        dst[2 * width - 2] = dst[2 * width - 1] = src[width - 1];
        src += src_pitch;
        dst += dst_pitch;
    }
}
#endif


void conv422to444(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    const int right = width - 1;
    width /= 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 16) {
            __m128i s0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src + x));
            __m128i s1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + x + 1));
            s1 = _mm_avg_epu8(s1, s0);
            __m128i d0 = _mm_unpacklo_epi8(s0, s1);
            __m128i d1 = _mm_unpackhi_epi8(s0, s1);
            _mm_store_si128(reinterpret_cast<__m128i*>(dst + 2 * x), d0);
            _mm_store_si128(reinterpret_cast<__m128i*>(dst + 2 * x + 16), d1);
        }
        dst[right] = dst[right - 1];
        src += src_pitch;
        dst += dst_pitch;
    }
}


#if 0
const int64_t mmmask_0001 = 0x0001000100010001;
const int64_t mmmask_0128 = 0x0080008000800080;

void conv444toRGB24(const uint8_t *py, const uint8_t *pu, const uint8_t *pv,
    uint8_t *dst, int src_pitchY, int src_pitchUV, int dst_pitch, int width,
    int height, int matrix, int pc_scale)
{
    __int64 RGB_Offset, RGB_Scale, RGB_CBU, RGB_CRV, RGB_CGX;
    int dst_modulo = dst_pitch-(3*width);

    if (pc_scale)
    {
        RGB_Scale = 0x1000254310002543;
        RGB_Offset = 0x0010001000100010;
        if (matrix == 7) // SMPTE 240M (1987)
        {
            RGB_CBU = 0x0000428500004285;
            RGB_CGX = 0xF7BFEEA3F7BFEEA3;
            RGB_CRV = 0x0000396900003969;
        }
        else if (matrix == 6 || matrix == 5) // SMPTE 170M/ITU-R BT.470-2 -- BT.601
        {
            RGB_CBU = 0x0000408D0000408D;
            RGB_CGX = 0xF377E5FCF377E5FC;
            RGB_CRV = 0x0000331300003313;
        }
        else if (matrix == 4) // FCC
        {
            RGB_CBU = 0x000040D8000040D8;
            RGB_CGX = 0xF3E9E611F3E9E611;
            RGB_CRV = 0x0000330000003300;
        }
        else // ITU-R Rec.709 (1990) -- BT.709
        {
            RGB_CBU = 0x0000439A0000439A;
            RGB_CGX = 0xF92CEEF1F92CEEF1;
            RGB_CRV = 0x0000395F0000395F;
        }
    }
    else
    {
        RGB_Scale = 0x1000200010002000;
        RGB_Offset = 0x0000000000000000;
        if (matrix == 7) // SMPTE 240M (1987)
        {
            RGB_CBU = 0x00003A6F00003A6F;
            RGB_CGX = 0xF8C0F0BFF8C0F0BF;
            RGB_CRV = 0x0000326E0000326E;
        }
        else if (matrix == 6 || matrix == 5) // SMPTE 170M/ITU-R BT.470-2 -- BT.601
        {
            RGB_CBU = 0x000038B4000038B4;
            RGB_CGX = 0xF4FDE926F4FDE926;
            RGB_CRV = 0x00002CDD00002CDD;
        }
        else if (matrix == 4) // FCC
        {
            RGB_CBU = 0x000038F6000038F6;
            RGB_CGX = 0xF561E938F561E938;
            RGB_CRV = 0x00002CCD00002CCD;
        }
        else // ITU-R Rec.709 (1990) -- BT.709
        {
            RGB_CBU = 0x00003B6200003B62;
            RGB_CGX = 0xFA00F104FA00F104;
            RGB_CRV = 0x0000326600003266;
        }
    }

    __asm
    {
        mov         eax, [py]  // eax = py
        mov         ebx, [pu]  // ebx = pu
        mov         ecx, [pv]  // ecx = pv
        mov         edx, [dst] // edx = dst
        mov         edi, width // edi = width
        xor         esi, esi
        pxor        mm0, mm0

        convRGB24:
        movd        mm1, [eax+esi]
            movd        mm3, [ebx+esi]
            punpcklbw   mm1, mm0
            punpcklbw   mm3, mm0
            movd        mm5, [ecx+esi]
            punpcklbw   mm5, mm0
            movq        mm7, [mmmask_0128]
            psubw       mm3, mm7
            psubw       mm5, mm7

            psubw       mm1, RGB_Offset
            movq        mm2, mm1
            movq        mm7, [mmmask_0001]
            punpcklwd   mm1, mm7
            punpckhwd   mm2, mm7
            movq        mm7, RGB_Scale
            pmaddwd     mm1, mm7
            pmaddwd     mm2, mm7

            movq        mm4, mm3
            punpcklwd   mm3, mm0
            punpckhwd   mm4, mm0
            movq        mm7, RGB_CBU
            pmaddwd     mm3, mm7
            pmaddwd     mm4, mm7
            paddd       mm3, mm1
            paddd       mm4, mm2
            psrad       mm3, 13
            psrad       mm4, 13
            packuswb    mm3, mm0
            packuswb    mm4, mm0

            movq        mm6, mm5
            punpcklwd   mm5, mm0
            punpckhwd   mm6, mm0
            movq        mm7, RGB_CRV
            pmaddwd     mm5, mm7
            pmaddwd     mm6, mm7
            paddd       mm5, mm1
            paddd       mm6, mm2
            psrad       mm5, 13
            psrad       mm6, 13
            packuswb    mm5, mm0
            packuswb    mm6, mm0

            punpcklbw   mm3, mm5
            punpcklbw   mm4, mm6
            movq        mm5, mm3
            movq        mm6, mm4
            psrlq       mm5, 16
            psrlq       mm6, 16
            por         mm3, mm5
            por         mm4, mm6

            movd        mm5, [ebx+esi]
            movd        mm6, [ecx+esi]
            punpcklbw   mm5, mm0
            punpcklbw   mm6, mm0
            movq        mm7, [mmmask_0128]
            psubw       mm5, mm7
            psubw       mm6, mm7

            movq        mm7, mm6
            punpcklwd   mm6, mm5
            punpckhwd   mm7, mm5
            movq        mm5, RGB_CGX
            pmaddwd     mm6, mm5
            pmaddwd     mm7, mm5
            paddd       mm6, mm1
            paddd       mm7, mm2

            psrad       mm6, 13
            psrad       mm7, 13
            packuswb    mm6, mm0
            packuswb    mm7, mm0

            punpcklbw   mm3, mm6
            punpcklbw   mm4, mm7

            movq        mm1, mm3
            movq        mm5, mm4
            movq        mm6, mm4

            psrlq       mm1, 32
            psllq       mm1, 24
            por         mm1, mm3

            psrlq       mm3, 40
            psllq       mm6, 16
            por         mm3, mm6
            movd        [edx], mm1

            psrld       mm4, 16
            psrlq       mm5, 24
            por         mm5, mm4
            movd        [edx+4], mm3

            add         edx, 0x0c
            add         esi, 0x04
            cmp         esi, edi
            movd        [edx-4], mm5

            jl          convRGB24

            add         eax, src_pitchY
            add         ebx, src_pitchUV
            add         ecx, src_pitchUV
            add         edx, dst_modulo
            xor         esi, esi
            dec         height
            jnz         convRGB24

            emms
    }
}


void conv422PtoYUY2(const uint8_t *py, uint8_t *pu, uint8_t *pv, uint8_t *dst,
    int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
    width /= 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 8) {
            __m128i u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pu + x));
            __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pv + x));
            __m128i uv = _mm_unpacklo_epi8(u, v);
            __m128i y = _mm_load_si128(reinterpret_cast<const __m128i*>(py + 2 * x));
            __m128i yuyv0 = _mm_unpacklo_epi8(y, uv);
            __m128i yuyv1 = _mm_unpackhi_epi8(y, uv);
            _mm_stream_si128(reinterpret_cast<__m128i*>(dst + 4 * x), yuyv0);
            _mm_stream_si128(reinterpret_cast<__m128i*>(dst + 4 * x + 16), yuyv1);
        }
        py += pitch1Y;
        pu += pitch1UV;
        pv += pitch1UV;
        dst += pitch2;
    }
}


void convYUY2to422P(const uint8_t *src, uint8_t *py, uint8_t *pu, uint8_t *pv,
    int pitch1, int pitch2y, int pitch2uv, int width, int height)
{
    width /= 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 8) {
            __m128i s0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src + 4 * x));
            __m128i s1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src + 4 * x + 16));

            __m128i s2 = _mm_unpacklo_epi8(s0, s1);
            __m128i s3 = _mm_unpackhi_epi8(s0, s1);

            s0 = _mm_unpacklo_epi8(s2, s3);
            s1 = _mm_unpackhi_epi8(s2, s3);

            s2 = _mm_unpacklo_epi8(s0, s1);
            s3 = _mm_unpackhi_epi8(s0, s1);

            s0 = _mm_unpacklo_epi8(s2, s3);
            s2 = _mm_srli_si128(s2, 8);
            s3 = _mm_srli_si128(s3, 8);
            _mm_store_si128(reinterpret_cast<__m128i*>(py + 2 * x), s0);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(pu + x), s2);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(pv + x), s3);
        }
        src += pitch1;
        py += pitch2y;
        pu += pitch2uv;
        pv += pitch2uv;
    }
}
#endif

