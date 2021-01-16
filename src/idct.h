#ifndef MPEG2DECPLUS_IDCT_H
#define MPEG2DECPLUS_IDCT_H

#include <cstdint>
#ifndef _WIN32
#include "win_import_min.h"
#endif

void __fastcall idct_ref_sse3(int16_t* block) noexcept;

void __fastcall prefetch_ref() noexcept;

void __fastcall idct_ap922_sse2(int16_t* block) noexcept;

void __fastcall prefetch_ap922() noexcept;

void __fastcall idct_llm_float_sse2(int16_t* block) noexcept;

void __fastcall idct_llm_float_avx2(int16_t* block) noexcept;

void __fastcall prefetch_llm_float_sse2() noexcept;

void __fastcall prefetch_llm_float_avx2() noexcept;

#endif
