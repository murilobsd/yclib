// /**************************************************************************
//  * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
//  * File        : rns_timer.h
//  * Version     : YMS 1.0.0
//  * Description : -
//  * 
//  * modification history
//  * ------------------------
//  * author : guoyao 2017-01-18 14:46:43
//  * 定时器使用epoll和timerfd实现, 像crontab那样的需要应用自己实现.
//  * -------------------------
// **************************************************************************/

#ifndef _RNS_TIMER_H_
#define _RNS_TIMER_H_

#include "comm.h"
#include "rns_heap.h"
#include "rns_epoll.h"
#include "rns_pipe.h"
#include <sys/timerfd.h>

// typedef struct rns_timer_s
// {
//     rns_epoll_cb_t ecb;
//     rns_cb_t cb;
//     int32_t timerfd;
//     struct itimerspec t;
// }rns_timer_t;

// rns_timer_t* rns_timer_create();
// void rns_timer_destroy(rns_timer_t** timer);
// int32_t rns_timer_get(rns_timer_t* timer);
// int32_t rns_timer_set(rns_timer_t* timer, uint32_t delay, rns_cb_t* cb);
// int32_t rns_timer_setdelay(rns_timer_t* timer, uint32_t delay);
// void rns_timer_cancel(rns_timer_t* timer);

#endif

