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

#include <stdexcept>
#include <malloc.h>
#include "YV12PICT.h"

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

// Change to use constructor/destructor
// chikuzen - Sep 6, 2016


YV12PICT::YV12PICT(int height, int width, int chroma_format) :
    allocated(true),
    ywidth(width), uvwidth(width),
    yheight(height), uvheight(height)
{
    if (chroma_format < 3) {
        uvwidth /= 2;
    }
    if (chroma_format < 2) {
        uvheight /= 2;
    }

    uvpitch = (uvwidth + 15) & ~15;
    ypitch = (ywidth + 31) & ~31;

    y = reinterpret_cast<uint8_t*>(_aligned_malloc(height * ypitch, 32));
    u = reinterpret_cast<uint8_t*>(_aligned_malloc(uvheight * uvpitch, 16));
    v = reinterpret_cast<uint8_t*>(_aligned_malloc(uvheight * uvpitch, 16));
    if (!y || !u || !v) {
        _aligned_free(y);
        _aligned_free(u);
        throw std::runtime_error("failed to new YV12PICT");
    }
}


YV12PICT::YV12PICT(PVideoFrame& frame) :
    allocated(false),
    y(frame->GetWritePtr(PLANAR_Y)),
    u(frame->GetWritePtr(PLANAR_U)),
    v(frame->GetWritePtr(PLANAR_V)),
    ypitch(frame->GetPitch(PLANAR_Y)), uvpitch(frame->GetPitch(PLANAR_U)),
    ywidth(frame->GetRowSize(PLANAR_Y)), uvwidth(frame->GetRowSize(PLANAR_U)),
    yheight(frame->GetHeight(PLANAR_Y)), uvheight(frame->GetHeight(PLANAR_U))
{}


YV12PICT::YV12PICT(uint8_t* py, uint8_t* pu, uint8_t*pv, int yw, int cw, int h) :
    allocated(false),
    y(py), u(pu), v(pv),
    ypitch((yw + 31) & ~31), uvpitch((cw + 15) & ~15),
    ywidth(yw), uvwidth(cw), yheight(h), uvheight(h)
{}


YV12PICT::~YV12PICT()
{
    if (allocated) {
        _aligned_free(y);
        _aligned_free(u);
        _aligned_free(v);
    }
    y = u = v = nullptr;
}

