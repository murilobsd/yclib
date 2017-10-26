/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : yms_comm.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-27 17:45:51
 * -------------------------
**************************************************************************/

#ifndef _YMS_COMM_H_
#define _YMS_COMM_H_

#define BLOCK_SIZE 524288
#define BLOCK_NUM  2000
#define MAX_DURATION 540000  // duration 6 * 90000
#define BLOCK_MASK 0xFFFFFFFFFFF80000


#define YMS_STATE_VALID 1
#define YMS_STATE_INVALID 0

#define YMS_BLOCK_STATE_WRITING    0
#define YMS_BLOCK_STATE_FULL       1
#define YMS_BLOCK_STATE_SAVING     2
#define YMS_BLOCK_STATE_SAVED      3


#include "rns_http.h"
#include <stdint.h>
#include "ts.h"
#include "hls.h"
#include "rns.h"
#include "rns_log.h"
#include <stdlib.h>

typedef struct gop_info_s
{
    uint32_t idx;
    uint64_t offset;
    uint64_t len;
    double time_start;
    double time_len;
}gop_info_t;

typedef struct gop_s
{
    list_head_t list;
    uint32_t index;
    gop_info_t info;
}gop_t;

typedef struct block_s
{
    rbt_node_t node;
    uchar_t* buf;
    uint32_t count;
    uint32_t state;
}block_t;

extern rns_log_t* lp;


#endif

