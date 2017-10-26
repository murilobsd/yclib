/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_buf.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-06 17:16:55
 * -------------------------
**************************************************************************/

#ifndef _RNS_BUF_H_
#define _RNS_BUF_H_

#include "comm.h"


typedef struct rns_buf_s
{
    uchar_t* buf;
    uint32_t size;
    uint32_t read_pos;
    uint32_t write_pos;
}rns_buf_t;

rns_buf_t* rns_buf_create(uint32_t size);
void rns_buf_destroy(rns_buf_t** buf);

#endif

