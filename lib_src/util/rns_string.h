/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_string.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-05-03 17:53:24
 * -------------------------
**************************************************************************/

#ifndef _RNS_STRING_H_
#define _RNS_STRING_H_

#include "comm.h"

typedef struct rns_str_s
{
    uchar_t* str;
    uint32_t len;
}rns_str_t;

#endif

