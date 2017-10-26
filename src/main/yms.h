/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : yms.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2016-12-21 15:27:54
 * -------------------------
**************************************************************************/


#ifndef _YMS_H_
#define _YMS_H_


#include "rns_log.h"
#include "yms_live.h"
#include "yms_vod.h"
#include "yms_comm.h"

#define YMS_STATE_VALID   1
#define YMS_STATE_INVALID 0

typedef struct ocs_s
{
    rns_addr_t addr;
    int32_t state;
    rns_http_t* ohttp;
    
    uchar_t resources_uri[128];
    uchar_t channels_uri[128];
    
    uchar_t login_uri[128];
    
    uchar_t report_channel_uri[128];
}ocs_t;


typedef struct yms_s
{
    uchar_t id[32];
    uchar_t type[32];
    uchar_t mac[32];
    rns_epoll_t* ep;
    rns_timer_t* timer;
    
    uchar_t recddir[128];
    uint64_t recdsize;

    uint32_t gopsize;
    uint32_t tfragsize;

    rbt_root_t channels;
    rbt_root_t resources;
    rns_mpool_t* resource_mp;
    ocs_t* ocs;
    
    rns_pipe_t* pipe;
    uchar_t report_buf[1024];

    rns_http_t* hls;
}yms_t;

#endif
