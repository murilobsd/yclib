/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_os.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2016-12-22 14:23:22
 * -------------------------
**************************************************************************/

#ifndef _RNS_OS_H_
#define _RNS_OS_H_

#include "comm.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

uchar_t* rns_time_time2str(time_t t);
void rns_time_time2tm(time_t* t, struct tm* tm);
void rns_time_time2tml(time_t* t, struct tm* tm);
time_t rns_time_tm2time(struct tm* tm);
uchar_t* rns_time_tm2str(struct tm* tm, uchar_t* buf, uint32_t size, uchar_t* format);
time_t rns_time_str2time(uchar_t* buf, uint32_t size, uchar_t* format);
void rns_time_str2tm(uchar_t* buf, uint32_t size, uchar_t* format, struct tm* tm);

int32_t rns_os_mac(uchar_t* hdware, uchar_t* buf, uint32_t size);
int32_t rns_limit_set(uint32_t type, uint64_t soft, uint64_t hard);
int32_t rns_limit_get(uint32_t type, uint64_t* soft, uint64_t* hard);


uint64_t rns_time_ms();
#endif

