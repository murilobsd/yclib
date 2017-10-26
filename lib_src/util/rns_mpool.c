#include "rns_mpool.h"
#include "list_head.h"
#include "rns_memory.h"


rns_mpool_t* rns_mpool_init(void* addr, uint32_t size, uint32_t block_size)
{
    if(size < block_size || size < sizeof(rns_mpool_t) || addr == NULL || size % block_size != 0)
    {
        return NULL;
    }
	
    rns_mpool_t* header = (rns_mpool_t*)addr;
	
    header->block_size = block_size;
	
    uint32_t num = size / block_size;
	
	
    uint32_t header_size = sizeof(list_head_t) * num + sizeof(rns_mpool_t);
	
    uint32_t h = header_size / block_size;
    uint32_t m = header_size % block_size;
	
    if(m != 0)
    {
        header->head_block_num = h + 1;
    }
    else
    {
        header->head_block_num = h;
    }
    header->avail_block_num = num - header->head_block_num;
	
    header->buf = (uchar_t*)header + header->head_block_num * header->block_size;

    INIT_LIST_HEAD(&header->head);
    list_head_t* walker = (list_head_t* )((uchar_t*)header + sizeof(rns_mpool_t));
    uint32_t i = 0;
    for(i = 0; i < header->avail_block_num; ++i, ++walker)
    {
        rns_list_add(walker,&header->head);
    }
	
    return header;
}

void* rns_mpool_alloc(rns_mpool_t* mpool_header)
{
    list_head_t* ptr = mpool_header->head.next;
    if(ptr == mpool_header->head.prev)
    {
        return NULL;
    }
    uint32_t idx = ((uchar_t*)ptr - (uchar_t*)mpool_header - sizeof(rns_mpool_t))/ sizeof(list_head_t);
	
    --mpool_header->avail_block_num;
    rns_list_del((list_head_t*)mpool_header->head.next);
    return mpool_header->buf + idx * mpool_header->block_size;
}

void rns_mpool_free(rns_mpool_t* mpool_header, void* block)
{
    rns_memset(block, mpool_header->block_size);
    uint32_t idx = (uint32_t)((uchar_t*)block - mpool_header->buf) / mpool_header->block_size;
	
    list_head_t* ptr = (list_head_t*)((uchar_t*)mpool_header + sizeof(rns_mpool_t) + idx * sizeof(list_head_t));
	
    rns_list_add(ptr, &mpool_header->head);
    ++mpool_header->avail_block_num;
}
