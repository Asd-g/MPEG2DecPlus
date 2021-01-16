/*
 *  Avisynth 2.5 API for MPEG2Dec3
 *
 *  Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *  based of the intial MPEG2Dec Avisytnh API Copyright (C) Mathias Born - May 2001
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

#ifndef MPEG2DECPLUS_AVS_API_H
#define MPEG2DECPLUS_AVS_API_H

#include <cstdint>

#include "avisynth.h"
#include "MPEG2Decoder.h"


class D2VSource: public IClip {
    VideoInfo vi;
    //int _PP_MODE;
    uint8_t *bufY, *bufU, *bufV; // for 4:2:2 input support
    CMPEG2Decoder* decoder;
    bool luminanceFlag;
    uint8_t luminanceTable[256];
    bool has_at_least_v8;

public:
  D2VSource(const char* d2v, int idct, bool showQ, int _info, int _upConv, bool _i420, int iCC, IScriptEnvironment* env);
  ~D2VSource() {}
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env) {};
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  int __stdcall SetCacheHints(int hints, int) { return hints == CACHE_GET_MTMODE ? MT_SERIALIZED : 0; };
  static AVSValue __cdecl create(AVSValue args, void*, IScriptEnvironment* env);
};

#endif
