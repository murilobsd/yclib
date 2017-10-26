/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_epoll.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-15 17:17:27
 * -------------------------
**************************************************************************/

#ifndef _RNS_EPOLL_H_
#define _RNS_EPOLL_H_

#define RNS_EVENT_READ   0
#define RNS_EVENT_LISTEN 1
#define RNS_EVENT_TIMER  2

#include "rns_thread.h"
#include "rns_rbt.h"
#include "comm.h"

#include <sys/epoll.h>

#define RNS_EPOLL_IN  EPOLLIN
#define RNS_EPOLL_OUT EPOLLOUT
#define RNS_EPOLL_ALL EPOLLIN | EPOLLOUT


typedef struct rns_cb_s
{
    void (*func)(void*);
    void* data;
}rns_cb_t;


typedef struct rns_cbs_s
{
    rbt_node_t t;
    rbt_node_t i;
    rns_cb_t cb;
}rns_cbs_t;

typedef struct rns_epoll_s
{
    int32_t epfd;
    uint32_t exit;
    rbt_root_t i;
    rbt_root_t t;
}rns_epoll_t;


typedef struct rns_epoll_cb_s
{
    void (*read)(rns_epoll_t*, void*);
    void (*write)(rns_epoll_t*, void*);
}rns_epoll_cb_t;

rns_epoll_t* rns_epoll_init();
void rns_epoll_destroy(rns_epoll_t** ep);
void rns_epoll_wait(rns_epoll_t* ep);
int32_t rns_epoll_add(rns_epoll_t* ep, int32_t fd, void* data, int32_t flag);
void rns_epoll_delete(rns_epoll_t* ep, int32_t fd);

void* rns_timer_add(rns_epoll_t* ep, uint32_t delay, rns_cb_t* cb);
void rns_timer_delete(rns_epoll_t* ep, void* id);
void rns_timer_delay(rns_epoll_t* ep, void* id, uint32_t delay);
    
#endif

