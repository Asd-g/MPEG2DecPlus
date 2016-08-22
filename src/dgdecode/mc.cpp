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

#include "mc.h"
#include "global.h"

MCFuncPtr ppppf_motion[2][2][4];

void Choose_Prediction(bool fastMC)
{
//  if (cpu.mmx) => MMX is needed anyway

        ppppf_motion[0][0][0] = MC_put_8_mmx;
        ppppf_motion[0][0][1] = MC_put_y8_mmx;
        ppppf_motion[0][0][2] = MC_put_x8_mmx;
        ppppf_motion[0][0][3] = MC_put_xy8_mmx;

        ppppf_motion[0][1][0] = MC_put_16_mmx;
        ppppf_motion[0][1][1] = MC_put_y16_mmx;
        ppppf_motion[0][1][2] = MC_put_x16_mmx;
        ppppf_motion[0][1][3] = MC_put_xy16_mmx;

        ppppf_motion[1][0][0] = MC_avg_8_mmx;
        ppppf_motion[1][0][1] = MC_avg_y8_mmx;
        ppppf_motion[1][0][2] = MC_avg_x8_mmx;
        ppppf_motion[1][0][3] = MC_avg_xy8_mmx;

        ppppf_motion[1][1][0] = MC_avg_16_mmx;
        ppppf_motion[1][1][1] = MC_avg_y16_mmx;
        ppppf_motion[1][1][2] = MC_avg_x16_mmx;
        ppppf_motion[1][1][3] = MC_avg_xy16_mmx;

#if 0
    if (cpu._3dnow)
    {
        ppppf_motion[0][0][0] = MC_put_8_3dnow;
        ppppf_motion[0][0][1] = MC_put_y8_3dnow;
        ppppf_motion[0][0][2] = MC_put_x8_3dnow;

        ppppf_motion[0][1][0] = MC_put_16_3dnow;
        ppppf_motion[0][1][1] = MC_put_y16_3dnow;
        ppppf_motion[0][1][2] = MC_put_x16_3dnow;

        ppppf_motion[1][0][0] = MC_avg_8_3dnow;
        ppppf_motion[1][0][1] = MC_avg_y8_3dnow;
        ppppf_motion[1][0][2] = MC_avg_x8_3dnow;

        ppppf_motion[1][1][0] = MC_avg_16_3dnow;
        ppppf_motion[1][1][1] = MC_avg_y16_3dnow;
        ppppf_motion[1][1][2] = MC_avg_x16_3dnow;

        if (fastMC)
        {
            ppppf_motion[0][0][3] = MC_put_xy8_3dnow_FAST;
            ppppf_motion[0][1][3] = MC_put_xy16_3dnow_FAST;
            ppppf_motion[1][0][3] = MC_avg_xy8_3dnow_FAST;
            ppppf_motion[1][1][3] = MC_avg_xy16_3dnow_FAST;
        }
        else
        {
            ppppf_motion[0][0][3] = MC_put_xy8_3dnow_AC;
            ppppf_motion[0][1][3] = MC_put_xy16_3dnow_AC;
            ppppf_motion[1][0][3] = MC_avg_xy8_3dnow_AC;
            ppppf_motion[1][1][3] = MC_avg_xy16_3dnow_AC;
        }
    }

    if (cpu.ssemmx)
    {
        ppppf_motion[0][0][0] = MC_put_8_mmxext;
        ppppf_motion[0][0][1] = MC_put_y8_mmxext;
        ppppf_motion[0][0][2] = MC_put_x8_mmxext;

        ppppf_motion[0][1][0] = MC_put_16_mmxext;
        ppppf_motion[0][1][1] = MC_put_y16_mmxext;
        ppppf_motion[0][1][2] = MC_put_x16_mmxext;

        ppppf_motion[1][0][0] = MC_avg_8_mmxext;
        ppppf_motion[1][0][1] = MC_avg_y8_mmxext;
        ppppf_motion[1][0][2] = MC_avg_x8_mmxext;


        ppppf_motion[1][1][0] = MC_avg_16_mmxext;
        ppppf_motion[1][1][1] = MC_avg_y16_mmxext;
        ppppf_motion[1][1][2] = MC_avg_x16_mmxext;

        if (fastMC) {
            ppppf_motion[0][0][3] = MC_put_xy8_mmxext_FAST;
            ppppf_motion[0][1][3] = MC_put_xy16_mmxext_FAST;
            ppppf_motion[1][0][3] = MC_avg_xy8_mmxext_FAST;
            ppppf_motion[1][1][3] = MC_avg_xy16_mmxext_FAST;
        } else {
            ppppf_motion[0][0][3] = MC_put_xy8_mmxext_AC;
            ppppf_motion[0][1][3] = MC_put_xy16_mmxext_AC;
            ppppf_motion[1][0][3] = MC_avg_xy8_mmxext_AC;
            ppppf_motion[1][1][3] = MC_avg_xy16_mmxext_AC;
        }
    }
#endif
}
