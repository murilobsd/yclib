/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_rtmp.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-25 18:55:14
 * -------------------------
**************************************************************************/

#ifndef _RNS_RTMP_H_
#define _RNS_RTMP_H_

#include "rns.h"


//---------------------------------------------------------------------------------------------
// script data value

typedef struct sd_ui8_s
{
    uchar_t value;
}sd_ui8_t;

typedef struct sd_dbl_s
{
    double number;
}sd_dbl_t;

typedef struct sd_str_s
{
    uint16_t len;
    uchar_t* str;
}sd_str_t;

typedef struct sd_value_s
{
    uchar_t type;
    void* value;
}sd_value_t;

typedef struct sd_objppt_s
{
    list_head_t list;
    sd_str_t* name;
    sd_value_t* value;
}sd_objppt_t;

typedef struct sd_obj_s
{
    list_head_t objppts;
}sd_obj_t;


typedef struct sd_ecma_s
{
    uint32_t size;
    sd_obj_t* obj;
}sd_ecma_t;

//---------------------------------------------------------------------------------------------------------------------


struct rns_rconn_s;
struct rns_rtmp_s;

typedef struct rns_sock_cb_s
{
    void (*connect)(void* conn, void* data);
    int32_t (*work)(void* conn, void* data);  // read or write
    void (*done)(void* conn);  // recv done, or write done
    void (*close)(void* conn, void* data); // recv 0
    void (*error)(void* conn, void* data); // recv -1 or write -1
    void (*clean)(void* data); // write done
    void* data;
}rns_sock_cb_t;

typedef struct rns_sock_task_s
{
    list_head_t list;
    rns_sock_cb_t cb;
}rns_sock_task_t;

typedef struct rns_rtmp_cb_s
{
    void (*connect)(struct rns_rconn_s* rconn, void* data);
    void (*publish)(struct rns_rconn_s* rconn, void* data);
    void (*play)(struct rns_rconn_s* rconn, void* data);
    void (*packet)(struct rns_rconn_s* rconn, void* data);
    void* data;
    
}rns_rtmp_cb_t;

typedef struct rns_rtmp_msg_s
{
    list_head_t list;
    uchar_t type;
    uint32_t size;
    uint32_t timestamp;
    uint32_t streamid;
    uint64_t offset;
    uint32_t len;
    uchar_t key;
    uchar_t packettype;
}rns_rtmp_msg_t;

typedef struct rns_rtmp_block_s
{
    rbt_node_t node;
    uchar_t* data;
    uint32_t len;
    uint32_t size;
}rns_rtmp_block_t;

typedef struct rns_rtmp_chunk_s
{
    uchar_t fmt;
    uint32_t channel;
    uint32_t timestamp;
    uint32_t msglen;
    uint32_t msgtype;
    uint32_t streamid;
}rns_rtmp_chunk_t;


typedef struct rns_rtmp_chunkstream_s
{
    rbt_node_t node;
    rns_rtmp_chunk_t chunk;

}rns_rtmp_chunkstream_t;


typedef struct rns_rtmp_msg_stream_s
{
    rbt_node_t node;
    rbt_node_t nnode;
    uchar_t* name;
    list_head_t msgs;
    rbt_root_t blocks;
    rns_mpool_t* mp;
    list_head_t players;
    struct rns_rtmp_s* rtmp;
    rns_rtmp_msg_t* videometa;
    rns_rtmp_msg_t* audiometa;
    rns_rtmp_msg_t* ctlmeta;
    sd_ecma_t* meta;
    
    uint32_t type;
    uint32_t state;
    uint32_t netstreamid;
    uint64_t offset;
    
}rns_rtmp_msg_stream_t;


typedef struct rns_rtmp_s
{
    rns_epoll_cb_t ecb;
    rns_rtmp_cb_t cb;
    rns_addr_t addr;
    int32_t listenfd;
    rns_epoll_t* ep;
    uint32_t type;
    uint32_t sindex;  // netstream is different with msg stream

    rbt_root_t mstreams;
    rbt_root_t nstreams;
    uchar_t* app;
    list_head_t rconns;
}rns_rtmp_t;

typedef struct rns_rconn_s
{
    rns_epoll_cb_t ecb;
    rns_rtmp_cb_t cb;
    rns_sock_cb_t scb;
    int32_t fd;
    rns_epoll_t* ep;
    rns_rtmp_t* rtmp;
    rns_buf_t* buf;
    uint32_t ichunksize;
    uint32_t ochunksize;
    uint32_t bw;
    uint32_t state;
    uchar_t CS0;
    uchar_t CS1[1536];
    uchar_t CS2[1536];
    uchar_t cbuf[1024];
    uint32_t cbuflen;

    uint32_t cindex;
    rbt_root_t chunkstreams;
    uint32_t videochunk;
    uint32_t audiochunk;
    uint32_t type;
    list_head_t tasks;
    
    list_head_t player; // for play
    rns_rtmp_msg_stream_t* mstream; // for
    
    double playseek;
    uchar_t rvideo;
    uchar_t raudio;
    uchar_t pause;
    double ptime;
    uint32_t curchunk;
    uint32_t curfmt;
    uint32_t curlen;
    list_head_t list;
    uint32_t netstream;
    uint32_t pstate;  // play state
    
}rns_rconn_t;

rns_rtmp_t* rns_rtmpserver_create(uchar_t* app, rns_addr_t* addr, rns_rtmp_cb_t* cb);
 

#endif

