/*

MPEG2Dec's colorspace convertions Copyright (C) Chia-chen Kuo - April 2001

*/

// modified to be pitch != width friendly
// tritical - May 16, 2005

// lots of bug fixes and new isse 422->444 routine
// tritical - August 18, 2005


#include <emmintrin.h>
#include "color_convert.h"


constexpr int64_t mmmask_0001 = 0x0001000100010001;
constexpr int64_t mmmask_0002 = 0x0002000200020002;
constexpr int64_t mmmask_0003 = 0x0003000300030003;
constexpr int64_t mmmask_0004 = 0x0004000400040004;
constexpr int64_t mmmask_0005 = 0x0005000500050005;
constexpr int64_t mmmask_0007 = 0x0007000700070007;
constexpr int64_t mmmask_0016 = 0x0010001000100010;
constexpr int64_t mmmask_0128 = 0x0080008000800080;
constexpr int64_t lastmask    = 0xFF00000000000000;
constexpr int64_t mmmask_0101 = 0x0101010101010101;


void conv420to422I_MMX(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch, int width, int height)
{
    int src_pitch2 = src_pitch << 1;
    int dst_pitch2 = dst_pitch << 1;
    int dst_pitch4 = dst_pitch << 2;
    int INTERLACED_HEIGHT = (height >> 2) - 2;
    int PROGRESSIVE_HEIGHT = (height >> 1) - 2;
    int HALF_WIDTH = width >> 1; // chroma width

    __asm
    {
        mov         eax, [src] // eax = src
        mov         ecx, [dst] // ecx = dst
        xor         esi, esi
        pxor        mm0, mm0
        movq        mm3, [mmmask_0003]
        movq        mm4, [mmmask_0004]
        movq        mm5, [mmmask_0005]
        mov         edi, HALF_WIDTH

        convyuv422topi:
        movd        mm1, [eax+esi]
            mov         ebx, eax
            add         ebx, src_pitch2
            movd        mm2, [ebx+esi]
            movd        [ecx+esi], mm1
            sub         ebx, src_pitch

            punpcklbw   mm1, mm0
            movq        mm6, [ebx+esi]
            pmullw      mm1, mm5
            add         ebx, src_pitch2
            punpcklbw   mm2, mm0
            movq        mm7, [ebx+esi]
            pmullw      mm2, mm3
            paddusw     mm2, mm1
            paddusw     mm2, mm4
            psrlw       mm2, 0x03
            packuswb    mm2, mm0

            mov         edx, ecx
            add         edx, dst_pitch2
            movd        [edx+esi], mm2
            sub         edx, dst_pitch

            movd        [edx+esi], mm6
            punpcklbw   mm6, mm0
            pmullw      mm6, [mmmask_0007]
            punpcklbw   mm7, mm0
            paddusw     mm7, mm6
            paddusw     mm7, mm4
            psrlw       mm7, 0x03
            packuswb    mm7, mm0

            add         edx, dst_pitch2
            add         esi, 0x04
            cmp         esi, edi
            movd        [edx+esi-4], mm7

            jl          convyuv422topi

            add         eax, src_pitch2
            add         ecx, dst_pitch4
            xor         esi, esi

            convyuv422i:
        movd        mm1, [eax+esi]
            punpcklbw   mm1, mm0
            movq        mm6, mm1
            mov         ebx, eax
            sub         ebx, src_pitch2
            movd        mm3, [ebx+esi]
            pmullw      mm1, [mmmask_0007]
            punpcklbw   mm3, mm0
            paddusw     mm3, mm1
            paddusw     mm3, mm4
            psrlw       mm3, 0x03
            packuswb    mm3, mm0

            add         ebx, src_pitch
            movq        mm1, [ebx+esi]
            add         ebx, src_pitch2
            movd        [ecx+esi], mm3

            movq        mm3, [mmmask_0003]
            movd        mm2, [ebx+esi]

            punpcklbw   mm1, mm0
            pmullw      mm1, mm3
            punpcklbw   mm2, mm0
            movq        mm7, mm2
            pmullw      mm2, mm5
            paddusw     mm2, mm1
            paddusw     mm2, mm4
            psrlw       mm2, 0x03
            packuswb    mm2, mm0

            pmullw      mm6, mm5
            mov         edx, ecx
            add         edx, dst_pitch
            movd        [edx+esi], mm2

            add         ebx, src_pitch
            movd        mm2, [ebx+esi]
            punpcklbw   mm2, mm0
            pmullw      mm2, mm3
            paddusw     mm2, mm6
            paddusw     mm2, mm4
            psrlw       mm2, 0x03
            packuswb    mm2, mm0

            pmullw      mm7, [mmmask_0007]
            add         edx, dst_pitch
            add         ebx, src_pitch
            movd        [edx+esi], mm2

            movd        mm2, [ebx+esi]
            punpcklbw   mm2, mm0
            paddusw     mm2, mm7
            paddusw     mm2, mm4
            psrlw       mm2, 0x03
            packuswb    mm2, mm0

            add         edx, dst_pitch
            add         esi, 0x04
            cmp         esi, edi
            movd        [edx+esi-4], mm2

            jl          convyuv422i
            add         eax, src_pitch2
            add         ecx, dst_pitch4
            xor         esi, esi
            dec         INTERLACED_HEIGHT
            jnz         convyuv422i

            convyuv422bottomi:
        movd        mm1, [eax+esi]
            movq        mm6, mm1
            punpcklbw   mm1, mm0
            mov         ebx, eax
            sub         ebx, src_pitch2
            movd        mm3, [ebx+esi]
            punpcklbw   mm3, mm0
            pmullw      mm1, [mmmask_0007]
            paddusw     mm3, mm1
            paddusw     mm3, mm4
            psrlw       mm3, 0x03
            packuswb    mm3, mm0

            add         ebx, src_pitch
            movq        mm1, [ebx+esi]
            punpcklbw   mm1, mm0
            movd        [ecx+esi], mm3

            pmullw      mm1, [mmmask_0003]
            add         ebx, src_pitch2
            movd        mm2, [ebx+esi]
            movq        mm7, mm2
            punpcklbw   mm2, mm0
            pmullw      mm2, mm5
            paddusw     mm2, mm1
            paddusw     mm2, mm4
            psrlw       mm2, 0x03
            packuswb    mm2, mm0

            mov         edx, ecx
            add         edx, dst_pitch
            movd        [edx+esi], mm2
            add         edx, dst_pitch
            movd        [edx+esi], mm6

            add         edx, dst_pitch
            add         esi, 0x04
            cmp         esi, edi
            movd        [edx+esi-4], mm7

            jl          convyuv422bottomi

            emms
    }
}
void conv420to422P_MMX(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch, int width, int height)
{
    int src_pitch2 = src_pitch<<1;
    int dst_pitch2 = dst_pitch<<1;
    int dst_pitch4 = dst_pitch<<2;
    int INTERLACED_HEIGHT = (height>>2) - 2;
    int PROGRESSIVE_HEIGHT = (height>>1) - 2;
    int HALF_WIDTH = width>>1; // chroma width

    __asm
    {
        mov         eax, [src] // eax = src
        mov         ebx, [dst] // ebx = dst
        mov         ecx, ebx   // ecx = dst
        add         ecx, dst_pitch // ecx = dst + dst_pitch
        xor         esi, esi
        movq        mm3, [mmmask_0003]
        pxor        mm0, mm0
        movq        mm4, [mmmask_0002]

        mov         edx, eax // edx = src
        add         edx, src_pitch // edx = src + src_pitch
        mov         edi, HALF_WIDTH

        convyuv422topp:
        movd        mm1, [eax+esi]
            movd        mm2, [edx+esi]
            movd        [ebx+esi], mm1
            punpcklbw   mm1, mm0
            pmullw      mm1, mm3
            paddusw     mm1, mm4
            punpcklbw   mm2, mm0
            paddusw     mm2, mm1
            psrlw       mm2, 0x02
            packuswb    mm2, mm0

            add         esi, 0x04
            cmp         esi, edi
            movd        [ecx+esi-4], mm2
            jl          convyuv422topp

            add         eax, src_pitch
            add         ebx, dst_pitch2
            add         ecx, dst_pitch2
            xor         esi, esi

            convyuv422p:
        movd        mm1, [eax+esi]

            punpcklbw   mm1, mm0
            mov         edx, eax // edx = src

            pmullw      mm1, mm3
            sub         edx, src_pitch

            movd        mm5, [edx+esi]
            add         edx, src_pitch2
            movd        mm2, [edx+esi]

            punpcklbw   mm5, mm0
            punpcklbw   mm2, mm0
            paddusw     mm5, mm1
            paddusw     mm2, mm1
            paddusw     mm5, mm4
            paddusw     mm2, mm4
            psrlw       mm5, 0x02
            psrlw       mm2, 0x02
            packuswb    mm5, mm0
            packuswb    mm2, mm0

            add         esi, 0x04
            cmp         esi, edi
            movd        [ebx+esi-4], mm5
            movd        [ecx+esi-4], mm2

            jl          convyuv422p

            add         eax, src_pitch
            add         ebx, dst_pitch2
            add         ecx, dst_pitch2
            xor         esi, esi
            dec         PROGRESSIVE_HEIGHT
            jnz         convyuv422p

            mov         edx, eax
            sub         edx, src_pitch

            convyuv422bottomp:
        movd        mm1, [eax+esi]
            movd        mm5, [edx+esi]
            punpcklbw   mm5, mm0
            movd        [ecx+esi], mm1

            punpcklbw   mm1, mm0
            pmullw      mm1, mm3
            paddusw     mm5, mm1
            paddusw     mm5, mm4
            psrlw       mm5, 0x02
            packuswb    mm5, mm0

            add         esi, 0x04
            cmp         esi, edi
            movd        [ebx+esi-4], mm5
            jl          convyuv422bottomp

            emms
    }
}


void conv420to422P_iSSE(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    int src_pitch2 = src_pitch<<1;
    int dst_pitch2 = dst_pitch<<1;
    int PROGRESSIVE_HEIGHT = (height>>1) - 2;
    int HALF_WIDTH = width>>1; // chroma width

    __asm
    {
        mov         eax, [src] // eax = src
        mov         ebx, [dst] // ebx = dst
        mov         ecx, ebx   // ecx = dst
        add         ecx, dst_pitch // ecx = dst + dst_pitch
        mov         edx, eax // edx = src
        add         edx, src_pitch // edx = src + src_pitch
        xor         esi, esi
        mov         edi, HALF_WIDTH
        movq        mm6, mmmask_0101
        movq        mm7, mm6

        convyuv422topp:
        movq        mm0, [eax+esi]
            movq        mm1, [edx+esi]
            movq        [ebx+esi], mm0
            psubusb     mm1, mm7
            pavgb       mm1, mm0
            pavgb       mm1, mm0
            movq        [ecx+esi], mm1
            add         esi, 0x08
            cmp         esi, edi
            jl          convyuv422topp
            add         eax, src_pitch
            add         ebx, dst_pitch2
            add         ecx, dst_pitch2
            xor         esi, esi

            convyuv422p:
        movq        mm0, [eax+esi]
            mov         edx, eax // edx = src
            movq        mm1, mm0
            sub         edx, src_pitch
            movq        mm2, [edx+esi]
            add         edx, src_pitch2
            movq        mm3, [edx+esi]
            psubusb     mm2, mm6
            psubusb     mm3, mm7
            pavgb       mm2, mm0
            pavgb       mm3, mm1
            pavgb       mm2, mm0
            pavgb       mm3, mm1
            movq        [ebx+esi], mm2
            movq        [ecx+esi], mm3
            add         esi, 0x08
            cmp         esi, edi
            jl          convyuv422p
            add         eax, src_pitch
            add         ebx, dst_pitch2
            add         ecx, dst_pitch2
            xor         esi, esi
            dec         PROGRESSIVE_HEIGHT
            jnz         convyuv422p
            mov         edx, eax
            sub         edx, src_pitch

            convyuv422bottomp:
        movq        mm0, [eax+esi]
            movq        mm1, [edx+esi]
            movq        [ecx+esi], mm0
            psubusb     mm1, mm7
            pavgb       mm1, mm0
            pavgb       mm1, mm0
            movq        [ebx+esi], mm1
            add         esi, 0x08
            cmp         esi, edi
            jl          convyuv422bottomp
            emms
    }
}

void conv422to444_MMX(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    int HALF_WIDTH_D8 = (width>>1)-8;

    __asm
    {
        mov         eax, [src]  // eax = src
        mov         ebx, [dst]  // ebx = dst
        mov         edi, height // edi = height
        mov         ecx, HALF_WIDTH_D8  // ecx = (width>>1)-8
        movq        mm1, mmmask_0001    // mm1 = 1's
        pxor        mm0, mm0    // mm0 = 0's

        convyuv444init:
        movq        mm7, [eax]  // mm7 = hgfedcba
            xor         esi, esi    // esi = 0

            convyuv444:
        movq        mm2, mm7    // mm2 = hgfedcba
            movq        mm7, [eax+esi+8] // mm7 = ponmlkji
            movq        mm3, mm2    // mm3 = hgfedcba
            movq        mm4, mm7    // mm4 = ponmlkji

            psrlq       mm3, 8      // mm3 = 0hgfedcb
            psllq       mm4, 56     // mm4 = i0000000
            por         mm3, mm4    // mm3 = ihgfedcb

            movq        mm4, mm2    // mm4 = hgfedcba
            movq        mm5, mm3    // mm5 = ihgfedcb

            punpcklbw   mm4, mm0    // 0d0c0b0a
            punpcklbw   mm5, mm0    // 0e0d0c0b

            movq        mm6, mm4    // mm6 = 0d0c0b0a
            paddusw     mm4, mm1
            paddusw     mm4, mm5
            psrlw       mm4, 1      // average mm4/mm5 (d/e,c/d,b/c,a/b)
            psllq       mm4, 8      // mm4 = z0z0z0z0
            por         mm4, mm6    // mm4 = zdzczbza

            punpckhbw   mm2, mm0    // 0h0g0f0e
            punpckhbw   mm3, mm0    // 0i0h0g0f

            movq        mm6, mm2    // mm6 = 0h0g0f0e
            paddusw     mm2, mm1
            paddusw     mm2, mm3
            psrlw       mm2, 1      // average mm2/mm3 (h/i,g/h,f/g,e/f)
            psllq       mm2, 8      // mm2 = z0z0z0z0
            por         mm2, mm6    // mm2 = zhzgzfze

            movq        [ebx+esi*2], mm4    // store zdzczbza
            movq        [ebx+esi*2+8], mm2  // store zhzgzfze

            add         esi, 8
            cmp         esi, ecx
            jl          convyuv444  // loop back if not to last 8 pixels

            movq        mm2, mm7    // mm2 = ponmlkji

            punpcklbw   mm2, mm0    // mm2 = 0l0k0j0i
            punpckhbw   mm7, mm0    // mm7 = 0p0o0n0m

            movq        mm3, mm2    // mm3 = 0l0k0j0i
            movq        mm4, mm7    // mm4 = 0p0o0n0m

            psrlq       mm2, 16     // mm2 = 000l0k0j
            psllq       mm4, 48     // mm4 = 0m000000
            por         mm2, mm4    // mm2 = 0m0l0k0j

            paddusw     mm2, mm1
            paddusw     mm2, mm3
            psrlw       mm2, 1      // average mm2/mm3 (m/l,l/k,k/j,j/i)
            psllq       mm2, 8      // mm2 = z0z0z0z0
            por         mm2, mm3    // mm2 = zlzkzjzi

            movq        mm6, mm7    // mm6 = 0p0o0n0m
            movq        mm4, mm7    // mm4 = 0p0o0n0m

            psrlq       mm6, 48     // mm6 = 0000000p
            psrlq       mm4, 16     // mm4 = 000p0o0n
            psllq       mm6, 48     // mm6 = 0p000000
            por         mm4,mm6     // mm4 = 0p0p0o0n

            paddusw     mm4, mm1
            paddusw     mm4, mm7
            psrlw       mm4, 1      // average mm4/mm7 (p/p,p/o,o/n,n/m)
            psllq       mm4, 8      // mm4 = z0z0z0z0
            por         mm4, mm7    // mm4 = zpzoznzm

            movq        [ebx+esi*2], mm2    // store mm2
            movq        [ebx+esi*2+8], mm4  // store mm4

            add         eax, src_pitch
            add         ebx, dst_pitch
            dec         edi
            jnz         convyuv444init

            emms
    }
}

void conv422to444_iSSE(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height)
{
    int HALF_WIDTH_D8 = (width>>1)-8;

    __asm
    {
        mov         eax, [src]  // eax = src
        mov         ebx, [dst]  // ebx = dst
        mov         edi, height // edi = height
        mov         ecx, HALF_WIDTH_D8  // ecx = (width>>1)-8
        mov         edx, src_pitch
        xor         esi, esi    // esi = 0
        movq        mm7, lastmask

        xloop:
        movq        mm0, [eax+esi]  // mm7 = hgfedcba
            movq        mm1, [eax+esi+1]// mm1 = ihgfedcb
            pavgb       mm1, mm0
            movq        mm2, mm0
            punpcklbw   mm0,mm1
            punpckhbw   mm2,mm1

            movq        [ebx+esi*2], mm0    // store mm0
            movq        [ebx+esi*2+8], mm2  // store mm2

            add         esi, 8
            cmp         esi, ecx
            jl          xloop   // loop back if not to last 8 pixels

            movq        mm0, [eax+esi]  // mm7 = hgfedcba
            movq        mm1, mm0        // mm1 = hgfedcba
            movq        mm2, mm0        // mm2 = hgfedcba
            psrlq       mm1, 8          // mm1 = 0hgfedcb
            pand        mm2, mm7        // mm2 = h0000000
            por         mm1, mm2        // mm1 = hhgfedcb
            pavgb       mm1, mm0
            movq        mm2, mm0
            punpcklbw   mm0,mm1
            punpckhbw   mm2,mm1

            movq        [ebx+esi*2], mm0    // store mm0
            movq        [ebx+esi*2+8], mm2  // store mm2

            add         eax, edx
            add         ebx, dst_pitch
            xor         esi, esi
            dec         edi
            jnz         xloop
            emms
    }
}

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


