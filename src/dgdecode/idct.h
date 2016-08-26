#ifndef MPEG2DECPLUS_IDCT_H
#define MPEG2DECPLUS_IDCT_H

#include <cstdint>

void __fastcall REF_IDCT(int16_t* block) noexcept;

void __fastcall idct_8x8_sse2(int16_t* block) noexcept;

void __fastcall prefetch_tables() noexcept;

#endif
