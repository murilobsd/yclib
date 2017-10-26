/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : foss.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-08-05 11:55:52
 * -------------------------
**************************************************************************/

#ifndef _FOSS_H_
#define _FOSS_H_
#include "rns.h"
#include "rns_http.h"
#include "rns_dbpool.h"

struct foss_s;

typedef struct trsmgr_s
{
    rbt_node_t nnode;
    rbt_node_t snode;
    uchar_t* id;
    rns_addr_t addr;
    uint32_t number;
    void* timer;
    rns_hconn_t* hconn;
    struct foss_s* foss;
}trsmgr_t;

typedef struct foss_s
{
    uchar_t* id;
    uchar_t* type;
    rbt_root_t strsmgr;
    rbt_root_t ntrsmgr;
    rns_http_t* http;
    rns_dbpool_t* foss;
    rns_epoll_t* ep;
    rns_dbpool_t* dbpool;
    rns_http_t* ocs;
    void* timer;
    rns_addr_t postaddr;
}foss_t;


#endif

