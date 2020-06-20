/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user with any license fee or
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

#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <cstdint>


//#include "misc.h"


#ifdef GLOBAL
#define XTN
#else
#define XTN extern
#endif

enum {
    MACROBLOCK_INTRA           = 1,
    MACROBLOCK_PATTERN         = 2,
    MACROBLOCK_MOTION_BACKWARD = 4,
    MACROBLOCK_MOTION_FORWARD  = 8,
    MACROBLOCK_QUANT           = 16,
};


/* default intra quantization matrix */
XTN uint8_t default_intra_quantizer_matrix[64]
#ifdef GLOBAL
=
{
    8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
}
#endif
;

/* zig-zag and alternate scan patterns */
XTN uint8_t scan[2][64]
#ifdef GLOBAL
=
{
    { /* Zig-Zag scan pattern  */
        0,  1,  8, 16,  9,  2,  3, 10,
       17, 24, 32, 25, 18, 11,  4,  5,
       12, 19, 26, 33, 40, 48, 41, 34,
       27, 20, 13,  6,  7, 14, 21, 28,
       35, 42, 49, 56, 57, 50, 43, 36,
       29, 22, 15, 23, 30, 37, 44, 51,
       58, 59, 52, 45, 38, 31, 39, 46,
       53, 60, 61, 54, 47, 55, 62, 63
    }
    ,
    { /* Alternate scan pattern */
        0,  8, 16, 24,  1,  9,  2, 10,
       17, 25, 32, 40, 48, 56, 57, 49,
       41, 33, 26, 18,  3, 11, 4,  12,
       19, 27, 34, 42, 50, 58, 35, 43,
       51, 59, 20, 28,  5, 13,  6, 14,
       21, 29, 36, 44, 52, 60, 37, 45,
       53, 61, 22, 30,  7, 15, 23, 31,
       38, 46, 54, 62, 39, 47, 55, 63
    }
}
#endif
;

/* non-linear quantization coefficient table */
XTN uint8_t Non_Linear_quantizer_scale[32]
#ifdef GLOBAL
=
{
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 10, 12, 14, 16, 18, 20, 22,
    24, 28, 32, 36, 40, 44, 48, 52,
    56, 64, 72, 80, 88, 96, 104, 112
}
#endif
;

#define ERROR_VALUE (-1)

struct DCTtab {
    char run, level, len;
};

struct VLCtab {
    char val, len;
};

/* Table B-10, motion_code, codes 0001 ... 01xx */
XTN VLCtab MVtab0[8]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0}, {3,3}, {2,2}, {2,2}, {1,1}, {1,1}, {1,1}, {1,1}
}
#endif
;

/* Table B-10, motion_code, codes 0000011 ... 000011x */
XTN VLCtab MVtab1[8]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {7,6}, {6,6}, {5,6}, {4,5}, {4,5}
}
#endif
;

/* Table B-10, motion_code, codes 0000001100 ... 000001011x */
XTN VLCtab MVtab2[12]
#ifdef GLOBAL
=
{
    {16,9}, {15,9}, {14,9}, {13,9},
    {12,9}, {11,9}, {10,8}, {10,8},
    {9,8},  {9,8},  {8,8},  {8,8}
}
#endif
;

/* Table B-9, coded_block_pattern, codes 01000 ... 111xx */
XTN VLCtab CBPtab0[32]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0},
    {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0},
    {62,5}, {2,5},  {61,5}, {1,5},  {56,5}, {52,5}, {44,5}, {28,5},
    {40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
    {8,4},  {8,4},  {4,4},  {4,4},  {60,3}, {60,3}, {60,3}, {60,3}
}
#endif
;

/* Table B-9, coded_block_pattern, codes 00000100 ... 001111xx */
XTN VLCtab CBPtab1[64]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0}, {ERROR_VALUE,0},
    {58,8}, {54,8}, {46,8}, {30,8},
    {57,8}, {53,8}, {45,8}, {29,8}, {38,8}, {26,8}, {37,8}, {25,8},
    {43,8}, {23,8}, {51,8}, {15,8}, {42,8}, {22,8}, {50,8}, {14,8},
    {41,8}, {21,8}, {49,8}, {13,8}, {35,8}, {19,8}, {11,8}, {7,8},
    {34,7}, {34,7}, {18,7}, {18,7}, {10,7}, {10,7}, {6,7},  {6,7},
    {33,7}, {33,7}, {17,7}, {17,7}, {9,7},  {9,7},  {5,7},  {5,7},
    {63,6}, {63,6}, {63,6}, {63,6}, {3,6},  {3,6},  {3,6},  {3,6},
    {36,6}, {36,6}, {36,6}, {36,6}, {24,6}, {24,6}, {24,6}, {24,6}
}
#endif
;

/* Table B-9, coded_block_pattern, codes 000000001 ... 000000111 */
XTN VLCtab CBPtab2[8]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0}, {0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9}
}
#endif
;

/* Table B-1, macroblock_address_increment, codes 00010 ... 011xx */
XTN VLCtab MBAtab1[16]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0}, {ERROR_VALUE,0}, {7,5}, {6,5}, {5,4}, {5,4}, {4,4},
    {4,4}, {3,3}, {3,3}, {3,3}, {3,3}, {2,3}, {2,3}, {2,3}, {2,3}
}
#endif
;

/* Table B-1, macroblock_address_increment, codes 00000011000 ... 0000111xxxx */
XTN VLCtab MBAtab2[104]
#ifdef GLOBAL
=
{
    {33,11}, {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11},
    {25,11}, {24,11}, {23,11}, {22,11}, {21,10}, {21,10}, {20,10}, {20,10},
    {19,10}, {19,10}, {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {16,10},
    {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},
    {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},
    {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},
    {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},
    {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},
    {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},
    {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
    {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
    {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},
    {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7}
}
#endif
;

/* Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
XTN VLCtab DClumtab0[32]
#ifdef GLOBAL
=
{
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
    {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {ERROR_VALUE, 0}
}
#endif
;

/* Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
XTN VLCtab DClumtab1[16]
#ifdef GLOBAL
=
{
    {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
    {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10,9}, {11,9}
}
#endif
;

/* Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
XTN VLCtab DCchromtab0[32]
#ifdef GLOBAL
=
{
    {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {ERROR_VALUE, 0}
}
#endif
;

/* Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
XTN VLCtab DCchromtab1[32]
#ifdef GLOBAL
=
{
    {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
    {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
    {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7},
    {8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10,10}, {11,10}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for first (DC) coefficient)
 */
XTN DCTtab DCTtabfirst[12]
#ifdef GLOBAL
=
{
    {0,2,4}, {2,1,4}, {1,1,3}, {1,1,3},
    {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1},
    {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for all other coefficients)
 */
XTN DCTtab DCTtabnext[12]
#ifdef GLOBAL
=
{
    {0,2,4},  {2,1,4},  {1,1,3},  {1,1,3},
    {64,0,2}, {64,0,2}, {64,0,2}, {64,0,2}, /* EOB */
    {0,1,2},  {0,1,2},  {0,1,2},  {0,1,2}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 000001xx ... 00111xxx
 */
XTN DCTtab DCTtab0[60]
#ifdef GLOBAL
=
{
    {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
    {2,2,7}, {2,2,7}, {9,1,7}, {9,1,7},
    {0,4,7}, {0,4,7}, {8,1,7}, {8,1,7},
    {7,1,6}, {7,1,6}, {7,1,6}, {7,1,6},
    {6,1,6}, {6,1,6}, {6,1,6}, {6,1,6},
    {1,2,6}, {1,2,6}, {1,2,6}, {1,2,6},
    {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
    {13,1,8}, {0,6,8}, {12,1,8}, {11,1,8},
    {3,2,8}, {1,3,8}, {0,5,8}, {10,1,8},
    {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
    {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
    {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
    {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
    {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
    {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5}
}
#endif
;

/* Table B-15, DCT coefficients table one,
 * codes 000001xx ... 11111111
*/
XTN DCTtab DCTtab0a[252]
#ifdef GLOBAL
=
{
    {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
    {7,1,7}, {7,1,7}, {8,1,7}, {8,1,7},
    {6,1,7}, {6,1,7}, {2,2,7}, {2,2,7},
    {0,7,6}, {0,7,6}, {0,7,6}, {0,7,6},
    {0,6,6}, {0,6,6}, {0,6,6}, {0,6,6},
    {4,1,6}, {4,1,6}, {4,1,6}, {4,1,6},
    {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
    {1,5,8}, {11,1,8}, {0,11,8}, {0,10,8},
    {13,1,8}, {12,1,8}, {3,2,8}, {1,4,8},
    {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
    {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
    {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
    {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
    {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
    {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
    {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4}, /* EOB */
    {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
    {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
    {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
    {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
    {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
    {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
    {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
    {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
    {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
    {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
    {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
    {9,1,7}, {9,1,7}, {1,3,7}, {1,3,7},
    {10,1,7}, {10,1,7}, {0,8,7}, {0,8,7},
    {0,9,7}, {0,9,7}, {0,12,8}, {0,13,8},
    {2,3,8}, {4,2,8}, {0,14,8}, {0,15,8}
}
#endif
;

/* Table B-14, DCT coefficients table zero,
 * codes 0000001000 ... 0000001111
 */
XTN DCTtab DCTtab1[8]
#ifdef GLOBAL
=
{
    {16,1,10}, {5,2,10}, {0,7,10}, {2,3,10},
    {1,4,10}, {15,1,10}, {14,1,10}, {4,2,10}
}
#endif
;

/* Table B-15, DCT coefficients table one,
 * codes 000000100x ... 000000111x
 */
XTN DCTtab DCTtab1a[8]
#ifdef GLOBAL
=
{
    {5,2,9}, {5,2,9}, {14,1,9}, {14,1,9},
    {2,4,10}, {16,1,10}, {15,1,9}, {15,1,9}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000010000 ... 000000011111
 */
XTN DCTtab DCTtab2[16]
#ifdef GLOBAL
=
{
    {0,11,12}, {8,2,12}, {4,3,12}, {0,10,12},
    {2,4,12}, {7,2,12}, {21,1,12}, {20,1,12},
    {0,9,12}, {19,1,12}, {18,1,12}, {1,5,12},
    {3,3,12}, {0,8,12}, {6,2,12}, {17,1,12}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000010000 ... 0000000011111
 */
XTN DCTtab DCTtab3[16]
#ifdef GLOBAL
=
{
    {10,2,13}, {9,2,13}, {5,3,13}, {3,4,13},
    {2,5,13}, {1,7,13}, {1,6,13}, {0,15,13},
    {0,14,13}, {0,13,13}, {0,12,13}, {26,1,13},
    {25,1,13}, {24,1,13}, {23,1,13}, {22,1,13}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 00000000010000 ... 00000000011111
 */
XTN DCTtab DCTtab4[16]
#ifdef GLOBAL
=
{
    {0,31,14}, {0,30,14}, {0,29,14}, {0,28,14},
    {0,27,14}, {0,26,14}, {0,25,14}, {0,24,14},
    {0,23,14}, {0,22,14}, {0,21,14}, {0,20,14},
    {0,19,14}, {0,18,14}, {0,17,14}, {0,16,14}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000000010000 ... 000000000011111
 */
XTN DCTtab DCTtab5[16]
#ifdef GLOBAL
=
{
    {0,40,15}, {0,39,15}, {0,38,15}, {0,37,15},
    {0,36,15}, {0,35,15}, {0,34,15}, {0,33,15},
    {0,32,15}, {1,14,15}, {1,13,15}, {1,12,15},
    {1,11,15}, {1,10,15}, {1,9,15}, {1,8,15}
}
#endif
;

/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000000010000 ... 0000000000011111
 */
XTN DCTtab DCTtab6[16]
#ifdef GLOBAL
=
{
    {1,18,16}, {1,17,16}, {1,16,16}, {1,15,16},
    {6,3,16}, {16,2,16}, {15,2,16}, {14,2,16},
    {13,2,16}, {12,2,16}, {11,2,16}, {31,1,16},
    {30,1,16}, {29,1,16}, {28,1,16}, {27,1,16}
}
#endif
;
// add extra table of table ptrs for performance - trbarry 5/2003
XTN DCTtab *pDCTtabNonI[28]         // ptr to non_intra tables
#ifdef GLOBAL
=
{
    &DCTtab6[0] - 16,   // bsf val = 4,   code => 16
    &DCTtab5[0] - 16,   // bsf val = 5,   code => 32
    &DCTtab4[0] - 16,   // bsf val = 6,   code => 64
    &DCTtab3[0] - 16,   // bsf val = 7,   code => 128
    &DCTtab2[0] - 16,   // bsf val = 8,   code => 256
    &DCTtab1[0] - 8,    // bsf val = 9,   code => 512
    &DCTtab0[0] - 4,    // bsf val = 10,  code => 1024
    &DCTtab0[0] - 4,    // bsf val = 11,  code => 2048, same
    &DCTtab0[0] - 4,    // bsf val = 12,  code => 4096, same
    &DCTtab0[0] - 4,    // bsf val = 13,  code => 8192, same
    &DCTtab0[0] - 4,    // bsf val = 14,  code => 16384, same
    &DCTtab0[0] - 4,    // bsf val = 15,  how big can this get??
    &DCTtab0[0] - 4,    // bsf val = 16,  same?
    &DCTtab0[0] - 4,    // bsf val = 17,  same?
    &DCTtab0[0] - 4,    // bsf val = 18,  same?
    &DCTtab0[0] - 4,    // bsf val = 19,  same?
    &DCTtab0[0] - 4,    // bsf val = 20,  same?
    &DCTtab0[0] - 4,    // bsf val = 21,  same?
    &DCTtab0[0] - 4,    // bsf val = 22,  same?
    &DCTtab0[0] - 4,    // bsf val = 23,  same?
    &DCTtab0[0] - 4,    // bsf val = 24,  same?
    &DCTtab0[0] - 4,    // bsf val = 25,  same?
    &DCTtab0[0] - 4,    // bsf val = 26,  same?
    &DCTtab0[0] - 4,    // bsf val = 27,  same?
    &DCTtab0[0] - 4,    // bsf val = 28,  same?
    &DCTtab0[0] - 4,    // bsf val = 29,  same?
    &DCTtab0[0] - 4,    // bsf val = 30,  same?
    &DCTtab0[0] - 4     // bsf val = 31,  same?
}
#endif
;
// same as above but for when intra_vlc_format - trbarry 5/2003
XTN DCTtab *pDCTtab_intra[28]       // ptr to non_intra tables
#ifdef GLOBAL
=
{
    &DCTtab6[0] - 16,   // bsf val = 4,   code => 16
    &DCTtab5[0] - 16,   // bsf val = 5,   code => 32
    &DCTtab4[0] - 16,   // bsf val = 6,   code => 64
    &DCTtab3[0] - 16,   // bsf val = 7,   code => 128
    &DCTtab2[0] - 16,   // bsf val = 8,   code => 256
    &DCTtab1a[0] - 8,   // bsf val = 9,   code => 512
    &DCTtab0a[0] - 4,   // bsf val = 10,  code => 1024
    &DCTtab0a[0] - 4,   // bsf val = 11,  code => 2048, same
    &DCTtab0a[0] - 4,   // bsf val = 12,  code => 4096, same
    &DCTtab0a[0] - 4,   // bsf val = 13,  code => 8192, same
    &DCTtab0a[0] - 4,   // bsf val = 14   code => 16384, same
    &DCTtab0a[0] - 4,   // bsf val = 15,  code => how big can this get?
    &DCTtab0a[0] - 4,   // bsf val = 16,  same?
    &DCTtab0a[0] - 4,   // bsf val = 17,  same?
    &DCTtab0a[0] - 4,   // bsf val = 18,  same?
    &DCTtab0a[0] - 4,   // bsf val = 19,  same?
    &DCTtab0a[0] - 4,   // bsf val = 20,  same?
    &DCTtab0a[0] - 4,   // bsf val = 21,  same?
    &DCTtab0a[0] - 4,   // bsf val = 22,  same?
    &DCTtab0a[0] - 4,   // bsf val = 23,  same?
    &DCTtab0a[0] - 4,   // bsf val = 24,  same?
    &DCTtab0a[0] - 4,   // bsf val = 25,  same?
    &DCTtab0a[0] - 4,   // bsf val = 26,  same?
    &DCTtab0a[0] - 4,   // bsf val = 27,  same?
    &DCTtab0a[0] - 4,   // bsf val = 28,  same?
    &DCTtab0a[0] - 4,   // bsf val = 29,  same?
    &DCTtab0a[0] - 4,   // bsf val = 30,  same?
    &DCTtab0a[0] - 4    // bsf val = 31,  same?
}
#endif
;

// add extra table of shift amounts for performance - trbarry 5/2003
XTN int DCTShiftTab[28] // amounts to shift code
#ifdef GLOBAL
=
{
    0,                  // bsf val = 4,   code => 16
    1,                  // bsf val = 5,   code => 32
    2,                  // bsf val = 6,   code => 64
    3,                  // bsf val = 7,   code => 128
    4,                  // bsf val = 8,   code => 256
    6,                  // bsf val = 9,   code => 512
    8,                  // bsf val = 10,  code => 1024
    8,                  // bsf val = 11,  code => 2048, same
    8,                  // bsf val = 12,  code => 4096, same
    8,                  // bsf val = 13,  code => 8192, same
    8,                  // bsf val = 14,  code => 16384, same
    8,                  // bsf val = 15,  how big can this get?
    8,                  // bsf val = 16,  same?
    8,                  // bsf val = 17,  same?
    8,                  // bsf val = 18,  same?
    8,                  // bsf val = 19,  same?
    8,                  // bsf val = 20,  same?
    8,                  // bsf val = 21,  same?
    8,                  // bsf val = 22,  same?
    8,                  // bsf val = 23,  same?
    8,                  // bsf val = 24,  same?
    8,                  // bsf val = 25,  same?
    8,                  // bsf val = 26,  same?
    8,                  // bsf val = 27,  same?
    8,                  // bsf val = 28,  same?
    8,                  // bsf val = 29,  same?
    8,                  // bsf val = 30,  same?
    8                   // bsf val = 31,  same?
}
#endif
;

/* Table B-3, macroblock_type in P-pictures, codes 001..1xx */
XTN VLCtab PMBtab0[8]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0},
    {MACROBLOCK_MOTION_FORWARD,3},
    {MACROBLOCK_PATTERN,2}, {MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}
}
#endif
;

/* Table B-3, macroblock_type in P-pictures, codes 000001..00011x */
XTN VLCtab PMBtab1[8]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0},
    {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
    {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_INTRA,5}, {MACROBLOCK_INTRA,5}
}
#endif
;

/* Table B-4, macroblock_type in B-pictures, codes 0010..11xx */
XTN VLCtab BMBtab0[16]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0},
    {ERROR_VALUE,0},
    {MACROBLOCK_MOTION_FORWARD,4},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,4},
    {MACROBLOCK_MOTION_BACKWARD,3},
    {MACROBLOCK_MOTION_BACKWARD,3},
    {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
    {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
    {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2}
}
#endif
;

/* Table B-4, macroblock_type in B-pictures, codes 000001..00011x */
XTN VLCtab BMBtab1[8]
#ifdef GLOBAL
=
{
    {ERROR_VALUE,0},
    {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,6},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,6},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
    {MACROBLOCK_INTRA,5},
    {MACROBLOCK_INTRA,5}
}
#endif
;

#undef XTN

#endif  // __GLOBAL_H
