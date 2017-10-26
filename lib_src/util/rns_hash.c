#include "rns_hash.h"

uint32_t rns_hash_str(uchar_t *p, uint32_t num)
{
    unsigned int h = 0;
    for (; *p; p++)
        h = MULT *h + *p;
    return h % num;
}


