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

#include <cstring>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <malloc.h>
#include "AvisynthAPI.h"
#include "postprocess.h"
#include "color_convert.h"
#include "misc.h"
#include "idct.h"


#define VERSION "MPEG2DecPlus 0.1.1"


static const char* get_error_msg(int status)
{
    const char* msg[] = {
        "Invalid D2V file, it's empty!",

        "DGIndex/MPEG2DecPlus mismatch. You are picking up\n"
        "a version of MPEG2DecPlus, possibly from your plugins directory,\n"
        "that does not match the version of DGIndex used to make the D2V\n"
        "file. Search your hard disk for all copies of MPEG2DecPlus.dll\n"
        "and delete or rename all of them except for the one that\n"
        "has the same version number as the DGIndex.exe that was used\n"
        "to make the D2V file.",

        "Could not open one of the input files.",

        "Could not find a sequence header in the input stream.",

        "The input file is not a D2V project file.",

        "Force Film mode not supported for frame repeats.",
    };

    return msg[status - 1];
}


bool PutHintingData(uint8_t *video, uint32_t hint)
{
    constexpr uint32_t MAGIC_NUMBER = 0xdeadbeef;

    for (int i = 0; i < 32; ++i) {
        *video &= ~1;
        *video++ |= ((MAGIC_NUMBER & (1 << i)) >> i);
    }
    for (int i = 0; i < 32; i++) {
        *video &= ~1;
        *video++ |= ((hint & (1 << i)) >> i);
    }
    return false;
}


static void luminance_filter(uint8_t* luma, const int width, const int height,
    const int pitch, const uint8_t* table)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            luma[x] = table[luma[x]];
        }
        luma += pitch;
    }
}


static void show_info(int n, CMPEG2Decoder& d, PVideoFrame& frame,
    const VideoInfo& vi, IScriptEnvironment* env)
{
    uint32_t raw = std::max(d.FrameList[n].bottom, d.FrameList[n].top);
    if (raw < d.BadStartingFrames)
        raw = d.BadStartingFrames;

    uint32_t gop = 0;
    do {
        if (raw >= d.GOPList[gop].number)
            if (raw < d.GOPList[gop+1].number)
                break;
    } while (++gop < d.GOPList.size() - 1);

    const auto& rgop = d.GOPList[gop];
    const auto& fraw = d.FrameList[raw];
    const auto& fn = d.FrameList[n];

    int pct = fraw.pct == I_TYPE ? 'I' : fraw.pct == B_TYPE ? 'B' : 'P';

    const char* matrix[8] = {
        "Forbidden",
        "ITU-R BT.709",
        "Unspecified Video",
        "Reserved",
        "FCC",
        "ITU-R BT.470-2 System B, G",
        "SMPTE 170M",
        "SMPTE 240M (1987)",
    };

    if (d.info == 1) {
        char msg1[1024];
        sprintf(msg1,"%s\n"
            "---------------------------------------\n"
            "Source:        %s\n"
            "Frame Rate:    %3.6f fps (%u/%u) %s\n"
            "Field Order:   %s\n"
            "Picture Size:  %d x %d\n"
            "Aspect Ratio:  %s\n"
            "Progr Seq:     %s\n"
            "GOP Number:    %u (%u)  GOP Pos = %I64d\n"
            "Closed GOP:    %s\n"
            "Display Frame: %d\n"
            "Encoded Frame: %d (top) %d (bottom)\n"
            "Frame Type:    %c (%d)\n"
            "Progr Frame:   %s\n"
            "Colorimetry:   %s (%d)\n"
            "Quants:        %d/%d/%d (avg/min/max)\n",
            VERSION,
            d.Infilename[rgop.file].c_str(),
            double(d.VF_FrameRate_Num) / double(d.VF_FrameRate_Den),
            d.VF_FrameRate_Num,
            d.VF_FrameRate_Den,
            (d.VF_FrameRate == 25000 || d.VF_FrameRate == 50000) ? "(PAL)" :
            d.VF_FrameRate == 29970 ? "(NTSC)" : "",
            d.Field_Order == 1 ? "Top Field First" : "Bottom Field First",
            d.getLumaWidth(), d.getLumaHeight(),
            d.Aspect_Ratio,
            rgop.progressive ? "True" : "False",
            gop, rgop.number, rgop.position,
            rgop.closed ? "True" : "False",
            n,
            fn.top, fn.bottom,
            pct, raw,
            fraw.pf ? "True" : "False",
            matrix[rgop.matrix], rgop.matrix,
            d.avgquant, d.minquant, d.maxquant);
        env->ApplyMessage(&frame, vi, msg1, 150, 0xdfffbf, 0x0, 0x0);

    } else if (d.info == 2) {
        dprintf("MPEG2DecPlus: %s\n", VERSION);
        dprintf("MPEG2DecPlus: Source:            %s\n", d.Infilename[rgop.file]);
        dprintf("MPEG2DecPlus: Frame Rate:        %3.6f fps (%u/%u) %s\n",
            double(d.VF_FrameRate_Num) / double(d.VF_FrameRate_Den),
            d.VF_FrameRate_Num, d.VF_FrameRate_Den,
            (d.VF_FrameRate == 25000 || d.VF_FrameRate == 50000) ? "(PAL)" : d.VF_FrameRate == 29970 ? "(NTSC)" : "");
        dprintf("MPEG2DecPlus: Field Order:       %s\n", d.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
        dprintf("MPEG2DecPlus: Picture Size:      %d x %d\n", d.getLumaWidth(), d.getLumaHeight());
        dprintf("MPEG2DecPlus: Aspect Ratio:      %s\n", d.Aspect_Ratio);
        dprintf("MPEG2DecPlus: Progressive Seq:   %s\n", rgop.progressive ? "True" : "False");
        dprintf("MPEG2DecPlus: GOP Number:        %d (%d)  GOP Pos = %I64d\n", gop, rgop.number, rgop.position);
        dprintf("MPEG2DecPlus: Closed GOP:        %s\n", rgop.closed ? "True" : "False");
        dprintf("MPEG2DecPlus: Display Frame:     %d\n", n);
        dprintf("MPEG2DecPlus: Encoded Frame:     %d (top) %d (bottom)\n", fn.top, fn.bottom);
        dprintf("MPEG2DecPlus: Frame Type:        %c (%d)\n", pct, raw);
        dprintf("MPEG2DecPlus: Progressive Frame: %s\n", fraw.pf ? "True" : "False");
        dprintf("MPEG2DecPlus: Colorimetry:       %s (%d)\n", matrix[rgop.matrix], rgop.matrix);
        dprintf("MPEG2DecPlus: Quants:            %d/%d/%d (avg/min/max)\n", d.avgquant, d.minquant, d.maxquant);

    } else if (d.info == 3) {
        constexpr uint32_t PROGRESSIVE = 0x00000001;
        constexpr int COLORIMETRY_SHIFT = 2;

        uint32_t hint = 0;
        if (fraw.pf == 1) hint |= PROGRESSIVE;
        hint |= ((rgop.matrix & 7) << COLORIMETRY_SHIFT);
        PutHintingData(frame->GetWritePtr(PLANAR_Y), hint);
    }
}


MPEG2Source::MPEG2Source(const char* d2v, int cpu, int idct, int iPP,
                         int moderate_h, int moderate_v, bool showQ,
                         bool fastMC, const char* _cpu2, int _info,
                         int _upConv, bool _i420, int iCC,
                         IScriptEnvironment* env) :
    bufY(nullptr), bufU(nullptr), bufV(nullptr)
{
    //if (iPP != -1 && iPP != 0 && iPP != 1)
    //    env->ThrowError("MPEG2Source: iPP must be set to -1, 0, or 1!");

    if (iCC != -1 && iCC != 0 && iCC != 1)
        env->ThrowError("MPEG2Source: iCC must be set to -1, 0, or 1!");

    if (_upConv != 0 && _upConv != 1 && _upConv != 2)
        env->ThrowError("MPEG2Source: upConv must be set to 0, 1, or 2!");

    if (idct > 7)
        env->ThrowError("MPEG2Source: iDCT invalid (1:MMX,2:SSEMMX,3:SSE2,4:FPU,5:REF,6:Skal's,7:Simple)");

    FILE *f = fopen(d2v, "r");
    if (f == nullptr)
        env->ThrowError("MPEG2Source: unable to load D2V file \"%s\" ", d2v);

    decoder.iCC = iCC;
    decoder.info = _info;
    decoder.showQ = showQ;
    decoder.upConv = _upConv;
    decoder.i420 = _i420;
//    m_decoder.iPP = iPP;
//    m_decoder.moderate_h = moderate_h;
//    m_decoder.moderate_v = moderate_v;

    int status = decoder.Open(f, d2v);
    if (status != 0) {
        if (f) fclose(f);
        f = nullptr;
        env->ThrowError("MPEG2Source: %s", get_error_msg(status));
    }

    int chroma_format = decoder.getChromaFormat();
    if (chroma_format != 1 && chroma_format != 2) {
        env->ThrowError("MPEG2Source:  currently unsupported input color format (4:4:4)");
    }

    memset(&vi, 0, sizeof(vi));
    vi.width = decoder.Clip_Width;
    vi.height = decoder.Clip_Height;
    if (_upConv == 2) vi.pixel_type = VideoInfo::CS_YV24;
    else if (chroma_format == 2 || _upConv == 1) vi.pixel_type = VideoInfo::CS_YV16;
    else if (decoder.i420 == true) vi.pixel_type = VideoInfo::CS_I420;
    else vi.pixel_type = VideoInfo::CS_YV12;

    vi.SetFPS(decoder.VF_FrameRate_Num, decoder.VF_FrameRate_Den);
    vi.num_frames = decoder.VF_FrameLimit;
    vi.SetFieldBased(false);
#if 0
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

    decoder.pp_mode = _PP_MODE;
#endif

    if (idct > 0) {
        dprintf("Overiding iDCT With: %d", idct);
        decoder.setIDCT(idct);
    }

    if (_upConv == 2) {
        auto free_buf = [](void* p, IScriptEnvironment* e) {
            _aligned_free(p);
            p = nullptr;
        };
        size_t ysize = ((vi.width + 31) & ~31) * (vi.height + 1);
        size_t uvsize = ((decoder.getChromaWidth() + 15) & ~15) * (vi.height + 1);
        void* ptr = _aligned_malloc(ysize + 2 * uvsize, 32);
        if (!ptr) {
            env->ThrowError("MPEG2Source:  malloc failure (bufY, bufU, bufV)!");
        }
        env->AtExit(free_buf, ptr);
        bufY = reinterpret_cast<uint8_t*>(ptr);
        bufU = bufY + ysize;
        bufV = bufU + uvsize;
    }

    luminanceFlag = (decoder.lumGamma != 0 || decoder.lumOffset != 0);
    if (luminanceFlag) {
        int lg = decoder.lumGamma;
        int lo = decoder.lumOffset;
        for (int i = 0; i < 256; ++i) {
            double value = 255.0 * pow(i / 255.0, pow(2.0, -lg / 128.0)) + lo + 0.5;

            if (value < 0)
                luminanceTable[i] = 0;
            else if (value > 255.0)
                luminanceTable[i] = 255;
            else
                luminanceTable[i] = static_cast<uint8_t>(value);
        }
    }
}

MPEG2Source::~MPEG2Source()
{
    decoder.Close();
}


bool __stdcall MPEG2Source::GetParity(int)
{
    return decoder.Field_Order == 1;
}


PVideoFrame __stdcall MPEG2Source::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame frame = env->NewVideoFrame(vi, 32);
    YV12PICT out = {};

    if (decoder.upConv != 2) { // YV12 or YV16 output
        out.y = frame->GetWritePtr(PLANAR_Y);
        out.u = frame->GetWritePtr(PLANAR_U);
        out.v = frame->GetWritePtr(PLANAR_V);
        out.ypitch = frame->GetPitch(PLANAR_Y);
        out.uvpitch = frame->GetPitch(PLANAR_U);
        out.ywidth = frame->GetRowSize(PLANAR_Y);
        out.uvwidth = frame->GetRowSize(PLANAR_U);
        out.yheight = frame->GetHeight(PLANAR_Y);
        out.uvheight = frame->GetHeight(PLANAR_V);
    } else { // YV24 output
        int cw = decoder.getChromaWidth();
        out.y = bufY;
        out.u = bufU;
        out.v = bufV;
        out.ypitch = (vi.width + 31) & ~31;
        out.uvpitch = (cw + 15) & ~15;
        out.ywidth = vi.width;
        out.uvwidth = cw;
        out.yheight = vi.height;
        out.uvheight = vi.height;
    }

    decoder.Decode(n, &out);

    if (luminanceFlag )
        luminance_filter(out.y, out.ywidth, out.yheight, out.ypitch, luminanceTable);

    if (decoder.upConv == 2) { // convert 4:2:2 (planar) to 4:4:4 (planar)
        env->BitBlt(frame->GetWritePtr(PLANAR_Y), frame->GetPitch(PLANAR_Y),
                    bufY, out.ypitch, vi.width, vi.height);
        conv422to444(out.u, frame->GetWritePtr(PLANAR_U), out.uvpitch,
                     frame->GetPitch(PLANAR_U), vi.width, vi.height);
        conv422to444(out.v, frame->GetWritePtr(PLANAR_V), out.uvpitch,
                     frame->GetPitch(PLANAR_V), vi.width, vi.height);
    }

    if (decoder.info != 0)
        show_info(n, decoder, frame, vi, env);

    return frame;
}


static void set_user_default(FILE* def, char* d2v, int& idct, bool& showq,
                             int& info, int upcnv, bool& i420, int& icc)
{
    char buf[512];
    auto load_str = [&buf](char* var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            std::vector<char> tmp(512, 0);
            sscanf(buf, name, tmp.data());
            tmp.erase(std::remove(tmp.begin(), tmp.end(), '"'), tmp.end());
            strncpy(var, tmp.data(), tmp.size());
        }
    };
    auto load_int = [&buf](int& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            sscanf(buf, name, &var);
        }
    };
    auto load_bool = [&buf](bool& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            char tmp[16];
            sscanf(buf, name, &tmp);
            var = tmp[0] == 't';
        }
    };
    auto load_bint = [&buf](int& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            char tmp[16];
            sscanf(buf, name, &tmp);
            var = tmp[0] == 't' ? 1 : 0;
        }
    };

    while(fgets(buf, 511, def) != 0) {
        load_str(d2v, "d2v=%s", 4);
        load_int(idct, "idct=%d", 5);
        load_bool(showq, "showQ=%s", 6);
        load_int(info, "info=%d", 5);
        load_int(upcnv, "upConv=%d", 7);
        load_bool(i420, "i420=%s", 5);
        load_bint(icc, "iCC=%s", 4);
    }
}


AVSValue __cdecl MPEG2Source::create(AVSValue args, void*, IScriptEnvironment* env)
{
    char d2v[512];
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

    FILE *def = fopen("MPEG2DecPlus.def", "r");
    if (def != nullptr) {
        set_user_default(def, d2v, idct, showQ, info, upConv, i420, iCC);
        fclose(def);
    }

    // check for uninitialised strings
    if (strlen(d2v) >= _MAX_PATH) d2v[0] = 0;
    if (strlen(cpu2) >= 255) cpu2[0] = 0;

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
    if (dec->decoder.Clip_Top    ||
        dec->decoder.Clip_Bottom ||
        dec->decoder.Clip_Left   ||
        dec->decoder.Clip_Right ||
        // This is cheap but it works. The intent is to allow the
        // display size to be different from the encoded size, while
        // not requiring massive revisions to the code. So we detect the
        // difference and crop it off.
        dec->decoder.vertical_size != dec->decoder.Clip_Height ||
        dec->decoder.horizontal_size != dec->decoder.Clip_Width ||
        dec->decoder.vertical_size == 1088)
    {
        int vertical;
        // Special case for 1088 to 1080 as directed by DGIndex.
        if (dec->decoder.vertical_size == 1088 && dec->decoder.D2V_Height == 1080)
            vertical = 1080;
        else
            vertical = dec->decoder.vertical_size;
        AVSValue CropArgs[5] =
        {
            dec,
            dec->decoder.Clip_Left,
            dec->decoder.Clip_Top,
            -(dec->decoder.Clip_Right + (dec->decoder.Clip_Width - dec->decoder.horizontal_size)),
            -(dec->decoder.Clip_Bottom + (dec->decoder.Clip_Height - vertical))
        };

        return env->Invoke("crop", AVSValue(CropArgs,5));
    }

    return dec;
}


const AVS_Linkage* AVS_linkage = nullptr;


extern "C" __declspec(dllexport) const char* __stdcall
AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    const char* msargs =
        "[d2v]s"
        "[cpu]i"
        "[idct]i"
        "[iPP]b"
        "[moderate_h]i"
        "[moderate_v]i"
        "[showQ]b"
        "[fastMC]b"
        "[cpu2]s"
        "[info]i"
        "[upConv]i"
        "[i420]b"
        "[iCC]b";

    env->AddFunction("MPEG2Source", msargs, MPEG2Source::create, nullptr);
    env->AddFunction("LumaYUV","c[lumoff]i[lumgain]f", LumaYUV::create, nullptr);
    env->AddFunction("Deblock", "c[quant]i[aOffset]i[bOffset]i", Deblock::create, nullptr);
   // env->AddFunction("BlindPP", "c[quant]i[cpu]i[cpu2]s[iPP]b[moderate_h]i[moderate_v]i", BlindPP::create, nullptr);

    return VERSION;
}

