/*
 *  Misc Stuff (profiling) for MPEG2Dec3
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

#ifndef MPEG2DECPLUS_MISC_H
#define MPEG2DECPLUS_MISC_H


#include <cstdint>
#include <cstdarg>

void fast_copy(const uint8_t *src, const int src_stride, uint8_t *dst,
               const int dst_stride, const int horizontal_size,
               const int vertical_size) noexcept;

int __cdecl dprintf(char* fmt, ...);

#endif
