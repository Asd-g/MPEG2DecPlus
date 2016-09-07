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


/* decode headers from one input stream */
int CMPEG2Decoder::Get_Hdr()
{
    for (;;)
    {
        /* look for next_start_code */
        Next_Start_Code();
        if (Fault_Flag == OUT_OF_BITS)
        {
            // We've run dry on data from the stream.
            return 0;
        }

        switch (Get_Bits(32))
        {
            case SEQUENCE_HEADER_CODE:
                Sequence_Header();
                Second_Field = 0;
                break;

            case GROUP_START_CODE:
                group_of_pictures_header();
                Second_Field = 0;
                break;

            case PICTURE_START_CODE:
                picture_header();
                return 1;
        }
    }
}


/* decode group of pictures header */
/* ISO/IEC 13818-2 section 6.2.2.6 */
__forceinline void CMPEG2Decoder::group_of_pictures_header()
{
#if 0
    Get_Bits(1); //drop_flag
    Get_Bits(5); //gop_hour
    Get_Bits(6); //gop_minute
    Flush_Buffer(1);    // marker bit
    Get_Bits(6); //gop_sec
    Get_Bits(6); //gop_frame
    Get_Bits(1); //closed_gop
    Get_Bits(1); //broken_link
#else
    Flush_Buffer(27);
#endif
    extension_and_user_data();
}


/* decode picture header */
/* ISO/IEC 13818-2 section 6.2.3 */
inline void CMPEG2Decoder::picture_header()
{
    temporal_reference  = Get_Bits(10);
    picture_coding_type = Get_Bits(3);
    Flush_Buffer(16);//Get_Bits(16); //vbv_delay

    if (picture_coding_type==P_TYPE || picture_coding_type==B_TYPE)
    {
        full_pel_forward_vector = Get_Bits(1);
        forward_f_code = Get_Bits(3);
    }

    if (picture_coding_type==B_TYPE)
    {
        full_pel_backward_vector = Get_Bits(1);
        backward_f_code = Get_Bits(3);
    }

    // MPEG1 defaults. May be overriden by picture coding extension.
    intra_dc_precision = 0;
    picture_structure = FRAME_PICTURE;
    top_field_first = 1;
    frame_pred_frame_dct = 1;
    concealment_motion_vectors = 0;
    q_scale_type = 0;
    intra_vlc_format = 0;
    alternate_scan = 0;
    repeat_first_field = 0;
    progressive_frame = 1;

    pf_current = progressive_frame;

    extra_bit_information(); // extra information byte count
    extension_and_user_data();
}


/* decode sequence header */
void CMPEG2Decoder::Sequence_Header()
{
    int i;

    horizontal_size = Get_Bits(12);
    vertical_size   = Get_Bits(12);
#if 0
    Get_Bits(4); //aspect_ratio_information
    Get_Bits(4); //frame_rate_code
    Get_Bits(18); //bit_rate_value
    Flush_Buffer(1); // marker bit
    Get_Bits(10); //vbv_buffer_size
    Get_Bits(1); //constrained_parameters_flag
#else
    Flush_Buffer(32);
    Flush_Buffer(6);
#endif

    if (load_intra_quantizer_matrix = Get_Bits(1))
    {
        for (i=0; i<64; i++)
            intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
    }
    else
    {
        for (i=0; i<64; i++)
            intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
    }

    if (load_non_intra_quantizer_matrix = Get_Bits(1))
    {
        for (i=0; i<64; i++)
            non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
    }
    else
    {
        for (i=0; i<64; i++)
            non_intra_quantizer_matrix[i] = 16;
    }

    /* copy luminance to chrominance matrices */
    for (i=0; i<64; i++)
    {
        chroma_intra_quantizer_matrix[i] = intra_quantizer_matrix[i];
        chroma_non_intra_quantizer_matrix[i] = non_intra_quantizer_matrix[i];
    }

    // These are MPEG1 defaults. These will be overridden if we have MPEG2
    // when the sequence header extension is parsed.
    progressive_sequence = 1;
    chroma_format = CHROMA420;
    matrix_coefficients = 5;

    extension_and_user_data();
}

/* decode slice header */
/* ISO/IEC 13818-2 section 6.2.4 */
int CMPEG2Decoder::slice_header()
{
    int slice_vertical_position_extension = 0;
    if (mpeg_type == IS_MPEG2 && vertical_size > 2800) {
        slice_vertical_position_extension = Get_Bits(3);
    }

    int quantizer_scale_code = Get_Bits(5);
    if (mpeg_type == IS_MPEG2)
        quantizer_scale = q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] : quantizer_scale_code<<1;
    else
        quantizer_scale = quantizer_scale_code;

    while (Get_Bits(1)) Flush_Buffer(8);

    return slice_vertical_position_extension;
}

/* decode extension and user data */
/* ISO/IEC 13818-2 section 6.2.2.2 */
void CMPEG2Decoder::extension_and_user_data()
{
    int code, ext_ID;

    Next_Start_Code();

    while ((code = Show_Bits(32))==EXTENSION_START_CODE || code==USER_DATA_START_CODE)
    {
        if (Fault_Flag == OUT_OF_BITS) return;

        if (code==EXTENSION_START_CODE)
        {
            Flush_Buffer(32);
            ext_ID = Get_Bits(4);

            switch (ext_ID)
            {
                case SEQUENCE_EXTENSION_ID:
                    sequence_extension();
                    break;

                case SEQUENCE_DISPLAY_EXTENSION_ID:
                    sequence_display_extension();
                    break;

                case QUANT_MATRIX_EXTENSION_ID:
                    quant_matrix_extension();
                    break;

                case PICTURE_DISPLAY_EXTENSION_ID:
                    picture_display_extension();
                    break;

                case PICTURE_CODING_EXTENSION_ID:
                    picture_coding_extension();
                    break;

                case COPYRIGHT_EXTENSION_ID:
                    copyright_extension();
                    break;
            }
            Next_Start_Code();
        }
        else
        {
            Flush_Buffer(32);
            Next_Start_Code();
        }
    }
}

/* decode sequence extension */
/* ISO/IEC 13818-2 section 6.2.2.3 */
__forceinline void CMPEG2Decoder::sequence_extension()
{
    Flush_Buffer(8); //Get_Bits(8); //profile_and_level_indication
    progressive_sequence          = Get_Bits(1);
    chroma_format                 = Get_Bits(2);
    int horizontal_size_extension = Get_Bits(2) << 12;
    int vertical_size_extension   = Get_Bits(2) << 12;
#if 0
    Get_Bits(12); //bit_rate_extension
    Flush_Buffer(1);    // marker bit
    Get_Bits(8); //vbv_buffer_size_extension
    Get_Bits(1); //low_delay

    Get_Bits(2); //frame_rate_extension_n
    Get_Bits(5); //frame_rate_extension_d
#else
    Flush_Buffer(29);
#endif

    horizontal_size = horizontal_size_extension | (horizontal_size & 0x0fff);
    vertical_size = vertical_size_extension | (vertical_size & 0x0fff);
}

/* decode sequence display extension */
__forceinline void CMPEG2Decoder::sequence_display_extension()
{
    Flush_Buffer(3);// Get_Bits(3); //video_format

    matrix_coefficients = 1;
    if (Get_Bits(1)) //color_description
    {
        //Get_Bits(8); //color_primaries
        //Get_Bits(8); //transfer_characteristics
        Flush_Buffer(16);
        matrix_coefficients = Get_Bits(8);
    }
#if 0
    Get_Bits(14); //display_horizontal_size
    Flush_Buffer(1); // marker bit
    Get_Bits(14); //display_vertical_size
#else
    Flush_Buffer(29);
#endif
}

/* decode quant matrix entension */
/* ISO/IEC 13818-2 section 6.2.3.2 */
void CMPEG2Decoder::quant_matrix_extension()
{
    int i;

    if (load_intra_quantizer_matrix = Get_Bits(1))
        for (i=0; i<64; i++)
            chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
                = intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

    if (load_non_intra_quantizer_matrix = Get_Bits(1))
        for (i=0; i<64; i++)
            chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
                = non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

    if (load_chroma_intra_quantizer_matrix = Get_Bits(1))
        for (i=0; i<64; i++)
            chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

    if (load_chroma_non_intra_quantizer_matrix = Get_Bits(1))
        for (i=0; i<64; i++)
            chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
}

/* decode picture display extension */
/* ISO/IEC 13818-2 section 6.2.3.3. */
void CMPEG2Decoder::picture_display_extension()
{
    int frame_center_horizontal_offset[3];
    int frame_center_vertical_offset[3];

    int i;
    int number_of_frame_center_offsets;

    /* based on ISO/IEC 13818-2 section 6.3.12
       (November 1994) Picture display extensions */

    /* derive number_of_frame_center_offsets */
    if (progressive_sequence)
    {
        if (repeat_first_field)
        {
            if (top_field_first)
                number_of_frame_center_offsets = 3;
            else
                number_of_frame_center_offsets = 2;
        }
        else
            number_of_frame_center_offsets = 1;
    }
    else
    {
        if (picture_structure!=FRAME_PICTURE)
            number_of_frame_center_offsets = 1;
        else
        {
            if (repeat_first_field)
                number_of_frame_center_offsets = 3;
            else
                number_of_frame_center_offsets = 2;
        }
    }

    /* now parse */
    for (i=0; i<number_of_frame_center_offsets; i++)
    {
        frame_center_horizontal_offset[i] = Get_Bits(16);
        Flush_Buffer(1);    // marker bit

        frame_center_vertical_offset[i] = Get_Bits(16);
        Flush_Buffer(1);    // marker bit
    }
}

/* decode picture coding extension */
void CMPEG2Decoder::picture_coding_extension()
{
    f_code[0][0] = Get_Bits(4);
    f_code[0][1] = Get_Bits(4);
    f_code[1][0] = Get_Bits(4);
    f_code[1][1] = Get_Bits(4);

    intra_dc_precision         = Get_Bits(2);
    picture_structure          = Get_Bits(2);
    top_field_first            = Get_Bits(1);
    frame_pred_frame_dct       = Get_Bits(1);
    concealment_motion_vectors = Get_Bits(1);
    q_scale_type               = Get_Bits(1);
    intra_vlc_format           = Get_Bits(1);
    alternate_scan             = Get_Bits(1);
    repeat_first_field         = Get_Bits(1);
    Get_Bits(1); //uint32_t chroma_420_type
    progressive_frame          = Get_Bits(1);
    uint32_t composite_display_flag = Get_Bits(1);

    if (picture_structure != FRAME_PICTURE)
    {
        if (picture_structure == TOP_FIELD)
            top_field_first = 1;
        else
            top_field_first = 0;
        repeat_first_field = 0;
        progressive_frame  = 0;
    }

    pf_current = progressive_frame;

    if (composite_display_flag != 0)
    {
        uint32_t v_axis            = Get_Bits(1);
        uint32_t field_sequence    = Get_Bits(3);
        uint32_t sub_carrier       = Get_Bits(1);
        uint32_t burst_amplitude   = Get_Bits(7);
        uint32_t sub_carrier_phase = Get_Bits(8);
    }
}

/* decode extra bit information */
/* ISO/IEC 13818-2 section 6.2.3.4. */
__forceinline int CMPEG2Decoder::extra_bit_information()
{
    int byte_count = 0;

    while (Get_Bits(1))
    {
        if (Fault_Flag == OUT_OF_BITS)
            return byte_count;
        Flush_Buffer(8);
        ++byte_count;
    }

    return byte_count;
}

/* Copyright extension */
/* ISO/IEC 13818-2 section 6.2.3.6. */
/* (header added in November, 1994 to the IS document) */
__forceinline void CMPEG2Decoder::copyright_extension()
{
#if 0
    Get_Bits(1); //copyright_flag
    Get_Bits(8); //copyright_identifier
    Get_Bits(1); //original_or_copy

    /* reserved */
    Get_Bits(7); //reserved_data

    Flush_Buffer(1); // marker bit
    Get_Bits(20); //copyright_number_1
    Flush_Buffer(1); // marker bit
    Get_Bits(22); //copyright_number_2
    Flush_Buffer(1); // marker bit
    Get_Bits(22); //copyright_number_3
#else
    Flush_Buffer(32);
    Flush_Buffer(32);
    Flush_Buffer(20);
#endif
}
