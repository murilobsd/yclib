/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_log.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-13 11:19:40
 * -------------------------
**************************************************************************/

#ifndef _RNS_LOG_H_
#define _RNS_LOG_H_



#define RNS_LOG_LEVEL_DEBUG 0

#define RNS_LOG_LEVEL_INFO  1
#define RNS_LOG_LEVEL_WARN  2
#define RNS_LOG_LEVEL_ERROR 3

#define RNS_LOG_LEVEL_TRACE 4

#include "rns.h"

typedef struct rns_log_s
{
    rns_epoll_cb_t ecb;
    uint32_t level;
    uchar_t trace[1024];
    rns_fmgr_t* fmgr;
    rns_file_t* fp;
    rns_pipe_t* pipe;
    uchar_t* read_buf;
    uchar_t* write_buf;
    uint32_t size;
    uchar_t module[64];
}rns_log_t;

rns_log_t* rns_log_init();
void rns_log_destroy(rns_log_t** log);
void rns_log(rns_log_t* log, uint32_t level, char_t* file, const char_t* func, uint32_t line, char_t* format, ...);

#define LOG_DEBUG(log, ...)  rns_log(log, RNS_LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__,  __VA_ARGS__)

#define LOG_INFO(log, ...)  rns_log(log, RNS_LOG_LEVEL_INFO, __FILE__, __func__, __LINE__,  __VA_ARGS__)
#define LOG_WARN(log, ...)  rns_log(log, RNS_LOG_LEVEL_WARN, __FILE__, __func__, __LINE__,  __VA_ARGS__)
#define LOG_ERROR(log, ...)  rns_log(log, RNS_LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__,  __VA_ARGS__)

#endif

