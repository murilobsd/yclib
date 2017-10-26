#include <string.h>
#include <stdlib.h>

#include "rns_memory.h"

rns_node_t* rns_split_key_value(uchar_t* str, uchar_t delim)
{
    rns_node_t* item = (rns_node_t*)rns_malloc(sizeof(rns_node_t));
    if(item == NULL)
    {
        return NULL;
    }
    uchar_t* start = str;
    uchar_t* walker = start;
	
    walker = rns_strchr(start, delim);
    if(walker == NULL)
    {
        rns_free(item);
        return NULL;
    }
    
    rbt_set_key_nstr(&item->node, start, walker - start);

    ++walker;
    while(*walker == ' ')++walker;

    item->size = rns_strlen(walker);
    item->data = rns_malloc(item->size + 1);
    if(item->data == NULL)
    {
        rns_free(item);
        return NULL;
    }
    rns_strncpy(item->data, walker, item->size);

    return item;
}


uchar_t* rns_remalloc(uchar_t** ptr, uint32_t size)
{
    if(*ptr != NULL)
    {
        rns_free(*ptr);
    }

    *ptr = rns_malloc(size);
    return *ptr;
}

uchar_t* rns_dup(uchar_t* str)
{
    if(str == NULL)
    {
        return NULL;
    }
    uchar_t* dst = rns_malloc(rns_strlen(str) + 1);
    if(dst == NULL)
    {
        return NULL;
    }

    rns_memcpy(dst, str, rns_strlen(str));
    return dst;
}
