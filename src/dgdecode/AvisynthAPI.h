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

#include <cstdint>
#include <avisynth.h>
#include <avs/win.h>
#include "global.h"


class MPEG2Source: public IClip {
protected:
  VideoInfo vi;
  int ovr_idct;
  int _PP_MODE;
  YV12PICT *out;
  uint8_t *bufY, *bufU, *bufV; // for 4:2:2 input support
  uint8_t *u444, *v444;        // for RGB24 output
  CMPEG2Decoder m_decoder;

  void override(int ovr_idct);

public:
  MPEG2Source(const char* d2v, int cpu, int idct, int iPP, int moderate_h, int moderate_v, bool showQ, bool fastMC, const char* _cpu2, int _info, int _upConv, bool _i420, int iCC, IScriptEnvironment* env);
  ~MPEG2Source();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {};
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  int __stdcall SetCacheHints(int hints, int) { return hints == CACHE_GET_MTMODE ? MT_SERIALIZED : 0; };
  static AVSValue __cdecl create(AVSValue args, void*, IScriptEnvironment* env);
};

class BlindPP : public GenericVideoFilter {
    int* QP;
    bool iPP;
    int PP_MODE;
    int moderate_h, moderate_v;
    YV12PICT *out;
public:
    BlindPP(AVSValue args, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    ~BlindPP();
    static AVSValue __cdecl create(AVSValue args, void*, IScriptEnvironment* env);
};


class Deblock : public GenericVideoFilter {
   int nQuant;
   int nAOffset, nBOffset;
   int nWidth, nHeight;

public:
    Deblock(PClip _child, int q, int aOff, int bOff, IScriptEnvironment* env);
    ~Deblock() {}
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    int __stdcall SetCacheHints(int hints, int) { return hints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0; }
    static AVSValue __cdecl create(AVSValue args, void*, IScriptEnvironment* env);
};


class LumaYV12: public GenericVideoFilter {
    double  lumgain;
    int     lumoff;
    bool use_SSE2;
    bool use_ISSE;
    bool SepFields;
    int lumGain;

public:
    LumaYV12(PClip _child, int _Lumaoffset,double _Lumagain, IScriptEnvironment* env);
    ~LumaYV12() {}
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    int __stdcall SetCacheHints(int hints, int) { return hints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0; }
    static AVSValue __cdecl create(AVSValue args, void*, IScriptEnvironment* env);

};
