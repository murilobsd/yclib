/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_http.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-25 18:51:20
 * -------------------------
**************************************************************************/

#ifndef _RNS_HTTP_H_
#define _RNS_HTTP_H_

#include "rns.h"



#define RNS_HTTP_METHOD_NONE 0
#define RNS_HTTP_METHOD_POST 1
#define RNS_HTTP_METHOD_GET  2
#define RNS_HTTP_METHOD_PUT  3

#define RNS_HTTP_INFO_STATE_INIT      0
#define RNS_HTTP_INFO_STATE_CODE      1
#define RNS_HTTP_INFO_STATE_METHOD    2
#define RNS_HTTP_INFO_STATE_URI       3
#define RNS_HTTP_INFO_STATE_ARGS      4
#define RNS_HTTP_INFO_STATE_ARGSITEM  5
#define RNS_HTTP_INFO_STATE_HEADER    6
#define RNS_HTTP_INFO_STATE_CHUNK     7
#define RNS_HTTP_INFO_STATE_BODY      8
#define RNS_HTTP_INFO_STATE_DONE      9
#define RNS_HTTP_INFO_STATE_ERROR     10

#define RNS_HTTP_TYPE_SERVER 0
#define RNS_HTTP_TYPE_CLIENT 1


typedef struct rns_http_url_s
{
    uchar_t* uri;
    rbt_root_t args;
    uchar_t* addr;
    uint32_t port;
}rns_http_url_t;

typedef struct rns_http_info_s
{
    uint32_t type;
    uint32_t code;
    uint32_t method;
    uchar_t* uri;
    rbt_root_t headers;
    rbt_root_t args;
    uchar_t* body;
    uint32_t total_size;
    uint32_t curr_size;
    uint32_t body_size;
    uint32_t chunk;
    uint32_t chunksize;
    uint32_t state;
}rns_http_info_t;

struct rns_hconn_s;
struct rns_http_s;


// 这里的cb有两种含义, 一种是作为receiver, 一种是作为sender
// sender的是在task中作为cb来用的, 主要是在write_func
// receiver 是在tune的cb中, 主要是在 read_func
// 有的情况下, 两种方式的cb会有混合, 比如在 作为server的时候, connection 为 close, 那需要在task 中去close 但是, cb 是在tune中的.这种注意

// only reciever can have cb. whether recv req or resp.
// work : recv a full buffer, or the entire req/resp.
// done : the entir req/resp
// close : recv 0
// error : recv rst
// clean : 
typedef struct rns_http_cb_s
{
    void (*conct)(struct rns_hconn_s*, void*);
    int32_t (*work)(struct rns_hconn_s*, void*);
    void (*done)(struct rns_hconn_s*, void*);
    void (*close)(struct rns_hconn_s*, void*);
    void (*error)(struct rns_hconn_s*, void*);
    void (*clean)(void*);
    void* data;
}rns_http_cb_t;

typedef struct rns_htune_s
{
    rns_http_info_t read_info;
    rns_http_info_t write_info;
    rns_http_cb_t cb;
   
}rns_htune_t;

typedef struct rns_hconn_s
{
    rns_epoll_cb_t ecb;

    rns_htune_t tune[2];
    
    list_head_t list;
    int32_t fd;
    rns_addr_t addr;

    uint32_t read_type;
    uint32_t write;
    rns_buf_t* buf;
    rns_epoll_t* ep;
    struct rns_http_s* http;
    list_head_t tasks;
    uint32_t type;
    uint32_t phase;
    uint32_t destroy;
    void* timer;
    uint32_t expire;
}rns_hconn_t;

typedef struct rns_http_s
{
    rns_epoll_cb_t ecb;

    rns_http_cb_t cb;
    
    list_head_t hconns;
    rns_addr_t addr;
    int32_t listenfd;
    uint32_t type;
    rns_epoll_t* ep;
    uint32_t expire;
}rns_http_t;

typedef struct rns_http_task_s
{
    list_head_t list;
    rns_http_cb_t cb;
}rns_http_task_t;



void rns_http_destroy(rns_http_t** http);
rns_http_t* rns_httpserver_create(rns_addr_t* addr, rns_http_cb_t* cb, uint32_t expire);
rns_http_t* rns_httpclient_create(rns_epoll_t* ep, rns_addr_t* addr, uint32_t num, rns_http_cb_t* cbpush);

//----------------------------------------------------------------------------------


rns_http_info_t* rns_hconn_resp_info(rns_hconn_t* hconn);
rns_http_info_t* rns_hconn_req_info(rns_hconn_t* hconn);

void rns_hconn_close(rns_hconn_t* hconn);
void rns_hconn_destroy(rns_hconn_t** hconn);
rns_hconn_t* rns_hconn_get(rns_http_t* http, rns_http_cb_t* cb);
void rns_hconn_free(rns_hconn_t* hconn);
int32_t rns_hconn_header_range(rns_hconn_t* hconn, uint32_t* start, uint32_t* end);

int32_t rns_hconn_req(rns_hconn_t* hconn, uchar_t* buf, uint32_t size, uint32_t clean);
int32_t rns_hconn_req_header_init(rns_hconn_t* hconn);
int32_t rns_hconn_req_header_insert(rns_hconn_t* hconn, uchar_t* key, uchar_t* value);
int32_t rns_hconn_req_header(rns_hconn_t* hconn, uchar_t* method, uchar_t* uri);
int32_t rns_hconn_req2(rns_hconn_t* hconn, uchar_t* method, uchar_t* uri, uchar_t* buf, uint32_t size, uint32_t clean, uint32_t keepalive);
void rns_hconn_req_cb(rns_hconn_t* hconn, rns_http_cb_t* cb);

int32_t rns_hconn_resp_header_init(rns_hconn_t* hconn);
int32_t rns_hconn_resp_header_insert(rns_hconn_t* hconn, uchar_t* key, uchar_t* value);
int32_t rns_hconn_resp_header(rns_hconn_t* hconn, uint32_t retCode);
int32_t rns_hconn_resp(rns_hconn_t* hconn);
int32_t rns_hconn_resp2(rns_hconn_t* hconn, uint32_t code, uchar_t* buf, uint32_t len, uint32_t clean);
// -----------------------------------------------------------------------------
int32_t rns_http_download(rns_hconn_t* hconn, rns_file_t* fp, uint32_t start, uint32_t end);

int32_t rns_http_attach_buffer(rns_hconn_t* hconn, uchar_t* buf, uint32_t size, uint32_t clean);
void rns_http_save_buffer(rns_hconn_t* hconn);
void rns_http_clean_buffer(rns_hconn_t* hconn);

//-----------------------------------------------------------------------------
rns_http_url_t* rns_http_url_create(uchar_t* url);
void rns_http_url_destroy(rns_http_url_t** http_url);

#endif
