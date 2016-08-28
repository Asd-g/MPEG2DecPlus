/*
A rewite of Deblock filter for support all planar 8bit YUV.
Almost same as tp7's version.
*/

#include "AvisynthAPI.h"


static inline int sat(int x, int min, int max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}


static inline int absdiff(int x, int y)
{
    return x > y ? x - y : y - x;
}


static const int alphas[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 4, 4,
    5, 6, 7, 8, 9, 10,
    12, 13, 15, 17, 20,
    22, 25, 28, 32, 36,
    40, 45, 50, 56, 63,
    71, 80, 90, 101, 113,
    127, 144, 162, 182,
    203, 226, 255, 255
};

static const int betas[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 2,
    2, 3, 3, 3, 3, 4,
    4, 4, 6, 6,
    7, 7, 8, 8, 9, 9,
    10, 10, 11, 11, 12,
    12, 13, 13, 14, 14,
    15, 15, 16, 16, 17,
    17, 18, 18
};

static const int cs[52] = {
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 3,
    3, 3, 4, 4, 5, 5,
    6, 7, 8, 8, 10,
    11, 12, 13, 15, 17
};


static void deblock_h_edge(uint8_t *srcp, int srcPitch, const int alpha, const int beta, const int c0)
{
    uint8_t *sq0 = srcp;
    uint8_t *sq1 = srcp + srcPitch;
    uint8_t *sq2 = srcp + 2 * srcPitch;
    uint8_t *sp0 = srcp - srcPitch;
    uint8_t *sp1 = srcp - 2 * srcPitch;
    uint8_t *sp2 = srcp - 3 * srcPitch;

    for (int i = 0; i < 4; ++i) {
        if ((absdiff(sp0[i], sq0[i]) < alpha)
                && (absdiff(sp1[i], sp0[i]) < beta)
                && (absdiff(sq0[i], sq1[i]) < beta)) {
            int ap = absdiff(sp2[i], sp0[i]);
            int aq = absdiff(sq2[i], sq0[i]);

            int c = c0;
            if (aq < beta) ++c;
            if (ap < beta) ++c;

            int delta = sat((((sq0[i] - sp0[i]) * 4) + (sp1[i] - sq1[i]) + 4) >> 3, -c, c);
            int avg = (sp0[i] + sq0[i] + 1) / 2;
            int deltap1 = sat((sp2[i] + avg - sp1[i] * 2) / 2, -c0, c0);
            int deltaq1 = sat((sq2[i] + avg - sq1[i] * 2) / 2, -c0, c0);

            if (ap < beta) sp1[i] = sp1[i] + deltap1;
            sp0[i] = sat(sp0[i] + delta, 0, 255);
            sq0[i] = sat(sq0[i] - delta, 0, 255);
            if (aq < beta) sq1[i] = sq1[i] + deltaq1;
        }
    }
}


static void deblock_v_edge(uint8_t *s, int srcPitch, const int alpha, const int beta, const int c0)
{
    for (int i = 0; i < 4; ++i) {
        if ((absdiff(s[0], s[-1]) < alpha)
                && (absdiff(s[0], s[1]) < beta)
                && (absdiff(s[-1], s[-2]) < beta)) {
            int ap = absdiff(s[0], s[2]);
            int aq = absdiff(s[-3], s[-1]);

            int c = c0;
            if (aq < beta) ++c;
            if (ap < beta) ++c;

            int delta = sat((((s[0] - s[-1]) * 4) + (s[-2] - s[1]) + 4) / 8, -c, c);
            int avg = (s[0] + s[-1] + 1) / 2;
            int deltaq1 = sat((s[2] + avg - s[1] * 2) / 2, -c0, c0);
            int deltap1 = sat((s[-3] + avg - s[-2] * 2) / 2, -c0, c0);

            if (aq < beta) s[-2] = (s[-2] + deltap1);
            s[-1] = sat(s[-1] + delta, 0, 255);
            s[0] = sat(s[0] - delta, 0, 255);
            if (ap < beta) s[1] = (s[1] + deltaq1);
        }
        s += srcPitch;
    }
}


static void deblock_picture(uint8_t *srcp, int srcPitch, int w, int h,
                            const int alpha, const int beta, const int c0)
{
    for (int j = 0; j < h; j += 4 ) {
        for (int i = 0; i < w; i += 4) {
            if ( j > 0 )
                deblock_h_edge(srcp + i, srcPitch, alpha, beta, c0);
            if ( i > 0 )
                deblock_v_edge(srcp + i, srcPitch, alpha, beta, c0);
        }
        srcp += 4 * srcPitch;
    }
}


Deblock::Deblock(PClip _child, int q, int aoff, int boff, IScriptEnvironment* env) :
    GenericVideoFilter(_child)
{
    q = sat(q, 0, 51);
    int a = sat(q + aoff, 0, 51);
    int b = sat(q + boff, 0, 51);
    alpha = alphas[a];
    beta = betas[b];
    c0 = cs[a];

    if (!vi.IsPlanar() || !vi.IsYUV())
        env->ThrowError("Deblock: need planar yuv input.");
    if (vi.IsYUVA()) {
        env->ThrowError("Deblock: alpha is not supported.");
    }
    if ((vi.width & 7) || (vi.height & 7))
        env->ThrowError("Deblock : width and height must be mod 8");

    numPlanes = vi.IsY8() ? 1 : 3;
}


PVideoFrame __stdcall Deblock::GetFrame(int n, IScriptEnvironment *env)
{
    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);

    static const int planes[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
    for (int p = 0; p < numPlanes; ++p) {
        const int plane = planes[p];
        deblock_picture(src->GetWritePtr(plane), src->GetPitch(plane),
                        src->GetRowSize(plane), src->GetHeight(plane),
                        alpha, beta, c0);
    }

    return src;
}


AVSValue __cdecl Deblock::create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    int quant = args[1].AsInt(25);
    int aoffset = args[2].AsInt(0);
    int boffset = args[3].AsInt(0);
    return new Deblock(args[0].AsClip(), quant, aoffset, boffset, env);
}
