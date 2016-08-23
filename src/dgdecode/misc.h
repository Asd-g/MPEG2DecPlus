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

#include <cstdint>
#include <cstdarg>

struct ts
{
    uint64_t idct;
    uint64_t conv;
    uint64_t mcpy;
    uint64_t post;
    uint64_t dec;
    uint64_t bit;
    uint64_t decMB;
    uint64_t mcmp;
    uint64_t addb;
    uint64_t overall;
    uint64_t sum;
    int div;
    uint64_t freq;
};

uint64_t read_counter(void);
uint64_t get_freq(void);

void init_first(ts* timers);
void init_timers(ts* timers);
void start_timer();
void start_timer2(uint64_t* timer);
void stop_timer(uint64_t* timer);
void stop_timer2(uint64_t* timer);
void timer_debug(ts* tim);

void fast_copy(const uint8_t *src, const int src_stride, uint8_t *dst,
               const int dst_stride, const int horizontal_size,
               const int vertical_size) noexcept;

int __cdecl dprintf(char* fmt, ...);
