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
//#define MPEG2DEC_EXPORTS

#include <emmintrin.h>
#include "global.h"
#include "mc.h"

const uint8_t cc_table[12] = {
    0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 1, 2
};

int dcount = 0;

void CMPEG2Decoder::Decode_Picture(YV12PICT *dst)
{
    if (picture_structure == FRAME_PICTURE && Second_Field)
        Second_Field = 0;

    if (picture_coding_type != B_TYPE)
    {
        pf_forward = pf_backward;
        pf_backward = pf_current;
    }

    Update_Picture_Buffers();

    picture_data();

    if (Fault_Flag == OUT_OF_BITS) return;

    if (picture_structure == FRAME_PICTURE || Second_Field)
    {
        if (picture_coding_type == B_TYPE)
            assembleFrame(auxframe, pf_current, dst);
        else
            assembleFrame(forward_reference_frame, pf_forward, dst);
    }

    if (picture_structure != FRAME_PICTURE)
        Second_Field = !Second_Field;
}

/* reuse old picture buffers as soon as they are no longer needed */
void CMPEG2Decoder::Update_Picture_Buffers()
{
    int cc;              /* color component index */
    uint8_t *tmp;  /* temporary swap pointer */

    for (cc=0; cc<3; cc++)
    {
        /* B pictures  do not need to be save for future reference */
        if (picture_coding_type==B_TYPE)
        {
            current_frame[cc] = auxframe[cc];
        }
        else
        {
            if (!Second_Field)
            {
                /* only update at the beginning of the coded frame */
                tmp = forward_reference_frame[cc];

                /* the previously decoded reference frame is stored coincident with the
                   location where the backward reference frame is stored (backwards
                   prediction is not needed in P pictures) */
                forward_reference_frame[cc] = backward_reference_frame[cc];

                /* update pointer for potential future B pictures */
                backward_reference_frame[cc] = tmp;
            }

            /* can erase over old backward reference frame since it is not used
               in a P picture, and since any subsequent B pictures will use the
               previously decoded I or P frame as the backward_reference_frame */
            current_frame[cc] = backward_reference_frame[cc];
        }

        if (picture_structure==BOTTOM_FIELD)
            current_frame[cc] += (cc==0) ? Coded_Picture_Width : Chroma_Width;
    }
}

/* decode all macroblocks of the current picture */
/* stages described in ISO/IEC 13818-2 section 7 */
void CMPEG2Decoder::picture_data()
{
    int MBAmax;
    uint32_t code;

    /* number of macroblocks per picture */
    MBAmax = mb_width * mb_height;

    if (picture_structure != FRAME_PICTURE)
        MBAmax >>= 1;

    for (;;)
    {
        if (Fault_Flag == OUT_OF_BITS)
            break;
        next_start_code();
        code = Show_Bits(32);
        if (code < SLICE_START_CODE_MIN || code > SLICE_START_CODE_MAX)
            break;
        Flush_Buffer(32);
        slice(MBAmax, code);
    }
}

/* decode all macroblocks of the current picture */
/* ISO/IEC 13818-2 section 6.3.16 */
void CMPEG2Decoder::slice(int MBAmax, uint32_t code)
{
    /* decode slice header (may change quantizer_scale) */
    int slice_vert_pos_ext = slice_header();

    /* decode macroblock address increment */
    Fault_Flag = 0;
    int MBAinc = Get_macroblock_address_increment();
    if (MBAinc < 0)
    {
        // End of slice but we didn't process any macroblocks!
        Fault_Flag = 4;
        return;
    }

    /* set current location */
    /* NOTE: the arithmetic used to derive macroblock_address below is
       equivalent to ISO/IEC 13818-2 section 6.3.17: Macroblock */
    int MBA = ((slice_vert_pos_ext << 7) + (code & 255) - 1) * mb_width + MBAinc - 1;
    MBAinc = 1; // first macroblock in slice: not skipped

    /* reset all DC coefficient and motion vector predictors */
    /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
    int dc_dct_pred[3] = { 0 };

    /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
    int PMV[2][2][2] = { 0 };

    // Set up pointer for storing quants for info and showQ.
    int* qp = (picture_coding_type == B_TYPE) ? auxQP : backwardQP;
    if (picture_structure == BOTTOM_FIELD)
        qp += mb_width*mb_height / 2;

    // This while loop condition just prevents us from processing more than
    // the maximum number of macroblocks possible in a picture. The loop is
    // unlikely to ever terminate on this condition. Usually, it will
    // terminate at the end of the slice. The end of a slice is indicated
    // by 23 zeroes after a macroblock. To detect that, we use a trick in
    // Get_macroblock_address_increment(). See that function for an
    // explanation.

    int macroblock_type, motion_type, dct_type = 0;
    int motion_vertical_field_select[2][2], dmvector[2];

    while (MBA < MBAmax) {
        if (MBAinc == 0) {
            /* decode macroblock address increment */
            MBAinc = Get_macroblock_address_increment();
            if (MBAinc < 0 || Fault_Flag == OUT_OF_BITS) {
                // End of slice or out of data.
                break;
            }
        }
        if (MBAinc == 1) {
            decode_macroblock(&macroblock_type, &motion_type, &dct_type, PMV,
                              dc_dct_pred, motion_vertical_field_select, dmvector);
        } else {
            /* ISO/IEC 13818-2 section 7.6.6 */
            skipped_macroblock(dc_dct_pred, PMV, &motion_type, motion_vertical_field_select, &macroblock_type);
        }

        if (Fault_Flag) {
            break;
        }

        QP[MBA] = qp[MBA] = quantizer_scale;

        /* ISO/IEC 13818-2 section 7.6 */
        motion_compensation(MBA, macroblock_type, motion_type, PMV,
                            motion_vertical_field_select, dmvector, dct_type);

        /* advance to next macroblock */
        ++MBA;
        --MBAinc;
    }
}

/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
void CMPEG2Decoder::macroblock_modes(int *pmacroblock_type, int *pmotion_type,
                             int *pmotion_vector_count, int *pmv_format,
                             int *pdmv, int *pmvscale, int *pdct_type)
{
    /* get macroblock_type */
    int macroblock_type = Get_macroblock_type();
    if (Fault_Flag)
        return;

    /* get frame/field motion type */
    int motion_type;
    if (macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD)) {
        if (picture_structure == FRAME_PICTURE)
            motion_type = frame_pred_frame_dct ? MC_FRAME : Get_Bits(2);
        else
            motion_type = Get_Bits(2);
    } else {
        motion_type = (picture_structure == FRAME_PICTURE) ? MC_FRAME : MC_FIELD;
    }

    /* derive motion_vector_count, mv_format and dmv, (table 6-17, 6-18) */
    int motion_vector_count, mv_format;
    if (picture_structure == FRAME_PICTURE) {
        motion_vector_count = (motion_type==MC_FIELD) ? 2 : 1;
        mv_format = (motion_type==MC_FRAME) ? MV_FRAME : MV_FIELD;
    } else {
        motion_vector_count = (motion_type == MC_16X8) ? 2 : 1;
        mv_format = MV_FIELD;
    }

    *pdmv = (motion_type==MC_DMV); /* dual prime */

    /*
       field mv predictions in frame pictures have to be scaled
       ISO/IEC 13818-2 section 7.6.3.1 Decoding the motion vectors
    */
    *pmvscale = (mv_format==MV_FIELD && picture_structure==FRAME_PICTURE);

    /* get dct_type (frame DCT / field DCT) */
    *pdct_type = (picture_structure==FRAME_PICTURE) && (!frame_pred_frame_dct)
                 && (macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA)) ? Get_Bits(1) : 0;

    /* return values */
    *pmacroblock_type = macroblock_type;
    *pmotion_type = motion_type;
    *pmotion_vector_count = motion_vector_count;
    *pmv_format = mv_format;
}

/* move/add 8x8-Block from block[comp] to backward_reference_frame */
/* copy reconstructed 8x8 block from block[comp] to current_frame[]
   ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data
   This stage also embodies some of the operations implied by:
   - ISO/IEC 13818-2 section 7.6.7: Combining predictions
   - ISO/IEC 13818-2 section 6.1.3: Macroblock
*/

void CMPEG2Decoder::Add_Block(int count, int bx, int by, int dct_type, int addflag)
{
    alignas(16) static const uint64_t mmmask_128C[2] = {
        0x8080808080808080, 0x8080808080808080
    };

    const __m128i mask = addflag ? _mm_setzero_si128() :
        _mm_load_si128(reinterpret_cast<const __m128i*>(mmmask_128C));

    for (int comp = 0; comp < count; ++comp) {
        int16_t* blockp = block[comp];
        int cc = cc_table[comp];
        int bxh = bx;
        int byh = by;
        int iincr;
        uint8_t* rfp;

        if (cc == 0) {
            if (picture_structure == FRAME_PICTURE) {
                if (dct_type) {
                    rfp = current_frame[0] + Coded_Picture_Width * (by + (comp & 2) / 2) + bx + (comp & 1) * 8;
                    iincr = Coded_Picture_Width * 2;
                } else {
                    rfp = current_frame[0] + Coded_Picture_Width * (by + (comp & 2) * 4) + bx + (comp & 1) * 8;
                    iincr = Coded_Picture_Width;
                }
            } else {
                rfp = current_frame[0] + (Coded_Picture_Width * 2) * (by + (comp & 2) * 4) + bx + (comp & 1) * 8;
                iincr = Coded_Picture_Width * 2;
            }
        } else {
            if (chroma_format != CHROMA444) bxh /= 2;
            if (chroma_format==CHROMA420) byh /= 2;

            if (picture_structure==FRAME_PICTURE) {
                if (dct_type && chroma_format != CHROMA420) {
                    /* field DCT coding */
                    rfp = current_frame[cc] + Chroma_Width * (byh + (comp & 2) / 2) + bxh + (comp & 8);
                    iincr = Chroma_Width * 2;
                } else {
                    /* frame DCT coding */
                    rfp = current_frame[cc] + Chroma_Width * (byh + (comp & 2) * 4) + bxh + (comp & 8);
                    iincr = Chroma_Width;
                }
            } else {
                /* field picture */
                rfp = current_frame[cc] + (Chroma_Width * 2) * (byh + (comp & 2) * 4) + bxh + (comp & 8);
                iincr = Chroma_Width * 2;
            }
        }

        uint8_t* rfp1 = rfp + iincr;
        iincr *= 2;
        __m128i r0, r1;

        if (addflag) {
            r0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp));
            r1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp1));
            r0 = _mm_unpacklo_epi8(r0, mask);
            r1 = _mm_unpacklo_epi8(r1, mask);
            r0 = _mm_adds_epi16(r0, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp)));
            r1 = _mm_adds_epi16(r1, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 8)));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), _mm_packus_epi16(r0, r0));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_packus_epi16(r1, r1));

            rfp += iincr;
            rfp1 += iincr;
            r0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp));
            r1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp1));
            r0 = _mm_unpacklo_epi8(r0, mask);
            r1 = _mm_unpacklo_epi8(r1, mask);
            r0 = _mm_adds_epi16(r0, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 16)));
            r1 = _mm_adds_epi16(r1, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 24)));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), _mm_packus_epi16(r0, r0));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_packus_epi16(r1, r1));

            rfp += iincr;
            rfp1 += iincr;
            r0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp));
            r1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp1));
            r0 = _mm_unpacklo_epi8(r0, mask);
            r1 = _mm_unpacklo_epi8(r1, mask);
            r0 = _mm_adds_epi16(r0, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 32)));
            r1 = _mm_adds_epi16(r1, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 40)));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), _mm_packus_epi16(r0, r0));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_packus_epi16(r1, r1));

            rfp += iincr;
            rfp1 += iincr;
            r0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp));
            r1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(rfp1));
            r0 = _mm_unpacklo_epi8(r0, mask);
            r1 = _mm_unpacklo_epi8(r1, mask);
            r0 = _mm_adds_epi16(r0, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 48)));
            r1 = _mm_adds_epi16(r1, _mm_loadu_si128(reinterpret_cast<const __m128i*>(blockp + 56)));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), _mm_packus_epi16(r0, r0));
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_packus_epi16(r1, r1));

        } else {
            r0 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp));
            r1 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 8));
            r0 = _mm_add_epi8(_mm_packs_epi16(r0, r1), mask);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), r0);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_srli_si128(r0, 8));

            rfp += iincr;
            rfp1 += iincr;
            r0 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 16));
            r1 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 24));
            r0 = _mm_add_epi8(_mm_packs_epi16(r0, r1), mask);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), r0);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_srli_si128(r0, 8));

            rfp += iincr;
            rfp1 += iincr;
            r0 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 32));
            r1 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 40));
            r0 = _mm_add_epi8(_mm_packs_epi16(r0, r1), mask);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), r0);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_srli_si128(r0, 8));

            rfp += iincr;
            rfp1 += iincr;
            r0 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 48));
            r1 = _mm_load_si128(reinterpret_cast<const __m128i*>(blockp + 56));
            r0 = _mm_add_epi8(_mm_packs_epi16(r0, r1), mask);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp), r0);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(rfp1), _mm_srli_si128(r0, 8));
        }
    }
}


/* set scratch pad macroblock to zero */
void CMPEG2Decoder::Clear_Block(int count)
{
    const __m128i zero = _mm_setzero_si128();
    for (int comp = 0; comp < count; ++comp) {
        //memset(block[comp], 0, 128);
        __m128i* p = reinterpret_cast<__m128i*>(block[comp]);
        _mm_store_si128(p, zero);
        _mm_store_si128(++p, zero);
        _mm_store_si128(++p, zero);
        _mm_store_si128(++p, zero);
        _mm_store_si128(++p, zero);
        _mm_store_si128(++p, zero);
        _mm_store_si128(++p, zero);
        _mm_store_si128(++p, zero);
    }
}

/* ISO/IEC 13818-2 section 7.6 */
void CMPEG2Decoder::motion_compensation(int MBA, int macroblock_type, int motion_type,
                                int PMV[2][2][2], int motion_vertical_field_select[2][2],
                                int dmvector[2], int dct_type)
{
    int bx, by;
    int comp;
    int16_t* prefetchpointer2;

    if(IDCT_Flag == IDCT_SSE2MMX)
        IDCT_CONST_PREFETCH();

    /* derive current macroblock position within picture */
    /* ISO/IEC 13818-2 section 6.3.1.6 and 6.3.1.7 */
    bx = 16*(MBA%mb_width);
    by = 16*(MBA/mb_width);

    /* motion compensation */
    if (!(macroblock_type & MACROBLOCK_INTRA))
        form_predictions(bx, by, macroblock_type, motion_type, PMV,
            motion_vertical_field_select, dmvector);

    // idct is now a pointer
    if ( IDCT_Flag == IDCT_SSE2MMX )
    {
        for (comp=0; comp<block_count-1; comp++)
        {
            prefetchpointer2=block[comp+1];
            __asm {
                mov eax,prefetchpointer2
                prefetcht0 [eax]
            };

            SSE2MMX_IDCT(block[comp]);
        };
        SSE2MMX_IDCT(block[comp]);
    }
    else
    {
        for (comp=0; comp<block_count; comp++)
            idctFunc(block[comp]);
    }

    Add_Block(block_count, bx, by, dct_type, (macroblock_type & MACROBLOCK_INTRA)==0);

}

/* ISO/IEC 13818-2 section 7.6.6 */
void CMPEG2Decoder::skipped_macroblock(int dc_dct_pred[3], int PMV[2][2][2], int *motion_type,
                               int motion_vertical_field_select[2][2], int *macroblock_type)
{
    Clear_Block(block_count);

    /* reset intra_dc predictors */
    /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
    dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2]=0;

    /* reset motion vector predictors */
    /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
    if (picture_coding_type == P_TYPE)
        PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;

    /* derive motion_type */
    if (picture_structure == FRAME_PICTURE)
        *motion_type = MC_FRAME;
    else
    {
        *motion_type = MC_FIELD;
        motion_vertical_field_select[0][0] = motion_vertical_field_select[0][1] =
            (picture_structure == BOTTOM_FIELD);
    }

    if (picture_coding_type == I_TYPE)
    {
        Fault_Flag = true;
        return;
    }

    /* clear MACROBLOCK_INTRA */
    *macroblock_type &= ~MACROBLOCK_INTRA;
}

/* ISO/IEC 13818-2 sections 7.2 through 7.5 */
void CMPEG2Decoder::decode_macroblock(int *macroblock_type, int *motion_type, int *dct_type,
                             int PMV[2][2][2], int dc_dct_pred[3],
                             int motion_vertical_field_select[2][2], int dmvector[2])
{
    int quantizer_scale_code, comp, motion_vector_count, mv_format;
    int dmv, mvscale, coded_block_pattern;

    /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
    macroblock_modes(macroblock_type, motion_type, &motion_vector_count, &mv_format,
                     &dmv, &mvscale, dct_type);
    if (Fault_Flag)
    {
        return; // go to next slice
    }

    if (*macroblock_type & MACROBLOCK_QUANT)
    {
        quantizer_scale_code = Get_Bits(5);

        /* ISO/IEC 13818-2 section 7.4.2.2: Quantizer scale factor */
        if (mpeg_type == IS_MPEG2)
        {
            quantizer_scale = q_scale_type ?
                Non_Linear_quantizer_scale[quantizer_scale_code] : (quantizer_scale_code << 1);
        }
        else
        {
            quantizer_scale = quantizer_scale_code;
        }
    }

    /* ISO/IEC 13818-2 section 6.3.17.2: Motion vectors */
    /* decode forward motion vectors */
    if ((*macroblock_type & MACROBLOCK_MOTION_FORWARD)
        || ((*macroblock_type & MACROBLOCK_INTRA) && concealment_motion_vectors))
    {
        if (mpeg_type == IS_MPEG2)
        {
            motion_vectors(PMV, dmvector, motion_vertical_field_select, 0,
                           motion_vector_count, mv_format, f_code[0][0]-1, f_code[0][1]-1, dmv, mvscale);
        }
        else
        {
            motion_vector(PMV[0][0], dmvector, forward_f_code-1, forward_f_code-1, dmv, mvscale, full_pel_forward_vector);
        }
    }
    if (Fault_Flag)
    {
        return; // go to next slice
    }

    /* decode backward motion vectors */
    if (*macroblock_type & MACROBLOCK_MOTION_BACKWARD)
    {
        if (mpeg_type == IS_MPEG2)
        {
            motion_vectors(PMV, dmvector, motion_vertical_field_select, 1,
                           motion_vector_count,mv_format, f_code[1][0]-1, f_code[1][1]-1, 0, mvscale);
        }
        else
        {
            motion_vector(PMV[0][1], dmvector, backward_f_code-1, backward_f_code-1, dmv, mvscale, full_pel_backward_vector);
        }
    }
    if (Fault_Flag)
    {
        return; // go to next slice
    }

    if ((*macroblock_type & MACROBLOCK_INTRA) && concealment_motion_vectors)
        Flush_Buffer(1);    // marker bit

    /* macroblock_pattern */
    /* ISO/IEC 13818-2 section 6.3.17.4: Coded block pattern */
    if (*macroblock_type & MACROBLOCK_PATTERN)
    {
        coded_block_pattern = Get_coded_block_pattern();

        if (chroma_format==CHROMA422)
            coded_block_pattern = (coded_block_pattern<<2) | Get_Bits(2);
        else if (chroma_format==CHROMA444)
            coded_block_pattern = (coded_block_pattern<<6) | Get_Bits(6);
    }
    else
        coded_block_pattern = (*macroblock_type & MACROBLOCK_INTRA) ? (1<<block_count)-1 : 0;

    if (Fault_Flag)
    {
        return; // go to next slice
    }

    Clear_Block(block_count);

    /* decode blocks */
    for (comp=0; comp < block_count; comp++)
    {
        if (coded_block_pattern & (1<<(block_count-1-comp)))
        {
            if (*macroblock_type & MACROBLOCK_INTRA)
            {
                if (mpeg_type == IS_MPEG2)
                    Decode_MPEG2_Intra_Block(comp, dc_dct_pred);
                else
                    Decode_MPEG1_Intra_Block(comp, dc_dct_pred);
            }
            else
            {
                if (mpeg_type == IS_MPEG2)
                    Decode_MPEG2_Non_Intra_Block(comp);
                else
                    Decode_MPEG1_Non_Intra_Block(comp);
            }
            if (Fault_Flag)
            {
                return; // go to next slice
            }
        }
    }

    /* reset intra_dc predictors */
    /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
    if (!(*macroblock_type & MACROBLOCK_INTRA))
        dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

    /* reset motion vector predictors */
    if ((*macroblock_type & MACROBLOCK_INTRA) && !concealment_motion_vectors)
    {
        /* intra mb without concealment motion vectors */
        /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
        PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
        PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
    }

    /* special "No_MC" macroblock_type case */
    /* ISO/IEC 13818-2 section 7.6.3.5: Prediction in P pictures */
    if ((picture_coding_type==P_TYPE)
        && !(*macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_INTRA)))
    {
        /* non-intra mb without forward mv in a P picture */
        /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
        PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;

        /* derive motion_type */
        /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes, frame_motion_type */
        if (picture_structure==FRAME_PICTURE)
            *motion_type = MC_FRAME;
        else
        {
            *motion_type = MC_FIELD;
            motion_vertical_field_select[0][0] = (picture_structure==BOTTOM_FIELD);
        }
    }

    /* successfully decoded macroblock */
}

/* decode one intra coded MPEG-1 block */
void CMPEG2Decoder::Decode_MPEG1_Intra_Block(int comp, int dc_dct_pred[])
{
    long code, val = 0, i, j, sign;
    const DCTtab *tab;
    int16_t *bp;

    bp = block[comp];

    /* ISO/IEC 13818-2 section 7.2.1: decode DC coefficients */
    switch (cc_table[comp])
    {
        case 0:
            val = (dc_dct_pred[0] += Get_Luma_DC_dct_diff());
            break;

        case 1:
            val = (dc_dct_pred[1] += Get_Chroma_DC_dct_diff());
            break;

        case 2:
            val = (dc_dct_pred[2] += Get_Chroma_DC_dct_diff());
            break;
    }

    bp[0] = (int16_t) (val << 3);

    if (picture_coding_type == D_TYPE)
        return;

    /* decode AC coefficients */
    for (i=1; ; i++)
    {
        code = Show_Bits(16);

        if (code >=16384)
            tab = &DCTtabnext[(code>>12)-4];
        else if (code >= 1024)
            tab = &DCTtab0[(code>>8)-4];
        else if (code >= 512)
            tab = &DCTtab1[(code>>6)-8];
        else if (code >= 256)
            tab = &DCTtab2[(code>>4)-16];
        else if (code >= 128)
            tab = &DCTtab3[(code>>3)-16];
        else if (code >= 64)
            tab = &DCTtab4[(code>>2)-16];
        else if (code >= 32)
            tab = &DCTtab5[(code>>1)-16];
        else if (code >= 16)
            tab = &DCTtab6[code-16];
        else
        {
            Fault_Flag = 1;
            break;
        }

        Flush_Buffer(tab->len);
        val = tab->run;

        if (val == 65)
        {
            // escape
            i+= Get_Bits(6);
            val = Get_Bits(8);
            if (val == 0)
                val = Get_Bits(8);
            else if (val == 128)
                val = Get_Bits(8) - 256;
            else if (val > 128)
                val -= 256;
            sign = (val < 0);
            if (sign)
                val = - val;
        }
        else
        {
            if (val == 64)
                break;
            i += val;
            val = tab->level;
            sign = Get_Bits(1);
        }
        if (i >= 64)
        {
            Fault_Flag = 1;
            break;
        }

        j = scan[0][i];
        val = (val * quantizer_scale * intra_quantizer_matrix[j]) >> 3;
        if (val)
            val = (val - 1) | 1; // mismatch
        if (val >= 2048) val = 2047 + sign; // saturation
        if (sign)
            val = -val;
        bp[j] = (int16_t) val;
    }
}

/* decode one non-intra coded MPEG-1 block */
void CMPEG2Decoder::Decode_MPEG1_Non_Intra_Block(int comp)
{
    int32_t code, val=0, i, j, sign;
    const DCTtab *tab;
    int16_t *bp;

    bp = block[comp];

    /* decode AC coefficients */
    for (i=0; ; i++)
    {
        code = Show_Bits(16);

        if (code >= 16384)
        {
            if (i)
                tab = &DCTtabnext[(code>>12)-4];
            else
                tab = &DCTtabfirst[(code>>12)-4];
        }
        else if (code >= 1024)
            tab = &DCTtab0[(code>>8)-4];
        else if (code >= 512)
            tab = &DCTtab1[(code>>6)-8];
        else if (code >= 256)
            tab = &DCTtab2[(code>>4)-16];
        else if (code >= 128)
            tab = &DCTtab3[(code>>3)-16];
        else if (code >= 64)
            tab = &DCTtab4[(code>>2)-16];
        else if (code >= 32)
            tab = &DCTtab5[(code>>1)-16];
        else if (code >= 16)
            tab = &DCTtab6[code-16];
        else
        {
            Fault_Flag = 1;
            break;
        }

        Flush_Buffer(tab->len);
        val = tab->run;

        if (val == 65)
        {
            // escape
            i+= Get_Bits(6);
            val = Get_Bits(8);
            if (val == 0)
                val = Get_Bits(8);
            else if (val == 128)
                val = Get_Bits(8) - 256;
            else if (val > 128)
                val -= 256;

            sign = (val<0);
            if (sign)
                val = - val;
        }
        else
        {
            if (val == 64)
                break;
            i += val;
            val = tab->level;
            sign = Get_Bits(1);
        }
        if (i >= 64)
        {
            Fault_Flag = 1;
            break;
        }

        j = scan[0][i];
        val = (((val<<1)+1) * quantizer_scale * non_intra_quantizer_matrix[j]) >> 4;
        if (val)
            val = (val - 1) | 1; // mismatch
        if (val >= 2048)
            val = 2047 + sign; //saturation
        if (sign)
            val = -val;
        bp[j] = (int16_t) val;
    }
}

/* decode one intra coded MPEG-2 block */
void CMPEG2Decoder::Decode_MPEG2_Intra_Block(int comp, int dc_dct_pred[])
{
    int32_t code, val = 0, i, j, sign, sum;
    const DCTtab *tab;
    int16_t *bp;
    int *qmat;

    bp = block[comp];
    qmat = (comp<4 || chroma_format==CHROMA420)
        ? intra_quantizer_matrix : chroma_intra_quantizer_matrix;

    /* ISO/IEC 13818-2 section 7.2.1: decode DC coefficients */
    switch (cc_table[comp])
    {
        case 0:
            val = (dc_dct_pred[0] += Get_Luma_DC_dct_diff());
            break;

        case 1:
            val = (dc_dct_pred[1] += Get_Chroma_DC_dct_diff());
            break;

        case 2:
            val = (dc_dct_pred[2] += Get_Chroma_DC_dct_diff());
            break;
    }

    sum = val << (3 - intra_dc_precision);
    bp[0] = (int16_t) sum;

    /* decode AC coefficients */
    for (i=1; ; i++)
    {
        code = Show_Bits(16);

        if (code >= 16384)
        {
            if (intra_vlc_format)
                tab = &DCTtab0a[(code>>8)-4];
            else
                tab = &DCTtabnext[(code>>12)-4];
        }
        else if (code >= 1024)
        {
            if (intra_vlc_format)
                tab = &DCTtab0a[(code>>8)-4];
            else
                tab = &DCTtab0[(code>>8)-4];
        }
        else if (code >= 512)
        {
            if (intra_vlc_format)
                tab = &DCTtab1a[(code>>6)-8];
            else
                tab = &DCTtab1[(code>>6)-8];
        }
        else if (code >= 256)
            tab = &DCTtab2[(code>>4)-16];
        else if (code >= 128)
            tab = &DCTtab3[(code>>3)-16];
        else if (code >= 64)
            tab = &DCTtab4[(code>>2)-16];
        else if (code >= 32)
            tab = &DCTtab5[(code>>1)-16];
        else if (code >= 16)
            tab = &DCTtab6[code-16];
        else
        {
            Fault_Flag = 1;
            break;
        }

        Flush_Buffer(tab->len);
        val = tab->run;

        if (val == 65)
        {
            // escape
            i+= Get_Bits(6);
            val = Get_Bits(12);
            if (!(val & 2047))
            {
                Fault_Flag = 1;
                break;
            }
            sign = (val >= 2048);
            if (sign)
                val = 4096 - val;
        }
        else
        {
            if (val == 64)
                break;
            i+= val;
            val = tab->level;
            sign = Get_Bits(1);
        }
        if (i >= 64)
        {
            Fault_Flag = 1;
            break;
        }
        j = scan[alternate_scan][i];
        val = (val * quantizer_scale * qmat[j]) >> 4;
        if (val >= 2048)
            val = 2047 + sign; // saturation
        if (sign)
            val = -val;
        bp[j] = (int16_t) val;
        sum ^= val;     // mismatch
    }

    if (!Fault_Flag && !(sum & 1))
        bp[63] ^= 1;   // mismatch control
}

/* decode one non-intra coded MPEG-2 block */
void CMPEG2Decoder::Decode_MPEG2_Non_Intra_Block(int comp)
{
    int32_t code, val = 0, i, j, sign, sum;
    const DCTtab *tab;
    int16_t *bp;
    int *qmat;

    bp = block[comp];
    qmat = (comp<4 || chroma_format==CHROMA420)
        ? non_intra_quantizer_matrix : chroma_non_intra_quantizer_matrix;

    /* decode AC coefficients */
    sum = 0;
    for (i=0; ; i++)
    {
        code = Show_Bits(16);

        if (code >= 16384)
        {
            if (i)
                tab = &DCTtabnext[(code>>12)-4];
            else
                tab = &DCTtabfirst[(code>>12)-4];
        }
        else if (code >= 1024)
            tab = &DCTtab0[(code>>8)-4];
        else if (code >= 512)
            tab = &DCTtab1[(code>>6)-8];
        else if (code >= 256)
            tab = &DCTtab2[(code>>4)-16];
        else if (code >= 128)
            tab = &DCTtab3[(code>>3)-16];
        else if (code >= 64)
            tab = &DCTtab4[(code>>2)-16];
        else if (code >= 32)
            tab = &DCTtab5[(code>>1)-16];
        else if (code >= 16)
            tab = &DCTtab6[code-16];
        else
        {
            Fault_Flag = 1;
            break;
        }

        Flush_Buffer(tab->len);
        val = tab->run;

        if (val == 65)
        {
            // escape
            i+= Get_Bits(6);
            val = Get_Bits(12);
            if (!(val & 2047))
            {
                Fault_Flag = 1;
                break;
            }
            sign = (val >= 2048);
            if (sign)
                val = 4096 - val;
        }
        else
        {
            if (val == 64)
                break;
            i+= val;
            val = tab->level;
            sign = Get_Bits(1);
        }
        if (i >= 64)
        {
            Fault_Flag = 1;
            break;
        }

        j = scan[alternate_scan][i];
        val = (((val<<1)+1) * quantizer_scale * qmat[j]) >> 5;
        if (val >= 2048)
            val = 2047 + sign; // saturation
        if (sign)
            val = -val;
        bp[j] = (int16_t) val;
        sum ^= val;             // mismatch
    }

    if (!Fault_Flag && !(sum & 1))
        bp[63] ^= 1;   // mismatch control
}

int CMPEG2Decoder::Get_macroblock_type()
{
    int macroblock_type = 0;

    switch (picture_coding_type)
    {
        case I_TYPE:
            macroblock_type = Get_I_macroblock_type();
            break;
        case P_TYPE:
            macroblock_type = Get_P_macroblock_type();
            break;
        case B_TYPE:
            macroblock_type = Get_B_macroblock_type();
            break;
        case D_TYPE:
            macroblock_type = Get_D_macroblock_type();
            break;
        default:
            Fault_Flag = 2;
    }

    return macroblock_type;
}

int CMPEG2Decoder::Get_I_macroblock_type()
{
    int code = Show_Bits(2);

    // the only valid codes are 1, or 01
    if (code & 2)
    {
        Flush_Buffer(1);
        return 1;
    }
    if (code & 1)
    {
        Flush_Buffer(2);
    }
    else
        Fault_Flag = 2;

    return 17;
}

int CMPEG2Decoder::Get_P_macroblock_type()
{
    int code = Show_Bits(6);

    if (code >= 8)
    {
        code >>= 3;
        Flush_Buffer(PMBtab0[code].len);
        code = PMBtab0[code].val;
    }
    else if (code)
    {
        Flush_Buffer(PMBtab1[code].len);
        code = PMBtab1[code].val;
    }
    else
        Fault_Flag = 2;

    return code;
}

int CMPEG2Decoder::Get_B_macroblock_type()
{
    int code = Show_Bits(6);

    if (code >= 8)
    {
        code >>= 2;
        Flush_Buffer(BMBtab0[code].len);
        code = BMBtab0[code].val;
    }
    else if (code)
    {
        Flush_Buffer(BMBtab1[code].len);
        code = BMBtab1[code].val;
    }
    else
        Fault_Flag = 2;

    return code;
}

int CMPEG2Decoder::Get_D_macroblock_type()
{
    if (Get_Bits(1))
        Flush_Buffer(1);
    else
        Fault_Flag = 2;
    return 1;
}

int CMPEG2Decoder::Get_coded_block_pattern()
{
    int code = Show_Bits(9);
    if (code >= 128)
    {
        code >>= 4;
        Flush_Buffer(CBPtab0[code].len);
        code = CBPtab0[code].val;
    }
    else if (code >= 8)
    {
        code >>= 1;
        Flush_Buffer(CBPtab1[code].len);
        code = CBPtab1[code].val;
    }
    else if (code)
    {
        Flush_Buffer(CBPtab2[code].len);
        code = CBPtab2[code].val;
    }
    else
        Fault_Flag = 3;

    return code;
}

int CMPEG2Decoder::Get_macroblock_address_increment()
{
    int code, val;

    val = 0;

    for (val = 0;;)
    {
        if (Fault_Flag == OUT_OF_BITS)
            return -1;
        code = Show_Bits(11);
        if (code >= 24)
            break;
        if (code != 15) /* if not macroblock_stuffing */
        {
            if (code == 8)
            {
                /* macroblock_escape */
                val+= 33;
            }
            else if (code == 0)
            {
                // The variable length code for the macroblock address
                // increment cannot be all zeros. If we see all zeros here,
                // then we must have run into the end of the slice, which is marked
                // by 23 zeroes. We return a negative increment to signal end
                // of slice.
                return -1;
            }
            else
            {
                Fault_Flag = 5;
                return -1;
            }
        }
        Flush_Buffer(11);
    }

    /* macroblock_address_increment == 1 */
    /* ('1' is in the MSB position of the lookahead) */
    if (code >= 1024)
    {
        Flush_Buffer(1);
        val++;
    }

    /* codes 00010 ... 011xx */
    else if (code >= 128)
    {
        /* remove leading zeros */
        code >>= 6;
        Flush_Buffer(MBAtab1[code].len);
        val += MBAtab1[code].val;
    }

    /* codes 00000011000 ... 0000111xxxx */
    else
    {
        code -= 24; /* remove common base */
        Flush_Buffer(MBAtab2[code].len);
        val += MBAtab2[code].val;
    }

    return val;
}

/*
   parse VLC and perform dct_diff arithmetic.
   MPEG-2:  ISO/IEC 13818-2 section 7.2.1

   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/
int CMPEG2Decoder::Get_Luma_DC_dct_diff()
{
    int code, size, dct_diff;

    /* decode length */
    code = Show_Bits(5);

    if (code<31)
    {
        size = DClumtab0[code].val;
        Flush_Buffer(DClumtab0[code].len);
    }
    else
    {
        code = Show_Bits(9) - 0x1f0;
        size = DClumtab1[code].val;
        Flush_Buffer(DClumtab1[code].len);
    }

    if (size==0)
        dct_diff = 0;
    else
    {
        dct_diff = Get_Bits(size);

        if ((dct_diff & (1<<(size-1)))==0)
            dct_diff-= (1<<size) - 1;
    }

    return dct_diff;
}

int CMPEG2Decoder::Get_Chroma_DC_dct_diff()
{
    int code, size, dct_diff;

    /* decode length */
    code = Show_Bits(5);

    if (code<31)
    {
        size = DCchromtab0[code].val;
        Flush_Buffer(DCchromtab0[code].len);
    }
    else
    {
        code = Show_Bits(10) - 0x3e0;
        size = DCchromtab1[code].val;
        Flush_Buffer(DCchromtab1[code].len);
    }

    if (size==0)
        dct_diff = 0;
    else
    {
        dct_diff = Get_Bits(size);

        if ((dct_diff & (1<<(size-1)))==0)
            dct_diff-= (1<<size) - 1;
    }

    return dct_diff;
}

/*
static int currentfield;
static unsigned char **predframe;
static int DMV[2][2];
static int stw;
*/

void CMPEG2Decoder::form_predictions(int bx, int by, int macroblock_type, int motion_type,
                      int PMV[2][2][2], int motion_vertical_field_select[2][2],
                      int dmvector[2])
{
    int currentfield;
    uint8_t **predframe;
    int DMV[2][2];
    int stw;

    stw = 0;


    if ((macroblock_type & MACROBLOCK_MOTION_FORWARD) || (picture_coding_type==P_TYPE))
    {
        if (picture_structure==FRAME_PICTURE)
        {
            if ((motion_type==MC_FRAME) || !(macroblock_type & MACROBLOCK_MOTION_FORWARD))
            {
                /* frame-based prediction (broken into top and bottom halves
                   for spatial scalability prediction purposes) */
                form_prediction(forward_reference_frame, 0, current_frame, 0, Coded_Picture_Width,
                    Coded_Picture_Width<<1, 16, 8, bx, by, PMV[0][0][0], PMV[0][0][1], stw);

                form_prediction(forward_reference_frame, 1, current_frame, 1, Coded_Picture_Width,
                    Coded_Picture_Width<<1, 16, 8, bx, by, PMV[0][0][0], PMV[0][0][1], stw);
            }
            else if (motion_type==MC_FIELD) /* field-based prediction */
            {
                /* top field prediction */
                form_prediction(forward_reference_frame, motion_vertical_field_select[0][0],
                    current_frame, 0, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8,
                    bx, by>>1, PMV[0][0][0], PMV[0][0][1]>>1, stw);

                /* bottom field prediction */
                form_prediction(forward_reference_frame, motion_vertical_field_select[1][0],
                    current_frame, 1, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8,
                    bx, by>>1, PMV[1][0][0], PMV[1][0][1]>>1, stw);
            }
            else if (motion_type==MC_DMV) /* dual prime prediction */
            {

                /* calculate derived motion vectors */
                Dual_Prime_Arithmetic(DMV, dmvector, PMV[0][0][0], PMV[0][0][1]>>1);

                /* predict top field from top field */
                form_prediction(forward_reference_frame, 0, current_frame, 0,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8, bx, by>>1,
                    PMV[0][0][0], PMV[0][0][1]>>1, 0);

                /* predict and add to top field from bottom field */
                form_prediction(forward_reference_frame, 1, current_frame, 0,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8, bx, by>>1,
                    DMV[0][0], DMV[0][1], 1);

                /* predict bottom field from bottom field */
                form_prediction(forward_reference_frame, 1, current_frame, 1,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8, bx, by>>1,
                    PMV[0][0][0], PMV[0][0][1]>>1, 0);

                /* predict and add to bottom field from top field */
                form_prediction(forward_reference_frame, 0, current_frame, 1,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8, bx, by>>1,
                    DMV[1][0], DMV[1][1], 1);
            }
        }
        else
        {
            /* field picture */
            currentfield = (picture_structure==BOTTOM_FIELD);

            /* determine which frame to use for prediction */
            if (picture_coding_type==P_TYPE && Second_Field && currentfield!=motion_vertical_field_select[0][0])
                predframe = backward_reference_frame;
            else
                predframe = forward_reference_frame;

            if ((motion_type==MC_FIELD) || !(macroblock_type & MACROBLOCK_MOTION_FORWARD))
            {
                form_prediction(predframe, motion_vertical_field_select[0][0], current_frame, 0,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 16, bx, by,
                    PMV[0][0][0], PMV[0][0][1], stw);
            }
            else if (motion_type==MC_16X8)
            {
                form_prediction(predframe, motion_vertical_field_select[0][0], current_frame, 0,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8, bx, by,
                    PMV[0][0][0], PMV[0][0][1], stw);

                if (picture_coding_type==P_TYPE && Second_Field && currentfield!=motion_vertical_field_select[1][0])
                    predframe = backward_reference_frame;
                else
                    predframe = forward_reference_frame;

                form_prediction(predframe, motion_vertical_field_select[1][0], current_frame,
                    0, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8, bx, by+8,
                    PMV[1][0][0], PMV[1][0][1], stw);
            }
            else if (motion_type==MC_DMV)
            {
                if (Second_Field)
                    predframe = backward_reference_frame;
                else
                    predframe = forward_reference_frame;

                /* calculate derived motion vectors */
                Dual_Prime_Arithmetic(DMV, dmvector, PMV[0][0][0], PMV[0][0][1]);

                /* predict from field of same parity */
                form_prediction(forward_reference_frame, currentfield, current_frame, 0,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 16, bx, by,
                    PMV[0][0][0], PMV[0][0][1], 0);

                /* predict from field of opposite parity */
                form_prediction(predframe, !currentfield, current_frame, 0,
                    Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 16, bx, by,
                    DMV[0][0], DMV[0][1], 1);
            }
        }

        stw = 1;
    }

    if (macroblock_type & MACROBLOCK_MOTION_BACKWARD)
    {
        if (picture_structure==FRAME_PICTURE)
        {
            if (motion_type==MC_FRAME)
            {
                /* frame-based prediction */
                form_prediction(backward_reference_frame, 0, current_frame, 0,
                    Coded_Picture_Width, Coded_Picture_Width<<1, 16, 8, bx, by,
                    PMV[0][1][0], PMV[0][1][1], stw);

                form_prediction(backward_reference_frame, 1, current_frame, 1,
                    Coded_Picture_Width, Coded_Picture_Width<<1, 16, 8, bx, by,
                    PMV[0][1][0], PMV[0][1][1], stw);
            }
            else /* field-based prediction */
            {
                /* top field prediction */
                form_prediction(backward_reference_frame, motion_vertical_field_select[0][1],
                    current_frame, 0, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8,
                    bx, by>>1, PMV[0][1][0], PMV[0][1][1]>>1, stw);

                /* bottom field prediction */
                form_prediction(backward_reference_frame, motion_vertical_field_select[1][1],
                    current_frame, 1, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8,
                    bx, by>>1, PMV[1][1][0], PMV[1][1][1]>>1, stw);
            }
        }
        else
        {
            /* field picture */
            if (motion_type==MC_FIELD)
            {
                /* field-based prediction */
                form_prediction(backward_reference_frame, motion_vertical_field_select[0][1],
                    current_frame, 0, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 16,
                    bx, by, PMV[0][1][0], PMV[0][1][1], stw);
            }
            else if (motion_type==MC_16X8)
            {
                form_prediction(backward_reference_frame, motion_vertical_field_select[0][1],
                    current_frame, 0, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8,
                    bx, by, PMV[0][1][0], PMV[0][1][1], stw);

                form_prediction(backward_reference_frame, motion_vertical_field_select[1][1],
                    current_frame, 0, Coded_Picture_Width<<1, Coded_Picture_Width<<1, 16, 8,
                    bx, by+8, PMV[1][1][0], PMV[1][1][1], stw);
            }
        }
    }
}

// Minor rewrite to use the function pointer array - Vlad59 04-20-2002
void  CMPEG2Decoder::form_prediction(uint8_t *src[], int sfield, uint8_t *dst[],
                            int dfield, int lx, int lx2, int w, int h, int x, int y,
                            int dx, int dy, int average_flag)
{
    if ( y+(dy>>1) < 0 )
    {
        return;
    }

    uint8_t *s = (src[0]+(sfield?lx2>>1:0)) + lx * (y + (dy>>1)) + x + (dx>>1);
    uint8_t *d = (dst[0]+(dfield?lx2>>1:0)) + lx * y + x;
    int flag = ((dx & 1)<<1) + (dy & 1);

    ppppf_motion[average_flag][w>>4][flag] (d, s, lx2, lx, h);

    if (chroma_format!=CHROMA444)
    {
        lx>>=1; lx2>>=1; w>>=1; x>>=1; dx/=2;
    }

    if (chroma_format==CHROMA420)
    {
        h>>=1; y>>=1; dy/=2;
    }

    /* Cb */
    s = (src[1]+(sfield?lx2>>1:0)) + lx * (y + (dy>>1)) + x + (dx>>1);
    d = (dst[1]+(dfield?lx2>>1:0)) + lx * y + x;
    flag = ((dx & 1)<<1) + (dy & 1);
    ppppf_motion[average_flag][w>>4][flag] (d, s, lx2, lx, h);

    /* Cr */
    s = (src[2]+(sfield?lx2>>1:0)) + lx * (y + (dy>>1)) + x + (dx>>1);
    d = (dst[2]+(dfield?lx2>>1:0)) + lx * y + x;
    ppppf_motion[average_flag][w>>4][flag] (d, s, lx2, lx, h);
}
