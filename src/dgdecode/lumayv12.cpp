/**************************************************************************************
    LumaYV12 plugin for Avisynth 2.5 by ARDA.

    It assumes [0->255] YUV range, and not CCIR 601 [16->235].
    Use limiter() afterwards if you think you need it.

    Syntax: LumaYV12(lumoff=param,lumgain=param) or LumaYV12(param,param)

    lumoff=-256 to 256 (integer) ; default 0 and will do nothing.
    lumgain=0 to 2.0 (float) ; 1.0 is default and will do nothing.

    This plugins was done for my own use trying to achieve a better perfomance
    in my pentium4 cpu; I've also included Integer SSE and MMX routines but
    performance was not tested.

    I've also included row routines to avoid first line problem after separatefields.
    Seems there is not real two frames, but a redefintion of pointers and pitches.
    Maybe there's another way to avoid that problem but I couldn't find an easier one.
    Row loops will be here till SeparateFields could be modified.

    Actually they are exercises; so there are a lot of unnecessary duplicated code.
    There must be a lot of spanglish variable names; my apologizes for that.

    For any bugs or suggestions address to ardanaz@excite.com;
    or in Avisynth Usage of Doom9's Forum.

    Thanks to all avisynth developers and users; but also to all those
    who share their knowledge and time in this wonderful community.

 ***************************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ***************************************************************************************/

#include "AvisynthAPI.h"

LumaYV12::LumaYV12(PClip _child, int _Lumaoffset,double _Lumagain, IScriptEnvironment* env):
          GenericVideoFilter(_child), lumoff(_Lumaoffset), lumgain(_Lumagain)
{
    if (env->GetCPUFlags() & !CPUF_MMX) //maybe not needed nowadays
    env->ThrowError("LumaYV12: needs MMX cpu or higher");

    if (!vi.IsPlanar())
    env->ThrowError("LumaYV12: only planar YV12 input");

    use_SSE2 = (env->GetCPUFlags() & CPUF_SSE2) !=0;
    use_ISSE = (env->GetCPUFlags() & CPUF_INTEGER_SSE) !=0;

    SepFields=(vi.IsFieldBased());
    int testsize=vi.width;

    lumGain = (int)(lumgain*128);

    if (testsize & 3)
        env->ThrowError("LumaYV12: width must be a multiple of 4");


    if(!SepFields)//still need to be tested
    {
        if (testsize&15)testsize=((testsize>>4)+1)<<4;//dont know if it is worth while
            testsize=testsize*vi.height;
    }
    //sure we do line loops

    //These controls could be deleted, nobody uses so small sizes, still here just in case.//
    if(testsize<79)
    {
        if(use_SSE2 && testsize<79)
        {
            if (lumoff!=0 && lumGain==128 && testsize<79) use_SSE2=false;
            if (lumGain!=128 && testsize<48)use_SSE2=false;
        }

        if(!use_SSE2 && testsize<32)
        {
            if (lumoff!=0 && lumGain==128 && testsize<32)
                env->ThrowError("LumaYV12:for such param values width (or frame size) must be minimum 32");

            if (lumGain!=128 && testsize<16)
                env->ThrowError("LumaYV12:for such param values width (or frame size) must be minimum 16");
        }
    }

    if (lumgain < 0.0 || lumgain >2.0)
        env->ThrowError("LumaYV12: lumgain must be between 0 to 2.0");

    if (lumoff < -255 || lumoff > 255)
         env->ThrowError("LumaYV12: lumoff must be between -255 to 255");

    if (lumoff==0 && lumGain==128)
        env->ThrowError("LumaYV12: default values will do nothing");
}


void asm_Brightoneline_ISSE_SUBADD
                        (BYTE* srcp,
                        int sizef,
                        int brite)
{
    __asm{
        mov         eax,[brite]
        mov         ecx,[sizef]
        mov         esi,[srcp]
        sar         ecx,5

        test        eax,eax
        js          isasubstr

        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4
    align 16
    x_loopadd:
        prefetchnta [esi+128]

        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        paddusb     mm7,mm4
        paddusb     mm6,mm4
        paddusb     mm5,mm4
        paddusb     mm3,mm4

        movq        [esi],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loopadd
        jmp         endend
    align 16
    isasubstr:
        neg         al
        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4
    align 16
    x_loop:
        prefetchnta [esi+128]

        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        psubusb     mm7,mm4
        psubusb     mm6,mm4
        psubusb     mm5,mm4
        psubusb     mm3,mm4

        movq        [esi],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loop
    align 16
    endend:
        emms
    };
}
/////////////////////////////////////////////////////////////////
void asm_Brightoneline_MMX_SUBADD
                    (BYTE* srcp,
                    int sizef,
                    int brite)
{
    __asm{
        mov         eax,[brite]
        mov         ecx,[sizef]
        mov         esi,[srcp]
        sar         ecx,5

        test        eax,eax
        js          isasubstr

        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4
    align 16
    x_loopadd:
        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        paddusb     mm7, mm4
        paddusb     mm6, mm4
        paddusb     mm5, mm4
        paddusb     mm3, mm4

        movq        [esi],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loopadd
        jmp         endend
    align 16
    isasubstr:
        neg         al
        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4
    align 16
    x_loop:
        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        psubusb     mm7,mm4
        psubusb     mm6,mm4
        psubusb     mm5,mm4
        psubusb     mm3,mm4

        movq        [esi],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loop
    align 16
    endend:
        emms
    };
}
/////////////////////////////////////////////////////////////////
void asm_Brightrow_MMX_SUBADD
                (BYTE* srcp,
                int row,
                int height,
                int modulosrc,
                int brite)
{
    __asm{
        mov         esi,[srcp]
        mov         eax,[row]
        mov         ebx,[height]
        mov         ecx,[brite]
        mov         edi,[modulosrc]

        test        ecx,ecx
        js          issubstr

        mov         ch,cl
        movzx       edx,cx
        shl         ecx,16
        or          ecx,edx

        movd        mm4,ecx
        punpckldq   mm4,mm4
    align 16
    y_loopadd:
        mov         ecx,eax//[row]
        mov         edx,eax
        sar         ecx,5
        and         edx,31
    align 16
    x_loop32add:
        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        paddusb     mm7,mm4
        paddusb     mm6,mm4
        paddusb     mm5,mm4
        paddusb     mm3,mm4

        movq        [esi+0],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loop32add

        test        edx,edx
        jz          endadd

        sar         edx,2
    align 16
    rest4add:
        movd        mm7,[esi]
        paddusb     mm7,mm4
        movd        [esi],mm7
        add         esi,4
        dec         edx
        jne         rest4add
    align 16
    endadd:
        add         esi,edi//modulosrc
        dec         ebx
        jne         y_loopadd
        jmp         endend
//////////////////////////////////////////
    align 16
    issubstr:
        neg         cl
        mov         ch,cl
        movzx       edx,cx
        shl         ecx,16
        or          ecx,edx

        movd        mm4,ecx
        punpckldq   mm4,mm4
    align 16
    y_loop:
        mov         ecx,eax//[row]
        mov         edx,eax
        sar         ecx,5
        and         edx,31
    align 16
    x_loop32:
        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        psubusb     mm7,mm4
        psubusb     mm6,mm4
        psubusb     mm5,mm4
        psubusb     mm3,mm4

        movq        [esi+0],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loop32

        test        edx,edx
        jz          endaddbr

        sar         edx,2
    align 16
    rest4:
        movd        mm7,[esi]
        psubusb     mm7,mm4
        movd        [esi],mm7
        add         esi,4
        dec         edx
        jne         rest4
    align 16
    endaddbr:
        add         esi,edi//modulosrc
        dec         ebx
        jne         y_loop
    align 16
    endend:
        emms
    };
}
/////////////////////////////////////////////////////////////////
void asm_Brightrow_ISSE_SUBADD
                    (BYTE* srcp,
                    int row,
                    int height,
                    int modulosrc,
                    int brite)
{
    __asm{
        mov         esi,[srcp]
        mov         eax,[row]
        mov         ebx,[height]
        mov         ecx,[brite]
        mov         edi,[modulosrc]

        test        ecx,ecx
        js          issubstr

        mov         ch,cl
        movzx       edx,cx
        shl         ecx,16
        or          ecx,edx

        movd        mm4,ecx
        punpckldq   mm4,mm4

        prefetchnta [esi]
    align 16
    y_loopadd:
        mov         ecx,eax//[row]
        mov         edx,eax
        sar         ecx,5
        and         edx,31
    align 16
    x_loop32add:
        prefetchnta [esi+128]

        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        paddusb     mm7,mm4
        paddusb     mm6,mm4
        paddusb     mm5,mm4
        paddusb     mm3,mm4

        movq        [esi+0],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loop32add

        test        edx,edx
        jz          endadd

        sar         edx,2
    align 16
    rest4add:
        movd        mm7,[esi]
        paddusb     mm7,mm4
        movd        [esi],mm7
        add         esi,4
        dec         edx
        jne         rest4add
    align 16
    endadd:
        add         esi,edi//modulosrc
        dec         ebx
        jne         y_loopadd
        jmp         endend
    //////////////////////////////////////////
    align 16
    issubstr:
        neg         cl
        mov         ch,cl
        movzx       edx,cx
        shl         ecx,16
        or          ecx,edx

        movd        mm4,ecx
        punpckldq   mm4,mm4

        prefetchnta [esi]
    align 16
    y_loop:
        mov         ecx,eax//[row]
        mov         edx,eax
        sar         ecx,5
        and         edx,31
    align 16
    x_loop32:
        prefetchnta [esi+128]

        movq        mm7,[esi]
        movq        mm6,[esi+8]
        movq        mm5,[esi+16]
        movq        mm3,[esi+24]

        psubusb     mm7,mm4
        psubusb     mm6,mm4
        psubusb     mm5,mm4
        psubusb     mm3,mm4

        movq        [esi+0],mm7
        movq        [esi+8],mm6
        movq        [esi+16],mm5
        movq        [esi+24],mm3

        add         esi,32
        dec         ecx
        jne         x_loop32

        test        edx,edx
        jz          endaddbr

        sar         edx,2
    align 16
    rest4:
        movd        mm7,[esi]
        psubusb     mm7,mm4
        movd        [esi],mm7
        add         esi,4
        dec         edx
        jne         rest4
    align 16
    endaddbr:
        add         esi,edi//modulosrc
        dec         ebx
        jne         y_loop
    align 16
    endend:
        emms
    };
}
////////////////////////////////////////////////////////////////////
void asm_Brightrow_SSE2_UNSUBADD
                    (BYTE* srcp,
                    int row,
                    int height,
                    int modulosrc,
                    int brite)
{
    __asm{
        mov         eax,[brite]
        mov         esi,[srcp]
        mov         ebx,[height]
        mov         edi,[modulosrc]

        test        eax,eax
        js          isasubst

        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4

        movq2dq     xmm4,mm4
        punpckldq   xmm4,xmm4

        mov         eax,[row]
        prefetcht0  [esi]
    align 16
    y_loopadd:
        mov         ecx,eax
    align 16
    testaladd:
        test        esi,15
        movq        mm7,[esi]
        jz          prex_loop64add

        paddusb     mm7,mm4

        test        esi,3
        movd        edx,mm7
        jz          test4aladd

        mov         [esi],dx
        sub         ecx,2
        lea         esi,[esi+2]
        jmp         testaladd
    align 16
    test4aladd:
        movd        [esi],mm7
        sub         ecx,4
        lea         esi,[esi+4]
        test        esi,15
        jz          prex_loop64add
        movq        mm7,[esi]
        paddusb     mm7,mm4
        jmp         test4aladd
    align 16
    prex_loop64add:
        mov         edx,ecx
        sar         ecx,6
        and         edx,63
    align 16
    x_loop64add:
        prefetchnta [esi+320]

        sub         ecx,1

        movdqa      xmm7,[esi]
        movdqa      xmm6,[esi+16]
        movdqa      xmm5,[esi+32]
        movdqa      xmm3,[esi+48]

        paddusb     xmm7,xmm4
        paddusb     xmm6,xmm4
        paddusb     xmm5,xmm4
        paddusb     xmm3,xmm4

        movdqa      [esi],xmm7
        movdqa      [esi+16],xmm6
        movdqa      [esi+32],xmm5
        movdqa      [esi+48],xmm3

        lea         esi,[esi+64]
        jne         x_loop64add

        test        edx,edx
        jz          endadd

        mov         ecx,edx

        cmp         edx,16
        jl          prerest4add

        sar         ecx,4
        and         edx,15
    align 16
    rest16add:
        movdqa      xmm7,[esi]
        paddusb     xmm7,xmm4
        movdqa      [esi],xmm7
        sub         ecx,1
        lea         esi,[esi+16]
        jne         rest16add

        test        edx,edx
        jz          endadd
    align 16
    prerest4add:
        cmp         ecx,4
        jl          prerest2add
        mov         ecx,edx
        sar         edx,2
        and         ecx,3
    align 16
    rest4add:
        movd        mm7,[esi]
        paddusb     mm7,mm4
        movd        [esi],mm7
        sub         edx,1
        lea         esi,[esi+4]
        jne         rest4add

        test        ecx,ecx
        jz          endadd
    align 16
    prerest2add:
        movd        mm7,[esi]
        paddusb     mm7,mm4
        movd        edx,mm7
        mov         [esi],dx
        add         esi,2
    align 16
    endadd:
        sub         ebx,1
        lea         esi,[esi+edi]
        jne         y_loopadd
        jmp         endend
    ////////////////////////////////////////
    align 16
    isasubst:
        neg         al
        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4

        movq2dq     xmm4,mm4
        punpckldq   xmm4,xmm4

        mov         eax,[row]
        prefetcht0  [esi]
    align 16
    y_loop:
        mov         ecx,eax
    align 16
    testal:
        test        esi,15
        movq        mm7,[esi]
        jz          prex_loop64

        psubusb     mm7,mm4

        test        esi,3
        movd        edx,mm7
        jz          test4al

        mov         [esi],dx
        sub         ecx,2
        lea         esi,[esi+2]
        jmp         testal
    align 16
    test4al:
        movd        [esi],mm7
        sub         ecx,4
        lea         esi,[esi+4]
        test        esi,15
        jz          prex_loop64
        movq        mm7,[esi]
        psubusb     mm7,mm4
        jmp         test4al
    align 16
    prex_loop64:
        mov         edx,ecx
        sar         ecx,6
        and         edx,63
    align 16
    x_loop64:
        prefetchnta [esi+320]

        sub         ecx,1

        movdqa      xmm7,[esi]
        movdqa      xmm6,[esi+16]
        movdqa      xmm5,[esi+32]
        movdqa      xmm3,[esi+48]

        psubusb     xmm7,xmm4
        psubusb     xmm6,xmm4
        psubusb     xmm5,xmm4
        psubusb     xmm3,xmm4

        movdqa      [esi],xmm7
        movdqa      [esi+16],xmm6
        movdqa      [esi+32],xmm5
        movdqa      [esi+48],xmm3

        lea         esi,[esi+64]
        jne         x_loop64

        test        edx,edx
        jz          endaddbr

        mov         ecx,edx

        cmp         edx,16
        jl          prerest4

        sar         ecx,4
        and         edx,15
    align 16
    rest16:
        movdqa      xmm7,[esi]
        psubusb     xmm7,xmm4
        movdqa      [esi],xmm7
        sub         ecx,1
        lea         esi,[esi+16]
        jne         rest16

        test        edx,edx
        jz          endaddbr
    align 16
    prerest4:
        cmp         ecx,4
        jl          prerest2
        mov         ecx,edx
        sar         edx,2
        and         ecx,3
    align 16
    rest4:
        movd        mm7,[esi]
        psubusb     mm7,mm4
        movd        [esi],mm7
        sub         edx,1
        lea         esi,[esi+4]
        jne         rest4

        test        ecx,ecx
        jz          endaddbr
    align 16
    prerest2:
        movd        mm7,[esi]
        psubusb     mm7,mm4
        movd        edx,mm7
        mov         [esi],dx
        add         esi,2
    align 16
    endaddbr:
        sub         ebx,1
        lea         esi,[esi+edi]
        jne         y_loop
    align 16
    endend:
        emms
    };
}
///////////////////////////////////////////////////////////////////////
void asm_BrightonelineSUBADD_SSE2_UN
                        (BYTE* srcp,
                        int sizef,
                        int brite)
{
    __asm{
        mov         eax,[brite]
        mov         esi,[srcp]
        mov         ecx,[sizef]

        test        eax,eax
        js          isasubst

        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4
        movq2dq     xmm4,mm4
        punpckldq   xmm4,xmm4

        prefetcht0  [esi]

        test        esi,15
        movq        mm7,[esi]
        jz          prex_loop64add

        paddusb     mm7,mm4

        test        esi,3
        movd        eax,mm7
        jz          test4aladd

        mov         [esi],ax
        sub         ecx,2
        lea         esi,[esi+2]//??

        test        esi,15
        movq        mm7,[esi]
        jz          prex_loop64add
        paddusb     mm7,mm4
    align 16
    test4aladd:
        movd        [esi],mm7
        sub         ecx,4
        lea         esi,[esi+4]
        test        esi,15
        jz          prex_loop64add
        movq        mm7,[esi]
        paddusb     mm7,mm4
        jmp         test4aladd
    align 16
    prex_loop64add:
        mov         edx,ecx
        sar         ecx,6
        and         edx,63
    align 16
    x_loop64add:
        prefetcht0  [esi+1024]
        sub         ecx,1

        movdqa      xmm7,[esi]
        movdqa      xmm6,[esi+16]
        movdqa      xmm5,[esi+32]
        movdqa      xmm3,[esi+48]

        paddusb     xmm7,xmm4
        paddusb     xmm6,xmm4
        paddusb     xmm5,xmm4
        paddusb     xmm3,xmm4

        movdqa      [esi],xmm7
        movdqa      [esi+16],xmm6
        movdqa      [esi+32],xmm5
        movdqa      [esi+48],xmm3

        lea         esi,[esi+64]
        jne         x_loop64add

        test        edx,edx
        jz          endaddbr

        cmp         edx,16
        mov         ecx,edx
        jl          prerest4add

        sar         ecx,4
        and         edx,15
    align 16
    rest16add:
        movdqa      xmm7,[esi]
        paddusb     xmm7,xmm4
        movdqa      [esi],xmm7
        sub         ecx,1
        lea         esi,[esi+16]
        jne         rest16add

        test        edx,edx
        jz          endaddbr

        cmp         edx,4
        jl          rest2add
        mov         ecx,edx
    align 16
    prerest4add:
        sar         edx,2
        and         ecx,3
    align 16
    rest4add:
        movd        mm7,[esi]
        paddusb     mm7,mm4
        movd        [esi],mm7
        sub         edx,1
        lea         esi,[esi+4]
        jne         rest4add

        test        ecx,ecx
        jz          endaddbr
    align 16
    rest2add:
        movd        mm7,[esi]
        paddusb     mm7,mm4
        movd        eax,mm7
        mov         [esi],ax
        jmp         endaddbr
    align 16
    isasubst:
        neg         al
        mov         ah,al
        movzx       edx,ax
        shl         eax,16
        or          eax,edx

        movd        mm4,eax
        punpckldq   mm4,mm4
        movq2dq     xmm4,mm4
        punpckldq   xmm4,xmm4

        prefetcht0  [esi]


        test        esi,15
        movq        mm7,[esi]
        jz          prex_loop64

        psubusb     mm7,mm4

        test        esi,3
        movd        eax,mm7
        jz          test4al

        mov         [esi],ax
        sub         ecx,2
        lea         esi,[esi+2]

        test        esi,15
        movq        mm7,[esi]
        jz          prex_loop64
        psubusb     mm7,mm4
    align 16
    test4al:
        movd        [esi],mm7
        sub         ecx,4
        lea         esi,[esi+4]
        test        esi,15
        jz          prex_loop64
        movq        mm7,[esi]
        psubusb     mm7,mm4
        jmp         test4al
    align 16
    prex_loop64:
        mov         edx,ecx
        sar         ecx,6
        and         edx,63
    align 16
    x_loop64:
        prefetcht0  [esi+1024]
        sub         ecx,1

        movdqa      xmm7,[esi]
        movdqa      xmm6,[esi+16]
        movdqa      xmm5,[esi+32]
        movdqa      xmm3,[esi+48]

        psubusb     xmm7,xmm4
        psubusb     xmm6,xmm4
        psubusb     xmm5,xmm4
        psubusb     xmm3,xmm4

        movdqa      [esi],xmm7
        movdqa      [esi+16],xmm6
        movdqa      [esi+32],xmm5
        movdqa      [esi+48],xmm3

        lea         esi,[esi+64]
        jne         x_loop64

        test        edx,edx
        jz          endaddbr

        cmp         edx,16
        mov         ecx,edx
        jl          prerest4

        sar         ecx,4
        and         edx,15
    align 16
    rest16:
        movdqa      xmm7,[esi]
        psubusb     xmm7,xmm4
        movdqa      [esi],xmm7
        sub         ecx,1
        lea         esi,[esi+16]
        jne         rest16

        test        edx,edx
        jz          endaddbr

        cmp         edx,4
        jl          rest2
        mov         ecx,edx
    align 16
    prerest4:
        sar         edx,2
        and         ecx,3
    align 16
    rest4:
        movd        mm7,[esi]
        psubusb     mm7,mm4
        movd        [esi],mm7
        sub         edx,1
        lea         esi,[esi+4]
        jne         rest4

        test        ecx,ecx
        jz          endaddbr
    align 16
    rest2:
        movd        mm7,[esi]
        psubusb     mm7,mm4
        movd        eax,mm7
        mov         [esi],ax
    align 16
    endaddbr:
        emms
    };
}
////////////////////////////////////////////////////////////////////
void asm_LumaYV1216_SSE2_UN
                (BYTE* srcp,
                int sizefr,
                int lgm,
                int lom)
{
    __asm{
        mov         esi,[srcp]
        mov         eax,[sizefr]
        mov         edx,0x00400040

        movd        mm5,[lom]
        movd        mm6,[lgm]
        movd        mm7,edx

        pshufw      mm5,mm5,0
        pshufw      mm6,mm6,0
        punpckldq   mm7,mm7
        pxor        mm0,mm0

        prefetcht0  [esi]

        movq2dq     xmm5,mm5
        movq2dq     xmm6,mm6
        movq2dq     xmm7,mm7

        punpckldq   xmm5,xmm5
        punpckldq   xmm6,xmm6
        pxor        xmm0,xmm0
        punpckldq   xmm7,xmm7
    align 16
    lumconv_p4:
        test        esi,15
        movq        mm1,[esi]
        jz          prelumconvsse2_un

        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0

        test        esi,3
        movd        edx,mm1
        jz          test4al

        mov         [esi],dx
        sub         eax,2
        lea         esi,[esi+2]
        jmp         lumconv_p4
    align 16
    test4al:
        movd        [esi],mm1
        sub         eax,4
        lea         esi,[esi+4]

        test        esi,15
        movq        mm1,[esi]
        jz          prelumconvsse2_un

        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        jmp         test4al
    align 16
    prelumconvsse2_un:
        mov         ecx,eax
        sar         ecx,5
        and         eax,31
    align 16
    lumconvsse2_un:
        prefetcht0  [esi+640]

        movdqa      xmm1,[esi]
        movdqa      xmm3,[esi+16]

        movdqa      xmm2,xmm1
        movdqa      xmm4,xmm3

        punpcklbw   xmm1,xmm0
        punpckhbw   xmm2,xmm0

        pmullw      xmm1,xmm6
        pmullw      xmm2,xmm6

        punpcklbw   xmm3,xmm0
        punpckhbw   xmm4,xmm0

        pmullw      xmm3,xmm6
        pmullw      xmm4,xmm6

        paddw       xmm1,xmm7
        paddw       xmm2,xmm7

        psrlw       xmm1,7
        psrlw       xmm2,7

        paddw       xmm3,xmm7
        paddw       xmm4,xmm7

        psrlw       xmm3,7
        psrlw       xmm4,7

        paddw       xmm1,xmm5
        paddw       xmm2,xmm5
        paddw       xmm3,xmm5
        paddw       xmm4,xmm5

        packuswb    xmm1,xmm2
        packuswb    xmm3,xmm4

        movdqa      [esi],xmm1
        movdqa      [esi+16],xmm3

        sub         ecx,1

        lea         esi,[esi+32]
        jne         lumconvsse2_un

        test        eax,eax
        je          exitp4_un

        cmp         eax,4
        mov         ecx,eax
        jl          rest2

        sar         eax,2
        and         ecx,3
    align 16
    rest_luma_4:
        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        movd        [esi],mm1
        sub         eax,1
        lea         esi,[esi+4]
        jne         rest_luma_4

        test        ecx,ecx
        jz          exitp4_un
    align 16
    rest2:
        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        movd        ebx,mm1
        mov         [esi],bx
    exitp4_un:
        emms
    };
}
////////////////////////////////////////////////////////////////////////
void asm_LumaYV1216_SSE2_ROW
                (BYTE* srcp,
                int row,
                int height,
                int modulosrc,
                int lgm,
                int lom)
{
    __asm{
        mov         esi,[srcp]
        mov         ebx,[row]
        mov         edx,[height]
        mov         edi,[modulosrc]
        mov         eax,0x00400040

        movd        mm5,[lom]
        movd        mm6,[lgm]
        movd        mm7,eax

        pshufw      mm5,mm5,0
        pshufw      mm6,mm6,0
        punpckldq   mm7,mm7
        pxor        mm0,mm0

        movq2dq     xmm5,mm5
        movq2dq     xmm6,mm6
        movq2dq     xmm7,mm7

        punpckldq   xmm5,xmm5
        punpckldq   xmm6,xmm6
        punpckldq   xmm7,xmm7
        pxor        xmm0,xmm0

        prefetcht0  [esi]
    align 16
    y_loop:
        mov         eax,ebx//[row]
    align 16
    lumconv_p4:
        test        esi,15
        movq        mm1,[esi]
        jz          prelumconvsse2_un

        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0

        test        esi,3
        movd        ecx,mm1
        jz          test4al

        mov         [esi],cx
        sub         eax,2
        lea         esi,[esi+2]
        jmp         lumconv_p4
    align 16
    test4al:
        movd        [esi],mm1
        sub         eax,4
        lea         esi,[esi+4]

        test        esi,15
        jz          prelumconvsse2_un

        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        jmp         test4al
    align 16
    prelumconvsse2_un:
        mov         ecx,eax
        sar         ecx,5
        and         eax,31
    align 16
    lumconvsse2_un:
        prefetchnta [esi+ecx+256]

        movdqa      xmm1,[esi]
        movdqa      xmm3,[esi+16]

        movdqa      xmm2,xmm1
        movdqa      xmm4,xmm3

        punpcklbw   xmm1,xmm0
        punpckhbw   xmm2,xmm0

        pmullw      xmm1,xmm6
        pmullw      xmm2,xmm6

        punpcklbw   xmm3,xmm0
        punpckhbw   xmm4,xmm0

        pmullw      xmm3,xmm6
        pmullw      xmm4,xmm6

        paddw       xmm1,xmm7
        paddw       xmm2,xmm7

        psrlw       xmm1,7
        psrlw       xmm2,7

        paddw       xmm3,xmm7
        paddw       xmm4,xmm7

        psrlw       xmm3,7
        psrlw       xmm4,7

        paddw       xmm1,xmm5
        paddw       xmm2,xmm5
        paddw       xmm3,xmm5
        paddw       xmm4,xmm5

        packuswb    xmm1,xmm2
        packuswb    xmm3,xmm4

        movdqa      [esi],xmm1
        movdqa      [esi+16],xmm3
        sub         ecx,1

        lea         esi,[esi+32]
        jne         lumconvsse2_un

        test        eax,eax
        jz          exitp4_un

        cmp         eax,4
        jl          rest2

        mov         ecx,eax
        sar         eax,2
        and         ecx,3
    align 16
    rest_luma_4:
        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        movd        [esi],mm1
        sub         eax,1
        lea         esi,[esi+4]
        jne         rest_luma_4

        test        ecx,ecx
        jz          exitp4_un
    align 16
    rest2:
        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        movd        eax,mm1
        mov         [esi],ax
        add         esi,2
    align 16
    exitp4_un:
        sub         edx,1
        lea         esi,[esi+edi]
        jne         y_loop
        emms
    };
}
////////////////////////////////////
void asm_LumaYV1216_ISSE
            (BYTE* srcp,
            int sizefr,
            int lgm,
            int lom)
{
    __asm{
        mov         ecx,[sizefr]
        mov         esi,[srcp]
        sar         ecx,4
        mov         edx,0x00400040
        movd        mm5,[lom]
        movd        mm6,[lgm]
        movd        mm7,edx

        pshufw      mm5,mm5,0
        pshufw      mm6,mm6,0
        punpckldq   mm7,mm7

        pxor        mm0,mm0
    align 16
    lumconv:
        prefetchnta [esi+128]

        movq        mm1,[esi]
        movq        mm3,[esi+8]
        movq        mm2,mm1

        punpcklbw   mm1,mm0
        movq        mm4,mm3
        punpckhbw   mm2,mm0

        pmullw      mm1,mm6
        punpcklbw   mm3,mm0

        pmullw      mm2,mm6
        punpckhbw   mm4,mm0

        paddw       mm1,mm7
        pmullw      mm3,mm6

        paddw       mm2,mm7
        pmullw      mm4,mm6

        psrlw       mm1,7
        paddw       mm3,mm7

        psrlw       mm2,7
        paddw       mm4,mm7

        paddw       mm1,mm5
        psrlw       mm3,7

        paddw       mm2,mm5
        psrlw       mm4,7
        paddw       mm3,mm5

        packuswb    mm1,mm2
        paddw       mm4,mm5
        packuswb    mm3,mm4

        movq        [esi],mm1
        movq        [esi+8],mm3

        add         esi,16
        dec         ecx
        jne         lumconv
        emms
    };
}
//////////////////////////////////////////////////
void asm_LumaYV1216_ISSE_ROW
                (BYTE* srcp,
                int row,
                int height,
                int modulo,
                int lgm,
                int lom)
{
    __asm{
        mov     edx,0x00400040

        movd        mm5,[lom]
        movd        mm6,[lgm]
        movd        mm7,edx

        pshufw      mm5,mm5,0
        pshufw      mm6,mm6,0

        punpckldq   mm7,mm7
        pxor        mm0,mm0

        mov         esi,[srcp]
        mov         edx,[row]
        mov         ebx,[height]
        mov         edi,[modulo]
    align 16
    y_loop:
        mov         eax,edx//[row]
        mov         ecx,edx
        and         eax,15
        sar         ecx,4
    align 16
    lumconv:
        prefetchnta [esi+128]

        movq        mm1,[esi]
        movq        mm3,[esi+8]
        movq        mm2,mm1

        punpcklbw   mm1,mm0
        movq        mm4,mm3
        punpckhbw   mm2,mm0

        pmullw      mm1,mm6
        punpcklbw   mm3,mm0

        pmullw      mm2,mm6
        punpckhbw   mm4,mm0

        paddw       mm1,mm7
        pmullw      mm3,mm6

        paddw       mm2,mm7
        pmullw      mm4,mm6

        psrlw       mm1,7
        paddw       mm3,mm7

        psrlw       mm2,7
        paddw       mm4,mm7

        paddw       mm1,mm5
        psrlw       mm3,7

        paddw       mm2,mm5
        psrlw       mm4,7
        paddw       mm3,mm5

        packuswb    mm1,mm2
        paddw       mm4,mm5
        packuswb    mm3,mm4

        movq        [esi],mm1
        movq        [esi+8],mm3

        add         esi,16
        dec         ecx
        jne         lumconv

        test        eax,eax
        jz          exitp4_un

        sar         eax,2
    align 16
    rest_luma_4:
        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        movd        [esi],mm1
        add         esi,4
        dec         eax
        jne         rest_luma_4
    align 16
    exitp4_un:
        add         esi,edi
        dec         ebx
        jne         y_loop
        emms
    };
}
////////////////////////////////////////////////////////////////////////
void asm_LumaYV1216_MMX
            (BYTE* srcp,
            int sizefr,
            int lgm,
            int lom)
{
    __asm{
        mov         eax,[lgm]
        mov         ecx,[lom]

        movzx       edx,ax
        movzx       ebx,cx

        shl         eax,16
        shl         ecx,16

        or          eax,edx
        or          ecx,ebx

        mov         edx,0x00400040

        movd        mm5,ecx
        movd        mm6,eax
        movd        mm7,edx

        punpckldq   mm5,mm5
        punpckldq   mm6,mm6
        punpckldq   mm7,mm7

        pxor        mm0,mm0

        mov         ecx,[sizefr]
        mov         esi,[srcp]
        sar         ecx,4
    align 16
    lumconv:
        movq        mm1,[esi]
        movq        mm3,[esi+8]
        movq        mm2,mm1

        punpcklbw   mm1,mm0
        movq        mm4,mm3
        punpckhbw   mm2,mm0

        pmullw      mm1,mm6
        punpcklbw   mm3,mm0

        pmullw      mm2,mm6
        punpckhbw   mm4,mm0

        paddw       mm1,mm7
        pmullw      mm3,mm6

        paddw       mm2,mm7
        pmullw      mm4,mm6

        psrlw       mm1,7
        paddw       mm3,mm7

        psrlw       mm2,7
        paddw       mm4,mm7

        paddw       mm1,mm5
        psrlw       mm3,7

        paddw       mm2,mm5
        psrlw       mm4,7
        paddw       mm3,mm5

        packuswb    mm1,mm2
        paddw       mm4,mm5
        packuswb    mm3,mm4

        movq        [esi],mm1
        movq        [esi+8],mm3

        add         esi,16
        dec         ecx
        jne         lumconv
        emms
    };
}
////////////////////////////////////////////////////////////////////////
void asm_LumaYV1216_MMX_ROW
                (BYTE* srcp,
                int row,
                int height,
                int modulosrc,
                int lgm,
                int lom)
{
    __asm{
        mov         eax,[lgm]
        mov         ecx,[lom]

        movzx       edx,ax
        movzx       ebx,cx

        shl         eax,16
        shl         ecx,16

        or          eax,edx
        or          ecx,ebx

        mov         edx,0x00400040

        movd        mm5,ecx
        movd        mm6,eax
        movd        mm7,edx

        punpckldq   mm5,mm5
        punpckldq   mm6,mm6
        punpckldq   mm7,mm7

        pxor        mm0,mm0

        mov         esi,[srcp]
        mov         edx,[row]
        mov         ebx,[height]
        mov         edi,[modulosrc]
    align 16
    y_loop:
        mov         eax,edx
        mov         ecx,edx
        and         eax,15
        sar         ecx,4
    align 16
    lumconv:
        movq        mm1,[esi]
        movq        mm3,[esi+8]
        movq        mm2,mm1

        punpcklbw   mm1,mm0
        movq        mm4,mm3
        punpckhbw   mm2,mm0

        pmullw      mm1,mm6
        punpcklbw   mm3,mm0

        pmullw      mm2,mm6
        punpckhbw   mm4,mm0

        paddw       mm1,mm7
        pmullw      mm3,mm6

        paddw       mm2,mm7
        pmullw      mm4,mm6

        psrlw       mm1,7
        paddw       mm3,mm7

        psrlw       mm2,7
        paddw       mm4,mm7

        paddw       mm1,mm5
        psrlw       mm3,7

        paddw       mm2,mm5
        psrlw       mm4,7
        paddw       mm3,mm5

        packuswb    mm1,mm2
        paddw       mm4,mm5
        packuswb    mm3,mm4

        movq        [esi],mm1
        movq        [esi+8],mm3

        add         esi,16
        dec         ecx
        jne         lumconv

        test        eax,eax
        jz          exitp4_un

        sar         eax,2
    align 16
    rest_luma_4:
        movq        mm1,[esi]
        punpcklbw   mm1,mm0
        pmullw      mm1,mm6
        paddw       mm1,mm7
        psrlw       mm1,7
        paddw       mm1,mm5
        packuswb    mm1,mm0
        movd        [esi],mm1
        add         esi,4
        dec         eax
        jne         rest_luma_4
    align 16
    exitp4_un:
        add         esi,edi
        dec         ebx
        jne         y_loop
        emms
    };
}
//////////////////////////////////////////////////////////////////////////////////////
PVideoFrame __stdcall LumaYV12::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);

    BYTE* srcp  = src->GetWritePtr(PLANAR_Y);
    const int src_pitch = src->GetPitch(PLANAR_Y);
    const int height = src->GetHeight(PLANAR_Y);
    const int row_size = src->GetRowSize(PLANAR_Y_ALIGNED);//ES NECESARIO?

    if(lumGain==128)
    {
    /////////////////////////////////////////////////////////////////////////////////////
        if(use_SSE2)
        {
            if(SepFields)
            {
                asm_Brightrow_SSE2_UNSUBADD
                (srcp,
                row_size,
                height,
                src_pitch-row_size,
                lumoff);
            }
            else{
                asm_BrightonelineSUBADD_SSE2_UN
                (srcp,
                src_pitch*height,
                lumoff);
            }
        }
    ////////////////END SSE2/////////////////////////////////////////////////////////
        else if (use_ISSE)
        {
            if(SepFields)
            {
                asm_Brightrow_ISSE_SUBADD
                (srcp,
                row_size,
                height,
                src_pitch-row_size,
                lumoff);
            }
            else{
                asm_Brightoneline_ISSE_SUBADD
                (srcp,
                src_pitch*height,
                lumoff);
            }
        }
    ///////////////////////////END ISSE///////////////////////////////////////////////
        else
        {   //if (MMX)
            if(SepFields)
            {
                asm_Brightrow_MMX_SUBADD
                (srcp,
                row_size,
                height,
                src_pitch-row_size,
                lumoff);
            }
            else{
                asm_Brightoneline_MMX_SUBADD
                (srcp,
                src_pitch*height,
                lumoff);
            }
        }
        return src;
    }
    ///////////////////////both parameters///////////////////////////////////////////////
    else
    {
        if (use_SSE2)
        {
            if(SepFields)
            {
                asm_LumaYV1216_SSE2_ROW
                (srcp,
                row_size,
                height,
                src_pitch-row_size,
                lumGain,
                lumoff);
            }
            else{
                asm_LumaYV1216_SSE2_UN
                (srcp,
                src_pitch*height,
                lumGain,
                lumoff);
            }
        }
    ////////////////////////end sse2/////////////////////////////////////////////////
        else if (use_ISSE)
        {
            if(SepFields)
            {
                asm_LumaYV1216_ISSE_ROW
                (srcp,
                row_size,
                height,
                src_pitch-row_size,
                lumGain,
                lumoff);
            }
            else{
                asm_LumaYV1216_ISSE
                (srcp,
                src_pitch*height,
                lumGain,
                lumoff);
            }
        }
    ////////////////////////end isse////////////////////////
        else
        { //CPUF_MMX
            if(SepFields)
            {
                asm_LumaYV1216_MMX_ROW
                (srcp,
                row_size,
                height,
                src_pitch-row_size,
                lumGain,
                lumoff);
            }
            else{
                asm_LumaYV1216_MMX
                (srcp,
                src_pitch*height,
                lumGain,
                lumoff);
            }
        }
    }
    return src;
}


AVSValue __cdecl LumaYV12::create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new LumaYV12(args[0].AsClip(),
        args[1].AsInt(0),                   //lumoff
        args[2].AsFloat(1.00),              //lumgain
        env);
}

