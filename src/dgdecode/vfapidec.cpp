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
#include <malloc.h>
//#include "global.h"
#include "MPEG2Decoder.h"
#include "mc.h"
#include "misc.h"
#include "shlwapi.h"

static const int ChromaFormat[4] = {
    0, 6, 8, 12
};

CMPEG2Decoder::CMPEG2Decoder()
{
  VF_File = 0;
  VF_FrameLimit = 0;
  VF_GOPLimit = 0;
  VF_FrameRate = VF_FrameRate_Num = VF_FrameRate_Den = 0;
  prev_frame = 0xfffffffe;
  memset(Rdbfr, 0, sizeof(Rdbfr));
  Rdptr = Rdmax = 0;
  CurrentBfr = NextBfr = BitsLeft = Val = Read = 0;
  Fault_Flag = File_Flag = File_Limit = FO_Flag = IDCT_Flag = SystemStream_Flag = 0;
  Luminance_Flag = false;
  BufferOp = 0;
  memset(intra_quantizer_matrix, 0, sizeof(intra_quantizer_matrix));
  memset(non_intra_quantizer_matrix, 0, sizeof(non_intra_quantizer_matrix));
  memset(chroma_intra_quantizer_matrix, 0, sizeof(chroma_intra_quantizer_matrix));
  memset(chroma_non_intra_quantizer_matrix, 0, sizeof(chroma_non_intra_quantizer_matrix));
  load_intra_quantizer_matrix =
  load_non_intra_quantizer_matrix =
  load_chroma_intra_quantizer_matrix =
  load_chroma_non_intra_quantizer_matrix = 0;
  q_scale_type =
  alternate_scan =
  quantizer_scale = 0;
  upConv = 0;
  iPP = -1;
  iCC = -1;
  moderate_h = 20;
  moderate_v = 40;
  i420 = false;
  pc_scale = 1;
  maxquant = minquant = avgquant = 0;
  u422 = v422 = NULL;
  DirectAccess = NULL;
  FrameList = NULL;
  GOPList = NULL;
  GOPListSize = 0;
}

// Open function modified by Donald Graft as part of fix for dropped frames and random frame access.
int CMPEG2Decoder::Open(const char *path)
{
    char buf[2048], *buf_p;

    Choose_Prediction(this->fastMC);

    char ID[80], PASS[80] = "DGIndexProjectFile16";
    uint32_t i, j, size, code, type, tff, rff, film, ntsc, gop, top, bottom, mapping;
    int repeat_on, repeat_off, repeat_init;
    int vob_id, cell_id;
    __int64 position;
    int HadI;

    CMPEG2Decoder* out = this;

    out->VF_File = fopen(path, "r");
    if (fgets(ID, 79, out->VF_File)==NULL)
        return 1;
    if (strstr(ID, "DGIndexProjectFile") == NULL)
        return 5;
    if (strncmp(ID, PASS, 20))
        return 2;

    fscanf(out->VF_File, "%d\n", &File_Limit);
    i = File_Limit;
    while (i)
    {
        Infilename[File_Limit-i] = (char*)_aligned_malloc(_MAX_PATH, 16);
        fgets(Infilename[File_Limit-i], _MAX_PATH - 1, out->VF_File);
        // Strip newline.
        Infilename[File_Limit-i][strlen(Infilename[File_Limit-i])-1] = 0;
        if (PathIsRelative(Infilename[File_Limit-i]))
        {
            char *p, d2v_stem[_MAX_PATH], open_path[_MAX_PATH];

            if (PathIsRelative(path))
            {
                GetCurrentDirectory(_MAX_PATH, d2v_stem);
                if (*(d2v_stem + strlen(d2v_stem) - 1) != '\\')
                    strcat(d2v_stem, "\\");
                strcat(d2v_stem, path);
            }
            else
            {
                strcpy(d2v_stem, path);
            }
            p = d2v_stem + strlen(d2v_stem);
            while (*p != '\\' && p != d2v_stem) p--;
            if (p != d2v_stem)
            {
                p[1] = 0;
                strcat(d2v_stem, Infilename[File_Limit-i]);
                PathCanonicalize(open_path, d2v_stem);
                if ((Infile[File_Limit-i] = _open(open_path, _O_RDONLY | _O_BINARY))==-1)
                    return 3;
            }
            else
            {
                // This should never happen because the code should guarantee that d2v_stem has a '\' character.
                return 3;
            }
        }
        else
        {
            if ((Infile[File_Limit-i] = _open(Infilename[File_Limit-i], _O_RDONLY | _O_BINARY))==-1)
                return 3;
        }
        i--;
    }

    fscanf(out->VF_File, "\nStream_Type=%d\n", &SystemStream_Flag);
    if (SystemStream_Flag == 2)
    {
        fscanf(out->VF_File, "MPEG2_Transport_PID=%x,%x,%x\n",
               &MPEG2_Transport_VideoPID, &MPEG2_Transport_AudioPID, &MPEG2_Transport_PCRPID);
        fscanf(out->VF_File, "Transport_Packet_Size=%d\n", &TransportPacketSize);
    }
    fscanf(out->VF_File, "MPEG_Type=%d\n", &mpeg_type);
    fscanf(out->VF_File, "iDCT_Algorithm=%d\n", &IDCT_Flag);

    switch (IDCT_Flag) {
    case IDCT_REF:
        if (!refinit)
        {
            refinit = true;
        }
        idctFunc = REF_IDCT;
        break;
    default:
        idctFunc = SSE2MMX_IDCT;
        break;
    }

    File_Flag = 0;
    _lseeki64(Infile[0], 0, SEEK_SET);
    Initialize_Buffer();

    do
    {
        if (Fault_Flag == OUT_OF_BITS) return 4;
        next_start_code();
        code = Get_Bits(32);
    }
    while (code!=SEQUENCE_HEADER_CODE);

    sequence_header();

    mb_width = (horizontal_size+15)/16;
    mb_height = progressive_sequence ? (vertical_size+15)/16 : 2*((vertical_size+31)/32);

    QP = (int*)_aligned_malloc(sizeof(int)*mb_width*mb_height, 32);
    backwardQP = (int*)_aligned_malloc(sizeof(int)*mb_width*mb_height, 32);
    auxQP = (int*)_aligned_malloc(sizeof(int)*mb_width*mb_height, 32);

    Coded_Picture_Width = 16 * mb_width;
    Coded_Picture_Height = 16 * mb_height;

    Chroma_Width = (chroma_format==CHROMA444) ? Coded_Picture_Width : Coded_Picture_Width>>1;
    Chroma_Height = (chroma_format!=CHROMA420) ? Coded_Picture_Height : Coded_Picture_Height>>1;

    block_count = ChromaFormat[chroma_format];

    size_t blsize = sizeof(int16_t) * 64 + 64;
    void* ptr = _aligned_malloc(blsize * 8, 64);
    for (i = 0; i < 8; ++i) {
        size_t px = reinterpret_cast<size_t>(ptr) + i * blsize;
        p_block[i] = reinterpret_cast<int16_t*>(px);
        block[i]   = reinterpret_cast<int16_t*>(px + 64);
    }

    for (i=0; i<3; i++)
    {
        if (i==0)
            size = Coded_Picture_Width * Coded_Picture_Height;
        else
            size = Chroma_Width * Chroma_Height;

        backward_reference_frame[i] = (unsigned char*)_aligned_malloc(2*size+4096, 32);  //>>> cheap safety bump
        forward_reference_frame[i] = (unsigned char*)_aligned_malloc(2*size+4096, 32);
        auxframe[i] = (unsigned char*)_aligned_malloc(2*size+4096, 32);
    }

    fscanf(out->VF_File, "YUVRGB_Scale=%d\n", &pc_scale);

    fscanf(out->VF_File, "Luminance_Filter=%d,%d\n", &i, &j);
    if (i==0 && j==0)
        Luminance_Flag = 0;
    else
    {
        Luminance_Flag = 1;
        InitializeLuminanceFilter(i, j);
    }

    fscanf(out->VF_File, "Clipping=%d,%d,%d,%d\n",
           &Clip_Left, &Clip_Right, &Clip_Top, &Clip_Bottom);
    fscanf(out->VF_File, "Aspect_Ratio=%s\n", Aspect_Ratio);
    fscanf(out->VF_File, "Picture_Size=%dx%d\n", &D2V_Width, &D2V_Height);

    Clip_Width = Coded_Picture_Width;
    Clip_Height = Coded_Picture_Height;

    if (upConv > 0 && chroma_format == 1)
    {
        // these are labeled u422 and v422, but I'm only using them as a temporary
        // storage place for YV12 chroma before upsampling to 4:2:2 so that's why its
        // /4 and not /2  --  (tritical - 1/05/2005)
        int tpitch = (((Chroma_Width+15)>>4)<<4); // mod 16 chroma pitch needed to work with YV12PICTs
        u422 = (unsigned char*)_aligned_malloc((tpitch * Coded_Picture_Height / 2)+2048, 32);
        v422 = (unsigned char*)_aligned_malloc((tpitch * Coded_Picture_Height / 2)+2048, 32);
        auxFrame1 = create_YV12PICT(Coded_Picture_Height,Coded_Picture_Width,chroma_format+1);
        auxFrame2 = create_YV12PICT(Coded_Picture_Height,Coded_Picture_Width,chroma_format+1);
    }
    else
    {
        auxFrame1 = create_YV12PICT(Coded_Picture_Height,Coded_Picture_Width,chroma_format);
        auxFrame2 = create_YV12PICT(Coded_Picture_Height,Coded_Picture_Width,chroma_format);
    }
    saved_active = auxFrame1;
    saved_store = auxFrame2;

    fscanf(out->VF_File, "Field_Operation=%d\n", &FO_Flag);
    fscanf(out->VF_File, "Frame_Rate=%d (%u/%u)\n", &(out->VF_FrameRate), &(out->VF_FrameRate_Num), &(out->VF_FrameRate_Den));
    fscanf(out->VF_File, "Location=%d,%X,%d,%X\n", &i, &j, &i, &j);

    ntsc = film = top = bottom = gop = mapping = repeat_on = repeat_off = repeat_init = 0;
    HaveRFFs = false;

    // These start with sizes of 1000000 (the old fixed default).  If it
    // turns out that that is too small, the memory spaces are enlarged
    // 500000 at a time using realloc.  -- tritical May 16, 2005
    DirectAccess = (char *)calloc(1000000, sizeof(char));
    FrameList = (FRAMELIST*)calloc(1000000, sizeof(FRAMELIST));
    GOPList = (GOPLIST**)calloc(1000000, sizeof(GOPLIST*));
    GOPListSize = 1000000;
    int DirectAccessSize = 1000000;
    int FrameListSize = 1000000;

    fgets(buf, 2047, out->VF_File);
    buf_p = buf;
    while (true)
    {
        sscanf(buf_p, "%x", &type);
        if (type == 0xff)
            break;
        if (type & 0x800)   // New I-frame line start.
        {
            if ((int)gop >= GOPListSize)
            {
                GOPList = (GOPLIST**)realloc(GOPList, (GOPListSize+500000)*sizeof(GOPLIST*));
                for (int k=GOPListSize; k<GOPListSize+500000; ++k) GOPList[k] = NULL;
                GOPListSize += 500000;
            }
            GOPList[gop] = reinterpret_cast<GOPLIST*>(calloc(1, sizeof(GOPLIST)));
            GOPList[gop]->number = film;
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d", &(GOPList[gop]->matrix));
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d", &(GOPList[gop]->file));
            while (*buf_p++ != ' ');
            position = _atoi64(buf_p);
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d", &(GOPList[gop]->I_count));
            while (*buf_p++ != ' ');
            sscanf(buf_p, "%d %d", &vob_id, &cell_id);
            while (*buf_p++ != ' ');
            while (*buf_p++ != ' ');
            GOPList[gop]->position = position;
            GOPList[gop]->closed = (type & 0x400) ? 1 : 0;
            GOPList[gop]->progressive = (type & 0x200) ? 1 : 0;
            gop++;

            sscanf(buf_p, "%x", &type);
        }
        tff = (type & 0x2) >> 1;
        if (FO_Flag == FO_RAW)
            rff = 0;
        else
            rff = type & 0x1;
        if (FO_Flag != FO_FILM && FO_Flag != FO_RAW && rff) HaveRFFs = true;

        if (!film)
        {
            if (tff)
                Field_Order = 1;
            else
                Field_Order = 0;
        }

        if (((int)mapping)+2 >= FrameListSize || ((int)ntsc)+2 >= FrameListSize ||
            ((int)film)+2 >= FrameListSize)
        {
            FrameList = (FRAMELIST*)realloc(FrameList, (FrameListSize+500000)*sizeof(FRAMELIST));
            FrameListSize += 500000;
        }

        if (((int)film)+2 >= DirectAccessSize)
        {
            DirectAccess = (char*)realloc(DirectAccess, (DirectAccessSize+500000)*sizeof(char));
            DirectAccessSize += 500000;
        }

        if (FO_Flag==FO_FILM)
        {
            // Force Film not currently supported for frame repeats.
//          if (GOPList[gop-1]->progressive)
//                  return 6;
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
            if (GOPList[gop-1]->progressive)
            {
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
        if (!(type & 0x80))
            DirectAccess[film] = 0;
        else
            DirectAccess[film] = 1;

        film++;

        // Move to the next flags digit or get the next line.
        while (*buf_p != '\n' && *buf_p != ' ') buf_p++;
        if (*buf_p == '\n')
        {
            fgets(buf, 2047, out->VF_File);
            buf_p = buf;
        }
        else buf_p++;
    }
// dprintf("gop = %d, film = %d, ntsc = %d\n", gop, film, ntsc);
    out->VF_GOPLimit = gop;

    if (FO_Flag==FO_FILM)
    {
        while (FrameList[mapping-1].top >= film)
            mapping--;

        out->VF_FrameLimit = mapping;
    }
    else
    {
        while ((FrameList[ntsc-1].top >= film) || (FrameList[ntsc-1].bottom >= film))
            ntsc--;

        out->VF_FrameLimit = ntsc;
    }

#if 0
    {
        unsigned int i;
        char buf[80];
        for (i = 0; i < ntsc; i++)
        {
            sprintf(buf, "DGDecode: %d: top = %d, bot = %d\n", i, FrameList[i].top, FrameList[i].bottom);
            OutputDebugString(buf);
        }
    }
#endif

    // Count the number of nondecodable frames at the start of the clip
    // (due to an open GOP). This will be used to avoid displaying these
    // bad frames.
    File_Flag = 0;
    _lseeki64(Infile[File_Flag], GOPList[0]->position, SEEK_SET);
    Initialize_Buffer();

    closed_gop = -1;
    BadStartingFrames = 0;
    HadI = 0;
    while (true)
    {
        Get_Hdr();
        if (picture_coding_type == I_TYPE)
            HadI = 1;
        if (picture_structure != FRAME_PICTURE)
        {
            Get_Hdr();
        }
        if (HadI)
            break;
    }
    if (GOPList[0]->closed != 1)
    {
        // Leading B frames are non-decodable.
        while (true)
        {
            Get_Hdr();
            if (picture_coding_type != B_TYPE) break;
            BadStartingFrames++;
            if (picture_structure != FRAME_PICTURE)
                Get_Hdr();
        }
        // Frames pulled down from non-decodable frames are non-decodable.
        if (BadStartingFrames)
        {
            i = 0;
            while (true)
            {
                if ((FrameList[i].top > BadStartingFrames - 1) &&
                    (FrameList[i].bottom > BadStartingFrames - 1))
                    break;
                i++;
            }
            BadStartingFrames = i;
        }
    }
    return 0;
}

// Decode function rewritten by Donald Graft as part of fix for dropped frames and random frame access.
#define SEEK_THRESHOLD 7

void CMPEG2Decoder::Decode(uint32_t frame, YV12PICT *dst)
{
    unsigned int i, f, gop, count, HadI, requested_frame;
    YV12PICT *tmp;

__try
{
    Fault_Flag = 0;
    Second_Field = 0;

//  dprintf("DGDecode: %d: top = %d, bot = %d\n", frame, FrameList[frame]->top, FrameList[frame]->bottom);

    // Skip initial non-decodable frames.
    if (frame < BadStartingFrames) frame = BadStartingFrames;
    requested_frame = frame;

    // Decide whether to use random access or linear play to reach the
    // requested frame. If the seek is just a few frames forward, it will
    // be faster to play linearly to get there. This greatly speeds things up
    // when a decimation like SelectEven() follows this filter.
    if (frame && frame > BadStartingFrames && frame > prev_frame && frame < prev_frame + SEEK_THRESHOLD)
    {
        // Use linear play.
        for (frame = prev_frame + 1; frame <= requested_frame; frame++)
        {

            // If doing force film, we have to skip or repeat a frame as needed.
            // This handles skipping. Repeating is handled below.
            if ((FO_Flag==FO_FILM) &&
                (FrameList[frame].bottom == FrameList[frame-1].bottom + 2) &&
                (FrameList[frame].top == FrameList[frame-1].top + 2))
            {
                if (!Get_Hdr())
                {
                    // Flush the final frame.
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                Decode_Picture(dst);
                if (Fault_Flag == OUT_OF_BITS)
                {
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                if (picture_structure != FRAME_PICTURE)
                {
                    Get_Hdr();
                    Decode_Picture(dst);
                }
            }

            /* When RFFs are present or we are doing FORCE FILM, we may have already decoded
               the frame that we need. So decode the next frame only when we need to. */
            if (std::max(FrameList[frame].top, FrameList[frame].bottom) >
                std::max(FrameList[frame-1].top, FrameList[frame-1].bottom))
            {
                if (!Get_Hdr())
                {
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                Decode_Picture(dst);
                if (Fault_Flag == OUT_OF_BITS)
                {
                    assembleFrame(backward_reference_frame, pf_backward, dst);
                    return;
                }
                if (picture_structure != FRAME_PICTURE)
                {
                    Get_Hdr();
                    Decode_Picture(dst);
                }
                /* If RFFs are present we have to save the decoded frame to
                   be able to pull down from it when we decode the next frame.
                   If we are doing FORCE FILM, then we may need to repeat the
                   frame, so we'll save it for that purpose. */
                if (FO_Flag == FO_FILM || HaveRFFs == true)
                {
                    // Save the current frame without overwriting the last
                    // stored frame.
                    tmp = saved_active;
                    saved_active = saved_store;
                    saved_store = tmp;
                    CopyAll(dst, saved_store);
                }
            }
            else
            {
                // We already decoded the needed frame. Retrieve it.
                CopyAll(saved_store, dst);
            }

            // Perform pulldown if required.
            if (HaveRFFs == true)
            {
                if (FrameList[frame].top > FrameList[frame].bottom)
                {
                    CopyBot(saved_active, dst);
                }
                else if (FrameList[frame].top < FrameList[frame].bottom)
                {
                    CopyTop(saved_active, dst);
                }
            }
        }
        prev_frame = requested_frame;
        return;
    }
    else prev_frame = requested_frame;

    // Have to do random access.
    f = std::max(FrameList[frame].top, FrameList[frame].bottom);

    // Determine the GOP that the requested frame is in.
    for (gop = 0; gop < VF_GOPLimit-1; gop++)
    {
        if (f >= GOPList[gop]->number && f < GOPList[gop+1]->number)
        {
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
    if (gop)
    {
        if (DirectAccess[f] == 0 || (f && DirectAccess[f-1] == 0))
            gop--;
        // This covers the special case where a field of the previous GOP is
        // pulled down into the current GOP.
        else if (f == GOPList[gop]->number && FrameList[frame].top != FrameList[frame].bottom)
            gop--;
    }

    // Calculate how many frames to decode.
    count = f - GOPList[gop]->number;

    // Seek in the stream to the GOP to start decoding with.
    File_Flag = GOPList[gop]->file;
    _lseeki64(Infile[GOPList[gop]->file],
              GOPList[gop]->position,
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
    for (i = 0; i <= GOPList[gop]->I_count; i++)
    {
        HadI = 0;
        while (true)
        {
            if (!Get_Hdr())
            {
                // Something is really messed up.
                return;
            }
            Decode_Picture(dst);
            if (picture_coding_type == I_TYPE)
                HadI = 1;
            if (picture_structure != FRAME_PICTURE)
            {
                Get_Hdr();
                Decode_Picture(dst);
                if (picture_coding_type == I_TYPE)
                    HadI = 1;
                // The reason for this next test is quite technical. For details
                // refer to the file CodeNote1.txt.
                if (Second_Field == 1)
                {
                    Get_Hdr();
                    Decode_Picture(dst);
                }
            }
            if (HadI) break;
        }
    }
    Second_Field = 0;
    if (HaveRFFs == true && count == 0)
    {
        CopyAll(dst, saved_active);
    }
    if (!Get_Hdr())
    {
        // Flush the final frame.
        assembleFrame(backward_reference_frame, pf_backward, dst);
        return;
    }
    Decode_Picture(dst);
    if (Fault_Flag == OUT_OF_BITS)
    {
        assembleFrame(backward_reference_frame, pf_backward, dst);
        return;
    }
    if (picture_structure != FRAME_PICTURE)
    {
        Get_Hdr();
        Decode_Picture(dst);
    }
    if (HaveRFFs == true && count == 1)
    {
        CopyAll(dst, saved_active);
    }
    for (i = 0; i < count; i++)
    {
        if (!Get_Hdr())
        {
            // Flush the final frame.
            assembleFrame(backward_reference_frame, pf_backward, dst);
            return;
        }
        Decode_Picture(dst);
        if (Fault_Flag == OUT_OF_BITS)
        {
            assembleFrame(backward_reference_frame, pf_backward, dst);
            return;
        }
        if (picture_structure != FRAME_PICTURE)
        {
            Get_Hdr();
            Decode_Picture(dst);
        }
        if ((HaveRFFs == true) && (count > 1) && (i == count - 2))
        {
            CopyAll(dst, saved_active);
        }
    }

    if (HaveRFFs == true)
    {
        // Save for transition to non-random mode.
        CopyAll(dst, saved_store);

        // Pull down a field if needed.
        if (FrameList[frame].top > FrameList[frame].bottom)
        {
            CopyBot(saved_active, dst);
        }
        else if (FrameList[frame].top < FrameList[frame].bottom)
        {
            CopyTop(saved_active, dst);
        }
    }
    return;
}
__except(EXCEPTION_EXECUTE_HANDLER)
{
    return;
}
}

void CMPEG2Decoder::Close()
{
    int i;
    CMPEG2Decoder* in = this;

    if (in->VF_File)
    {
        fclose(in->VF_File);
        in->VF_File = NULL;
    }

    while (File_Limit)
    {
        File_Limit--;
        _close(Infile[File_Limit]);
        _aligned_free(Infilename[File_Limit]);
    }

    for (i=0; i<3; i++)
    {
        _aligned_free(backward_reference_frame[i]);
        _aligned_free(forward_reference_frame[i]);
        _aligned_free(auxframe[i]);
    }

    _aligned_free(QP);
    _aligned_free(backwardQP);
    _aligned_free(auxQP);

    if (u422 != NULL) _aligned_free(u422);
    if (v422 != NULL) _aligned_free(v422);

    destroy_YV12PICT(auxFrame1);
    destroy_YV12PICT(auxFrame2);

    _aligned_free(p_block[0]);

    if (GOPList != NULL)
    {
        for (i=0; i<GOPListSize; ++i)
        {
            if (GOPList[i] != NULL) { free(GOPList[i]); GOPList[i] = NULL; }
        }
        free(GOPList);
    }
    GOPListSize = 0;

    if (FrameList != NULL) free(FrameList);

    if (DirectAccess != NULL) free(DirectAccess);
}


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

void CMPEG2Decoder::CopyAll(YV12PICT *src, YV12PICT *dst)
{
    int tChroma_Height = (upConv > 0 && chroma_format == 1) ? Chroma_Height*2 : Chroma_Height;
    fast_copy(src->y,src->ypitch,dst->y,dst->ypitch,dst->ywidth,Coded_Picture_Height);
    fast_copy(src->u,src->uvpitch,dst->u,dst->uvpitch,dst->uvwidth,tChroma_Height);
    fast_copy(src->v,src->uvpitch,dst->v,dst->uvpitch,dst->uvwidth,tChroma_Height);
}

void CMPEG2Decoder::CopyTop(YV12PICT *src, YV12PICT *dst)
{
    int tChroma_Height = (upConv > 0 && chroma_format == 1) ? Chroma_Height*2 : Chroma_Height;
    fast_copy(src->y,src->ypitch*2,dst->y,dst->ypitch*2,dst->ywidth,Coded_Picture_Height>>1);
    fast_copy(src->u,src->uvpitch*2,dst->u,dst->uvpitch*2,dst->uvwidth,tChroma_Height>>1);
    fast_copy(src->v,src->uvpitch*2,dst->v,dst->uvpitch*2,dst->uvwidth,tChroma_Height>>1);
}

void CMPEG2Decoder::CopyBot(YV12PICT *src, YV12PICT *dst)
{
    int tChroma_Height = (upConv > 0 && chroma_format == 1) ? Chroma_Height*2 : Chroma_Height;
    fast_copy(src->y+src->ypitch,src->ypitch*2,dst->y+dst->ypitch,dst->ypitch*2,dst->ywidth,Coded_Picture_Height>>1);
    fast_copy(src->u+src->uvpitch,src->uvpitch*2,dst->u+dst->uvpitch,dst->uvpitch*2,dst->uvwidth,tChroma_Height>>1);
    fast_copy(src->v+src->uvpitch,src->uvpitch*2,dst->v+dst->uvpitch,dst->uvpitch*2,dst->uvwidth,tChroma_Height>>1);
}

void CMPEG2Decoder::CopyTopBot(YV12PICT *odd, YV12PICT *even, YV12PICT *dst)
{
    int tChroma_Height = (upConv > 0 && chroma_format == 1) ? Chroma_Height*2 : Chroma_Height;
    copy_oddeven(odd->y,odd->ypitch*2,even->y+even->ypitch,even->ypitch*2,dst->y,dst->ypitch,dst->ywidth,Coded_Picture_Height>>1);
    copy_oddeven(odd->u,odd->uvpitch*2,even->u+even->uvpitch,even->uvpitch*2,dst->u,dst->uvpitch,dst->uvwidth,tChroma_Height>>1);
    copy_oddeven(odd->v,odd->uvpitch*2,even->v+even->uvpitch,even->uvpitch*2,dst->v,dst->uvpitch,dst->uvwidth,tChroma_Height>>1);
}
