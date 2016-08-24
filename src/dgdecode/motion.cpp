/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include "global.h"
#include "MPEG2Decoder.h"


/* ISO/IEC 13818-2 sections 6.2.5.2, 6.3.17.2, and 7.6.3: Motion vectors */
void CMPEG2Decoder::motion_vectors(int PMV[2][2][2],int dmvector[2],
                    int motion_vertical_field_select[2][2], int s,
                    int motion_vector_count, int mv_format, int h_r_size,
                    int v_r_size, int dmv, int mvscale)
{
    if (motion_vector_count==1)
    {
        if (mv_format==MV_FIELD && !dmv)
            motion_vertical_field_select[1][s] =
            motion_vertical_field_select[0][s] = Get_Bits(1);

        motion_vector(PMV[0][s],dmvector,h_r_size,v_r_size,dmv,mvscale,0);

        /* update other motion vector predictors */
        PMV[1][s][0] = PMV[0][s][0];
        PMV[1][s][1] = PMV[0][s][1];
    }
    else
    {
        motion_vertical_field_select[0][s] = Get_Bits(1);
        motion_vector(PMV[0][s],dmvector,h_r_size,v_r_size,dmv,mvscale,0);
        motion_vertical_field_select[1][s] = Get_Bits(1);
        motion_vector(PMV[1][s],dmvector,h_r_size,v_r_size,dmv,mvscale,0);
    }
}


/* ISO/IEC 13818-2 section 7.6.3.6: Dual prime additional arithmetic */
void CMPEG2Decoder::Dual_Prime_Arithmetic(int DMV[][2],int *dmvector, int mvx,int mvy)
{
    if (picture_structure==FRAME_PICTURE)
    {
        if (top_field_first)
        {
            /* vector for prediction of top field from bottom field */
            DMV[0][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
            DMV[0][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] - 1;

            /* vector for prediction of bottom field from top field */
            DMV[1][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
            DMV[1][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] + 1;
        }
        else
        {
            /* vector for prediction of top field from bottom field */
            DMV[0][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
            DMV[0][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] - 1;

            /* vector for prediction of bottom field from top field */
            DMV[1][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
            DMV[1][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] + 1;
        }
    }
    else
    {
        /* vector for prediction from field of opposite 'parity' */
        DMV[0][0] = ((mvx+(mvx>0))>>1) + dmvector[0];
        DMV[0][1] = ((mvy+(mvy>0))>>1) + dmvector[1];

        /* correct for vertical field shift */
        if (picture_structure==TOP_FIELD)
            DMV[0][1]--;
        else
            DMV[0][1]++;
    }
}

/* get and decode motion vector and differential motion vector for one prediction */
void CMPEG2Decoder::motion_vector(int *PMV, int *dmvector, int h_r_size, int v_r_size,
                   int dmv, int mvscale, int full_pel_vector)
{
    int motion_code, motion_residual;

    /* horizontal component */
    /* ISO/IEC 13818-2 Table B-10 */
    motion_code = Get_motion_code();

    motion_residual = (h_r_size!=0 && motion_code!=0) ? Get_Bits(h_r_size) : 0;

    decode_motion_vector(&PMV[0],h_r_size,motion_code,motion_residual,full_pel_vector);

    if (dmv)
        dmvector[0] = Get_dmvector();

    /* vertical component */
    motion_code     = Get_motion_code();
    motion_residual = (v_r_size!=0 && motion_code!=0) ? Get_Bits(v_r_size) : 0;

    if (mvscale)
        PMV[1] >>= 1; /* DIV 2 */

    decode_motion_vector(&PMV[1],v_r_size,motion_code,motion_residual,full_pel_vector);

    if (mvscale)
        PMV[1] <<= 1;

    if (dmv)
        dmvector[1] = Get_dmvector();
}

/* calculate motion vector component */
/* ISO/IEC 13818-2 section 7.6.3.1: Decoding the motion vectors */
/* Note: the arithmetic here is more elegant than that which is shown
   in 7.6.3.1.  The end results (PMV[][][]) should, however, be the same.  */
void CMPEG2Decoder::decode_motion_vector(int *pred, int r_size, int motion_code,
                                 int motion_residual, int full_pel_vector)
{
    int lim, vec;

    lim = 16<<r_size;
    vec = full_pel_vector ? (*pred >> 1) : (*pred);

    if (motion_code>0)
    {
        vec+= ((motion_code-1)<<r_size) + motion_residual + 1;
        if (vec>=lim)
            vec-= lim + lim;
    }
    else if (motion_code<0)
    {
        vec-= ((-motion_code-1)<<r_size) + motion_residual + 1;
        if (vec<-lim)
            vec+= lim + lim;
    }

    *pred = full_pel_vector ? (vec<<1) : vec;
}

int CMPEG2Decoder::Get_motion_code()
{
    int code;

    if (Get_Bits(1))
        return 0;

    code = Show_Bits(9);
    if (code >= 64)
    {
        code >>= 6;
        Flush_Buffer(MVtab0[code].len);
        return Get_Bits(1) ? -MVtab0[code].val : MVtab0[code].val;
    }

    if (code >= 24)
    {
        code >>= 3;
        Flush_Buffer(MVtab1[code].len);
        return Get_Bits(1) ? -MVtab1[code].val : MVtab1[code].val;
    }

    if (code >= 12)
    {
        code -= 12;
        Flush_Buffer(MVtab2[code].len);
        return (Get_Bits(1) ? -MVtab2[code].val : MVtab2[code].val);
    }

    Fault_Flag = 6;
    return 0;
}

/* get differential motion vector (for dual prime prediction) */
int CMPEG2Decoder::Get_dmvector()
{
    if (Get_Bits(1))
        return Get_Bits(1) ? -1 : 1;
    else
        return 0;
}
