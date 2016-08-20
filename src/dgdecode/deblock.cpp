#include "AvisynthAPI.h"


static inline int sat(int x, int min, int max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}


Deblock::Deblock(PClip _child, int q, int aOff, int bOff, IScriptEnvironment* env) :
    GenericVideoFilter(_child)
{
    nQuant = sat(q, 0, 51);
    nAOffset = sat(aOff, -nQuant, 51 - nQuant);
    nBOffset = sat(bOff, -nQuant, 51 - nQuant);
    nWidth = vi.width;
    nHeight = vi.height;

    if (!vi.IsYV12())
        env->ThrowError("Deblock : need YV12 input");
    if (( nWidth & 7 ) || ( nHeight & 7 ))
        env->ThrowError("Deblock : width and height must be mod 8");
}


const int alphas[52] = {
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

const int betas[52] = {
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

const int cs[52] = {
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


static void deblock_h_edge(unsigned char *srcp, int srcPitch, int ia, int ib)
{
    int alpha = alphas[ia];
    int beta = betas[ib];
    int c, c0 = cs[ia];
    unsigned char *sq0 = srcp;
    unsigned char *sq1 = srcp + srcPitch;
    unsigned char *sq2 = srcp + 2 * srcPitch;
    unsigned char *sp0 = srcp - srcPitch;
    unsigned char *sp1 = srcp - 2 * srcPitch;
    unsigned char *sp2 = srcp - 3 * srcPitch;
    int delta, ap, aq, deltap1, deltaq1;

    for ( int i = 0; i < 4; i ++ )
    {
        if (( abs(sp0[i] - sq0[i]) < alpha ) && ( abs(sp1[i] - sp0[i]) < beta ) && ( abs(sq0[i] - sq1[i]) < beta ))
        {
            ap = abs(sp2[i] - sp0[i]);
            aq = abs(sq2[i] - sq0[i]);
            c = c0;
            if ( aq < beta ) c++;
            if ( ap < beta ) c++;
            delta = sat((((sq0[i] - sp0[i]) << 2) + (sp1[i] - sq1[i]) + 4) >> 3, -c, c);
            deltap1 = sat((sp2[i] + ((sp0[i] + sq0[i] + 1) >> 1) - (sp1[i] << 1)) >> 1, -c0, c0);
            deltaq1 = sat((sq2[i] + ((sp0[i] + sq0[i] + 1) >> 1) - (sq1[i] << 1)) >> 1, -c0, c0);
            sp0[i] = (unsigned char)sat(sp0[i] + delta, 0, 255);
            sq0[i] = (unsigned char)sat(sq0[i] - delta, 0, 255);
            if ( ap < beta )
                sp1[i] = (unsigned char)(sp1[i] + deltap1);
            if ( aq < beta )
                sq1[i] = (unsigned char)(sq1[i] + deltaq1);
        }
    }
}


static void deblock_v_edge(unsigned char *srcp, int srcPitch, int ia, int ib)
{
    int alpha = alphas[ia];
    int beta = betas[ib];
    int c, c0 = cs[ia];
    unsigned char *s = srcp;

    int delta, ap, aq, deltap1, deltaq1;

    for ( int i = 0; i < 4; i ++ )
    {
        if (( abs(s[0] - s[-1]) < alpha ) && ( abs(s[1] - s[0]) < beta ) && ( abs(s[-1] - s[-2]) < beta ))
        {
            ap = abs(s[2] - s[0]);
            aq = abs(s[-3] - s[-1]);
            c = c0;
            if ( aq < beta ) c++;
            if ( ap < beta ) c++;
            delta = sat((((s[0] - s[-1]) << 2) + (s[-2] - s[1]) + 4) >> 3, -c, c);
            deltaq1 = sat((s[2] + ((s[0] + s[-1] + 1) >> 1) - (s[1] << 1)) >> 1, -c0, c0);
            deltap1 = sat((s[-3] + ((s[0] + s[-1] + 1) >> 1) - (s[-2] << 1)) >> 1, -c0, c0);
            s[0] = (unsigned char)sat(s[0] - delta, 0, 255);
            s[-1] = (unsigned char)sat(s[-1] + delta, 0, 255);
            if ( ap < beta )
                s[1] = (unsigned char)(s[1] + deltaq1);
            if ( aq < beta )
                s[-2] = (unsigned char)(s[-2] + deltap1);
        }
        s += srcPitch;
    }
}


static void deblock_picture(unsigned char *srcp, int srcPitch, int w, int h,
                            int q, int aOff, int bOff)
{
    int indexa, indexb;
    for ( int j = 0; j < h; j += 4 )
    {
        for ( int i = 0; i < w; i += 4 )
        {
            indexa = sat(q + aOff, 0, 51);
            indexb = sat(q + bOff, 0, 51);
            if ( j > 0 )
                deblock_h_edge(srcp + i, srcPitch, indexa, indexb);
            if ( i > 0 )
                deblock_v_edge(srcp + i, srcPitch, indexa, indexb);
        }
        srcp += 4 * srcPitch;
    }
}


PVideoFrame __stdcall Deblock::GetFrame(int n, IScriptEnvironment *env)
{
    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);

    deblock_picture(YWPLAN(src), YPITCH(src), nWidth, nHeight,
        nQuant, nAOffset, nBOffset);
    deblock_picture(UWPLAN(src), UPITCH(src), nWidth / 2, nHeight / 2,
        nQuant, nAOffset, nBOffset);
    deblock_picture(VWPLAN(src), VPITCH(src), nWidth / 2, nHeight / 2,
        nQuant, nAOffset, nBOffset);

    return src;
}


AVSValue __cdecl Deblock::create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new Deblock(
        args[0].AsClip(),
        args[1].AsInt(25),
        args[2].AsInt(0),
        args[3].AsInt(0),
        env);
}
