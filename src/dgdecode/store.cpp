/*
 *  MPEG2Dec3 : YV12 & PostProcessing
 *
 *  Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *  based of the intial MPEG2Dec Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of MPEG2Dec3, a free MPEG-2 decoder
 *
 *  MPEG2Dec3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  MPEG2Dec3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "MPEG2Decoder.h"
//#include "postprocess.h"
#include "color_convert.h"
#include "misc.h"


// Write 2-digits numbers in a 16x16 zone.
__inline void MBnum(uint8_t* dst, int stride, int number)
{
    const uint8_t rien[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    const uint8_t nums[10][8] = {
        1, 4, 4, 4, 4, 4, 1, 0,
        3, 3, 3, 3, 3, 3, 3, 0,
        1, 3, 3, 1, 2, 2, 1, 0,
        1, 3, 3, 1, 3, 3, 1, 0,
        4, 4, 4, 1, 3, 3, 3, 0,
        1, 2, 2, 1, 3, 3, 1, 0,
        1, 2, 2, 1, 4, 4, 1, 0,
        1, 3, 3, 3, 3, 3, 3, 0,
        1, 4, 4, 1, 4, 4, 1, 0,
        1, 4, 4, 1, 3, 3, 1, 0,
    };

    auto write = [](const uint8_t* num, uint8_t* dst, const int stride) {
        for (int y = 0; y < 7; ++y) {
            if (num[y] == 1) {
                dst[1 + y * stride] = 0xFF;
                dst[2 + y * stride] = 0xFF;
                dst[3 + y * stride] = 0xFF;
                dst[4 + y * stride] = 0xFF;
            }
            if (num[y] == 2) {
                dst[1 + y * stride] = 0xFF;
            }
            if (num[y] == 3) {
                dst[4 + y * stride] = 0xFF;
            }
            if (num[y] == 4) {
                dst[1 + y * stride] = 0xFF;
                dst[4 + y * stride] = 0xFF;
            }
        }
    };
    const uint8_t* num;

    dst += 3*stride;
    int c = (number/100)%10;
    num = nums[c]; // x00
    if (c==0) num = rien;
    write(num, dst, stride);

    dst += 5;
    int d = (number/10)%10;
    num = nums[d]; // 0x0
    if (c==0 && d==0) num = rien;
    write(num, dst, stride);

    dst += 5;
    num = nums[number%10]; // 00x
    write(num, dst, stride);
}

void CMPEG2Decoder::assembleFrame(uint8_t *src[], int pf, YV12PICT *dst)
{
    int *qp;

    dst->pf = pf;
#if 0
    if (pp_mode != 0)
    {
        uint8_t* ppptr[3];
        if (!(upConv > 0 && chroma_format == 1))
        {
            ppptr[0] = dst->y;
            ppptr[1] = dst->u;
            ppptr[2] = dst->v;
        }
        else
        {
            ppptr[0] = dst->y;
            ppptr[1] = u422;
            ppptr[2] = v422;
        }
        bool iPPt;
        if (iPP == 1 || (iPP == -1 && pf == 0)) iPPt = true;
        else iPPt = false;
        postprocess(src, this->Coded_Picture_Width, this->Chroma_Width,
                ppptr, dst->ypitch, dst->uvpitch, this->Coded_Picture_Width,
                this->Coded_Picture_Height, this->QP, this->mb_width, pp_mode, moderate_h, moderate_v,
                chroma_format == 1 ? false : true, iPPt);
        if (upConv > 0 && chroma_format == 1)
        {
            if (iCC == 1 || (iCC == -1 && pf == 0))
            {
                conv420to422I(ppptr[1],dst->u,dst->uvpitch,dst->uvpitch,Coded_Picture_Width,Coded_Picture_Height);
                conv420to422I(ppptr[2],dst->v,dst->uvpitch,dst->uvpitch,Coded_Picture_Width,Coded_Picture_Height);
            }
            else
            {
                conv420to422P(ppptr[1],dst->u,dst->uvpitch,dst->uvpitch,Coded_Picture_Width,Coded_Picture_Height);
                conv420to422P(ppptr[2],dst->v,dst->uvpitch,dst->uvpitch,Coded_Picture_Width,Coded_Picture_Height);
            }
        }
    }
    else
#endif
    {
        fast_copy(src[0], Coded_Picture_Width, dst->y, dst->ypitch, Coded_Picture_Width, Coded_Picture_Height);
        if (upConv > 0 && chroma_format == 1)
        {
            if (iCC == 1 || (iCC == -1 && pf == 0))
            {
                conv420to422I(src[1], dst->u, Chroma_Width, dst->uvpitch, Coded_Picture_Width, Coded_Picture_Height);
                conv420to422I(src[2], dst->v, Chroma_Width, dst->uvpitch, Coded_Picture_Width, Coded_Picture_Height);
            }
            else
            {
                conv420to422P(src[1], dst->u, Chroma_Width, dst->uvpitch, Coded_Picture_Width, Coded_Picture_Height);
                conv420to422P(src[2], dst->v, Chroma_Width, dst->uvpitch, Coded_Picture_Width, Coded_Picture_Height);
            }
        } else {
            fast_copy(src[1], Chroma_Width, dst->u, dst->uvpitch, Chroma_Width, Chroma_Height);
            fast_copy(src[2], Chroma_Width, dst->v, dst->uvpitch, Chroma_Width, Chroma_Height);
        }
    }

    // Re-order quant data for display order.
    if (info == 1 || info == 2 || showQ)
    {
        if (picture_coding_type == B_TYPE)
            qp = auxQP;
        else
            qp = backwardQP;
    } else {
        return;
    }

    if (info == 1 || info == 2)
    {
        int x, y, temp;
        int quant;

        minquant = maxquant = qp[0];
        avgquant = 0;
        for(y=0; y<mb_height; ++y)
        {
            temp = y*mb_width;
            for(x=0; x<mb_width; ++x)
            {
                quant = qp[x+temp];
                if (quant > maxquant) maxquant = quant;
                if (quant < minquant) minquant = quant;
                avgquant += quant;
            }
        }
        avgquant = (int)(((float)avgquant/(float)(mb_height*mb_width)) + 0.5f);
    }

    if (showQ)
    {
        int x, y;
        for(y=0; y<this->mb_height; y++)
        {
            for(x=0;x<this->mb_width; x++)
            {
                MBnum(&dst->y[x*16+y*16*dst->ypitch],dst->ypitch,qp[x+y*this->mb_width]);
            }
        }
    }
}
