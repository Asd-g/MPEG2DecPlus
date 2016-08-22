#ifndef DGDECODE_UTILITIES_H
#define DGDECODE_UTILITIES_H
#include <cstdint>

bool PutHintingData(uint8_t* video, uint32_t hint);
bool GetHintingData(uint8_t* video, uint32_t *hint);

#define HINT_INVALID 0x80000000
#define PROGRESSIVE  0x00000001
#define IN_PATTERN   0x00000002
#define COLORIMETRY  0x0000001C
#define COLORIMETRY_SHIFT  2

#endif
