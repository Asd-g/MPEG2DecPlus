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
#include <stdexcept>
#include <malloc.h>

#include "AVISynthAPI.h"
#include "color_convert.h"
#include "misc.h"

#define VERSION "D2VSource 1.0.0"

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
            if (raw < d.GOPList[static_cast<int64_t>(gop)+1].number)
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
        sprintf_s(msg1,"%s\n"
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
        dprintf(const_cast<char*>("MPEG2DecPlus: %s\n"), VERSION);
        dprintf(const_cast<char*>("MPEG2DecPlus: Source:            %s\n"), d.Infilename[rgop.file].c_str());
        dprintf(const_cast<char*>("MPEG2DecPlus: Frame Rate:        %3.6f fps (%u/%u) %s\n"),
            double(d.VF_FrameRate_Num) / double(d.VF_FrameRate_Den),
            d.VF_FrameRate_Num, d.VF_FrameRate_Den,
            (d.VF_FrameRate == 25000 || d.VF_FrameRate == 50000) ? "(PAL)" : d.VF_FrameRate == 29970 ? "(NTSC)" : "");
        dprintf(const_cast<char*>("MPEG2DecPlus: Field Order:       %s\n"), d.Field_Order == 1 ? "Top Field First" : "Bottom Field First");
        dprintf(const_cast<char*>("MPEG2DecPlus: Picture Size:      %d x %d\n"), d.getLumaWidth(), d.getLumaHeight());
        dprintf(const_cast<char*>("MPEG2DecPlus: Aspect Ratio:      %s\n"), d.Aspect_Ratio);
        dprintf(const_cast<char*>("MPEG2DecPlus: Progressive Seq:   %s\n"), rgop.progressive ? "True" : "False");
        dprintf(const_cast<char*>("MPEG2DecPlus: GOP Number:        %d (%d)  GOP Pos = %I64d\n"), gop, rgop.number, rgop.position);
        dprintf(const_cast<char*>("MPEG2DecPlus: Closed GOP:        %s\n"), rgop.closed ? "True" : "False");
        dprintf(const_cast<char*>("MPEG2DecPlus: Display Frame:     %d\n"), n);
        dprintf(const_cast<char*>("MPEG2DecPlus: Encoded Frame:     %d (top) %d (bottom)\n"), fn.top, fn.bottom);
        dprintf(const_cast<char*>("MPEG2DecPlus: Frame Type:        %c (%d)\n"), pct, raw);
        dprintf(const_cast<char*>("MPEG2DecPlus: Progressive Frame: %s\n"), fraw.pf ? "True" : "False");
        dprintf(const_cast<char*>("MPEG2DecPlus: Colorimetry:       %s (%d)\n"), matrix[rgop.matrix], rgop.matrix);
        dprintf(const_cast<char*>("MPEG2DecPlus: Quants:            %d/%d/%d (avg/min/max)\n"), d.avgquant, d.minquant, d.maxquant);

    } else if (d.info == 3) {
        constexpr uint32_t PROGRESSIVE = 0x00000001;
        constexpr int COLORIMETRY_SHIFT = 2;

        uint32_t hint = 0;
        if (fraw.pf == 1) hint |= PROGRESSIVE;
        hint |= ((rgop.matrix & 7) << COLORIMETRY_SHIFT);
        PutHintingData(frame->GetWritePtr(PLANAR_Y), hint);
    }
}


MPEG2Source::MPEG2Source(const char* d2v, int idct, bool showQ,
                         int _info, int _upConv, bool _i420, int iCC,
                         IScriptEnvironment* env) :
    bufY(nullptr), bufU(nullptr), bufV(nullptr), decoder(nullptr)
{
    if (iCC != -1 && iCC != 0 && iCC != 1)
        env->ThrowError("MPEG2Source: iCC must be set to -1, 0, or 1!");

    if (_upConv != 0 && _upConv != 1 && _upConv != 2)
        env->ThrowError("MPEG2Source: upConv must be set to 0, 1, or 2!");

    if (idct > 7)
        env->ThrowError("MPEG2Source: iDCT invalid (1:MMX,2:SSEMMX,3:SSE2,4:FPU,5:REF,6:Skal's,7:Simple)");

    FILE* f;
    fopen_s(&f, d2v, "r");
    if (f == nullptr)
        env->ThrowError("MPEG2Source: unable to load D2V file \"%s\" ", d2v);

    try {
        decoder = new CMPEG2Decoder(f, d2v, idct, iCC, _upConv, _info, showQ, _i420);
    } catch (std::runtime_error& e) {
        if (f) fclose(f);
        env->ThrowError("MPEG2Source: %s", e.what());
    }

    env->AtExit([](void* p, IScriptEnvironment*) { delete reinterpret_cast<CMPEG2Decoder*>(p); p = nullptr; }, decoder);
    auto& d = *decoder;

    int chroma_format = d.getChromaFormat();

    memset(&vi, 0, sizeof(vi));
    vi.width = d.Clip_Width;
    vi.height = d.Clip_Height;
    if (_upConv == 2) vi.pixel_type = VideoInfo::CS_YV24;
    else if (chroma_format == 2 || _upConv == 1) vi.pixel_type = VideoInfo::CS_YV16;
    else if (d.i420 == true) vi.pixel_type = VideoInfo::CS_I420;
    else vi.pixel_type = VideoInfo::CS_YV12;

    vi.SetFPS(d.VF_FrameRate_Num, d.VF_FrameRate_Den);
    vi.num_frames = static_cast<int>(d.FrameList.size());
    vi.SetFieldBased(false);

    if (_upConv == 2) {
        auto free_buf = [](void* p, IScriptEnvironment* e) {
            _aligned_free(p);
            p = nullptr;
        };
        size_t ysize = ((static_cast<int64_t>(vi.width) + 31) & ~31) * (static_cast<int64_t>(vi.height) + 1);
        size_t uvsize = ((static_cast<int64_t>(d.getChromaWidth()) + 15) & ~15) * (static_cast<int64_t>(vi.height) + 1);
        void* ptr = _aligned_malloc(ysize + 2 * uvsize, 32);
        if (!ptr) {
            env->ThrowError("MPEG2Source:  malloc failure (bufY, bufU, bufV)!");
        }
        env->AtExit(free_buf, ptr);
        bufY = reinterpret_cast<uint8_t*>(ptr);
        bufU = bufY + ysize;
        bufV = bufU + uvsize;
    }

    luminanceFlag = (d.lumGamma != 0 || d.lumOffset != 0);
    if (luminanceFlag) {
        int lg = d.lumGamma;
        int lo = d.lumOffset;
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


bool __stdcall MPEG2Source::GetParity(int)
{
    return decoder->Field_Order == 1;
}


PVideoFrame __stdcall MPEG2Source::GetFrame(int n, IScriptEnvironment* env)
{
    auto& d = *decoder;
    PVideoFrame frame = env->NewVideoFrame(vi, 32);
    YV12PICT out = (d.upConv != 2) ? YV12PICT(frame) :
        YV12PICT(bufY, bufU, bufV, vi.width, d.getChromaWidth(), vi.height);

    d.Decode(n, out);

    if (luminanceFlag )
        luminance_filter(out.y, out.ywidth, out.yheight, out.ypitch, luminanceTable);

    if (d.upConv == 2) { // convert 4:2:2 (planar) to 4:4:4 (planar)
        env->BitBlt(frame->GetWritePtr(PLANAR_Y), frame->GetPitch(PLANAR_Y),
                    bufY, out.ypitch, vi.width, vi.height);
        conv422to444(out.u, frame->GetWritePtr(PLANAR_U), out.uvpitch,
                     frame->GetPitch(PLANAR_U), vi.width, vi.height);
        conv422to444(out.v, frame->GetWritePtr(PLANAR_V), out.uvpitch,
                     frame->GetPitch(PLANAR_V), vi.width, vi.height);
    }

    if (d.info != 0)
        show_info(n, d, frame, vi, env);

    return frame;
}


static void set_user_default(FILE* def, char* d2v, int& idct, bool& showq,
                             int& info, int upcnv, bool& i420, int& icc)
{
    char buf[512];
    auto load_str = [&buf](char* var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            std::vector<char> tmp(512, 0);
            sscanf_s(buf, name, tmp.data());
            tmp.erase(std::remove(tmp.begin(), tmp.end(), '"'), tmp.end());
            strncpy_s(var, tmp.size() + 1, tmp.data(), tmp.size());
        }
    };
    auto load_int = [&buf](int& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            sscanf_s(buf, name, &var);
        }
    };
    auto load_bool = [&buf](bool& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            char tmp[16];
            sscanf_s(buf, name, &tmp);
            var = tmp[0] == 't';
        }
    };
    auto load_bint = [&buf](int& var, const char* name, int len) {
        if (strncmp(buf, name, len) == 0) {
            char tmp[16];
            sscanf_s(buf, name, &tmp);
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
    int idct = -1;

    int iCC = args[6].IsBool() ? (args[6].AsBool() ? 1 : 0) : -1;

    bool showQ = false;
    int info = 0;
    int upConv = 0;
    bool i420 = false;

    FILE* def;
    fopen_s(&def, "MPEG2DecPlus.def", "r");
    if (def != nullptr) {
        set_user_default(def, d2v, idct, showQ, info, upConv, i420, iCC);
        fclose(def);
    }

    // check for uninitialised strings
    if (strlen(d2v) >= _MAX_PATH) d2v[0] = 0;

    MPEG2Source *dec = new MPEG2Source( args[0].AsString(d2v),
                                        args[1].AsInt(idct),
                                        args[2].AsBool(showQ),
                                        args[3].AsInt(info),
                                        args[4].AsInt(upConv),
                                        args[5].AsBool(i420),
                                        iCC,
                                        env );
    // Only bother invoking crop if we have to.
    auto& d = *dec->decoder;
    if (d.Clip_Top    || d.Clip_Bottom || d.Clip_Left   || d.Clip_Right ||
        // This is cheap but it works. The intent is to allow the
        // display size to be different from the encoded size, while
        // not requiring massive revisions to the code. So we detect the
        // difference and crop it off.
        d.vertical_size != d.Clip_Height || d.horizontal_size != d.Clip_Width ||
        d.vertical_size == 1088)
    {
        int vertical;
        // Special case for 1088 to 1080 as directed by DGIndex.
        if (d.vertical_size == 1088 && d.D2V_Height == 1080)
            vertical = 1080;
        else
            vertical = d.vertical_size;
        AVSValue CropArgs[] = {
            dec,
            d.Clip_Left,
            d.Clip_Top,
            -(d.Clip_Right + (d.Clip_Width - d.horizontal_size)),
            -(d.Clip_Bottom + (d.Clip_Height - vertical)),
            true
        };

        return env->Invoke("crop", AVSValue(CropArgs, 6));
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
        "[idct]i"
        "[showQ]b"
        "[info]i"
        "[upConv]i"
        "[i420]b"
        "[iCC]b";

    env->AddFunction("D2VSource", msargs, MPEG2Source::create, nullptr);

    return VERSION;
}

