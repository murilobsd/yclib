/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : flv.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-14 10:11:04
 * -------------------------
**************************************************************************/

#ifndef _FLV_H_
#define _FLV_H_

#define FLV_PACKET_TYPE_AUDIO 8
#define FLV_PACKET_TYPE_VIDEO 9
#define FLV_PACKET_TYPE_SCRIPT 18

#define FLV_STATE_BUF_NEED           0

#define FLV_STATE_FILE_HEADER        1
#define FLV_STATE_PREV_SIZE          2

#define FLV_STATE_TAG_HEADER         3

#define FLV_STATE_AUDIO_HEADER       4
#define FLV_STATE_AUDIO_PACKET_TYPE  5

#define FLV_STATE_VIDEO_HEADER       6
#define FLV_STATE_VIDEO_PACKET_TYPE  7

#define FLV_STATE_DATA_ATTR          8

#define FLV_STATE_DATA               9
#define FLV_STATE_DONE               10

#include <stdint.h>
#include "rns.h"

typedef struct flv_header_s
{
    uchar_t version;
    uchar_t audio;
    uchar_t video;
    uint32_t data_offset;
}flv_header_t;

typedef struct flv_tag_header_s
{
    uchar_t filter;
    uchar_t tag_type;
    uint32_t data_size;
    uint32_t time_stamp;
    uint32_t stream_id;
}flv_tag_header_t;

typedef struct flv_audio_header_s
{
    uchar_t sound_format;
    uchar_t rate;
    uchar_t sample_size;
    uchar_t type;
    uchar_t packet_type;
}flv_audio_header_t;

typedef struct flv_video_header_s
{
    uchar_t frame_type;
    uchar_t codec_id;
    uchar_t packet_type;
    uint32_t composition_time;
}flv_video_header_t;

typedef struct flv_meta_s
{
    uint32_t audiocodecid;
    uint32_t audiodatarate;
    uint32_t xaudiodelay; 
    uint32_t audiosamplerate; 
    uint32_t audiosamplesize; 
    uchar_t canSeekToEnd; 
    uchar_t* creationdate;
    double duration;
    uint32_t filesize; 
    uint32_t framerate; 
    uint32_t height; 
    uchar_t stereo;
    uint32_t videocodecid;  
    uint32_t videodatarate; 
    uint32_t width;
}flv_meta_t;

typedef union flv_data_header_s
{
    flv_audio_header_t audio_header;
    flv_video_header_t video_header;
}flv_data_header_t;

typedef struct flv_tag_s
{
    flv_tag_header_t tag_header;
    uint32_t size;
    flv_data_header_t data_header;
    // flv_data_attr_t data_attr;
    uchar_t* data;
    uint32_t state;
}flv_tag_t;

typedef struct flv_mod_s
{
    list_head_t mod_list;
    uint32_t prev_tag_size;
    uint32_t offset;
    flv_tag_t tag;
}flv_mod_t;

typedef struct flv_s
{
    list_head_t mod_list;
    flv_header_t header;
    uchar_t file[128];
    uint32_t state;
}flv_t;

int32_t flv_read_file(flv_t* flv, uchar_t* file);
int32_t flv_write_file(flv_t* flv, uchar_t* file);

uint32_t flv_read_tag( flv_tag_t* tag, uchar_t* buf, uchar_t* end);
uint32_t flv_write_tag(flv_tag_t* tag, uchar_t* buf, uchar_t* end);

uint32_t flv_write_header(flv_header_t* header, uchar_t* buf, uchar_t* end);
uint32_t flv_read_header(flv_header_t* header, uchar_t* buf, uchar_t* end);

uint32_t flv_write_tag_header(flv_tag_header_t* tag_header, uchar_t* buf, uchar_t* end);
uint32_t flv_read_tag_header(flv_tag_header_t* tag_header, uchar_t* buf, uchar_t* end);

int32_t flv_write_metadata(flv_meta_t* meta, uchar_t* buf, uchar_t* end);

#endif

