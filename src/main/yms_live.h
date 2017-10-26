/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : yms_live.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-14 16:27:10
 * -------------------------
**************************************************************************/

#ifndef _YMS_LIVE_H_
#define _YMS_LIVE_H_

#include "yms_comm.h"

#define YMS_CHANNEL_STATE_READY 0
#define YMS_CHANNEL_STATE_FLUENT 1
#define YMS_CHANNEL_STATE_DROP 2
#define YMS_CHANNEL_STATE_STOP 4

typedef struct channel_s
{
    rbt_node_t node;
    list_head_t gops;

    uint32_t gop_id;
    uchar_t sfrag_id[1024];
    uchar_t sfrag_init[1024];
    list_head_t sfrags;

    rns_timer_t* timer;

    rns_http_t* chttp;

    
    uchar_t pat[188];
    uchar_t pmt[188];
    
    uint32_t pmt_pid;
    uint32_t pcr_pid;
    
    float max_duration;
    uint32_t gopsize;
    uint32_t tfragsize;
    
    uchar_t* content_id;

    rns_addr_t addr;
    uchar_t* suri;
    uchar_t* suri_ts;
    
    uchar_t* recddir;
    uint32_t recdindex;
    uint32_t recdsize;

    uchar_t* gopfile;
    rns_file_t* gfp;
    
    rns_fmgr_t* fmgr;
    
    uint32_t state; // 
    uchar_t req_buf[1024];
    uchar_t resp_buf[1024];
    uchar_t m3u8_buf[1024];
    uint32_t m3u8_size;

    rbt_root_t blocks;
    rns_mpool_t* mp;
    rns_pipe_t* rpipe;
    rns_http_t* http;
    rns_file_t* fp;

    rns_epoll_t* ep;
    uchar_t* surl;
}channel_t;

typedef struct yms_live_req_s
{
    uchar_t content_id[64];
    uchar_t uri[128];
    uint32_t frag_id;
    int is_m3u8;
    uint32_t playseek;
}yms_live_req_t;

typedef struct sfrag_s
{
    uint32_t index;
    channel_t* channel;
}sfrag_t;

int32_t yms_channel_start(channel_t* channel);
void yms_channel_stop(channel_t* channel);
int32_t yms_live_req(rns_hconn_t *hconn, channel_t* channel);
channel_t* yms_channel_create(rns_epoll_t* ep, rns_pipe_t* pipe, uchar_t* content_id, uchar_t* surl, uchar_t* recddir, uint64_t recdsize, uint32_t gopsize, uint32_t tfragsize);
void yms_channel_destroy(channel_t** channel);


#endif

