/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : hls.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2016-12-17 14:56:35
 * -------------------------
 **************************************************************************/


#ifndef _HLS_H
#define _HLS_H

#include "rns.h"
#include <stdint.h>

#define M3U8_VERSION 3
#define MAX_M3U8_SIZE 1024
#define TS_SEG_NUM 20



typedef struct media_seg_s
{
    double time_len;
    uchar_t uri[256]; // this would crash in some cases. 
    uint32_t  start_bytes;
    uint32_t  end_bytes;
    uint32_t  frag_id;
}media_seg_t;

typedef struct media_m3u8_s
{
    uint32_t version;
    float max_duration;
    uint32_t discontinuity;
    uint32_t media_sequence;
    uint32_t seg_size;
    media_seg_t* seg;
    uint32_t end;
}media_m3u8_t;

media_m3u8_t* m3u8_read(uchar_t* buf, uint32_t size);
uint32_t m3u8_write(media_m3u8_t* m3u8, uchar_t* buf, uint32_t size);
void m3u8_destroy(media_m3u8_t** m3u8);

#endif
