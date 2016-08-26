#ifndef MPEG2DECODER_H
#define MPEG2DECODER_H


#include <cstdint>
#include <cstdio>
#include <cmath>
#include <io.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winreg.h>


#ifndef MPEG2DEC_EXPORTS
#define MPEG2DEC_EXPORTS
#endif


#ifdef MPEG2DEC_EXPORTS
#define MPEG2DEC_API __declspec(dllexport)
#else
#define MPEG2DEC_API __declspec(dllimport)
#endif

#ifdef _DEBUG
#define _INLINE_
#else
#define _INLINE_ inline
#endif

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
    IDCT_MMX        = 1,
    IDCT_SSEMMX     = 2,
    IDCT_SSE2MMX    = 3,
    IDCT_FPU        = 4,
    IDCT_REF        = 5,
    IDCT_SKALSSE    = 6,
    IDCT_SIMPLEIDCT = 7,
};

enum {
    FO_NONE = 0,
    FO_FILM = 1,
    FO_RAW  = 2,
};

// Fault_Flag values
#define OUT_OF_BITS 11


typedef void (WINAPI *PBufferOp) (uint8_t*, int, int);



struct YV12PICT {
    uint8_t *y, *u, *v;
    int ypitch, uvpitch;
    int ywidth, uvwidth;
    int yheight, uvheight;
    int pf;
};

extern "C" YV12PICT* create_YV12PICT(int height, int width, int chroma_format);
extern "C" void destroy_YV12PICT(YV12PICT* pict);

#define BUFFER_SIZE         2048
#define MAX_FILE_NUMBER     256


class MPEG2DEC_API CMPEG2Decoder
{
    friend class MPEG2Source;

protected:
    bool refinit,fpuinit,luminit;
    int moderate_h, moderate_v, pp_mode;

    // getbit.cpp
    void Initialize_Buffer(void);
    void Fill_Buffer(void);
    void Next_Transport_Packet(void);
    void Next_PVA_Packet(void);
    void Next_Packet(void);
    void Flush_Buffer_All(uint32_t N);
    uint32_t Get_Bits_All(uint32_t N);
    void Next_File(void);

    inline uint32_t Show_Bits(uint32_t N);
    inline uint32_t Get_Bits(uint32_t N);
    inline void Flush_Buffer(uint32_t N);
    inline void Fill_Next(void);
    inline uint32_t Get_Byte(void);
    inline uint32_t Get_Short(void);
    inline void next_start_code(void);

    uint8_t Rdbfr[BUFFER_SIZE], *Rdptr, *Rdmax;
    uint32_t CurrentBfr, NextBfr, BitsLeft, Val, Read;
    uint8_t *buffer_invalid;

    // gethdr.cpp
    int Get_Hdr(void);
    void sequence_header(void);
    int slice_header(void);
private:
    inline void group_of_pictures_header(void);
    inline void picture_header(void);
    void sequence_extension(void);
    void sequence_display_extension(void);
    void quant_matrix_extension(void);
    void picture_display_extension(void);
    void picture_coding_extension(void);
    void copyright_extension(void);
    int  extra_bit_information(void);
    void extension_and_user_data(void);

protected:
    // getpic.cpp
    void Decode_Picture(YV12PICT *dst);

private:
    inline void Update_Picture_Buffers(void);
    inline void picture_data(void);
    inline void slice(int MBAmax, uint32_t code);
    inline void macroblock_modes(int *pmacroblock_type, int *pmotion_type,
        int *pmotion_vector_count, int *pmv_format, int *pdmv, int *pmvscale, int *pdct_type);
    inline void Clear_Block(int count);
    inline void Add_Block(int count, int bx, int by, int dct_type, int addflag);
    inline void motion_compensation(int MBA, int macroblock_type, int motion_type,
        int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2], int dct_type);
    inline void skipped_macroblock(int dc_dct_pred[3], int PMV[2][2][2],
        int *motion_type, int motion_vertical_field_select[2][2], int *macroblock_type);
    inline void decode_macroblock(int *macroblock_type, int *motion_type, int *dct_type,
        int PMV[2][2][2], int dc_dct_pred[3], int motion_vertical_field_select[2][2], int dmvector[2]);
    inline void Decode_MPEG1_Intra_Block(int comp, int dc_dct_pred[]);
    inline void Decode_MPEG1_Non_Intra_Block(int comp);
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

    // inline?
    void form_predictions(int bx, int by, int macroblock_type, int motion_type,
        int PMV[2][2][2], int motion_vertical_field_select[2][2], int dmvector[2]);

    // inline?
    inline void form_prediction(uint8_t *src[], int sfield, uint8_t *dst[], int dfield,
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

protected:
    // store.cpp
    void assembleFrame(uint8_t *src[], int pf, YV12PICT *dst);

    // decoder operation control flags
    int Fault_Flag;
    int File_Flag;
    int File_Limit;
    int FO_Flag;
    int IDCT_Flag;
    void(__fastcall *idctFunction)(int16_t* block);
    void(__fastcall *prefetchTables)();
    int SystemStream_Flag;    // 0 = none, 1=program, 2=Transport 3=PVA

    int TransportPacketSize;
    int MPEG2_Transport_AudioPID;  // used only for transport streams
    int MPEG2_Transport_VideoPID;  // used only for transport streams
    int MPEG2_Transport_PCRPID;  // used only for transport streams

    int lfsr0, lfsr1;
    PBufferOp BufferOp;

    int Infile[MAX_FILE_NUMBER];
    char *Infilename[MAX_FILE_NUMBER];
    uint32_t BadStartingFrames;
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
    uint8_t *u422, *v422;
    YV12PICT *auxFrame1;
    YV12PICT *auxFrame2;
    YV12PICT *saved_active;
    YV12PICT *saved_store;


public:
    int Clip_Width, Clip_Height;
    int D2V_Width, D2V_Height;
    int Clip_Top, Clip_Bottom, Clip_Left, Clip_Right;
    char Aspect_Ratio[20];

protected:

#define ELEMENTARY_STREAM 0
#define MPEG1_PROGRAM_STREAM 1
#define MPEG2_PROGRAM_STREAM 2
#define IS_NOT_MPEG 0
#define IS_MPEG1 1
#define IS_MPEG2 2

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

    // interface
    struct GOPLIST {
        uint32_t        number;
        int             file;
        int64_t         position;
        uint32_t    I_count;
        int             closed;
        int             progressive;
        int             matrix;
    };
    GOPLIST **GOPList;
    int GOPListSize;

    struct FRAMELIST {
        uint32_t top;
        uint32_t bottom;
        uint8_t pf;
        uint8_t pct;
    };
    FRAMELIST *FrameList;

    char *DirectAccess;

public:
    int Field_Order;
    bool HaveRFFs;

private:
    inline void CopyAll(YV12PICT *src, YV12PICT *dst);
    inline void CopyTop(YV12PICT *src, YV12PICT *dst);
    inline void CopyBot(YV12PICT *src, YV12PICT *dst);
    inline void CopyTopBot(YV12PICT *odd, YV12PICT *even, YV12PICT *dst);
//    inline void CopyPlane(uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch,
//        int width, int height);

public:
    FILE*     VF_File;
    int       VF_FrameRate;
    uint32_t  VF_FrameRate_Num;
    uint32_t  VF_FrameRate_Den;
    uint32_t  VF_FrameLimit;
    uint32_t  VF_GOPLimit;
    uint32_t  prev_frame;
    int horizontal_size, vertical_size, mb_width, mb_height;


    CMPEG2Decoder();
    int Open(const char *path);
    void Close();
    void Decode(uint32_t frame, YV12PICT *dst);

    int iPP, iCC;
    bool fastMC;
    bool showQ;
    int *QP, *backwardQP, *auxQP;
    int upConv;
    bool i420;
    int pc_scale;

    // info option stuff
    int info;
    int minquant, maxquant, avgquant;

    // Luminance Code
    bool Luminance_Flag;
    uint8_t LuminanceTable[256];

    void InitializeLuminanceFilter(int LumGamma, int LumOffset)
    {
        double value;

        for (int i=0; i<256; i++)
        {
            value = 255.0 * pow(i/255.0, pow(2.0, -LumGamma/128.0)) + LumOffset + 0.5;

            if (value < 0)
                LuminanceTable[i] = 0;
            else if (value > 255.0)
                LuminanceTable[i] = 255;
            else
                LuminanceTable[i] = (uint8_t)value;
        }
    }

    void LuminanceFilter(uint8_t *src, int width_in, int height_in, int pitch_in)
    {
        for (int y=0; y<height_in; ++y)
        {
            for (int x=0; x<width_in; ++x)
            {
                src[x] = LuminanceTable[src[x]];
            }
            src += pitch_in;
        }
    }
    // end luminance code
};



#endif

