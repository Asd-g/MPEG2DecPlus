/*****************************************************************************
 * motion3dnow.c : 3Dnow motion compensation module for vlc
 *****************************************************************************
 * Copyright (C) 2001 VideoLAN
 * $Id: motionm3dnow.c,v 1.16 2002/02/15 13:32:53 sam Exp $
 *
 * Authors: Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *          Michel Lespinasse <walken@zoy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/*****************************************************************************
 * mc3dnow.c : Created by vlad59 - 04/05/2002
 * Based on the VideoLan project
 *****************************************************************************
 * Change list :
 *
 * - Renamed from motion3dnow.c to mc3dnow.c (I prefer 8.3 files)
 * - Changes the nasm code to masm
 * - Changes to parameters of the inlined function to accept the offs parameter
 * - Some cleanup of unused function
 * - Removed //femms (we don't need that)
 *
 * Todo :
 *
 * - Change this file to pure asm
 * - Or simply try the __fastcall mode
 *
 *****************************************************************************/
// Can define this just to stop the errors with ICL for now
// #define pavgusb movq

#pragma warning( disable : 4799 )

void MC_put_8_3dnow (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq [ebx], mm0
        add eax, [stride]
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_put_16_3dnow (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [eax+8]
        add eax, [stride]
        movq [ebx], mm0
        movq [ebx+8], mm1
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_avg_8_3dnow (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        pavgusb mm0, [ebx]
        add eax, [stride]
        movq [ebx], mm0
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_avg_16_3dnow (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [eax+8]
        pavgusb mm0, [ebx]
        pavgusb mm1, [ebx+8]
        movq [ebx], mm0
        add eax, [stride]
        movq [ebx+8], mm1
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_put_x8_3dnow (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        pavgusb mm0, [eax+1]
        add eax, [stride]
        movq [ebx], mm0
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_put_y8_3dnow (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ecx, eax
        add         ecx, [offs]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        pavgusb mm0, [ecx]
        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_put_x16_3dnow (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [eax+8]
        pavgusb mm0, [eax+1]
        pavgusb mm1, [eax+9]
        movq [ebx], mm0
        add eax, [stride]
        movq [ebx+8], mm1
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_put_y16_3dnow (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [eax+8]
        pavgusb mm0, [ecx]
        pavgusb mm1, [ecx+8]
        movq [ebx], mm0
        add eax, [stride]
        add ecx, [stride]
        movq [ebx+8], mm1
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_avg_x8_3dnow (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        pavgusb mm0, [eax+1]
        pavgusb mm0, [ebx]
        add eax, [stride]
        movq [ebx], mm0
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_avg_y8_3dnow (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        pavgusb mm0, [ecx]
        pavgusb mm0, [ebx]
        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_avg_x16_3dnow (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [eax+8]
        pavgusb mm0, [eax+1]
        pavgusb mm1, [eax+9]
        pavgusb mm0, [ebx]
        pavgusb mm1, [ebx+8]
        add eax, [stride]
        movq [ebx], mm0
        movq [ebx+8], mm1
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

void MC_avg_y16_3dnow (unsigned char * dest, unsigned char * ref,
                                   int stride, int offs, int height)
    {__asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [eax+8]
        pavgusb mm0, [ecx]
        pavgusb mm1, [ecx+8]
        pavgusb mm0, [ebx]
        pavgusb mm1, [ebx+8]
        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        movq [ebx+8], mm1
        add ebx, [stride]
        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}


static const __int64 mask_one = 0x0101010101010101;

// Accurate function
void MC_put_xy8_3dnow_AC (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]

    mc0:

        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm7, mm0
        movq mm2, [eax+1]
        pxor mm7, mm1
        movq mm3, [ecx]
        movq mm6, mm2
        pxor mm6, mm3
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        por mm7, mm6
        movq mm6, mm0
        pxor mm6, mm2
        pand mm7, mm6
        pand mm7, [mask_one]
        pavgusb mm0, mm2
        psubusb mm0, mm7

        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

// Fast function
void MC_put_xy8_3dnow_FAST (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]

    mc0:

        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm2, [eax+1]
        movq mm3, [ecx]
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        pavgusb mm0, mm2
        psubusb mm0, [mask_one]

        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

// Accurate function
void MC_put_xy16_3dnow_AC (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm7, mm0
        movq mm2, [eax+1]
        pxor mm7, mm1
        movq mm3, [ecx]
        movq mm6, mm2
        pxor mm6, mm3
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        por mm7, mm6
        movq mm6, mm0
        pxor mm6, mm2
        pand mm7, mm6
        pand mm7, [mask_one]
        pavgusb mm0, mm2
        psubusb mm0, mm7
        movq [ebx], mm0

        movq mm0, [eax+8]
        movq mm1, [ecx+9]
        movq mm7, mm0
        movq mm2, [eax+9]
        pxor mm7, mm1
        movq mm3, [ecx+8]
        movq mm6, mm2
        pxor mm6, mm3
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        por mm7, mm6
        movq mm6, mm0
        pxor mm6, mm2
        pand mm7, mm6
        pand mm7, [mask_one]
        pavgusb mm0, mm2
        psubusb mm0, mm7
        add eax, [stride]
        add ecx, [stride]
        movq [ebx+8], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

// Fast function
void MC_put_xy16_3dnow_FAST (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm2, [eax+1]
        movq mm3, [ecx]
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        pavgusb mm0, mm2
        psubusb mm0, [mask_one]
        movq [ebx], mm0

        movq mm0, [eax+8]
        movq mm1, [ecx+9]
        movq mm2, [eax+9]
        movq mm3, [ecx+8]
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        pavgusb mm0, mm2
        psubusb mm0, [mask_one]

        add eax, [stride]
        add ecx, [stride]
        movq [ebx+8], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}


// Accurate function
void MC_avg_xy8_3dnow_AC (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm7, mm0
        movq mm2, [eax+1]
        pxor mm7, mm1
        movq mm3, [ecx]
        movq mm6, mm2
        pxor mm6, mm3
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        por mm7, mm6
        movq mm6, mm0
        pxor mm6, mm2
        pand mm7, mm6
        pand mm7, [mask_one]
        pavgusb mm0, mm2
        psubusb mm0, mm7
        movq mm1, [ebx]
        pavgusb mm0, mm1

        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

// Fast function
void MC_avg_xy8_3dnow_FAST (unsigned char * dest, unsigned char * ref,
                              int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm2, [eax+1]
        movq mm3, [ecx]
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        pavgusb mm0, mm2
        psubusb mm0, [mask_one]
        movq mm1, [ebx]
        pavgusb mm0, mm1

        add eax, [stride]
        add ecx, [stride]
        movq [ebx], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

// Accurate function
void MC_avg_xy16_3dnow_AC (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm7, mm0
        movq mm2, [eax+1]
        pxor mm7, mm1
        movq mm3, [ecx]
        movq mm6, mm2
        pxor mm6, mm3
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        por mm7, mm6
        movq mm6, mm0
        pxor mm6, mm2
        pand mm7, mm6
        pand mm7, [mask_one]
        pavgusb mm0, mm2
        psubusb mm0, mm7
        movq mm1, [ebx]
        pavgusb mm0, mm1
        movq [ebx], mm0

        movq mm0, [eax+8]
        movq mm1, [ecx+9]
        movq mm7, mm0
        movq mm2, [eax+9]
        pxor mm7, mm1
        movq mm3, [ecx+8]
        movq mm6, mm2
        pxor mm6, mm3
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        por mm7, mm6
        movq mm6, mm0
        pxor mm6, mm2
        pand mm7, mm6
        pand mm7, [mask_one]
        pavgusb mm0, mm2
        psubusb mm0, mm7
        movq mm1, [ebx+8]
        pavgusb mm0, mm1
        add eax, [stride]
        add ecx, [stride]
        movq [ebx+8], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}

// Fast function
void MC_avg_xy16_3dnow_FAST (unsigned char * dest, unsigned char * ref,
                               int stride, int offs, int height)
{
    __asm {
        mov         eax, [ref]
        mov         ebx, [dest]
        mov         ecx, eax
        add         ecx, [offs]
        mov         edi, [height]
    mc0:
        movq mm0, [eax]
        movq mm1, [ecx+1]
        movq mm2, [eax+1]
        movq mm3, [ecx]
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        pavgusb mm0, mm2
        psubusb mm0, [mask_one]
        movq mm1, [ebx]
        pavgusb mm0, mm1
        movq [ebx], mm0

        movq mm0, [eax+8]
        movq mm1, [ecx+9]
        movq mm2, [eax+9]
        movq mm3, [ecx+8]
        pavgusb mm0, mm1
        pavgusb mm2, mm3
        pavgusb mm0, mm2
        psubusb mm0, [mask_one]
        movq mm1, [ebx+8]
        pavgusb mm0, mm1
        add eax, [stride]
        add ecx, [stride]
        movq [ebx+8], mm0
        add ebx, [stride]

        dec edi
        cmp edi, 0x00
        jg mc0

        //femms
    }
}
