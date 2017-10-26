/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : av.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-02 16:59:28
 * -------------------------
**************************************************************************/

#ifndef _AV_H_
#define _AV_H_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
}

#ifndef uchar_t
#define uchar_t unsigned char
#endif

int extract_image(uchar_t* src, uchar_t* dst);

#endif

