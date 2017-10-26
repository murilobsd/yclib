/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_hash.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-18 16:02:14
 * -------------------------
**************************************************************************/

#ifndef _RNS_HASH_H_
#define _RNS_HASH_H_

#define RNS_HASH_TENH 29989
#define MULT 31

#include "comm.h"

uint32_t rns_hash_str(uchar_t *p, uint32_t num);

#endif

