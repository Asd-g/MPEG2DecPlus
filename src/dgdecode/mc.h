/*
 *  Motion Compensation for MPEG2Dec3
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

#ifndef MPEG2DEC_MC_H
#define MPEG2DEC_MC_H

#include <cstdint>

typedef void (MCFunc) (uint8_t* dest, const uint8_t* ref, int stride, int offs, int height);
typedef MCFunc* MCFuncPtr;

MCFunc MC_put_8_mmx;
MCFunc MC_put_x8_mmx;
MCFunc MC_put_y8_mmx;
MCFunc MC_put_xy8_mmx;

MCFunc MC_put_16_mmx;
MCFunc MC_put_x16_mmx;
MCFunc MC_put_y16_mmx;
MCFunc MC_put_xy16_mmx;

MCFunc MC_avg_8_mmx;
MCFunc MC_avg_x8_mmx;
MCFunc MC_avg_y8_mmx;
MCFunc MC_avg_xy8_mmx;

MCFunc MC_avg_16_mmx;
MCFunc MC_avg_x16_mmx;
MCFunc MC_avg_y16_mmx;
MCFunc MC_avg_xy16_mmx;
#if 0
MCFunc MC_put_8_3dnow;
MCFunc MC_put_x8_3dnow;
MCFunc MC_put_y8_3dnow;
MCFunc MC_put_xy8_3dnow_AC;
MCFunc MC_put_xy8_3dnow_FAST;

MCFunc MC_put_16_3dnow;
MCFunc MC_put_x16_3dnow;
MCFunc MC_put_y16_3dnow;
MCFunc MC_put_xy16_3dnow_AC;
MCFunc MC_put_xy16_3dnow_FAST;

MCFunc MC_avg_8_3dnow;
MCFunc MC_avg_x8_3dnow;
MCFunc MC_avg_y8_3dnow;
MCFunc MC_avg_xy8_3dnow_AC;
MCFunc MC_avg_xy8_3dnow_FAST;

MCFunc MC_avg_16_3dnow;
MCFunc MC_avg_x16_3dnow;
MCFunc MC_avg_y16_3dnow;
MCFunc MC_avg_xy16_3dnow_AC;
MCFunc MC_avg_xy16_3dnow_FAST;

extern "C" {

MCFunc MC_put_8_mmxext;
MCFunc MC_put_x8_mmxext;
MCFunc MC_put_y8_mmxext;
MCFunc MC_put_xy8_mmxext_AC;
MCFunc MC_put_xy8_mmxext_FAST;

MCFunc MC_put_16_mmxext;
MCFunc MC_put_x16_mmxext;
MCFunc MC_put_y16_mmxext;
MCFunc MC_put_xy16_mmxext_AC;
MCFunc MC_put_xy16_mmxext_FAST;

MCFunc MC_avg_8_mmxext;
MCFunc MC_avg_x8_mmxext;
MCFunc MC_avg_y8_mmxext;
MCFunc MC_avg_xy8_mmxext_AC;
MCFunc MC_avg_xy8_mmxext_FAST;

MCFunc MC_avg_16_mmxext;
MCFunc MC_avg_x16_mmxext;
MCFunc MC_avg_y16_mmxext;
MCFunc MC_avg_xy16_mmxext_AC;
MCFunc MC_avg_xy16_mmxext_FAST;

}
#endif
// Form prediction (motion compensation) function pointer array (GetPic.c) - Vlad59 04-20-2002
extern MCFuncPtr ppppf_motion[2][2][4];
void Choose_Prediction(bool fastMC);

#endif // MPEG2DEC_MC_H
