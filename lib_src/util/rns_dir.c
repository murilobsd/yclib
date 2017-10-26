#include "rns_dir.h"
#include <errno.h>
#include "rns_memory.h"

int32_t rns_dir_create(uchar_t* path, uint32_t len, mode_t mode)
{
    uchar_t* str = rns_malloc(len + 1);
    int32_t ret = 0;
    if(str == NULL)
    {
        return -1;
    }
    rns_memcpy(str, path, len);

    uchar_t* walker = str;

    while(walker + 1 < str + len && (walker = rns_strchr(walker + 1, '/')) != NULL)
    {
        *walker = 0;
        ret = mkdir((char_t*)str, mode);
        if(ret < 0 && errno != EEXIST)
        {
            rns_free(str);
            return -2;
        }
        *walker = '/';
    }

    if(walker == NULL)
    {
        ret = mkdir((char_t*)str, mode);
        if(ret < 0 && errno != EEXIST)
        {
            rns_free(str);
            return -3;
        }
    }

    rns_free(str);
    return 0;
    
}

