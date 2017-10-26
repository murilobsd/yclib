/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : fts.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-07-12 14:47:27
 * -------------------------
**************************************************************************/

#ifndef _FTS_H_
#define _FTS_H_

#include "rns.h"
#include "rns_http.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
#include "libavutil/file.h"
#include "libavutil/opt.h"

typedef struct fts_input_s
{
    uint32_t pathtype;
    uchar_t* path;
}fts_input_t;

typedef struct fts_output_s
{
    uint32_t pathtype;
    uchar_t* path;
    uint32_t bitrate;
    uint32_t width;
    uint32_t height;
    enum AVPixelFormat pixfmt;
    uint32_t num;
    uint32_t den;
}fts_output_t;

typedef struct fts_s
{
    fts_input_t* input;
    fts_output_t* output;
}fts_t;


typedef struct mfts_s
{
    fts_t* fts;
    rns_http_t* http;
    rns_epoll_t* ep;
}mfts_t;


#endif

