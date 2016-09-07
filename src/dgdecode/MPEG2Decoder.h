#ifndef MPEG2DECODER_H
#define MPEG2DECODER_H


#include <cstdint>
#include <cstdio>
#include <cmath>
#include <vector>
#include <io.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winreg.h>

#include "yv12pict.h"


/* code definition */
enum {
    PICTURE_START_CODE      = 0x100,
    SLICE_START_CODE_MIN    = 0x101,
    SLICE_START_CODE_MAX    = 0x1AF,
    USER_DATA_START_CODE    = 0x1B2,
    SEQUENCE_HEADER_CODE    = 0x1B3,
    EXTENSION_START_CODE    = 0x1B5,
    SEQUENCE_END_CODE       = 0x1B7,
    GROUP_START_CODE        = 0x1B8,

    SYSTEM_END_CODE         = 0x1B9,
    PACK_START_CODE         = 0x1BA,
    SYSTEM_START_CODE       = 0x1BB,
    PRIVATE_STREAM_1        = 0x1BD,
    VIDEO_ELEMENTARY_STREAM = 0x1E0,
};

/* extension start code IDs */
enum {
    SEQUENCE_EXTENSION_ID         = 1,
    SEQUENCE_DISPLAY_EXTENSION_ID = 2,
    QUANT_MATRIX_EXTENSION_ID     = 3,
    COPYRIGHT_EXTENSION_ID        = 4,
    PICTURE_DISPLAY_EXTENSION_ID  = 7,
    PICTURE_CODING_EXTENSION_ID   = 8,
};

enum {
    ZIG_ZAG   =  0,
    MB_WEIGHT = 32,
    MB_CLASS4 = 64,
};

enum {
    I_TYPE = 1,
    P_TYPE = 2,
    B_TYPE = 3,
    D_TYPE = 4,
};

enum {
    TOP_FIELD     = 1,
    BOTTOM_FIELD  = 2,
    FRAME_PICTURE = 3,
};

enum {
    MC_FIELD = 1,
    MC_FRAME = 2,
    MC_16X8  = 2,
    MC_DMV   = 3,
};

enum {
    MV_FIELD,
    MV_FRAME,
};

enum {
    CHROMA420 = 1,
    CHROMA422 = 2,
    CHROMA444 = 3,
};




enum {
    IDCT_AUTO      = 0,
    IDCT_AP922_INT = 3,
    IDCT_LLM_FLOAT = 4,
    IDCT_REF       = 5,
};

enum {
    FO_NONE = 0,
    FO_FILM = 1,
    FO_RAW  = 2,
};

// Fault_Flag values
#define OUT_OF_BITS 11


struct GOPLIST {
    uint32_t        number;
    int             file;
    int64_t         position;
    uint32_t    I_count;
    int             closed;
    int             progressive;
    int             matrix;
    GOPLIST(int _film, int _matrix, int _file, int64_t pos, int ic, uint32_t type)
    {
        number = _film;
        matrix = (_matrix < 0 || _matrix > 7) ? 3 : _matrix; // 3:reserved
        file = _file;
        position = pos;
        I_count = ic;
        closed = !!(type & 0x0400);
        progressive = !!(type & 0x0200);
    }
};

struct FRAMELIST {
    uint32_t top;
    uint32_t bottom;
    uint8_t pf;
    uint8_t pct;
};


constexpr size_t BUFFER_SIZE = 128 * 1024; // 128KiB


class CMPEG2Decoder
{
    //int moderate_h, moderate_v, pp_mode;

    // getbit.cpp
    void Initialize_Buffer(void);
    void Fill_Buffer(void);
    void Next_Transport_Packet(void);
    void Next_PVA_Packet(void);
    void Next_Packet(void);
    void Next_File(void);

    uint32_t Show_Bits(uint32_t N);
    uint32_t Get_Bits(uint32_t N);
    void Flush_Buffer(uint32_t N);
    void Fill_Next(void);
    uint32_t Get_Byte(void);
    uint32_t Get_Short(void);
    void Next_Start_Code(void);

    std::vector<uint8_t> ReadBuffer;
    uint8_t *Rdbfr, *Rdptr, *Rdmax;
    uint32_t CurrentBfr, NextBfr, BitsLeft, Val, Read;
    uint8_t *buffer_invalid;

    // gethdr.cpp
    int Get_Hdr(void);
    void Sequence_Header(void);
    int slice_header(void);
    void group_of_pictures_header(void);
    void picture_header(void);
    void sequence_extension(void);
    void sequence_display_extension(void);
    void quant_matrix_extension(void);
    void picture_display_extension(void);
    void picture_coding_extension(void);
    void copyright_extension(void);
    int  extra_bit_information(void);
    void extension_and_user_data(void);

    // getpic.cpp
    void Decode_Picture(YV12PICT& dst);
    inline void update_picture_buffers(void);
    inline void picture_data(void);
    inline void slice(int MBAmax, uint32_t code);
    inline void macroblock_modes(int& pmacroblock_type, int& pmotion_type,
        int& pmotion_vector_count, int& pmv_format, int& pdmv, int& pmvscale, int& pdct_type);
    inline void clear_block(int count);
    inline void add_block(int count, int bx, int by, int dct_type, int addflag);
    inline void motion_compensation(int MBA, int macroblock_type, int motion_type,
        int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2], int dct_type);
    inline void skipped_macroblock(int dc_dct_pred[3], int PMV[2][2][2],
        int& motion_type, int motion_vertical_field_select[2][2], int& macroblock_type);
    inline void decode_macroblock(int& macroblock_type, int& motion_type, int& dct_type,
        int PMV[2][2][2], int dc_dct_pred[3], int motion_vertical_field_select[2][2], int dmvector[2]);
    inline void decode_mpeg1_intra_block(int comp, int dc_dct_pred[]);
    inline void decode_mpeg1_non_intra_block(int comp);
    inline void Decode_MPEG2_Intra_Block(int comp, int dc_dct_pred[]);
    inline void Decode_MPEG2_Non_Intra_Block(int comp);

    inline int Get_macroblock_type(void);
    inline int Get_I_macroblock_type(void);
    inline int Get_P_macroblock_type(void);
    inline int Get_B_macroblock_type(void);
    inline int Get_D_macroblock_type(void);
    inline int Get_coded_block_pattern(void);
    inline int Get_macroblock_address_increment(void);
    inline int Get_Luma_DC_dct_diff(void);
    inline int Get_Chroma_DC_dct_diff(void);

    void form_predictions(int bx, int by, int macroblock_type, int motion_type,
        int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2]);

    void form_prediction(uint8_t *src[], int sfield, uint8_t *dst[], int dfield,
        int lx, int lx2, int w, int h, int x, int y, int dx, int dy, int average_flag);

    // motion.cpp
    void motion_vectors(int PMV[2][2][2], int dmvector[2], int motion_vertical_field_select[2][2],
        int s, int motion_vector_count, int mv_format,
        int h_r_size, int v_r_size, int dmv, int mvscale);
    void Dual_Prime_Arithmetic(int DMV[][2], int *dmvector, int mvx, int mvy);

    inline void motion_vector(int *PMV, int *dmvector, int h_r_size, int v_r_size,
        int dmv, int mvscale, int full_pel_vector);
    inline void decode_motion_vector(int *pred, int r_size, int motion_code,
        int motion_residualesidual, int full_pel_vector);
    inline int Get_motion_code(void);
    inline int Get_dmvector(void);

    // store.cpp
    void assembleFrame(uint8_t *src[], int pf, YV12PICT& dst);

    // decoder operation control flags
    int Fault_Flag;
    int File_Flag;
    int FO_Flag;
    void(__fastcall *idctFunction)(int16_t* block);
    void(__fastcall *prefetchTables)();
    int SystemStream_Flag;    // 0 = none, 1=program, 2=Transport 3=PVA

    int TransportPacketSize;
    int MPEG2_Transport_AudioPID;  // used only for transport streams
    int MPEG2_Transport_VideoPID;  // used only for transport streams
    int MPEG2_Transport_PCRPID;  // used only for transport streams

    int lfsr0, lfsr1;

    std::vector<int> Infile;
    int closed_gop;

    int intra_quantizer_matrix[64];
    int non_intra_quantizer_matrix[64];
    int chroma_intra_quantizer_matrix[64];
    int chroma_non_intra_quantizer_matrix[64];

    int load_intra_quantizer_matrix;
    int load_non_intra_quantizer_matrix;
    int load_chroma_intra_quantizer_matrix;
    int load_chroma_non_intra_quantizer_matrix;

    int q_scale_type;
    int alternate_scan;
    int quantizer_scale;

    short *block[8], *p_block[8];
    int pf_backward, pf_forward, pf_current;

    // global values
    uint8_t *backward_reference_frame[3], *forward_reference_frame[3];
    uint8_t *auxframe[3], *current_frame[3];
    //uint8_t *u422, *v422;
    YV12PICT *auxFrame1;
    YV12PICT *auxFrame2;
    YV12PICT *saved_active;
    YV12PICT *saved_store;

    enum {
        ELEMENTARY_STREAM = 0,
        MPEG1_PROGRAM_STREAM,
        MPEG2_PROGRAM_STREAM,
    };

    enum {
        IS_NOT_MPEG = 0,
        IS_MPEG1,
        IS_MPEG2,
    };

    int mpeg_type;
    int Coded_Picture_Width, Coded_Picture_Height, Chroma_Width, Chroma_Height;
    int block_count, Second_Field;

    /* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
    int progressive_sequence;
    int chroma_format;
    int matrix_coefficients;

    /* ISO/IEC 13818-2 section 6.2.3: picture_header() */
    int picture_coding_type;
    int temporal_reference;
    int full_pel_forward_vector;
    int forward_f_code;
    int full_pel_backward_vector;
    int backward_f_code;

    /* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header */
    int f_code[2][2];
    int picture_structure;
    int frame_pred_frame_dct;
    int progressive_frame;
    int concealment_motion_vectors;
    int intra_dc_precision;
    int top_field_first;
    int repeat_first_field;
    int intra_vlc_format;

    void copy_all(YV12PICT& src, YV12PICT& dst);
    void copy_top(YV12PICT& src, YV12PICT& dst);
    void copy_bottom(YV12PICT& src, YV12PICT& dst);
    //inline void CopyTopBot(YV12PICT *odd, YV12PICT *even, YV12PICT *dst);

    int *QP, *backwardQP, *auxQP;
    uint32_t  prev_frame;

    std::vector<char> DirectAccess;

public:
    CMPEG2Decoder();
    int Open(FILE* file, const char* path);
    void Close();
    void Decode(uint32_t frame, YV12PICT& dst);

    std::vector<std::string> Infilename;
    uint32_t BadStartingFrames;

    int Clip_Width, Clip_Height;
    int D2V_Width, D2V_Height;
    int Clip_Top, Clip_Bottom, Clip_Left, Clip_Right;
    char Aspect_Ratio[20];

    std::vector<GOPLIST> GOPList;
    std::vector<FRAMELIST> FrameList;

    int Field_Order;
    bool HaveRFFs;

    int       VF_FrameRate;
    uint32_t  VF_FrameRate_Num;
    uint32_t  VF_FrameRate_Den;

    int horizontal_size, vertical_size, mb_width, mb_height;
    //int iPP;
    int iCC;
    bool showQ;
    int upConv;
    bool i420;

    // info option stuff
    int info;
    int minquant, maxquant, avgquant;

    // Luminance Code
    int lumGamma;
    int lumOffset;

    int getChromaFormat() { return chroma_format; }
    int getChromaWidth() { return Chroma_Width; }
    int getLumaWidth() { return Coded_Picture_Width; }
    int getLumaHeight() { return Coded_Picture_Height; }
    void setIDCT(int idct);
};


__forceinline uint32_t CMPEG2Decoder::Show_Bits(uint32_t N)
{
    if (N <= BitsLeft) {
        return (CurrentBfr << (32 - BitsLeft)) >> (32 - N);;
    } else {
        N -= BitsLeft;
        int shift = 32 - BitsLeft;
        //return (((CurrentBfr << shift) >> shift) << N) + (NextBfr >> (32 - N));;
        return ((CurrentBfr << shift) >> (shift - N)) | (NextBfr >> (32 - N));
    }
}

__forceinline uint32_t CMPEG2Decoder::Get_Bits(uint32_t N)
{
    if (N < BitsLeft) {
        Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - N);
        BitsLeft -= N;
        return Val;
    } else {
        N -= BitsLeft;
        int shift = 32 - BitsLeft;
        Val = (CurrentBfr << shift) >> shift;
        if (N != 0)
            Val = (Val << N) | (NextBfr >> (32 - N));
        CurrentBfr = NextBfr;
        BitsLeft = 32 - N;
        Fill_Next();
        return Val;
    }
}


__forceinline void CMPEG2Decoder::Flush_Buffer(uint32_t N)
{
    if (N < BitsLeft) {
        BitsLeft -= N;
    } else {
        CurrentBfr = NextBfr;
        BitsLeft += 32 - N;
        Fill_Next();
    }
}


__forceinline void CMPEG2Decoder::Fill_Next()
{
    if (SystemStream_Flag && Rdptr > Rdmax - 4) {
        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr = Get_Byte() << 24;

        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr |= Get_Byte() << 16;

        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr |= Get_Byte() << 8;

        if (Rdptr >= Rdmax)
            Next_Packet();
        NextBfr |= Get_Byte();
    } else if (Rdptr < Rdbfr + BUFFER_SIZE - 3) {
        //NextBfr = (*Rdptr << 24) + (*(Rdptr+1) << 16) + (*(Rdptr+2) << 8) + *(Rdptr+3);
        NextBfr = _byteswap_ulong(*reinterpret_cast<uint32_t*>(Rdptr));
        Rdptr += 4;
    } else {
        switch (Rdbfr + BUFFER_SIZE - Rdptr) {
        case 1:
            NextBfr = *Rdptr++ << 24;
            Fill_Buffer();
            NextBfr |= (Rdptr[0] << 16) | (Rdptr[1] << 8) | Rdptr[2];
            Rdptr += 3;
            break;
        case 2:
            NextBfr = (Rdptr[0] << 24) | (Rdptr[1] << 16);
            Rdptr += 2;
            Fill_Buffer();
            NextBfr |= (Rdptr[0] << 8) | Rdptr[1];
            Rdptr += 2;
            break;
        case 3:
            NextBfr = (Rdptr[0] << 24) | (Rdptr[1] << 16) | (Rdptr[2] << 8);
            Rdptr += 3;
            Fill_Buffer();
            NextBfr |= *Rdptr++;
            break;
        default:
            Fill_Buffer();
            NextBfr = _byteswap_ulong(*reinterpret_cast<uint32_t*>(Rdptr));
            Rdptr += 4;
        }
    }
}


__forceinline void CMPEG2Decoder::Fill_Buffer()
{
    Read = _read(Infile[File_Flag], Rdbfr, BUFFER_SIZE);

    if (Read < BUFFER_SIZE)
        Next_File();

    Rdptr = Rdbfr;

    if (SystemStream_Flag)
        Rdmax -= BUFFER_SIZE;
}


__forceinline uint32_t CMPEG2Decoder::Get_Byte()
{
    while (Rdptr >= (Rdbfr + BUFFER_SIZE)) {
        Read = _read(Infile[File_Flag], Rdbfr, BUFFER_SIZE);
        if (Read < BUFFER_SIZE)
            Next_File();
        Rdptr -= BUFFER_SIZE;
        Rdmax -= BUFFER_SIZE;
    }

    return *Rdptr++;
}

__forceinline uint32_t CMPEG2Decoder::Get_Short()
{
    uint32_t i = Get_Byte();
    return (i<<8) + Get_Byte();
}


__forceinline void CMPEG2Decoder::Next_Start_Code()
{
    // This is contrary to the spec but is more resilient to some
    // stream corruption scenarios.
    BitsLeft = (BitsLeft + 7) & ~7;

    do {
        uint32_t show = Show_Bits(24);
        if (Fault_Flag == OUT_OF_BITS)
            return;
        if (show == 0x000001)
            return;
        Flush_Buffer(8);
    } while (true);
}

#endif

