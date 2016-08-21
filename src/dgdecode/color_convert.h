#ifndef MPEG2DECPLUS_COLOR_CONVERT_H
#define MPEG2DECPLUS_COLOR_CONVERT_H

#include <cstdint>


void conv420to422P(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height);

void conv420to422I(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height);

void conv422to444(const uint8_t *src, uint8_t *dst, int src_pitch, int dst_pitch,
    int width, int height);

void conv444toRGB24(const uint8_t *py, const uint8_t *pu, const uint8_t *pv,
    uint8_t *dst, int src_pitchY, int src_pitchUV, int dst_pitch, int width,
    int height, int matrix, int pc_scale);

void convYUY2to422P(const uint8_t *src, uint8_t *py, uint8_t *pu, uint8_t *pv,
    int pitch1, int pitch2y, int pitch2uv, int width, int height);

void conv422PtoYUY2(const uint8_t *py, uint8_t *pu, uint8_t *pv, uint8_t *dst,
    int pitch1Y, int pitch1UV, int pitch2, int width, int height);

#endif

