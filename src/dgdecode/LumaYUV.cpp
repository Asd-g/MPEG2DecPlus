
// LumaYUV:
// a rewrite of LumaYV12 to eliminate inline ASM and support all planar 8bit YUV
// by OKA Motofumi - August 24, 2016



#include <algorithm>
#include <emmintrin.h>
#include "AvisynthAPI.h"


#if 0
// C implementation
// offset = lumoff, gain = int(lumgain * 128)
static void
main_proc_c(const uint8_t* srcp, uint8_t* dstp, const int spitch,
    const int dpitch,const int width, const int height,
    const int offset, const int gain) noexcept
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int val = srcp[x] * gain + 64 / 128 + offset;
            dstp[x] = val < 0 ? 0 : val > 255 ? 255 : val;
        }
        srcp += spitch;
        dstp += dpitch;
    }
}
#endif

enum {
    NORMAL,
    NO_GAIN_NEGATIVE_OFFSET,
    NO_GAIN_POSITIVE_OFFSET,
};


template <int PATTERN>
static void
main_proc_sse2(const uint8_t* srcp, uint8_t* dstp, const int spitch,
    const int dpitch, const int width, const int height,
    const int16_t* offsets, const int16_t* gains) noexcept
{
    const __m128i offs = _mm_load_si128(reinterpret_cast<const __m128i*>(offsets));

    __m128i gain, zero, round;
    if (PATTERN == NORMAL) {
        gain = _mm_load_si128(reinterpret_cast<const __m128i*>(gains));
        round = _mm_set1_epi16(64);
        zero = _mm_setzero_si128();
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; x += 16) {
            __m128i s0 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));

            if (PATTERN == NORMAL) {
                __m128i s1 = _mm_unpackhi_epi8(s0, zero);
                s0 = _mm_unpacklo_epi8(s0, zero);

                s0 = _mm_mullo_epi16(s0, gain);
                s1 = _mm_mullo_epi16(s1, gain);

                s0 = _mm_adds_epu16(s0, round);
                s1 = _mm_adds_epu16(s1, round);

                s0 = _mm_srli_epi16(s0, 7);
                s1 = _mm_srli_epi16(s1, 7);

                s0 = _mm_add_epi16(s0, offs);
                s1 = _mm_add_epi16(s1, offs);

                s0 = _mm_packus_epi16(s0, s1);

            } else if (PATTERN == NO_GAIN_NEGATIVE_OFFSET) {
                s0 = _mm_subs_epu8(s0, offs);

            } else {
                s0 = _mm_adds_epu8(s0, offs);
            }

            _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x), s0);
        }
        srcp += spitch;
        dstp += dpitch;
    }

}


LumaYUV::LumaYUV(PClip c, int16_t offset, int16_t gain, IScriptEnvironment* env) :
    GenericVideoFilter(c)
{
    numPlanes = vi.IsY8() ? 1 : vi.IsYUVA() ? 4 : 3;

    auto e2 = static_cast<IScriptEnvironment2*>(env);
    void* p = e2->Allocate(16 * sizeof(int16_t), 16, AVS_NORMAL_ALLOC);
    if (!p) {
        env->ThrowError("LumaYUV: failed to create masks.");
    }
    env->AtExit(
        [](void* p, IScriptEnvironment* e) {
            static_cast<IScriptEnvironment2*>(e)->Free(p);
            p = nullptr;
        }, p);

    offsetMask = reinterpret_cast<int16_t*>(p);
    gainMask = offsetMask + 8;

    if (gain != 128) {
        mainProc = main_proc_sse2<NORMAL>;
        std::fill_n(offsetMask, 8, offset);
        std::fill_n(gainMask, 8, gain);
    } else {
        if (offset < 0) {
            offset = -offset;
            mainProc = main_proc_sse2<NO_GAIN_NEGATIVE_OFFSET>;
        } else {
            mainProc = main_proc_sse2<NO_GAIN_POSITIVE_OFFSET>;
        }
        std::fill_n(reinterpret_cast<uint8_t*>(offsetMask), 16, static_cast<uint8_t>(offset));
    }
}


PVideoFrame __stdcall LumaYUV::GetFrame(int n, IScriptEnvironment* env)
{
    auto src = child->GetFrame(n, env);
    auto dst = env->NewVideoFrame(vi);

    static const int planes[] = { PLANAR_U, PLANAR_V, PLANAR_A };
    for (int p = 0; p < numPlanes - 1; ++p) {
        const int plane = planes[p];
        env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane),
            src->GetReadPtr(plane), src->GetPitch(plane),
            dst->GetRowSize(plane), dst->GetHeight(plane));
    }

    mainProc(src->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_Y),
        src->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_Y),vi.width,
        vi.height, offsetMask, gainMask);

    return dst;
}


AVSValue __cdecl LumaYUV::create(AVSValue args, void*, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();
    const VideoInfo& vi = clip->GetVideoInfo();
    if (!vi.IsPlanar() || vi.IsRGB() || vi.ComponentSize() != 1) {
        env->ThrowError("LumaYUV: unsupported format.");
    }
    int off = std::min(std::max(args[1].AsInt(0), -255), 255);
    float gain = std::min(std::max(args[2].AsFloatf(1.0f), 0.0f), 2.0f);
    gain *= 128;

    return new LumaYUV(clip, static_cast<int16_t>(off), static_cast<int16_t>(gain), env);
}
