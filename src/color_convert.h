#ifndef MPEG2DECPLUS_COLOR_CONVERT_H
#define MPEG2DECPLUS_COLOR_CONVERT_H

#include <cstdint>

void conv420to422P(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height);

void conv420to422I(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height);

void conv422to444(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height);

#endif

