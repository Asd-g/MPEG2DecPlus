/*
 *  Misc Stuff for MPEG2Dec3
 *
 *  Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
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


#include <cstdio>
#include <memory>
#include <cstdarg>

#ifndef _WIN32
#include <avisynth.h>
#include "win_import_min.h"
#endif
#include "misc.h"


size_t __cdecl dprintf(char* fmt, ...)
{
    char printString[1024];

    va_list argp;

    va_start(argp, fmt);
    vsprintf_s(printString, 1024, fmt, argp);
    va_end(argp);
    fprintf(stderr, printString);
    return strlen(printString);
}


void __stdcall
fast_copy(const uint8_t* src, const int src_stride, uint8_t* dst,
    const int dst_stride, const int horizontal_size, int vertical_size) noexcept
{
    if (vertical_size == 0) {
        return;
    } else if (horizontal_size == src_stride && src_stride == dst_stride) {
        memcpy(dst, src, static_cast<int64_t>(horizontal_size) * vertical_size);
    } else {
        do {
            memcpy(dst, src, horizontal_size);
            dst += dst_stride;
            src += src_stride;
        } while (--vertical_size != 0);
    }
}
