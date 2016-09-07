/*
 *  Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *  Ported to C++ by Mathias Born - May 2001
 *
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
  lots of code modified for YV12 / MPEG2Dec3 - MarcFD
*/

#include <algorithm>
#include <stdexcept>
#include <malloc.h>
#include "MPEG2Decoder.h"
#include "mc.h"
#include "misc.h"
#include "idct.h"
#include "shlwapi.h"


static const int ChromaFormat[4] = {
    0, 6, 8, 12
};


static void validate(bool cond, const char* msg)
{
    if (cond) throw std::runtime_error(msg);
}

// Open function modified by Donald Graft as part of fix for dropped frames and random frame access.
// change function to constructor - chikuzen.
CMPEG2Decoder::CMPEG2Decoder(FILE* d2vf, const char* path, int _idct, int icc,
                             int upconv, int _info, bool showq, bool _i420) :
    VF_FrameRate(0),
    VF_FrameRate_Num(0),
    VF_FrameRate_Den(0),
    prev_frame(0xfffffffe),
    Rdmax(nullptr),
    CurrentBfr(0),
    NextBfr(0),
    BitsLeft(0),
    Val(0),
    Read(0),
    Fault_Flag(0),
    File_Flag(0),
    FO_Flag(0),
    SystemStream_Flag(0),
    load_intra_quantizer_matrix(0),
    load_non_intra_quantizer_matrix(0),
    load_chroma_intra_quantizer_matrix(0),
    load_chroma_non_intra_quantizer_matrix(0),
    q_scale_type(0),
    alternate_scan(0),
    quantizer_scale(0),
    auxFrame1(nullptr),
    auxFrame2(nullptr),
    upConv(upconv),
    showQ(showq),
    info(_info),
    iCC(icc),
    i420(_i420)
{
    memset(intra_quantizer_matrix, 0, sizeof(intra_quantizer_matrix));
    memset(non_intra_quantizer_matrix, 0, sizeof(non_intra_quantizer_matrix));
    memset(chroma_intra_quantizer_matrix, 0, sizeof(chroma_intra_quantizer_matrix));
    memset(chroma_non_intra_quantizer_matrix, 0, sizeof(chroma_non_intra_quantizer_matrix));

    ReadBuffer.resize(BUFFER_SIZE, 0);
    Rdbfr = ReadBuffer.data();

    Choose_Prediction();

    char buf[2048];
    uint32_t i, j;

    validate(fgets(buf, 2047, d2vf) == nullptr, "Invalid D2V file, it's empty!");

    validate(strstr(buf, "DGIndexProjectFile") == nullptr,
             "The input file is not a D2V project file.");

    validate(strncmp(buf, "DGIndexProjectFile16", 20) != 0,
             "DGIndex/MPEG2DecPlus mismatch. You are picking up\n"
             "a version of MPEG2DecPlus, possibly from your plugins directory,\n"
             "that does not match the version of DGIndex used to make the D2V\n"
             "file. Search your hard disk for all copies of MPEG2DecPlus.dll\n"
             "and delete or rename all of them except for the one that\n"
             "has the same version number as the DGIndex.exe that was used\n"
             "to make the D2V file.");

    create_file_lists(d2vf, path, buf);

    fscanf(d2vf, "\nStream_Type=%d\n", &SystemStream_Flag);
    if (SystemStream_Flag == 2) {
        fscanf(d2vf, "MPEG2_Transport_PID=%x,%x,%x\n",
               &MPEG2_Transport_VideoPID, &MPEG2_Transport_AudioPID, &MPEG2_Transport_PCRPID);
        fscanf(d2vf, "Transport_Packet_Size=%d\n", &TransportPacketSize);
    }
    fscanf(d2vf, "MPEG_Type=%d\n", &mpeg_type);

    int idct;
    fscanf(d2vf, "iDCT_Algorithm=%d\n", &idct);
    if (_idct > 0) {
        idct = _idct;
    }
    setIDCT(idct);

    fscanf(d2vf, "YUVRGB_Scale=%d\n", &i);

    fscanf(d2vf, "Luminance_Filter=%d,%d\n", &lumGamma, &lumOffset);

    fscanf(d2vf, "Clipping=%d,%d,%d,%d\n",
        &Clip_Left, &Clip_Right, &Clip_Top, &Clip_Bottom);
    fscanf(d2vf, "Aspect_Ratio=%s\n", Aspect_Ratio);
    fscanf(d2vf, "Picture_Size=%dx%d\n", &D2V_Width, &D2V_Height);
    fscanf(d2vf, "Field_Operation=%d\n", &FO_Flag);
    fscanf(d2vf, "Frame_Rate=%d (%u/%u)\n", &(VF_FrameRate), &(VF_FrameRate_Num), &(VF_FrameRate_Den));
    fscanf(d2vf, "Location=%d,%X,%d,%X\n", &i, &j, &i, &j);

    create_gop_and_frame_lists(d2vf, buf);
    fclose(d2vf);
    d2vf = nullptr;

    set_clip_properties();

    allocate_buffers();

    search_bad_starting();
}


void CMPEG2Decoder::setIDCT(int idct)
{
    if (idct == IDCT_REF) {
        prefetchTables = prefetch_ref;
        idctFunction = idct_ref_sse3;
    } else if (idct == IDCT_LLM_FLOAT) {
        if (has_avx2()) {
            prefetchTables = prefetch_llm_float_avx2;
            idctFunction = idct_llm_float_avx2;
        } else {
            prefetchTables = prefetch_llm_float_sse2;
            idctFunction = idct_llm_float_sse2;
        }
    } else {
        prefetchTables = prefetch_ap922;
        idctFunction = idct_ap922_sse2;
    }
}


void CMPEG2Decoder::set_clip_properties()
{
    File_Flag = 0;
    _lseeki64(Infile[0], 0, SEEK_SET);
    Initialize_Buffer();

    do {
        validate(Fault_Flag == OUT_OF_BITS,
                 "Could not find a sequence header in the input stream.");
        Next_Start_Code();
    } while (Get_Bits(32) != SEQUENCE_HEADER_CODE);

    Sequence_Header();
    validate(chroma_format == CHROMA444,
             "currently unsupported input color format (4:4:4)");

    mb_width = (horizontal_size + 15) / 16;
    mb_height = progressive_sequence ? (vertical_size + 15) / 16 : 2 * ((vertical_size + 31) / 32);

    Coded_Picture_Width = 16 * mb_width;
    Coded_Picture_Height = 16 * mb_height;

    Chroma_Width = (chroma_format == CHROMA444) ? Coded_Picture_Width : Coded_Picture_Width >> 1;
    Chroma_Height = (chroma_format != CHROMA420) ? Coded_Picture_Height : Coded_Picture_Height >> 1;

    block_count = ChromaFormat[chroma_format];

    Clip_Width = Coded_Picture_Width;
    Clip_Height = Coded_Picture_Height;
}


void CMPEG2Decoder::create_file_lists(FILE* d2vf, const char* path, char* buf)
{
    int file_limit;
    fscanf(d2vf, "%d\n", &file_limit);
    Infile.reserve(file_limit);
    Infilename.reserve(file_limit);

    while (file_limit-- > 0) {
        fgets(buf, 2047, d2vf);
        auto temp = std::string(buf);
        temp.pop_back(); // Strip newline.

        if (PathIsRelativeA(temp.c_str())) {
            std::string d2v_stem;

            if (PathIsRelativeA(path)) {
                GetCurrentDirectoryA(_MAX_PATH, buf);
                d2v_stem += buf;
                if (d2v_stem.back() != '\\') d2v_stem.push_back('\\');
            }
            d2v_stem += path;

            while (d2v_stem.back() != '\\') d2v_stem.pop_back();
            d2v_stem += temp;
            PathCanonicalizeA(buf, d2v_stem.c_str());
            int in = _open(buf, _O_RDONLY | _O_BINARY);
            validate(in == -1, "Could not open one of the input files.");
            Infile.emplace_back(in);
            Infilename.emplace_back(buf);
        } else {
            int in = _open(temp.c_str(), _O_RDONLY | _O_BINARY);
            validate(in == -1, "Could not open one of the input files.");
            Infile.emplace_back(in);
            Infilename.emplace_back(temp);
        }
    }
}


void CMPEG2Decoder::create_gop_and_frame_lists(FILE* d2vf, char* buf)
{
    uint32_t type, tff, rff, film, ntsc, top, bottom, mapping;
    int repeat_on, repeat_off, repeat_init;
    ntsc = film = top = bottom = mapping = repeat_on = repeat_off = repeat_init = 0;
    HaveRFFs = false;
    
    // These start with sizes of 1000000 (the old fixed default).  If it
    // turns out that that is too small, the memory spaces are enlarged
    // 500000 at a time using realloc.  -- tritical May 16, 2005
    DirectAccess.reserve(1000000);
    FrameList.resize(1000000);
    GOPList.reserve(200000);
    
    fgets(buf, 2047, d2vf);
    char* buf_p = buf;
    
    while (true) {
        sscanf(buf_p, "%x", &type);
        if (type == 0xff)
            break;
        if (type & 0x800) {  // New I-frame line start.
            int matrix, file, ic, vob_id, cell_id;
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d", &matrix);
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d", &file);
            while (*buf_p++ != ' ');
            int64_t position = _atoi64(buf_p);
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d", &ic);
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d %d", &vob_id, &cell_id);
            while (*buf_p++ != ' ');
            while (*buf_p++ != ' ');
            GOPList.emplace_back(film, matrix, file, position, ic, type);
    
            sscanf(buf_p, "%x", &type);
        }
        tff = (type & 0x2) >> 1;
        if (FO_Flag == FO_RAW)
            rff = 0;
        else
            rff = type & 0x1;
        if (FO_Flag != FO_FILM && FO_Flag != FO_RAW && rff) HaveRFFs = true;
    
        if (!film) {
            if (tff)
                Field_Order = 1;
            else
                Field_Order = 0;
        }
    
        size_t listsize = FrameList.size() - 2;
        if (mapping >= listsize || ntsc >= listsize || film >= listsize) {
            FrameList.resize(listsize + 500002);
        }

        if (FO_Flag == FO_FILM) {
            if (rff)
                repeat_on++;
            else
                repeat_off++;

            if (repeat_init)
            {
                if (repeat_off-repeat_on == 5)
                {
                    repeat_on = repeat_off = 0;
                }
                else
                {
                    FrameList[mapping].top = FrameList[mapping].bottom = film;
                    mapping ++;
                }
    
                if (repeat_on-repeat_off == 5)
                {
                    repeat_on = repeat_off = 0;
                    FrameList[mapping].top = FrameList[mapping].bottom = film;
                    mapping ++;
                }
            }
            else
            {
                if (repeat_off-repeat_on == 3)
                {
                    repeat_on = repeat_off = 0;
                    repeat_init = 1;
                }
                else
                {
                    FrameList[mapping].top = FrameList[mapping].bottom = film;
                    mapping ++;
                }

                if (repeat_on-repeat_off == 3)
                {
                    repeat_on = repeat_off = 0;
                    repeat_init = 1;
                    FrameList[mapping].top = FrameList[mapping].bottom = film;
                    mapping ++;
                }
            }
        }
        else
        {
            if (GOPList.back().progressive) {
                FrameList[ntsc].top = film;
                FrameList[ntsc].bottom = film;
                ntsc++;
                if (FO_Flag != FO_RAW && rff)
                {
                    FrameList[ntsc].top = film;
                    FrameList[ntsc].bottom = film;
                    ntsc++;
                    if (tff)
                    {
                        FrameList[ntsc].top = film;
                        FrameList[ntsc].bottom = film;
                        ntsc++;
                    }
                }
            }
            else
            {
                if (top)
                {
                    FrameList[ntsc].bottom = film;
                    ntsc++;
                    FrameList[ntsc].top = film;
                }
                else if (bottom)
                {
                    FrameList[ntsc].top = film;
                    ntsc++;
                    FrameList[ntsc].bottom = film;
                }
                else
                {
                    FrameList[ntsc].top = film;
                    FrameList[ntsc].bottom = film;
                    ntsc++;
                }

                if (rff)
                {
                    if (tff)
                    {
                        FrameList[ntsc].top = film;
                        top = 1;
                    }
                    else
                    {
                        FrameList[ntsc].bottom = film;
                        bottom = 1;
                    }
    
                    if (top && bottom)
                    {
                        top = bottom = 0;
                        ntsc++;
                    }
                }
            }
        }

        FrameList[film].pct = (unsigned char)((type & 0x30) >> 4);
        FrameList[film].pf = (unsigned char)((type & 0x40) >> 6);

        // Remember if this encoded frame requires the previous GOP to be decoded.
        // The previous GOP is required if DVD2AVI has marked it as such.
        DirectAccess.emplace_back(!!(type & 0x80));

        film++;

        // Move to the next flags digit or get the next line.
        while (*buf_p != '\n' && *buf_p != ' ') buf_p++;
        if (*buf_p == '\n')
        {
            fgets(buf, 2047, d2vf);
            buf_p = buf;
        }
        else buf_p++;
    }

    GOPList.shrink_to_fit();
    DirectAccess.shrink_to_fit();

    if (FO_Flag==FO_FILM) {
        while (FrameList[mapping-1].top >= film)
            --mapping;
        FrameList.resize(mapping);
    } else {
        while ((FrameList[ntsc-1].top >= film) || (FrameList[ntsc-1].bottom >= film))
            --ntsc;
        FrameList.resize(ntsc);
    }
    FrameList.shrink_to_fit();
}


void CMPEG2Decoder::destroy()
{
    while (Infile.size() > 0) {
        _close(Infile.back());
        Infile.pop_back();
    }

    for (int i = 0; i < 3; ++i) {
        _aligned_free(backward_reference_frame[i]);
        _aligned_free(forward_reference_frame[i]);
        _aligned_free(auxframe[i]);
        backward_reference_frame[i] = forward_reference_frame[i] = auxframe[i] = nullptr;
    }

    _aligned_free(QP);
    _aligned_free(backwardQP);
    _aligned_free(auxQP);
    QP = backwardQP = auxQP = nullptr;

    if (auxFrame1) delete auxFrame1;
    if (auxFrame2) delete auxFrame2;
    auxFrame1 = auxFrame2 = nullptr;

    _aligned_free(p_block[0]);
    p_block[0] = nullptr;
}


void CMPEG2Decoder::allocate_buffers()
{
    QP = (int*)_aligned_malloc(sizeof(int)*mb_width*mb_height, 32);
    backwardQP = (int*)_aligned_malloc(sizeof(int)*mb_width*mb_height, 32);
    auxQP = (int*)_aligned_malloc(sizeof(int)*mb_width*mb_height, 32);
    if (!QP || !backwardQP || !auxQP) {
        destroy();
        throw std::runtime_error("failed to allocate QP buffers.");
    }

    size_t blsize = sizeof(int16_t) * 64 + 64;
    void* ptr = _aligned_malloc(blsize * 8, 64);
    if (!ptr) {
        destroy();
        throw std::runtime_error("failed to allocate macroblock buffers");
    }
    for (int i = 0; i < 8; ++i) {
        size_t px = reinterpret_cast<size_t>(ptr) + i * blsize;
        p_block[i] = reinterpret_cast<int16_t*>(px);
        block[i] = reinterpret_cast<int16_t*>(px + 64);
    }

    size_t size;
    for (int i = 0; i < 3; i++) {
        if (i == 0)
            size = Coded_Picture_Width * Coded_Picture_Height;
        else
            size = Chroma_Width * Chroma_Height;

        backward_reference_frame[i] = (unsigned char*)_aligned_malloc(2 * size + 4096, 32);  //>>> cheap safety bump
        forward_reference_frame[i] = (unsigned char*)_aligned_malloc(2 * size + 4096, 32);
        auxframe[i] = (unsigned char*)_aligned_malloc(2 * size + 4096, 32);
        if (!backward_reference_frame[i] || !forward_reference_frame[i] || !auxframe[i]) {
            destroy();
            throw std::runtime_error("failed to allocate reference frame buffers.");
        }
    }

    int cf = chroma_format;
    if (upConv > 0 && chroma_format == 1) ++cf;
    try {
        auxFrame1 = new YV12PICT(Coded_Picture_Height, Coded_Picture_Width, cf);
        auxFrame2 = new YV12PICT(Coded_Picture_Height, Coded_Picture_Width, cf);
    } catch (std::runtime_error& e) {
        destroy();
        throw e;
    }
    saved_active = auxFrame1;
    saved_store = auxFrame2;
}


void CMPEG2Decoder::search_bad_starting()
{
    // Count the number of nondecodable frames at the start of the clip
    // (due to an open GOP). This will be used to avoid displaying these
    // bad frames.
    File_Flag = 0;
    _lseeki64(Infile[File_Flag], GOPList[0].position, SEEK_SET);
    Initialize_Buffer();

    closed_gop = -1;
    BadStartingFrames = 0;
    for (int HadI = 0;;) {
        Get_Hdr();
        if (picture_coding_type == I_TYPE)
            HadI = 1;
        if (picture_structure != FRAME_PICTURE) {
            Get_Hdr();
        }
        if (HadI)
            break;
    }
    if (GOPList[0].closed != 1) {
        // Leading B frames are non-decodable.
        while (true) {
            Get_Hdr();
            if (picture_coding_type != B_TYPE) break;
            BadStartingFrames++;
            if (picture_structure != FRAME_PICTURE)
                Get_Hdr();
        }
        // Frames pulled down from non-decodable frames are non-decodable.
        if (BadStartingFrames) {
            int i;
            for (i = 0;; ++i) {
                if (FrameList[i].top > BadStartingFrames - 1
                    && FrameList[i].bottom > BadStartingFrames - 1) {
                    break;
                }
            }
            BadStartingFrames = i;
        }
    }
}

// Decode function rewritten by Donald Graft as part of fix for dropped frames and random frame access.
#define SEEK_THRESHOLD 7

void CMPEG2Decoder::Decode(uint32_t frame, YV12PICT& dst)
{
__try {
    Fault_Flag = 0;
    Second_Field = 0;

    // Skip initial non-decodable frames.
    frame = std::max(frame, BadStartingFrames);

    uint32_t requested_frame = frame;

    // Decide whether to use random access or linear play to reach the
    // requested frame. If the seek is just a few frames forward, it will
    // be faster to play linearly to get there. This greatly speeds things up
    // when a decimation like SelectEven() follows this filter.
    if (frame && frame > BadStartingFrames && frame > prev_frame
        && frame < prev_frame + SEEK_THRESHOLD) {
        // Use linear play.
        for (frame = prev_frame + 1; frame <= requested_frame; ++frame) {

            // If doing force film, we have to skip or repeat a frame as needed.
            // This handles skipping. Repeating is handled below.
            if ((FO_Flag==FO_FILM)
                && (FrameList[frame].bottom == FrameList[frame-1].bottom + 2)
                && (FrameList[frame].top == FrameList[frame-1].top + 2)) {
                if (!Get_Hdr()) {
                    // Flush the final frame.
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                Decode_Picture(dst);
                if (Fault_Flag == OUT_OF_BITS) {
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                if (picture_structure != FRAME_PICTURE) {
                    Get_Hdr();
                    Decode_Picture(dst);
                }
            }

            /* When RFFs are present or we are doing FORCE FILM, we may have already decoded
               the frame that we need. So decode the next frame only when we need to. */
            if (std::max(FrameList[frame].top, FrameList[frame].bottom) >
                std::max(FrameList[frame-1].top, FrameList[frame-1].bottom)) {
                if (!Get_Hdr()) {
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                Decode_Picture(dst);
                if (Fault_Flag == OUT_OF_BITS) {
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                if (picture_structure != FRAME_PICTURE) {
                    Get_Hdr();
                    Decode_Picture(dst);
                }
                /* If RFFs are present we have to save the decoded frame to
                   be able to pull down from it when we decode the next frame.
                   If we are doing FORCE FILM, then we may need to repeat the
                   frame, so we'll save it for that purpose. */
                if (FO_Flag == FO_FILM || HaveRFFs) {
                    // Save the current frame without overwriting the last
                    // stored frame.
                    YV12PICT* tmp = saved_active;
                    saved_active = saved_store;
                    saved_store = tmp;
                    copy_all(dst, *saved_store);
                }
            } else {
                // We already decoded the needed frame. Retrieve it.
                copy_all(*saved_store, dst);
            }

            // Perform pulldown if required.
            if (HaveRFFs) {
                if (FrameList[frame].top > FrameList[frame].bottom) {
                    copy_bottom(*saved_active, dst);
                } else if (FrameList[frame].top < FrameList[frame].bottom) {
                    copy_top(*saved_active, dst);
                }
            }
        }
        prev_frame = requested_frame;
        return;
    }

    prev_frame = requested_frame;
    // Have to do random access.
    uint32_t f = std::max(FrameList[frame].top, FrameList[frame].bottom);

    // Determine the GOP that the requested frame is in.
    uint32_t gop;
    for (gop = 0; gop < GOPList.size() - 1; ++gop) {
        if (f >= GOPList[gop].number && f < GOPList[gop + 1].number) {
            break;
        }
    }

    // Back off by one GOP if required. This ensures enough frames will
    // be decoded that the requested frame is guaranteed to be decodable.
    // The direct flag is written by DGIndex, who knows which frames
    // require the previous GOP to be decoded. We also test whether
    // the previous encoded frame requires it, because that one might be pulled
    // down for this frame. This can be improved by actually testing if the
    // previous frame will be pulled down.
    // Obviously we can't decrement gop if it is 0.
    if (gop) {
        if (DirectAccess[f] == 0 || (f && DirectAccess[f - 1] == 0))
            gop--;
        // This covers the special case where a field of the previous GOP is
        // pulled down into the current GOP.
        else if (f == GOPList[gop].number && FrameList[frame].top != FrameList[frame].bottom)
            gop--;
    }

    // Calculate how many frames to decode.
    uint32_t count = f - GOPList[gop].number;

    // Seek in the stream to the GOP to start decoding with.
    File_Flag = GOPList[gop].file;
    _lseeki64(Infile[GOPList[gop].file],
              GOPList[gop].position,
              SEEK_SET);
    Initialize_Buffer();

    // Start decoding. Stop when the requested frame is decoded.
    // First decode the required I picture of the D2V index line.
    // An indexed unit (especially a pack) can contain multiple I frames.
    // If we did nothing we would have multiple D2V lines with the same position
    // and the navigation code in DGDecode would be confused. We detect this in DGIndex
    // by seeing how many previous lines we have written with the same position.
    // We store that number in the D2V file and DGDecode uses that as the
    // number of I frames to skip before settling on the required one.
    // So, in fact, we are indexing position plus number of I frames to skip.
    for (uint32_t i = 0; i <= GOPList[gop].I_count; ++i) {
        bool HadI = false;
        while (true) {
            if (!Get_Hdr()) {
                // Something is really messed up.
                return;
            }
            Decode_Picture(dst);
            if (picture_coding_type == I_TYPE)
                HadI = true;
            if (picture_structure != FRAME_PICTURE) {
                Get_Hdr();
                Decode_Picture(dst);
                if (picture_coding_type == I_TYPE)
                    HadI = true;
                // The reason for this next test is quite technical. For details
                // refer to the file CodeNote1.txt.
                if (Second_Field == 1) {
                    Get_Hdr();
                    Decode_Picture(dst);
                }
            }
            if (HadI) break;
        }
    }
    Second_Field = 0;
    if (HaveRFFs == true && count == 0) {
        copy_all(dst, *saved_active);
    }
    if (!Get_Hdr()) {
        // Flush the final frame.
        assembleFrame(backward_reference_frame, pf_backward, dst);
        return;
    }
    Decode_Picture(dst);
    if (Fault_Flag == OUT_OF_BITS) {
        assembleFrame(backward_reference_frame, pf_backward, dst);
        return;
    }
    if (picture_structure != FRAME_PICTURE) {
        Get_Hdr();
        Decode_Picture(dst);
    }
    if (HaveRFFs && count == 1) {
        copy_all(dst, *saved_active);
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (!Get_Hdr()) {
            // Flush the final frame.
            assembleFrame(backward_reference_frame, pf_backward, dst);
            return;
        }
        Decode_Picture(dst);
        if (Fault_Flag == OUT_OF_BITS) {
            assembleFrame(backward_reference_frame, pf_backward, dst);
            return;
        }
        if (picture_structure != FRAME_PICTURE) {
            Get_Hdr();
            Decode_Picture(dst);
        }
        if ((HaveRFFs == true) && (count > 1) && (i == count - 2)) {
            copy_all(dst, *saved_active);
        }
    }

    if (HaveRFFs) {
        // Save for transition to non-random mode.
        copy_all(dst, *saved_store);

        // Pull down a field if needed.
        if (FrameList[frame].top > FrameList[frame].bottom) {
            copy_bottom(*saved_active, dst);
        } else if (FrameList[frame].top < FrameList[frame].bottom) {
            copy_top(*saved_active, dst);
        }
    }
    return;
} __except(EXCEPTION_EXECUTE_HANDLER) {
    return;
}
}


__forceinline void CMPEG2Decoder::copy_all(YV12PICT& src, YV12PICT& dst)
{
    int t = (upConv > 0 && chroma_format == 1) ? Chroma_Height*2 : Chroma_Height;
    fast_copy(src.y, src.ypitch, dst.y, dst.ypitch, dst.ywidth,Coded_Picture_Height);
    fast_copy(src.u, src.uvpitch, dst.u, dst.uvpitch, dst.uvwidth, t);
    fast_copy(src.v, src.uvpitch, dst.v, dst.uvpitch, dst.uvwidth, t);
}

__forceinline void CMPEG2Decoder::copy_top(YV12PICT& src, YV12PICT& dst)
{
    int t = (upConv > 0 && chroma_format == 1) ? Chroma_Height : Chroma_Height / 2;
    fast_copy(src.y, src.ypitch * 2, dst.y, dst.ypitch * 2, dst.ywidth, Coded_Picture_Height / 2);
    fast_copy(src.u, src.uvpitch * 2, dst.u, dst.uvpitch * 2, dst.uvwidth, t);
    fast_copy(src.v, src.uvpitch * 2, dst.v, dst.uvpitch * 2, dst.uvwidth, t);
}

__forceinline void CMPEG2Decoder::copy_bottom(YV12PICT& src, YV12PICT& dst)
{
    int t = (upConv > 0 && chroma_format == 1) ? Chroma_Height : Chroma_Height / 2;
    fast_copy(src.y + src.ypitch, src.ypitch * 2, dst.y + dst.ypitch, dst.ypitch * 2, dst.ywidth, Coded_Picture_Height / 2);
    fast_copy(src.u + src.uvpitch, src.uvpitch * 2, dst.u + dst.uvpitch, dst.uvpitch * 2, dst.uvwidth, t);
    fast_copy(src.v + src.uvpitch, src.uvpitch * 2, dst.v + dst.uvpitch, dst.uvpitch * 2, dst.uvwidth, t);
}

#if 0
static inline void
copy_oddeven(const uint8_t* odd, const size_t opitch, const uint8_t* even,
    const size_t epitch, uint8_t* dst, const size_t dpitch,
    const size_t width, size_t height) noexcept
{
    do {
        memcpy(dst, odd, width);
        dst += dpitch;
        memcpy(dst, even, width);
        dst += dpitch;
        odd += opitch;
        even += epitch;
    } while (--height != 0);
}

void CMPEG2Decoder::CopyTopBot(YV12PICT *odd, YV12PICT *even, YV12PICT *dst)
{
    int tChroma_Height = (upConv > 0 && chroma_format == 1) ? Chroma_Height*2 : Chroma_Height;
    copy_oddeven(odd->y,odd->ypitch*2,even->y+even->ypitch,even->ypitch*2,dst->y,dst->ypitch,dst->ywidth,Coded_Picture_Height>>1);
    copy_oddeven(odd->u,odd->uvpitch*2,even->u+even->uvpitch,even->uvpitch*2,dst->u,dst->uvpitch,dst->uvwidth,tChroma_Height>>1);
    copy_oddeven(odd->v,odd->uvpitch*2,even->v+even->uvpitch,even->uvpitch*2,dst->v,dst->uvpitch,dst->uvwidth,tChroma_Height>>1);
}
#endif
