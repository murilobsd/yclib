/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_pipe.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-15 17:16:14
 * -------------------------
**************************************************************************/

#ifndef _RNS_PIPE_H_
#define _RNS_PIPE_H_

#include "comm.h"
#include "rns_epoll.h"

typedef struct rns_pipe_t
{
    rns_epoll_cb_t ecb;
    rns_cb_t cb;
    int32_t fd[2];
}rns_pipe_t;

rns_pipe_t* rns_pipe_open();
int32_t rns_pipe_read(rns_pipe_t* p, uchar_t* buf, uint32_t size);
int32_t rns_pipe_write(rns_pipe_t* p, uchar_t* buf, uint32_t size);
void rns_pipe_close(rns_pipe_t** p);
int32_t rns_pipe_rfd(rns_pipe_t* p);

#endif

