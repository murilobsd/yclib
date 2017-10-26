/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File  : yms_vod.h
 * Version   : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-14 16:28:37
 * -------------------------
**************************************************************************/

#ifndef _YMS_VOD_H_
#define _YMS_VOD_H_

#include "yms_comm.h"

#define YMS_RESOURCE_CMD_NON 0
#define YMS_RESOURCE_CMD_ADD 1
#define YMS_RESOURCE_CMD_DEL 2
#define YMS_RESOURCE_CMD_DLD 3

#define YMS_RESOURCE_STATE_DLDING 0
#define YMS_RESOURCE_STATE_SCANNING 1
#define YMS_RESOURCE_STATE_READY 2

#define TS_READER_BUFFER_SIZE 18800
#define TS_PACKET_LENG          188

typedef struct vod_req_s
{
    uchar_t content_id[64];
    uchar_t frag_id;
    int m3u8_flag;
    uint32_t playseek;
    uint32_t offset;
    uint32_t size;
}vod_req_t;

typedef struct resource_task_s
{
    uchar_t url[128];
    uchar_t path[128];
    uint32_t offset;
    uint32_t size;
    uint32_t len;
}resource_task_t;

typedef struct resource_s
{
    rbt_node_t node;
    uchar_t content_id[64];
    
    list_head_t gop_list;
    uchar_t pat[188];
    uchar_t pmt[188];
    
    uint32_t pmt_pid;
    uint32_t pcr_pid;

    uchar_t download_url[128];
    uchar_t path[128];
    
    uchar_t url[128];
    uchar_t url_ts[128];
    
    uint32_t state;
    
    uchar_t req_buf[1024];
    uchar_t resp_buf[1024];
}resource_t;




#endif

