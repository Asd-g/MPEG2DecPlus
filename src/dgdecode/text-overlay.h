// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

// from AviSynth v2.55 sources

#ifndef __Text_overlay_H__
#define __Text_overlay_H__

// commented out includes and pasted the needed code
// from internal.h and convert.h into this file

//#include "internal.h"
//#include "convert.h"

#include <avisynth.h>
#include <avs/win.h>

// copy and pasted from internal.h

class AVSFunction {
  const char* name;
  const char* param_types;
  AVSValue (__cdecl *apply)(AVSValue args, void* user_data, IScriptEnvironment* env);
  void* user_data;
};

class _PixelClip {
  enum { buffer=320 };
  BYTE clip[256+buffer*2];
public:
  _PixelClip() {
    memset(clip, 0, buffer);
    for (int i=0; i<256; ++i) clip[i+buffer] = i;
    memset(clip+buffer+256, 255, buffer);
  }
  BYTE operator()(int i) { return clip[i+buffer]; }
};

static _PixelClip PixelClip;  // made static to fix linking

static __inline BYTE ScaledPixelClip(int i) {
  return PixelClip((i+32768) >> 16);
}


// copy and pasted from convert.h

inline void YUV2RGB(int y, int u, int v, BYTE* out)
{
  const int crv = int(1.596*65536+0.5);
  const int cgv = int(0.813*65536+0.5);
  const int cgu = int(0.391*65536+0.5);
  const int cbu = int(2.018*65536+0.5);

  int scaled_y = (y - 16) * int((255.0/219.0)*65536+0.5);

  _PixelClip PixelClip;

  out[0] = ScaledPixelClip(scaled_y + (u-128) * cbu); // blue
  out[1] = ScaledPixelClip(scaled_y - (u-128) * cgu - (v-128) * cgv); // green
  out[2] = ScaledPixelClip(scaled_y + (v-128) * crv); // red
}

inline int RGB2YUV(int rgb)
{
  const int cyb = int(0.114*219/255*65536+0.5);
  const int cyg = int(0.587*219/255*65536+0.5);
  const int cyr = int(0.299*219/255*65536+0.5);

  _PixelClip PixelClip;

  // y can't overflow
  int y = (cyb*(rgb&255) + cyg*((rgb>>8)&255) + cyr*((rgb>>16)&255) + 0x108000) >> 16;
  int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
  int b_y = ((rgb&255) << 16) - scaled_y;
  int u = ScaledPixelClip((b_y >> 10) * int(1/2.018*1024+0.5) + 0x800000);
  int r_y = (rgb & 0xFF0000) - scaled_y;
  int v = ScaledPixelClip((r_y >> 10) * int(1/1.596*1024+0.5) + 0x800000);
  return (y*256+u)*256+v;
}



/********************************************************************
********************************************************************/

// from original text-overlay.h

class Antialiaser
/**
  * Helper class to anti-alias text
 **/
{
public:
  Antialiaser(int width, int height, const char fontname[], int size);
  virtual ~Antialiaser();
  HDC GetDC();

  void Apply(const VideoInfo& vi, PVideoFrame* frame, int pitch, int textcolor, int halocolor);
  void ApplyYV12(BYTE* buf, int pitch, int textcolor, int halocolor, int UVpitch,BYTE* bufV,BYTE* bufU);
  void ApplyYUY2(BYTE* buf, int pitch, int textcolor, int halocolor);
  void ApplyRGB24(BYTE* buf, int pitch, int textcolor, int halocolor);
  void ApplyRGB32(BYTE* buf, int pitch, int textcolor, int halocolor);

private:
  const int w, h;
  HDC hdcAntialias;
  HBITMAP hbmAntialias;
  void* lpAntialiasBits;
  HFONT hfontDefault;
  HBITMAP hbmDefault;
  char* alpha_bits;
  bool dirty;

  void GetAlphaRect();
};



/**** Helper functions ****/

void ApplyMessage( PVideoFrame* frame, const VideoInfo& vi, const char* message, int size,
                   int textcolor, int halocolor, int bgcolor, IScriptEnvironment* env );




/**** Inline helper functions ****/

inline static HFONT LoadFont(const char name[], int size, bool bold, bool italic)
{
  return CreateFont( size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                     italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, name );
}


#endif  // __Text_overlay_H__
