#include "Utilities.h"


constexpr uint32_t MAGIC_NUMBER = 0xdeadbeef;


bool PutHintingData(uint8_t *video, uint32_t hint)
{
    for (int i = 0; i < 32; ++i)
    {
        *video &= ~1;
        *video++ |= ((MAGIC_NUMBER & (1 << i)) >> i);
    }
    for (int i = 0; i < 32; i++)
    {
        *video &= ~1;
        *video++ |= ((hint & (1 << i)) >> i);
    }
    return false;
}


bool GetHintingData(uint8_t* video, uint32_t* hint)
{
    uint32_t magic_number = 0;

    for (int i = 0; i < 32; i++)
    {
        magic_number |= ((*video++ & 1) << i);
    }
    if (magic_number != MAGIC_NUMBER)
    {
        return true; // error!
    }
    else
    {
        *hint = 0;
        for (int i = 0; i < 32; i++)
        {
            *hint |= ((*video++ & 1) << i);
        }
    }
    return false;
}
