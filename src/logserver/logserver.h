/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : logserver.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-07-10 18:42:25
 * -------------------------
**************************************************************************/

#ifndef _LOGSERVER_H_
#define _LOGSERVER_H_

#include "rns.h"
#include "rns_http.h"
#include "rns_log.h"

extern rns_log_t* lp;

typedef struct logfile_s
{
    rbt_node_t node;
    rns_file_t* fp;
}logfile_t;

typedef struct logserver_s
{
    rns_epoll_t* ep;
    rns_http_t* http;
    rbt_root_t fds;
    uchar_t* root;
    uint32_t size;
}logserver_t;


#endif

