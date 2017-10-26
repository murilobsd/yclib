/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_mpool.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2016-12-16 12:47:10
 * -------------------------
**************************************************************************/

#ifndef _RNS_MPOOL_H_
#define _RNS_MPOOL_H_

#include <ctype.h>
#include "list_head.h"
#include "comm.h"
#include <stdint.h>

typedef struct rns_mpool_s
{
    list_head_t head;
    uint32_t block_size;
    uint32_t avail_block_num;
    uint32_t head_block_num;
    uchar_t* buf;
}rns_mpool_t;

rns_mpool_t* rns_mpool_init(void* addr, uint32_t size, uint32_t block_size);
void* rns_mpool_alloc(rns_mpool_t* mpool_header);
void rns_mpool_free(rns_mpool_t* mpool_header, void* block);

#endif

