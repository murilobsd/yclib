/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_rtsp.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-06-10 13:51:18
 * -------------------------
**************************************************************************/

#ifndef _RNS_RTSP_H_
#define _RNS_RTSP_H_

#include "rns.h"

typedef struct rns_rtsp_s
{
    rns_epoll_cb_t ecb;
    int32_t fd;
}rns_rtsp_t;

#endif

