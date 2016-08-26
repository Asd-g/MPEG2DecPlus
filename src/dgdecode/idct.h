#ifndef MPEG2DECPLUS_IDCT_H
#define MPEG2DECPLUS_IDCT_H

#include <cstdint>

void __fastcall idct_ref_sse3(int16_t* block) noexcept;

void __fastcall prefetch_tables_ref() noexcept;

void __fastcall idct_ap922_sse2(int16_t* block) noexcept;

void __fastcall prefetch_tables_ap922() noexcept;

#endif
