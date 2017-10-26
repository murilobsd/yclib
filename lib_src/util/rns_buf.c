#include "rns_buf.h"
#include "rns_memory.h"

rns_buf_t* rns_buf_create(uint32_t size)
{
    rns_buf_t* buf = (rns_buf_t*)rns_malloc(sizeof(rns_buf_t));
    if(buf == NULL)
    {
        return NULL;
    }
    buf->buf = (uchar_t*)rns_malloc(size);
    if(buf->buf == NULL)
    {
        rns_free(buf);
        return NULL;
    }
    
    buf->size = size;
    buf->read_pos = 0;
    buf->write_pos = 0;
    
    return buf;
}

void rns_buf_destroy(rns_buf_t** buf)
{
    if(*buf != NULL)
    {
        if((*buf)->buf != NULL)
        {
            rns_free((*buf)->buf);
        }
        rns_free(*buf);
        *buf = NULL;
    }
}
