/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rtl.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-06-17 11:06:03
 * -------------------------
**************************************************************************/

#ifndef _RTL_H_
#define _RTL_H_

#include "rns.h"
#include "rns_rtmp.h"
#include "rns_log.h"
#include "rns_http.h"


extern rns_log_t* lp;

typedef struct mgr_s
{
    rns_http_t* hc;
    uchar_t* login_uri;
    uchar_t* stream_record_uri;
    uchar_t* stream_publish_uri;
}mgr_t;

typedef struct rtl_s
{
    rns_epoll_t* ep;
    rbt_root_t apps;
    rns_rtmp_t* rtmp;
    rns_timer_t* timer;
    rns_pipe_t* rp;
    mgr_t* mgr;
    uchar_t* reportbuf;
}rtl_t;

#endif

