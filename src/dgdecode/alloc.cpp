/*
 *  Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// replace with one that doesn't need fixed size table - trbarry 3-22-2002

#include <malloc.h>
#include "MPEG2Decoder.h"

//#define ptr_t unsigned int



// memory allocation for MPEG2Dec3.
//
// Changed this to handle/track both width/pitch for when
// width != pitch and it simply makes things easier to have all
// information in this struct.  It now uses 32y/16uv byte alignment
// by default, which makes internal bugs easier to catch.  This can
// easily be changed if needed.
//
// The definition of YV12PICT is in global.h
//
// tritical - May 16, 2005
YV12PICT* create_YV12PICT(int height, int width, int chroma_format)
{
    YV12PICT* pict;
    pict = (YV12PICT*)malloc(sizeof(YV12PICT));
    int uvwidth, uvheight;
    if (chroma_format == 1) // 4:2:0
    {
        uvwidth = width>>1;
        uvheight = height>>1;
    }
    else if (chroma_format == 2) // 4:2:2
    {
        uvwidth = width>>1;
        uvheight = height;
    }
    else // 4:4:4
    {
        uvwidth = width;
        uvheight = height;
    }
    int uvpitch = (uvwidth + 15) & ~15;
    int ypitch = uvpitch*2;
    size_t size = height * ypitch + 2 * uvheight * uvpitch;
    pict->y = (unsigned char*)_aligned_malloc(size, 32);
    pict->u = pict->y + uvheight * uvpitch;
    pict->v = pict->u + uvheight * uvpitch;
    pict->ypitch = ypitch;
    pict->uvpitch = uvpitch;
    pict->ywidth = width;
    pict->uvwidth = uvwidth;
    pict->yheight = height;
    pict->uvheight = uvheight;
    return pict;
}

void destroy_YV12PICT(YV12PICT * pict)
{
    _aligned_free(pict->y);
    free(pict);
    pict = nullptr;
}
