/*
 *  Avisynth 2.5 API for MPEG2Dec3
 *
 *  Changes to fix frame dropping and random frame access are
 *  Copyright (C) 2003-2008 Donald A. Graft
 *
 *  Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *  based of the intial MPEG2Dec Avisytnh API Copyright (C) Mathias Born - May 2001
 *
 *  This file is part of DGMPGDec, a free MPEG-2 decoder
 *
 *  DGMPGDec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  DGMPGDec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <string.h>
#include <cstdlib>
#include <avs/minmax.h>
#include "AvisynthAPI.h"
#include "postprocess.h"
#include "utilities.h"
#include "color_convert.h"


#define VERSION "DGDecode 1.5.8"

MPEG2Source::MPEG2Source(const char* d2v, int cpu, int idct, int iPP, int moderate_h, int moderate_v, bool showQ, bool fastMC, const char* _cpu2, int _info, int _upConv, bool _i420, int iCC, IScriptEnvironment* env)
{
    int status;

    CheckCPU();

#ifdef PROFILING
    init_first(&tim);
#endif

    /* override */
    m_decoder.refinit = false;
    m_decoder.fpuinit = false;
    m_decoder.luminit = false;

    if (iPP != -1 && iPP != 0 && iPP != 1)
        env->ThrowError("MPEG2Source: iPP must be set to -1, 0, or 1!");

    if (iCC != -1 && iCC != 0 && iCC != 1)
        env->ThrowError("MPEG2Source: iCC must be set to -1, 0, or 1!");

    if (_upConv != 0 && _upConv != 1 && _upConv != 2)
        env->ThrowError("MPEG2Source: upConv must be set to 0, 1, or 2!");

    ovr_idct = idct;
    m_decoder.iPP = iPP;
    m_decoder.iCC = iCC;
    m_decoder.info = _info;
    m_decoder.showQ = showQ;
    m_decoder.fastMC = fastMC;
    m_decoder.upConv = _upConv;
    m_decoder.i420 = _i420;
    m_decoder.moderate_h = moderate_h;
    m_decoder.moderate_v = moderate_v;
    m_decoder.maxquant = m_decoder.minquant = m_decoder.avgquant = 0;

    if (ovr_idct > 7)
    {
        env->ThrowError("MPEG2Source : iDCT invalid (1:MMX,2:SSEMMX,3:SSE2,4:FPU,5:REF,6:Skal's,7:Simple)");
    }

    const char *path = d2v;

    FILE *f;
    if ((f = fopen(path,"r"))==NULL)
        env->ThrowError("MPEG2Source : unable to load D2V file \"%s\" ",path);

    fclose(f);

    status = m_decoder.Open(path);
    if (status != 0 && m_decoder.VF_File)
    {
        fclose(m_decoder.VF_File);
        m_decoder.VF_File = NULL;
    }
    if (status == 1)
        env->ThrowError("MPEG2Source: Invalid D2V file, it's empty!");
    else if (status == 2)
        env->ThrowError("MPEG2Source: DGIndex/DGDecode mismatch. You are picking up\n"
                        "a version of DGDecode, possibly from your plugins directory,\n"
                        "that does not match the version of DGIndex used to make the D2V\n"
                        "file. Search your hard disk for all copies of DGDecode.dll\n"
                        "and delete or rename all of them except for the one that\n"
                        "has the same version number as the DGIndex.exe that was used\n"
                        "to make the D2V file.");
    else if (status == 3)
        env->ThrowError("MPEG2Source: Could not open one of the input files.");
    else if (status == 4)
        env->ThrowError("MPEG2Source: Could not find a sequence header in the input stream.");
    else if (status == 5)
        env->ThrowError("MPEG2Source: The input file is not a D2V project file.");
    else if (status == 6)
        env->ThrowError("MPEG2Source: Force Film mode not supported for frame repeats.");

    memset(&vi, 0, sizeof(vi));
    vi.width = m_decoder.Clip_Width;
    vi.height = m_decoder.Clip_Height;
    if (m_decoder.chroma_format == 1 && _upConv == 0)
    {
        if (m_decoder.i420 == true)
            vi.pixel_type = VideoInfo::CS_I420;
        else
            vi.pixel_type = VideoInfo::CS_YV12;
    }
    else if (m_decoder.chroma_format == 1 && _upConv == 1) vi.pixel_type = VideoInfo::CS_YUY2;
    else if (m_decoder.chroma_format == 1 && _upConv == 2) vi.pixel_type = VideoInfo::CS_BGR24;
    else if (m_decoder.chroma_format == 2 && _upConv != 2) vi.pixel_type = VideoInfo::CS_YUY2;
    else if (m_decoder.chroma_format == 2 && _upConv == 2) vi.pixel_type = VideoInfo::CS_BGR24;
    else env->ThrowError("MPEG2Source:  currently unsupported input color format (4:4:4)");
    vi.fps_numerator = m_decoder.VF_FrameRate_Num;
    vi.fps_denominator = m_decoder.VF_FrameRate_Den;
    vi.num_frames = m_decoder.VF_FrameLimit;
    vi.SetFieldBased(false);

    switch (cpu) {
        case 0 : _PP_MODE = 0; break;
        case 1 : _PP_MODE = PP_DEBLOCK_Y_H; break;
        case 2 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V; break;
        case 3 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H; break;
        case 4 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V; break;
        case 5 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y; break;
        case 6 : _PP_MODE = PP_DEBLOCK_Y_H | PP_DEBLOCK_Y_V | PP_DEBLOCK_C_H | PP_DEBLOCK_C_V | PP_DERING_Y | PP_DERING_C; break;
        default : env->ThrowError("MPEG2Source : cpu level invalid (0 to 6)");
    }

    const char* cpu2 = _cpu2;
    if (strlen(cpu2)==6) {
        _PP_MODE = 0;
        if (cpu2[0]=='x' || cpu2[0] == 'X') { _PP_MODE |= PP_DEBLOCK_Y_H; }
        if (cpu2[1]=='x' || cpu2[1] == 'X') { _PP_MODE |= PP_DEBLOCK_Y_V; }
        if (cpu2[2]=='x' || cpu2[2] == 'X') { _PP_MODE |= PP_DEBLOCK_C_H; }
        if (cpu2[3]=='x' || cpu2[3] == 'X') { _PP_MODE |= PP_DEBLOCK_C_V; }
        if (cpu2[4]=='x' || cpu2[4] == 'X') { _PP_MODE |= PP_DERING_Y; }
        if (cpu2[5]=='x' || cpu2[5] == 'X') { _PP_MODE |= PP_DERING_C; }
    }

    if ( ovr_idct != m_decoder.IDCT_Flag && ovr_idct > 0 )
    {
        dprintf("Overiding iDCT With: %d", ovr_idct);
        override(ovr_idct);
    }

    out = NULL;
    out = (YV12PICT*)aligned_malloc(sizeof(YV12PICT),0);
    if (out == NULL) env->ThrowError("MPEG2Source:  malloc failure (yv12 pic out)!");

    m_decoder.AVSenv = env;
    m_decoder.pp_mode = _PP_MODE;

    bufY = bufU = bufV = NULL;
    if (m_decoder.chroma_format != 1 || (m_decoder.chroma_format == 1 && _upConv > 0))
    {
        bufY = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,32);
        bufU = (unsigned char*)aligned_malloc(m_decoder.Chroma_Width*vi.height+2048,32);
        bufV =  (unsigned char*)aligned_malloc(m_decoder.Chroma_Width*vi.height+2048,32);
        if (bufY == NULL || bufU == NULL || bufV == NULL)
            env->ThrowError("MPEG2Source:  malloc failure (bufY, bufU, bufV)!");
    }

    u444 = v444 = NULL;
    if (_upConv == 2)
    {
        u444 = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,32);
        v444 = (unsigned char*)aligned_malloc(vi.width*vi.height+2048,32);
        if (u444 == NULL || v444 == NULL)
            env->ThrowError("MPEG2Source:  malloc failure (u444, v444)!");
    }
}

MPEG2Source::~MPEG2Source()
{
    m_decoder.Close();
    if (out != NULL) { aligned_free(out); out = NULL; }
    if (bufY != NULL) { aligned_free(bufY); bufY = NULL; }
    if (bufU != NULL) { aligned_free(bufU); bufU = NULL; }
    if (bufV != NULL) { aligned_free(bufV); bufV = NULL; }
    if (u444 != NULL) { aligned_free(u444); u444 = NULL; }
    if (v444 != NULL) { aligned_free(v444); v444 = NULL; }
}


bool __stdcall MPEG2Source::GetParity(int)
{
    return m_decoder.Field_Order == 1;
}


void MPEG2Source::override(int ovr_idct)
{
    if (ovr_idct > 0)
        m_decoder.IDCT_Flag = ovr_idct;

    switch (m_decoder.IDCT_Flag)
    {
        case IDCT_MMX:
            m_decoder.idctFunc = MMX_IDCT;
            break;

        case IDCT_SSEMMX:
            m_decoder.idctFunc = SSEMMX_IDCT;
            if (!cpu.ssemmx)
            {
                m_decoder.IDCT_Flag = IDCT_MMX;
                m_decoder.idctFunc = MMX_IDCT;
            }
            break;

        case IDCT_FPU:
            if (!m_decoder.fpuinit)
            {
                Initialize_FPU_IDCT();
                m_decoder.fpuinit = true;
            }
            m_decoder.idctFunc = FPU_IDCT;
            break;

        case IDCT_REF:
            m_decoder.refinit = true;
            m_decoder.idctFunc = REF_IDCT;
            break;

        case IDCT_SSE2MMX:
            m_decoder.idctFunc = SSE2MMX_IDCT;
            if (!cpu.sse2mmx)
            {
                m_decoder.IDCT_Flag = IDCT_SSEMMX;
                m_decoder.idctFunc = SSEMMX_IDCT;
                if (!cpu.ssemmx)
                {
                    m_decoder.IDCT_Flag = IDCT_MMX;
                    m_decoder.idctFunc = MMX_IDCT;
                }
            }
            break;

        case IDCT_SKALSSE:
            m_decoder.idctFunc = Skl_IDct16_Sparse_SSE; //Skl_IDct16_SSE;
            if (!cpu.ssemmx)
            {
                m_decoder.IDCT_Flag = IDCT_MMX;
                m_decoder.idctFunc = MMX_IDCT;
            }
            break;

        case IDCT_SIMPLEIDCT:
            m_decoder.idctFunc = simple_idct_mmx;
            if (!cpu.ssemmx)
            {
                m_decoder.IDCT_Flag = IDCT_MMX;
                m_decoder.idctFunc = MMX_IDCT;
            }
            break;
    }
}


PVideoFrame __stdcall MPEG2Source::GetFrame(int n, IScriptEnvironment* env)
{
    int gop, pct;
    char Matrix_s[40];
    unsigned int raw;
    unsigned int hint;

    if (m_decoder.info != 0 || m_decoder.upConv == 2)
    {
        raw = max(m_decoder.FrameList[n].bottom, m_decoder.FrameList[n].top);
        if (raw < (int)m_decoder.BadStartingFrames) raw = m_decoder.BadStartingFrames;

        for (gop = 0; gop < (int) m_decoder.VF_GOPLimit - 1; gop++)
        {
            if (raw >= (int)m_decoder.GOPList[gop]->number && raw < (int)m_decoder.GOPList[gop+1]->number)
            {
                break;
            }
        }
    }

    PVideoFrame frame = env->NewVideoFrame(vi);

    if (m_decoder.chroma_format == 1 && m_decoder.upConv == 0) // YV12
    {
        out->y = frame->GetWritePtr(PLANAR_Y);
        out->u = frame->GetWritePtr(PLANAR_U);
        out->v = frame->GetWritePtr(PLANAR_V);
        out->ypitch = frame->GetPitch(PLANAR_Y);
        out->uvpitch = frame->GetPitch(PLANAR_U);
        out->ywidth = frame->GetRowSize(PLANAR_Y);
        out->uvwidth = frame->GetRowSize(PLANAR_U);
        out->yheight = frame->GetHeight(PLANAR_Y);
        out->uvheight = frame->GetHeight(PLANAR_V);
    }
    else // its 4:2:2, pass our own buffers
    {
        out->y = bufY;
        out->u = bufU;
        out->v = bufV;
        out->ypitch = vi.width;
        out->uvpitch = m_decoder.Chroma_Width;
        out->ywidth = vi.width;
        out->uvwidth = m_decoder.Chroma_Width;
        out->yheight = vi.height;
        out->uvheight = vi.height;
    }

#ifdef PROFILING
    init_timers(&tim);
    start_timer2(&tim.overall);
#endif

    m_decoder.Decode(n, out);

    if ( m_decoder.Luminance_Flag )
        m_decoder.LuminanceFilter(out->y, out->ywidth, out->yheight, out->ypitch);

#ifdef PROFILING
    stop_timer2(&tim.overall);
    tim.sum += tim.overall;
    tim.div++;
    timer_debug(&tim);
#endif

    __asm emms;

    if ((m_decoder.chroma_format == 2 && m_decoder.upConv != 2) ||
        (m_decoder.chroma_format == 1 && m_decoder.upConv == 1)) // convert 4:2:2 (planar) to YUY2 (packed)
    {
        conv422toYUV422(out->y,out->u,out->v,frame->GetWritePtr(),out->ypitch,out->uvpitch,
            frame->GetPitch(),vi.width,vi.height);
    }

    if (m_decoder.upConv == 2) // convert 4:2:2 (planar) to 4:4:4 (planar) and then to RGB24
    {
        conv422to444_iSSE(out->u,u444,out->uvpitch,vi.width,vi.width,vi.height);
        conv422to444_iSSE(out->v,v444,out->uvpitch,vi.width,vi.width,vi.height);
        PVideoFrame frame2 = env->NewVideoFrame(vi);
        conv444toRGB24(out->y,u444,v444,frame2->GetWritePtr(),out->ypitch,vi.width,
            frame2->GetPitch(),vi.width,vi.height,m_decoder.GOPList[gop]->matrix,m_decoder.pc_scale);
        env->BitBlt(frame->GetWritePtr(), frame->GetPitch(),
            frame2->GetReadPtr() + (vi.height-1) * frame2->GetPitch(), -frame2->GetPitch(),
            frame->GetRowSize(), frame->GetHeight());
    }

    if (m_decoder.info != 0)
    {
        pct = m_decoder.FrameList[raw].pct == I_TYPE ? 'I' : m_decoder.FrameList[raw].pct == B_TYPE ? 'B' : 'P';

        switch (m_decoder.GOPList[gop]->matrix)
        {
        case 0:
            strcpy(Matrix_s, "Forbidden");
            break;
        case 1:
            strcpy(Matrix_s, "ITU-R BT.709");
            break;
        case 2:
            strcpy(Matrix_s, "Unspecified Video");
            break;
        case 3:
            strcpy(Matrix_s, "Reserved");
            break;
        case 4:
            strcpy(Matrix_s, "FCC");
            break;
        case 5:
            strcpy(Matrix_s, "ITU-R BT.470-2 System B, G");
            break;
        case 6:
            strcpy(Matrix_s, "SMPTE 170M");
            break;
        case 7:
            strcpy(Matrix_s, "SMPTE 240M (1987)");
            break;
        default:
            strcpy(Matrix_s, "Reserved");
            break;
        }
    }

    if (m_decoder.info == 1)
    {
        char msg1[1024];
        sprintf(msg1,"%s (c) 2007 Donald A. Graft (et al)\n" \
                     "---------------------------------------\n" \
                     "Source:        %s\n" \
                     "Frame Rate:    %3.6f fps (%u/%u) %s\n" \
                     "Field Order:   %s\n" \
                     "Picture Size:  %d x %d\n" \
                     "Aspect Ratio:  %s\n"  \
                     "Progr Seq:     %s\n" \
                     "GOP Number:    %d (%d)  GOP Pos = %I64d\n" \
                     "Closed GOP:    %s\n" \
                     "Display Frame: %d\n" \
                     "Encoded Frame: %d (top) %d (bottom)\n" \
                     "Frame Type:    %c (%d)\n" \
                     "Progr Frame:   %s\n" \
                     "Colorimetry:   %s (%d)\n" \
                     "Quants:        %d/%d/%d (avg/min/max)\n",
        VERSION,
        m_decoder.Infilename[m_decoder.GOPList[gop]->file],
        double(m_decoder.VF_FrameRate_Num) / double(m_decoder.VF_FrameRate_Den),
            m_decoder.VF_FrameRate_Num,
            m_decoder.VF_FrameRate_Den,
            (m_decoder.VF_FrameRate == 25000 || m_decoder.VF_FrameRate == 50000) ? "(PAL)" :
            m_decoder.VF_FrameRate == 29970 ? "(NTSC)" : "",
        m_decoder.Field_Order == 1 ? "Top Field First" : "Bottom Field First",
        m_decoder.Coded_Picture_Width, m_decoder.Coded_Picture_Height,
        m_decoder.Aspect_Ratio,
        m_decoder.GOPList[gop]->progressive ? "True" : "False",
        gop, m_decoder.GOPList[gop]->number, m_decoder.GOPList[gop]->position,
        m_decoder.GOPList[gop]->closed ? "True" : "False",
        n,
        m_decoder.FrameList[n].top, m_decoder.FrameList[n].bottom,
        pct, raw,
        m_decoder.FrameList[raw].pf ? "True" : "False",
        Matrix_s, m_decoder.GOPList[gop]->matrix,
        m_decoder.avgquant, m_decoder.minquant, m_decoder.maxquant);
        ApplyMessage(&frame, vi, msg1, 150, 0xdfffbf, 0x0, 0x0, env);
    }
    else if (m_decoder.info == 2)
    {
        dprintf("DGDecode: DGDecode %s (c) 2005 Donald A. Graft\n", VERSION);
        dprintf("DGDecode: Source:            %s\n", m_decoder.Infilename[m_decoder.GOPList[gop]->file]);
        dprintf("DGDecode: Frame Rate:        %3.6f fps (%u/%u) %s\n",
            double(m_decoder.VF_FrameRate_Num) / double(m_decoder.VF_FrameRate_Den),
            m_decoder.VF_FrameRate_Num, m_decoder.VF_FrameRate_Den,
            (m_decoder.VF_FrameRate == 25000 || m_decoder.VF_FrameRate == 50000) ? "(PAL)" : m_decoder.VF_FrameRate == 29970 ? "(NTSC)" : "");
        dprintf("DGDecode: Field Order:       %s\n", m_decoder.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
        dprintf("DGDecode: Picture Size:      %d x %d\n", m_decoder.Coded_Picture_Width, m_decoder.Coded_Picture_Height);
        dprintf("DGDecode: Aspect Ratio:      %s\n", m_decoder.Aspect_Ratio);
        dprintf("DGDecode: Progressive Seq:   %s\n", m_decoder.GOPList[gop]->progressive ? "True" : "False");
        dprintf("DGDecode: GOP Number:        %d (%d)  GOP Pos = %I64d\n", gop, m_decoder.GOPList[gop]->number, m_decoder.GOPList[gop]->position);
        dprintf("DGDecode: Closed GOP:        %s\n", m_decoder.GOPList[gop]->closed ? "True" : "False");
        dprintf("DGDecode: Display Frame:     %d\n", n);
        dprintf("DGDecode: Encoded Frame:     %d (top) %d (bottom)\n", m_decoder.FrameList[n].top, m_decoder.FrameList[n].bottom);
        dprintf("DGDecode: Frame Type:        %c (%d)\n", pct, raw);
        dprintf("DGDecode: Progressive Frame: %s\n", m_decoder.FrameList[raw].pf ? "True" : "False");
        dprintf("DGDecode: Colorimetry:       %s (%d)\n", Matrix_s, m_decoder.GOPList[gop]->matrix);
        dprintf("DGDecode: Quants:            %d/%d/%d (avg/min/max)\n", m_decoder.avgquant, m_decoder.minquant, m_decoder.maxquant);
    }
    else if (m_decoder.info == 3)
    {
        hint = 0;
        if (m_decoder.FrameList[raw].pf == 1) hint |= PROGRESSIVE;
        hint |= ((m_decoder.GOPList[gop]->matrix & 7) << COLORIMETRY_SHIFT);
        PutHintingData(frame->GetWritePtr(PLANAR_Y), hint);
    }

    return frame;
}


AVSValue __cdecl MPEG2Source::create(AVSValue args, void*, IScriptEnvironment* env)
{
//    char path[1024];
    char buf[80], *p;

    char d2v[255];
    int cpu = 0;
    int idct = -1;

    // check if iPP is set, if it is use the set value... if not
    // we set iPP to -1 which automatically switches between field
    // and progressive pp based on pf flag -- same for iCC
    int iPP = args[3].IsBool() ? (args[3].AsBool() ? 1 : 0) : -1;
    int iCC = args[12].IsBool() ? (args[12].AsBool() ? 1 : 0) : -1;

    int moderate_h = 20;
    int moderate_v = 40;
    bool showQ = false;
    bool fastMC = false;
    char cpu2[255];
    int info = 0;
    int upConv = 0;
    bool i420 = false;

    /* Based on D.Graft Msharpen default files code */
    /* Load user defaults if they exist. */
#define LOADINT(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
    p = buf; \
    while(*p++ != '='); \
    var = atoi(p); \
}

#define LOADSTR(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
    p = buf; \
    while(*p++ != '='); \
    char* dst = var; \
    int i=0; \
    if (*p++ == '"') while('"' != *p++) i++; \
    var[i] = 0; \
    strncpy(var,(p-i-1),i); \
}

#define LOADBOOL(var,name,len) \
if (strncmp(buf, name, len) == 0) \
{ \
    p = buf; \
    while(*p++ != '='); \
    if (*p == 't') var = true; \
    else var = false; \
}
    try
    {
        FILE *f;

        if ((f = fopen("DGDecode.def", "r")) != NULL)
        {
            while(fgets(buf, 80, f) != 0)
            {
                LOADSTR(d2v,"d2v=",4)
                LOADINT(cpu,"cpu=",4)
                LOADINT(idct,"idct=",5)
                LOADBOOL(iPP,"iPP=",4)
                LOADINT(moderate_h,"moderate_h=",11)
                LOADINT(moderate_v,"moderate_v=",11)
                LOADBOOL(showQ,"showQ=",6)
                LOADBOOL(fastMC,"fastMC=",7)
                LOADSTR(cpu2,"cpu2=",5)
                LOADINT(info,"info=",5);
                LOADINT(upConv,"upConv=",7);
                LOADBOOL(i420,"i420=",5);
                LOADBOOL(iCC,"iCC=",4);
            }
        }
    }
    catch (...)
    {
        // plugin directory not set
        // probably using an older version avisynth
    }

    // check for uninitialised strings
    if (strlen(d2v)>=255) d2v[0]=0;
    if (strlen(cpu2)>=255) cpu2[0]=0;

    MPEG2Source *dec = new MPEG2Source( args[0].AsString(d2v),
                                        args[1].AsInt(cpu),
                                        args[2].AsInt(idct),
                                        iPP,
                                        args[4].AsInt(moderate_h),
                                        args[5].AsInt(moderate_v),
                                        args[6].AsBool(showQ),
                                        args[7].AsBool(fastMC),
                                        args[8].AsString(cpu2),
                                        args[9].AsInt(info),
                                        args[10].AsInt(upConv),
                                        args[11].AsBool(i420),
                                        iCC,
                                        env );
    // Only bother invoking crop if we have to.
    if (dec->m_decoder.Clip_Top    ||
        dec->m_decoder.Clip_Bottom ||
        dec->m_decoder.Clip_Left   ||
        dec->m_decoder.Clip_Right ||
        // This is cheap but it works. The intent is to allow the
        // display size to be different from the encoded size, while
        // not requiring massive revisions to the code. So we detect the
        // difference and crop it off.
        dec->m_decoder.vertical_size != dec->m_decoder.Clip_Height ||
        dec->m_decoder.horizontal_size != dec->m_decoder.Clip_Width ||
        dec->m_decoder.vertical_size == 1088)
    {
        int vertical;
        // Special case for 1088 to 1080 as directed by DGIndex.
        if (dec->m_decoder.vertical_size == 1088 && dec->m_decoder.D2V_Height == 1080)
            vertical = 1080;
        else
            vertical = dec->m_decoder.vertical_size;
        AVSValue CropArgs[5] =
        {
            dec,
            dec->m_decoder.Clip_Left,
            dec->m_decoder.Clip_Top,
            -(dec->m_decoder.Clip_Right + (dec->m_decoder.Clip_Width - dec->m_decoder.horizontal_size)),
            -(dec->m_decoder.Clip_Bottom + (dec->m_decoder.Clip_Height - vertical))
        };

        return env->Invoke("crop", AVSValue(CropArgs,5));
    }

    return dec;
}


const AVS_Linkage* AVS_linkage = nullptr;


extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("MPEG2Source", "[d2v]s[cpu]i[idct]i[iPP]b[moderate_h]i[moderate_v]i[showQ]b[fastMC]b[cpu2]s[info]i[upConv]i[i420]b[iCC]b", MPEG2Source::create, nullptr);
    env->AddFunction("LumaYV12","c[lumoff]i[lumgain]f", LumaYV12::create, nullptr);
    env->AddFunction("BlindPP", "c[quant]i[cpu]i[cpu2]s[iPP]b[moderate_h]i[moderate_v]i", BlindPP::create, nullptr);
    env->AddFunction("Deblock", "c[quant]i[aOffset]i[bOffset]i[mmx]b[isse]b", Deblock::create, nullptr);
    return 0;
}

