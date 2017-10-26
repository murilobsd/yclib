/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : trs_comm.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-07-24 15:41:07
 * -------------------------
**************************************************************************/

#ifndef _TRS_COMM_H_
#define _TRS_COMM_H_
#include "rns_log.h"
#include "rns.h"
#include "rns_http.h"
#include "rns_dbpool.h"

extern rns_log_t* lp;


typedef struct trs_s
{
    uchar_t* id;
    uchar_t type[32];
    uint32_t interval;
    rns_epoll_t* ep;
    rns_pipe_t* pp;
    void* timer;
    
    rns_http_t* http;
    rns_http_t* foss;
    rns_http_t* cms;
    
    rns_dbpool_t* dbpool;
    
    rbt_root_t stbroot;
    rbt_root_t phoneroot;
    rbt_root_t stbs;
    rbt_root_t hconns;

    rns_addr_t postaddr;
    
}trs_t;

#endif
