#ifndef MPEG2DECPLUS_IDCT_H
#define MPEG2DECPLUS_IDCT_H

#include <cstdint>
#ifndef _WIN32
#include "win_import_min.h"
#endif

void idct_ref_sse3(int16_t* block);

void prefetch_ref();

void idct_ap922_sse2(int16_t* block);

void prefetch_ap922();

void idct_llm_float_sse2(int16_t* block);

void idct_llm_float_avx2(int16_t* block);

void prefetch_llm_float_sse2();

void prefetch_llm_float_avx2();

#endif
