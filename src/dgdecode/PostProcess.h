#ifndef _POSTPROCESS_H
#define _POSTPROCESS_H

#include <cstdint>

#define DEC_MBC         45+300
#define DEC_MBR         36+300

/**** mode flags to control postprocessing actions ****/
#define PP_DEBLOCK_Y_H  0x00000001  /* Luma horizontal deblocking   */
#define PP_DEBLOCK_Y_V  0x00000002  /* Luma vertical deblocking     */
#define PP_DEBLOCK_C_H  0x00000004  /* Chroma horizontal deblocking */
#define PP_DEBLOCK_C_V  0x00000008  /* Chroma vertical deblocking   */
#define PP_DERING_Y     0x00000010  /* Luma deringing               */
#define PP_DERING_C     0x00000020  /* Chroma deringing             */
#define PP_DONT_COPY    0x10000000  /* Postprocessor will not copy src -> dst */
                                    /* instead, it will operate on dst only   */

/******************* general, useful macros ****************/
#define SIGN(a)    ( (a)<0 ? -1 : 1 )

typedef int QP_STORE_T;

/******************** component function prototypes **************/
int deblock_horiz_useDC(uint8_t *v, int stride, int moderate_h);
int deblock_horiz_DC_on(uint8_t *v, int stride, int QP);
void deblock_horiz_lpf9(uint8_t *v, int stride, int QP);
void deblock_horiz_default_filter(uint8_t *v, int stride, int QP);
void deblock_horiz(uint8_t *image, int width, int stride, QP_STORE_T *QP_store, int QP_stride, int chromaFlag, int moderate_h);
int deblock_vert_useDC(uint8_t *v, int stride, int moderate_v);
int deblock_vert_DC_on(uint8_t *v, int stride, int QP);
void deblock_vert_copy_and_unpack(int stride, uint8_t *source, uint64_t *dest, int n);
void deblock_vert_choose_p1p2(uint8_t *v, int stride, uint64_t *p1p2, int QP);
void deblock_vert_lpf9(uint64_t *v_local, uint64_t *p1p2, uint8_t *v, int stride);
void deblock_vert_default_filter(uint8_t *v, int stride, int QP);
void deblock_vert( uint8_t *image, int width, int stride, QP_STORE_T *QP_store, int QP_stride, int chromaFlag, int moderate_v);
void fast_copy(uint8_t *src, int src_stride,
               uint8_t *dst, int dst_stride,
               int horizontal_size, int vertical_size);
void dering( uint8_t *image, int width, int height, int stride, int *QP_store, int QP_stride, int chroma);

void postprocess(uint8_t * src[], int src_stride, int UVsrc_stride,
                 uint8_t * dst[], int dst_stride, int UVdst_stride,

                 int horizontal_size,   int vertical_size,
                 QP_STORE_T *QP_store,  int QP_stride,
                 int mode, int moderate_h, int moderate_v, bool is422, bool iPP);
int __cdecl dprintf(char* fmt, ...);
void do_emms();

// New Deringing Algo:
struct DERING_INFO
{
    uint8_t *rangearray;
    uint8_t *thrarray;
};


#endif
