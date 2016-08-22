

/*****************************************************************************
 * mcmmx.c : Created by vlad59 - 04/05/2002
 * Based on Jackey's Form_Prediction code (GetPic.c)
 *****************************************************************************
 * I only made some copy-paste of Jackey's code and some clean up
 *****************************************************************************/

#include "mc.h"
#include <emmintrin.h>

//#pragma warning( disable : 4799 )

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

static __forceinline void storel(uint8_t* p, const __m128i& x)
{
    _mm_storel_epi64(reinterpret_cast<__m128i*>(p), x);
}

static __forceinline void storeu(uint8_t* p, const __m128i& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(p), x);
}


void MC_put_8_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq    mm0, [eax]
        movq    [ebx], mm0
        add     eax, [stride]
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        *reinterpret_cast<uint64_t*>(dest) = *reinterpret_cast<const uint64_t*>(ref);
        dest += stride;
        ref += stride;
    } while (--height > 0);
}
#endif

void MC_put_16_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq    mm0, [eax]
        movq    mm1, [eax+8]
        add     eax, [stride]
        movq    [ebx], mm0
        movq    [ebx+8], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storeu(dest, loadu(ref));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

static const __int64 mmmask_0001 = 0x0001000100010001;
static const __int64 mmmask_0002 = 0x0002000200020002;
static const __int64 mmmask_0003 = 0x0003000300030003;
static const __int64 mmmask_0006 = 0x0006000600060006;

void MC_avg_8_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0001]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [ebx]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storel(dest, avgu8(loadl(ref), loadl(dest)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_avg_16_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0001]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [ebx]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [ebx+8]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3


        add     eax, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storeu(dest, avgu8(loadu(ref), loadu(dest)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_put_x8_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0001]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storel(dest, avgu8(loadl(ref), loadl(ref + 1)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_put_y8_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0001]
        mov         eax, [ref]
        mov         ecx, eax
        add         ecx, [offs]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [ecx]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storel(dest, avgu8(loadl(ref), loadl(ref + offs)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_put_x16_mmx(uint8_t * dest, const uint8_t * ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0001]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [eax+9]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storeu(dest, avgu8(loadu(ref), loadu(ref + 1)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_put_y16_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0001]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [ecx]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [ecx+8]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        psrlw       mm1, 1
        psrlw       mm3, 1

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storeu(dest, avgu8(loadu(ref), loadu(ref + offs)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif


void MC_avg_x8_mmx(uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0003]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ebx]

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        movq        mm6, mm5
        punpcklbw   mm5, mm0
        punpckhbw   mm6, mm0

        psllw       mm5, 1
        psllw       mm6, 1

        paddsw      mm1, mm5
        paddsw      mm3, mm6

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storel(dest, avgu8(avgu8(loadl(ref), loadl(ref + 1)), loadl(dest)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_avg_y8_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0003]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [ecx]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ebx]

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        movq        mm6, mm5
        punpcklbw   mm5, mm0
        punpckhbw   mm6, mm0

        psllw       mm5, 1
        psllw       mm6, 1

        paddsw      mm1, mm5
        paddsw      mm3, mm6

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storel(dest, avgu8(avgu8(loadl(ref), loadl(ref + offs)), loadl(dest)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_avg_x16_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0003]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ebx]

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        movq        mm6, mm5
        punpcklbw   mm5, mm0
        punpckhbw   mm6, mm0

        psllw       mm5, 1
        psllw       mm6, 1

        paddsw      mm1, mm5
        paddsw      mm3, mm6

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [eax+9]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ebx+8]

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        movq        mm6, mm5
        punpcklbw   mm5, mm0
        punpckhbw   mm6, mm0

        psllw       mm5, 1
        psllw       mm6, 1

        paddsw      mm1, mm5
        paddsw      mm3, mm6

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storeu(dest, avgu8(avgu8(loadu(ref), loadu(ref + 1)), loadu(dest)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

void MC_avg_y16_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0003]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [ecx]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ebx]

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        movq        mm6, mm5
        punpcklbw   mm5, mm0
        punpckhbw   mm6, mm0

        psllw       mm5, 1
        psllw       mm6, 1

        paddsw      mm1, mm5
        paddsw      mm3, mm6

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [ecx+8]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ebx+8]

        paddsw      mm1, mm7
        paddsw      mm3, mm7

        movq        mm6, mm5
        punpcklbw   mm5, mm0
        punpckhbw   mm6, mm0

        psllw       mm5, 1
        psllw       mm6, 1

        paddsw      mm1, mm5
        paddsw      mm3, mm6

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]
        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
{
    do {
        storeu(dest, avgu8(avgu8(loadu(ref), loadu(ref + offs)), loadu(dest)));
        ref += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

alignas(16) static const uint8_t one_x16[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

// Accurate function
void MC_put_xy8_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0002]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ecx]
        paddsw      mm1, mm7

        movq        mm6, [ecx+1]
        paddsw      mm3, mm7

        movq        mm2, mm5
        movq        mm4, mm6

        punpcklbw   mm2, mm0
        punpckhbw   mm5, mm0

        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        paddsw      mm2, mm4
        paddsw      mm5, mm6

        paddsw      mm1, mm2
        paddsw      mm3, mm5

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]

        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
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
        __m128i t0 = _mm_or_si128(_mm_xor_si128(r0, r3), _mm_xor_si128(r1, r2));
        t0 = _mm_and_si128(_mm_and_si128(t0, _mm_xor_si128(avg0, avg1)), one);

        storel(dest, _mm_subs_epu8(avgu8(avg0, avg1), t0));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

// Accurate function
void MC_put_xy16_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0002]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ecx]
        paddsw      mm1, mm7

        movq        mm6, [ecx+1]
        paddsw      mm3, mm7

        movq        mm2, mm5
        movq        mm4, mm6

        punpcklbw   mm2, mm0
        punpckhbw   mm5, mm0

        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        paddsw      mm2, mm4
        paddsw      mm5, mm6

        paddsw      mm1, mm2
        paddsw      mm3, mm5

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [eax+9]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ecx+8]
        paddsw      mm1, mm7

        movq        mm6, [ecx+9]
        paddsw      mm3, mm7

        movq        mm2, mm5
        movq        mm4, mm6

        punpcklbw   mm2, mm0
        punpckhbw   mm5, mm0

        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        paddsw      mm2, mm4
        paddsw      mm5, mm6

        paddsw      mm1, mm2
        paddsw      mm3, mm5

        psrlw       mm1, 2
        psrlw       mm3, 2

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]

        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
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
        __m128i t0 = _mm_or_si128(_mm_xor_si128(r0, r3), _mm_xor_si128(r1, r2));
        t0 = _mm_and_si128(_mm_and_si128(t0, _mm_xor_si128(avg0, avg1)), one);

        storeu(dest, _mm_subs_epu8(avgu8(avg0, avg1), t0));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}
#endif

// Accurate function
void MC_avg_xy8_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0006]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ecx]
        paddsw      mm1, mm7

        movq        mm6, [ecx+1]
        paddsw      mm3, mm7

        movq        mm2, mm5
        movq        mm4, mm6

        punpcklbw   mm2, mm0
        punpckhbw   mm5, mm0

        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        paddsw      mm2, mm4
        paddsw      mm5, mm6

        paddsw      mm1, mm2
        paddsw      mm3, mm5

        movq        mm6, [ebx]

        movq        mm4, mm6
        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        psllw       mm4, 2
        psllw       mm6, 2

        paddsw      mm1, mm4
        paddsw      mm3, mm6

        psrlw       mm1, 3
        psrlw       mm3, 3

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx], mm1
        add     ebx, [stride]

        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
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

        __m128i t0 = _mm_or_si128(_mm_xor_si128(r0, r3), _mm_xor_si128(r1, r2));
        t0 = _mm_and_si128(_mm_and_si128(t0, _mm_xor_si128(avg0, avg1)), one);
        t0 = _mm_subs_epu8(avgu8(avg0, avg1), t0);
        storel(dest, avgu8(t0, loadl(dest)));

        ref += stride;
        ro += stride;
        dest += stride;
    } while (--height > 0);
}
#endif


// Accurate function
void MC_avg_xy16_mmx (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height)
#if 0
{
    __asm {
        pxor        mm0, mm0
        movq        mm7, [mmmask_0006]
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq        mm1, [eax]
        movq        mm2, [eax+1]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ecx]
        paddsw      mm1, mm7

        movq        mm6, [ecx+1]
        paddsw      mm3, mm7

        movq        mm2, mm5
        movq        mm4, mm6

        punpcklbw   mm2, mm0
        punpckhbw   mm5, mm0

        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        paddsw      mm2, mm4
        paddsw      mm5, mm6

        paddsw      mm1, mm2
        paddsw      mm3, mm5

        movq        mm6, [ebx]

        movq        mm4, mm6
        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        psllw       mm4, 2
        psllw       mm6, 2

        paddsw      mm1, mm4
        paddsw      mm3, mm6

        psrlw       mm1, 3
        psrlw       mm3, 3

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        movq        [ebx], mm1

        movq        mm1, [eax+8]
        movq        mm2, [eax+9]
        movq        mm3, mm1
        movq        mm4, mm2

        punpcklbw   mm1, mm0
        punpckhbw   mm3, mm0

        punpcklbw   mm2, mm0
        punpckhbw   mm4, mm0

        paddsw      mm1, mm2
        paddsw      mm3, mm4

        movq        mm5, [ecx+8]
        paddsw      mm1, mm7

        movq        mm6, [ecx+9]
        paddsw      mm3, mm7

        movq        mm2, mm5
        movq        mm4, mm6

        punpcklbw   mm2, mm0
        punpckhbw   mm5, mm0

        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        paddsw      mm2, mm4
        paddsw      mm5, mm6

        paddsw      mm1, mm2
        paddsw      mm3, mm5

        movq        mm6, [ebx+8]

        movq        mm4, mm6
        punpcklbw   mm4, mm0
        punpckhbw   mm6, mm0

        psllw       mm4, 2
        psllw       mm6, 2

        paddsw      mm1, mm4
        paddsw      mm3, mm6

        psrlw       mm1, 3
        psrlw       mm3, 3

        packuswb    mm1, mm0
        packuswb    mm3, mm0

        psllq       mm3, 32
        por         mm1, mm3

        add     eax, [stride]
        add     ecx, [stride]
        movq    [ebx+8], mm1
        add     ebx, [stride]

        dec     edi
        cmp     edi, 0x00
        jg      mc0

        //emms
    }
}
#else
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

    __m128i t0 = _mm_or_si128(_mm_xor_si128(r0, r3), _mm_xor_si128(r1, r2));
    t0 = _mm_and_si128(_mm_and_si128(t0, _mm_xor_si128(avg0, avg1)), one);
    t0 = _mm_subs_epu8(avgu8(avg0, avg1), t0);
    storeu(dest, avgu8(t0, loadu(dest)));

    ref += stride;
    ro += stride;
    dest += stride;
} while (--height > 0);

}
#endif
