#include "rns_rtmp.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#define RNS_HTTP_BUF_SIZE 409600
#define RTMP_BLOCK_MASK 0xFFFFFFFFFFF80000
#define RTMP_BLOCK_SIZE 524288
#define RTMP_BLOCK_NUM  1000
static void debug(char_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    vprintf(fmt, list);
    va_end(list);
    
    return;
}
//----------------------------------------------------------------------------------------------------------------------

#define AVC_PACKET_TYPE_SEQ 0
#define AVC_PACKET_TYPE_NALU 1
#define AVC_PACKET_TYPE_END 2

//---------------------------------------------------------

#define SD_TYPE_DOUBLE    0
#define SD_TYPE_UI8       1
#define SD_TYPE_STRING    2
#define SD_TYPE_OBJECT    3
#define SD_TYPE_NULL      5
#define SD_TYPE_UI16      7
#define SD_TYPE_ECMA      8
#define SD_TYPE_ARRAY     10
#define SD_TYPE_DATE      11
#define SD_TYPE_LONGSTR   12
#define SD_TYPE_END       13

void* sd_value_create(uchar_t type, void* value);
void* sd_value_read(uchar_t* buf, uint32_t size, uint32_t* len);
int32_t sd_value_write(void* sd, uchar_t* buf, uint32_t size);
void sd_value_destroy(void** data);
void* sd_obj_read(uchar_t* buf, uint32_t size, uint32_t* len);
void sd_obj_destroy(void** data);
int32_t sd_obj_write(void* sd, uchar_t* buf, uint32_t size);

void* sd_null_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    return NULL;
}

int32_t sd_null_write(void* sd, uchar_t* buf, uint32_t size)
{
    return 0;
}


void sd_null_destroy(void** data)
{
    return;
}

uchar_t sd_ui8_value(sd_ui8_t* ui8)
{
    return ui8->value;
}

uchar_t* sd_str_value(sd_str_t* str)
{
    return str->str;
}

double sd_dbl_value(sd_dbl_t* dbl)
{
    return dbl->number;
}

void* sd_ui8_create(uchar_t value)
{
    sd_ui8_t* ui8 = (sd_ui8_t*)rns_malloc(sizeof(sd_ui8_t));
    if(ui8 == NULL)
    {
        return NULL;
    }
    
    ui8->value = value;
    
    return ui8;
}

void* sd_ui8_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    *len = 0;
    if(size < 1)
    {
        return NULL;
    }
    sd_ui8_t* ui8 = sd_ui8_create(*buf);
    if(ui8 == NULL)
    {
        *len = size;
        return NULL;
    }

    *len = 1;
    return ui8;
}

int32_t sd_ui8_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_ui8_t* ui8 = (sd_ui8_t*)sd;
    uchar_t* walker = buf;
    
    *walker = ui8->value;
    walker += sizeof(ui8->value);
    
    return walker - buf;
}

void sd_ui8_destroy(void** data)
{
    if(*data == NULL)
    {
        return;
    }
    
    rns_free(*data);
    *data = NULL;
    
    return;
}

void* sd_dbl_create(double number)
{
    sd_dbl_t* dbl = (sd_dbl_t*)rns_malloc(sizeof(sd_dbl_t));
    if(dbl == NULL)
    {
        return NULL;
    }

    dbl->number = number;

    return dbl;
}

void* sd_dbl_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    *len = 0;
    sd_dbl_t* dbl = (sd_dbl_t*)rns_malloc(sizeof(sd_dbl_t));
    if(dbl == NULL)
    {
        *len = size;
        return NULL;
    }

    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    if(end < walker + sizeof(double))
    {
        *len = 0;
        return NULL;
    }
    
    
    dbl->number = rns_btohd(walker);
    walker += sizeof(double);
    *len = walker - buf;
    
    return dbl;
}

int32_t sd_dbl_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_dbl_t* dbl = (sd_dbl_t*)sd;
    uchar_t* walker = buf;

    rns_htobd(dbl->number, walker);
    walker += sizeof(double);

    return walker - buf;
}

void sd_dbl_destroy(void** data)
{
    if(*data == NULL)
    {
        return;
    }

    rns_free(*data);
    *data = NULL;

    return;
}

void* sd_str_create(uchar_t* str, uint32_t size)
{
    sd_str_t* sd = (sd_str_t*)rns_malloc(sizeof(sd_str_t));
    if(sd == NULL)
    {
        return NULL;
    }
    
    sd->len = size;
    sd->str = rns_malloc(sd->len + 1);
    if(sd->str == NULL)
    {
        rns_free(sd);
        return NULL;
    }
    rns_memcpy(sd->str, str, sd->len);

    return sd;
}

void* sd_str_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    *len = 0;
    sd_str_t* str = (sd_str_t*)rns_malloc(sizeof(sd_str_t));
    if(str == NULL)
    {
        *len = size;
        return NULL;
    }

    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    if(end - walker < 2)
    {
        *len = 0;
        rns_free(str);
        return NULL;
    }

    str->len = rns_btoh16(walker);
    walker += 2;

    if(end - walker < str->len)
    {
        *len = 0;
        rns_free(str);
        return NULL;
    }
    
    str->str = rns_malloc(str->len + 1);
    if(str->str == NULL)
    {
        *len = size;
        rns_free(str);
        return NULL;
    }
    rns_memcpy(str->str, walker, str->len);
    walker += str->len;

    debug("rtmp obj : %s\n", str->str);
    
    *len = walker - buf;
    return str;
}

int32_t sd_str_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_str_t* str = (sd_str_t*)sd;
    uchar_t* walker = buf;
    rns_htob16(str->len, walker);
    walker += 2;
    rns_memcpy(walker, str->str, str->len);

    return str->len + 2;
}

void sd_str_destroy(void** data)
{
    sd_str_t** str = (sd_str_t**)data;
    if(*str == NULL)
    {
        return;
    }

    if((*str)->str != NULL)
    {
        rns_free((*str)->str);
    }
    
    rns_free(*str);
    *str = NULL;

    return;
}


void* sd_objppt_create(sd_str_t* name, sd_value_t* value)
{
    sd_objppt_t* objppt = (sd_objppt_t*)rns_malloc(sizeof(sd_objppt_t));
    if(objppt == NULL)
    {
        return NULL;
    }

    objppt->name = name;
    objppt->value = value;

    return objppt;
}

void* sd_objppt_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    *len = 0;
    sd_objppt_t* objppt = (sd_objppt_t*)rns_malloc(sizeof(sd_objppt_t));
    if(objppt == NULL)
    {
        *len = size;
        return NULL;
    }

    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    uint32_t bytes = 0;
    objppt->name = sd_str_read(walker, end - walker, &bytes);
    if(objppt->name == NULL)
    {
        *len = bytes ? size : 0;
        rns_free(objppt);
        return NULL;
    }
    walker += bytes;

    objppt->value = sd_value_read(walker, end - walker, &bytes);
    if(objppt->value == NULL)
    {
        *len = bytes ? size : 0;
        sd_str_destroy((void**)&objppt->name);
        rns_free(objppt);
        return NULL;
    }
    walker += bytes;
    
    *len = walker - buf;
    
    return objppt;
}

int32_t sd_objppt_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_objppt_t* objppt = (sd_objppt_t*)sd;
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    walker += sd_str_write(objppt->name, walker, end - walker);
    walker += sd_value_write(objppt->value, walker, end - walker);

    return walker - buf;
}

void sd_objppt_destroy(void** data)
{
    sd_objppt_t** objppt = (sd_objppt_t**)data;
    if(*objppt == NULL)
    {
        return;
    }

    sd_str_destroy((void**)&(*objppt)->name);
    sd_value_destroy((void**)&(*objppt)->value);
    
    rns_free(*objppt);
    *objppt = NULL;
    
    return;
}

void* sd_ecma_create(uint32_t size, sd_obj_t* obj)
{
    sd_ecma_t* ecma = (sd_ecma_t*)rns_malloc(sizeof(sd_ecma_t));
    if(ecma == NULL)
    {
        return NULL;
    }
    
    ecma->size = size;
    ecma->obj = obj;
    
    return ecma;
}

void* sd_ecma_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    *len = 0;
    sd_ecma_t* ecma = (sd_ecma_t*)rns_malloc(sizeof(sd_ecma_t));
    if(ecma == NULL)
    {
        *len = size;
        return NULL;
    }
    
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    
    uint32_t bytes = 0;

    if(end - walker < 4)
    {
        *len = 0;
        rns_free(ecma);
        return NULL;
    }
    ecma->size = rns_btoh32(walker);
    walker += 4;

    ecma->obj = (sd_obj_t*)sd_obj_read(walker, end - walker, &bytes);
    if(ecma->obj == NULL)
    {
        *len = bytes ? size : 0;
        rns_free(ecma);
        return NULL;
    }
    walker += bytes;
    
    *len = walker - buf;
    return ecma;
}

int32_t sd_ecma_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_ecma_t* ecma = (sd_ecma_t*)sd;
    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    rns_htob32(ecma->size, walker);
    walker += 4;
    int32_t ret = sd_obj_write((void*)ecma->obj, walker, (uint32_t)(end - walker));
    if(ret < 0)
    {
        return 0;
    }
    walker += ret;
    return walker - buf;
}

void sd_ecma_destroy(void** data)
{
    sd_ecma_t** ecma = (sd_ecma_t**)data;
    if(*ecma == NULL)
    {
        return;
    }
    
    sd_obj_destroy((void**)&(*ecma)->obj);
    
    rns_free(*ecma);
    *ecma = NULL;
    
    return;
}


void* sd_obj_create()
{
    sd_obj_t* obj = (sd_obj_t*)rns_malloc(sizeof(sd_obj_t));
    if(obj == NULL)
    {
        return NULL;
    }
    INIT_LIST_HEAD(&obj->objppts);
    
    return obj;
}

void sd_obj_add(sd_obj_t* obj, sd_objppt_t* objppt)
{
    rns_list_add(&objppt->list, &obj->objppts);
    return;
}

int32_t sd_obj_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_obj_t* obj = (sd_obj_t*)sd;
    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    list_head_t* p = NULL;
    rns_list_for_each(p, &obj->objppts)
    {
        sd_objppt_t* objppt = rns_list_entry(p, sd_objppt_t, list);
        walker += sd_objppt_write(objppt, walker, end - walker);
    }

    *walker++ = 0;
    *walker++ = 0;
    *walker++ = 9;
    
    return walker - buf;
}

void* sd_obj_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    *len = 0;

    sd_obj_t* obj = (sd_obj_t*)rns_malloc(sizeof(sd_obj_t));
    if(obj == NULL)
    {
        *len = size;
        return NULL;
    }

    INIT_LIST_HEAD(&obj->objppts);
    
    while(walker < end && !(*walker == 0 && *(walker + 1) == 0 &&  *(walker + 2) == 9))
    {
        uint32_t bytes = 0;
        sd_objppt_t* objppt = sd_objppt_read(walker, end - walker, &bytes);
        if(objppt == NULL)
        {
            *len = bytes ? size : 0;
            sd_obj_destroy((void**)&obj);
            return NULL;
        }
        walker += bytes;
        rns_list_add(&objppt->list, &obj->objppts);
    }
    
    walker += 3;
    
    *len = walker - buf;
    return obj;
}

void sd_obj_destroy(void** data)
{
    sd_obj_t** obj = (sd_obj_t**)data;
    if(*obj == NULL)
    {
        return;
    }

    sd_objppt_t* objppt = NULL;
    while((objppt = rns_list_first(&(*obj)->objppts, sd_objppt_t, list)) != NULL)
    {
        rns_list_del(&objppt->list);
        sd_objppt_destroy((void**)&objppt);
    }
    
    rns_free(*obj);
    *obj = NULL;
    
    return;
}
    
void* (*sd_read[])(uchar_t* buf, uint32_t size, uint32_t* len) =
{
sd_dbl_read, sd_ui8_read, sd_str_read, sd_obj_read, sd_null_read, sd_null_read, sd_null_read, sd_null_read, sd_ecma_read
};

int32_t (*sd_write[])(void* sd, uchar_t* buf, uint32_t size) =
{
sd_dbl_write, sd_ui8_write, sd_str_write, sd_obj_write, sd_null_write, sd_null_write, sd_null_write, sd_null_write, sd_ecma_write
};

void (*sd_destroy[])(void**) =
{
sd_dbl_destroy, sd_ui8_destroy, sd_str_destroy, sd_obj_destroy, sd_null_destroy, sd_null_destroy, sd_null_destroy, sd_null_destroy, sd_ecma_destroy
};


void* sd_value_create(uchar_t type, void* value)
{
    sd_value_t* v = (sd_value_t*)rns_malloc(sizeof(sd_value_t));
    if(v == NULL)
    {
        return NULL;
    }

    v->type = type;
    v->value = value;
    
    return v;
}

void* sd_value_read(uchar_t* buf, uint32_t size, uint32_t* len)
{
    *len = 0;
    sd_value_t* value = (sd_value_t*)rns_malloc(sizeof(sd_value_t));
    if(value == NULL)
    {
        *len = size;
        return NULL;
    }

    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    if(end - walker < 1)
    {
        *len = 0;
        rns_free(value);
        return NULL;
    }
    value->type = *walker;
    walker += 1;

    if(value->type >= SD_TYPE_END)
    {
        *len = size;
        rns_free(value);
        return NULL;
    }
    uint32_t bytes = 0;

    value->value = (void*)sd_read[value->type](walker, end - walker, &bytes);
    walker += bytes;
    
    *len = walker - buf;
    
    return value;
}

int32_t sd_value_write(void* sd, uchar_t* buf, uint32_t size)
{
    sd_value_t* value = (sd_value_t*)sd;
    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    *walker = value->type;
    walker += 1;
    
    if(value->type >= SD_TYPE_END)
    {
        return size;
    }
    walker += sd_write[value->type](value->value, walker, end - walker);

    return walker - buf;
}


void sd_value_destroy(void** data)
{
    sd_value_t** value = (sd_value_t**)data;
    if(*value == NULL)
    {
        return;
    }

    if((*value)->type >= SD_TYPE_END)
    {
        return;
    }
    sd_destroy[(*value)->type](&(*value)->value);
    
    rns_free(*value);
    *value = NULL;
    
    return;
}

//--------------------------------------------------------------------------------------------------------------------

#define RNS_RTMP_BUF_SIZE 10000

#define RNS_RCONN_TYPE_PUBLISH 0
#define RNS_RCONN_TYPE_PLAY    1
#define RNS_RCONN_TYPE_PUSH    2
#define RNS_RCONN_TYPE_PULL    3

#define RNS_RTMP_HANDSHAKE_C0 0
#define RNS_RTMP_HANDSHAKE_C1 1
#define RNS_RTMP_HANDSHAKE_C2 2

#define RNS_RTMP_HANDSHAKE_S0 3
#define RNS_RTMP_HANDSHAKE_S1 4
#define RNS_RTMP_HANDSHAKE_S2 5

#define RNS_RTMP_CHUNK_FMT              6
#define RNS_RTMP_CHUNK_CHANNEL_1        7
#define RNS_RTMP_CHUNK_CHANNEL_2        8
#define RNS_RTMP_CHUNK_CHANNEL_3        9
#define RNS_RTMP_CHUNK_CHANNEL          10
#define RNS_RTMP_CHUNK_TIMESTAMP        11
#define RNS_RTMP_CHUNK_MSG_LEN          12
#define RNS_RTMP_CHUNK_MSG_TYPE         13
#define RNS_RTMP_CHUNK_MSG_STREAMID     14
#define RNS_RTMP_CHUNK_EXTRA_TIMESTAMP  15
#define RNS_RTMP_CHUNK_PAYLOAD          16
#define RNS_RTMP_CHUNK_DONE             17
#define RNS_RTMP_CHUNK_ERROR            18


#define RNS_RTMP_MSGTYPE_SIZE  1
#define RNS_RTMP_MSGTYPE_ABORT  2
#define RNS_RTMP_MSGTYPE_ACK    3
#define RNS_RTMP_MSGTYPE_UC  4
#define RNS_RTMP_MSGTYPE_WACK  5
#define RNS_RTMP_MSGTYPE_BW   6
/* #define RNS_RTMP_MSGTYPE_VIDEO 7 */
#define RNS_RTMP_MSGTYPE_AUDIO 8
#define RNS_RTMP_MSGTYPE_VIDEO 9
#define RNS_RTMP_MSGTYPE_COMMAND_AMF3 17
#define RNS_RTMP_MSGTYPE_DATAMSG      18
#define RNS_RTMP_MSGTYPE_COMMAND_AMF0      20


#define RNS_RTMP_UC_STREAM_BEGIN       0
#define RNS_RTMP_UC_STREAM_EOF         1
#define RNS_RTMP_UC_SET_BUFFER_LENGTH  3

#define RNS_RTMP_TYPE_CLIENT     0
#define RNS_RTMP_TYPE_SERVER     1


#define RNS_RTMP_STREAM_TYPE_LIVE    0
#define RNS_RTMP_STREAM_TYPE_RECORD  1
#define RNS_RTMP_STREAM_TYPE_APPEND  2

#define RNS_RTMP_STREAM_STATE_CREATE  0
#define RNS_RTMP_STREAM_STATE_PUBLISH 1

#define RNS_RTMP_STREAM_PSTATE_DATAMSG  0
#define RNS_RTMP_STREAM_PSTATE_SPSPPS   1
#define RNS_RTMP_STREAM_PSTATE_KEYFRAME 2
#define RNS_RTMP_STREAM_PSTATE_FRAME    3  

rns_rconn_t* rns_rconnserver_create(rns_rtmp_t* rtmp, int32_t fd);
rns_rconn_t* rns_rconnclient_create(rns_rtmp_t* rtmp);
static void read_func(rns_epoll_t* ep, void* data);
static void write_func(rns_epoll_t* ep, void* data);
void rns_rconn_destroy(rns_rconn_t** rconn);
static int32_t rns_rconn_read(rns_rconn_t* rconn, uchar_t* buf, int32_t size);
static int32_t rns_rconn_write(rns_rconn_t* rconn, uchar_t* buf, int32_t size);   

static int32_t rtmp_resp_connect(rns_rconn_t* rconn, double id, uint32_t type);
static int32_t rtmp_resp_call(rns_rconn_t* rconn, double id, uint32_t type);
static int32_t rtmp_resp_close(rns_rconn_t* rconn, double id, uint32_t type);
static int32_t rtmp_resp_createStream(rns_rconn_t* rconn, double id, uint32_t type, uint32_t streamid);
static int32_t rtmp_resp_closeStream(rns_rconn_t* rconn, double id, uint32_t type);
static void rtmp_resp_onstatus(rns_rconn_t* rconn, uint32_t tid, sd_value_t* ovalue);
static void rtmp_resp_reset(rns_rconn_t* rconn, uint32_t tid, uchar_t* name);
static void rtmp_resp_play(rns_rconn_t* rconn, uint32_t tid, uchar_t* name);
static void rtmp_resp_publish(rns_rconn_t* rconn, uint32_t tid, uchar_t* name, uint32_t result);
rns_rtmp_chunkstream_t* rns_rtmp_chunkstream_create(uint32_t channel);
int32_t rtmp_command_send(rns_rconn_t* rconn, uint32_t channel, uint32_t streamid, uchar_t* buf, uint32_t size, uint32_t clean);
rns_rtmp_msg_stream_t* rns_rtmp_msg_stream_create(rns_rtmp_t* rtmp, uchar_t* name);
void rns_rtmp_msg_stream_destroy(rns_rtmp_msg_stream_t** stream);
int32_t rtmp_resp_getStreamLength(rns_rconn_t* rconn, uint32_t tid, uint32_t len);
int32_t rtmp_datamsg_send(rns_rconn_t* rconn, uchar_t* name);
void rtmp_resp_checkbw(rns_rconn_t* rconn);

void rtmp_send_buffer_clean(void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    rns_free(rdata);
    return;
}

void rtmp_send_buffer_error(void* conn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    rns_free(rdata);
    return;
}

void rtmp_send_buffer_clean_deep(void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    if(rdata->data != NULL)
    {
        rns_free(rdata->data);
    }
    rns_free(rdata);
    return;
}

void rtmp_send_buffer_error_deep(void* conn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    if(rdata->data != NULL)
    {
        rns_free(rdata->data);
    }
    rns_free(rdata);
    return;
}

int32_t rtmp_send_buffer_work(void* conn, void* data)
{
    rns_rconn_t* rconn = (rns_rconn_t*)conn;
    rns_data_t* rdata = (rns_data_t*)data;
    
    if(rdata->size <= rdata->offset)
    {
        return 0;
    }
    
    int32_t ret = rns_rconn_write(rconn, (uchar_t*)rdata->data + rdata->offset, rdata->size - rdata->offset);
    if(ret < 0)
    {
        return ret;
    }
    /* if(rconn->type == RNS_RCONN_TYPE_PLAY) */
    /* { */
    /*     rns_file_write(fout, (uchar_t*)rdata->data + rdata->offset, ret); */
    /* } */
    rdata->offset += ret;
    
    return ret;
}

int32_t rtmp_chunk_write(rns_rtmp_chunk_t* chunk, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t e = false;
    uint32_t state = RNS_RTMP_CHUNK_FMT;
    
    while(!e)
    {
        switch(state)
        {
            case RNS_RTMP_CHUNK_FMT :
            {
                
                if(end - walker < 1)
                {
                    goto ERR_EXIT;
                }
                
                *walker = chunk->fmt << 6;
                state = RNS_RTMP_CHUNK_CHANNEL;
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL :
            {
                if(chunk->channel < 64)
                {
                    if(end - walker < 1)
                    {
                        goto ERR_EXIT;
                    }
                    *walker |= chunk->channel;
                    walker += 1;
                }
                else if(chunk->channel < 256 + 64)
                {
                    if(end - walker < 2)
                    {
                        goto ERR_EXIT;
                    }
                    *(walker + 1) = chunk->channel - 64;
                    walker += 2;
                }
                else if(chunk->channel > 256 + 64)
                {
                    if(end - walker < 3)
                    {
                        goto ERR_EXIT;
                    }
                    *walker += 1;
                    *(walker + 1) = 255;
                    *(walker + 2) = (chunk->channel - 256 - 64) >> 8;
                    walker += 3;
                }

                if(chunk->fmt != 3)
                {
                    state = RNS_RTMP_CHUNK_TIMESTAMP;
                }
                else
                {
                    state = RNS_RTMP_CHUNK_DONE;
                }
                
                break;
            }
            case RNS_RTMP_CHUNK_TIMESTAMP :
            {
                if(end - walker < 3)
                {
                    goto ERR_EXIT;
                }
                rns_htob24(chunk->timestamp, walker);
                walker += 3;
                
                if(chunk->fmt == 2)
                {
                    state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                }
                else
                {
                    state = RNS_RTMP_CHUNK_MSG_LEN;
                }
                break;
            }
            case RNS_RTMP_CHUNK_MSG_LEN :
            {
                if(end - walker < 3)
                {
                    goto ERR_EXIT;
                }
                rns_htob24(chunk->msglen, walker);
                walker += 3;
                state = RNS_RTMP_CHUNK_MSG_TYPE;
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_TYPE :
            {
                if(end - walker < 1)
                {
                    goto ERR_EXIT;
                }
                *walker = chunk->msgtype;
                walker += 1;
                if(chunk->fmt == 0)
                {
                    state = RNS_RTMP_CHUNK_MSG_STREAMID;
                }
                else
                {
                    state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                }
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_STREAMID :
            {
                if(end - walker < 4)
                {
                    goto ERR_EXIT;
                }
                rns_htol32(chunk->streamid, walker);
                walker += 4;
                state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                break;
            }
            case RNS_RTMP_CHUNK_EXTRA_TIMESTAMP :
            {
                if(chunk->timestamp == 0xFFFFFF)
                {
                    if(end - walker < 4)
                    {
                        goto ERR_EXIT;
                    }
                    
                    rns_htob32(chunk->timestamp, walker);
                    walker += 4;
                }
                state = RNS_RTMP_CHUNK_DONE;
                
                break;
            }
            case RNS_RTMP_CHUNK_DONE :
            {
                e = true;
                break;
            }
            default :
            {
                return 0;
            }
        }
    }

    return walker - buf;
ERR_EXIT:
    
    return 0;
}

int32_t rtmp_attach_buffer(rns_rconn_t* rconn, uchar_t* buf, uint32_t size, uint32_t clean)
{
    
    rns_sock_task_t* task = (rns_sock_task_t*)rns_malloc(sizeof(rns_sock_task_t));
    if(task == NULL)
    {
        return -1;
    }
    
    rns_data_t* rdata = (rns_data_t*)rns_malloc(sizeof(rns_data_t));
    if(rdata == NULL)
    {
        rns_free(task);
        return -2;
    }
    rdata->data = buf;
    rdata->size = size;
    
    task->cb.work = rtmp_send_buffer_work;
    if(clean)
    {
        task->cb.clean = rtmp_send_buffer_clean_deep;
        task->cb.error = rtmp_send_buffer_error_deep;
    }
    else
    {
        task->cb.clean = rtmp_send_buffer_clean;
        task->cb.error = rtmp_send_buffer_error;
    }
    task->cb.data = rdata;
    
    rns_list_add(&task->list, &rconn->tasks);
    return 0;
}


//must define because the return value 0;
int32_t rtmp_clean_buffer_work(void* conn, void* data)
{
    return 0;
}


void rtmp_clean_buffer_clean(void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    if(rdata->data != NULL)
    {
        rns_free(rdata->data);
    }
    rns_free(rdata);
    return;
}

void rtmp_clean_buffer_error(void* conn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    if(rdata->data != NULL)
    {
        rns_free(rdata->data);
    }
    rns_free(rdata);
    return;
}

    
int32_t rtmp_clean_buffer(rns_rconn_t* rconn, uchar_t* buf)
{
    rns_sock_task_t* task = (rns_sock_task_t*)rns_malloc(sizeof(rns_sock_task_t));
    if(task == NULL)
    {
        return -1;
    }
    
    rns_data_t* rdata = (rns_data_t*)rns_malloc(sizeof(rns_data_t));
    if(rdata == NULL)
    {
        rns_free(task);
        return -2;
    }
    rdata->data = buf;
    rdata->size = 0;
    
    task->cb.work = rtmp_clean_buffer_work;

    task->cb.clean = rtmp_clean_buffer_clean;
    task->cb.error = rtmp_clean_buffer_error;
   
    task->cb.data = rdata;
    
    rns_list_add(&task->list, &rconn->tasks);
    return 0;
}

void rtmp_send_C0(rns_rconn_t* rconn)
{
    rconn->CS0 = 3;
    int32_t ret = rtmp_attach_buffer(rconn, &rconn->CS0, 1, 0);
    if(ret < 0)
    {
        
    }
    return;
}

void rtmp_send_C1(rns_rconn_t* rconn)
{
    return;
}

void rtmp_send_C2(rns_rconn_t* rconn)
{
    
    return;
}

void rtmp_send_S0(rns_rconn_t* rconn)
{
    int32_t ret = rtmp_attach_buffer(rconn, &rconn->CS0, 1, 0);
    if(ret < 0)
    {
        
    }
    return;
}

void rtmp_send_S1(rns_rconn_t* rconn)
{
    char buf[1536];
    buf[1535] = 0;
    rns_memcpy(rconn->CS1, buf, 1536);

    rns_htob32(0, rconn->CS1);
    rns_htob32(0, rconn->CS1 + 4);
    int32_t ret = rtmp_attach_buffer(rconn, rconn->CS1, 1536, 0);
    if(ret < 0)
    {
        return;
    }
    return;
}

void rtmp_send_S2(rns_rconn_t* rconn)
{
    int32_t ret = rtmp_attach_buffer(rconn, rconn->CS2, 1536, 0);
    if(ret < 0)
    {
        return;
    }
    return;
}

int32_t rtmp_uc_sb(rns_rconn_t* rconn, uint32_t streamid)
{
    rns_rtmp_chunk_t chunk;
    rns_memset(&chunk, sizeof(rns_rtmp_chunk_t));
    chunk.fmt = 0;
    chunk.channel = 2;
    chunk.timestamp = 0;
    chunk.msglen = 6;
    chunk.msgtype = RNS_RTMP_MSGTYPE_UC;
    chunk.streamid = 0;
    
    uchar_t* buf = rns_malloc(32);
    if(buf == NULL)
    {
        return -2;
    }
    uchar_t* walker = buf;
    
    int32_t s = rtmp_chunk_write(&chunk, walker, 32);
    if(s <= 0)
    {
        return -1;
    }
    walker += s;
    *walker++ = 0;
    *walker++ = 0;
    rns_htob32(streamid, walker);
    walker += 4;
    
    return rtmp_attach_buffer(rconn, buf, walker - buf, 1);
}

int32_t rtmp_uc_resetplay(rns_rconn_t* rconn, uint32_t streamid)
{
    rns_rtmp_chunk_t chunk;
    rns_memset(&chunk, sizeof(rns_rtmp_chunk_t));
    chunk.fmt = 0;
    chunk.channel = 2;
    chunk.timestamp = 0;
    chunk.msglen = 6;
    chunk.msgtype = RNS_RTMP_MSGTYPE_UC;
    chunk.streamid = 0;
    
    uchar_t* buf = rns_malloc(32);
    if(buf == NULL)
    {
        return -2;
    }
    uchar_t* walker = buf;
    
    int32_t s = rtmp_chunk_write(&chunk, walker, 32);
    if(s <= 0)
    {
        return -1;
    }
    walker += s;
    rns_htob16(31, walker);
    walker += 2;
    rns_htob32(streamid, walker);
    walker += 4;
    
    return rtmp_attach_buffer(rconn, buf, walker - buf, 1);
}

int32_t rtmp_chunkctrl_acksize(rns_rconn_t* rconn, uint32_t size)
{
    rns_rtmp_chunk_t chunk;
    rns_memset(&chunk, sizeof(rns_rtmp_chunk_t));
    chunk.fmt = 0;
    chunk.channel = 2;
    chunk.timestamp = 0;
    chunk.msglen = 4;
    chunk.msgtype = RNS_RTMP_MSGTYPE_WACK;
    chunk.streamid = 0;

    uchar_t* buf = rns_malloc(32);
    if(buf == NULL)
    {
        return -2;
    }
    int32_t s = rtmp_chunk_write(&chunk, buf, 32);
    if(s <= 0)
    {
        return -1;
    }
    rns_htob32(size, buf + s);
    return rtmp_attach_buffer(rconn, buf, s + 4, 1);
}

int32_t rtmp_chunkctrl_bw(rns_rconn_t* rconn, uint32_t size)
{
    rns_rtmp_chunk_t chunk;
    rns_memset(&chunk, sizeof(rns_rtmp_chunk_t));
    chunk.fmt = 0;
    chunk.channel = 2;
    chunk.timestamp = 0;
    chunk.msglen = 5;
    chunk.msgtype = RNS_RTMP_MSGTYPE_BW;
    chunk.streamid = 0;
    
    uchar_t* buf = rns_malloc(32);
    if(buf == NULL)
    {
        return -2;
    }
    int32_t s = rtmp_chunk_write(&chunk, buf, 32);
    if(s <= 0)
    {
        return -1;
    }

    rns_htob32(size, buf + s);
    *(buf + s + 4) = 2;
    
    return rtmp_attach_buffer(rconn, buf, s + 5, 1);
}

int32_t rtmp_chunkctrl_chunksize(rns_rconn_t* rconn, uint32_t size)
{
    rns_rtmp_chunk_t chunk;
    rns_memset(&chunk, sizeof(rns_rtmp_chunk_t));
    chunk.fmt = 0;
    chunk.channel = 2;
    chunk.timestamp = 0;
    chunk.msglen = 4;
    chunk.msgtype = RNS_RTMP_MSGTYPE_SIZE;
    chunk.streamid = 0;
    
    uchar_t* buf = rns_malloc(32);
    if(buf == NULL)
    {
        return -2;
    }
    int32_t s = rtmp_chunk_write(&chunk, buf, 32);
    if(s <= 0)
    {
        return -1;
    }
    rns_htob32(size, buf + s);
    
    rconn->ochunksize = size;
    return rtmp_attach_buffer(rconn, buf, s + 4, 1);
}

int32_t rtmp_command_connect(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    int32_t ret = 0;

    /* sd_obj_t* args = NULL; */
    sd_obj_t* obj = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    // 由sd系列保证bytes 的值不会大于 end - walker

    walker += 1;
    obj = sd_obj_read(walker, end - walker, &bytes);
    if(obj == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    list_head_t* p;
    rns_list_for_each(p, &obj->objppts)
    {
        sd_objppt_t* objppt = rns_list_entry(p, sd_objppt_t, list);
        if(!rns_strncmp(objppt->name->str, "app", 3))
        {
            if(rns_strncmp(rconn->rtmp->app, sd_str_value((sd_str_t*)objppt->value->value), rns_strlen(rconn->rtmp->app)))
            {
                ret = rtmp_resp_connect(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), 0);
                if(ret < 0)
                {
                    walker = end;
                    goto EXIT;
                }
            }
            else
            {
                ret = rtmp_resp_connect(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), 1);
                if(ret < 0)
                {
                    walker = end;
                    goto EXIT;
                }
            }
        }
    }

    walker += 1;
    /* args = sd_obj_read(walker, end - walker, &bytes); */
    /* if(args == NULL) */
    /* { */
    /*     walker = bytes ? end : buf; */
    /*     goto EXIT; */
    /* } */
    /* walker += bytes; */

EXIT:
    sd_value_destroy((void**)&id);
    sd_obj_destroy((void**)&obj);
    /* sd_obj_destroy((void**)&args); */
    return walker - buf;

    
}

int32_t rtmp_resp_connect(rns_rconn_t* rconn, double tid, uint32_t type)
{
    uchar_t* buf = rns_malloc(1024);
    if(!buf)
    {
        return -1;
    }
    uchar_t* walker = buf;
    uchar_t* end = buf + 1024;
    sd_str_t* cmd = NULL;
    sd_value_t* rvalue = NULL;
    sd_value_t* tvalue = NULL;
    sd_value_t* ovalue = NULL;
    sd_value_t* ivalue = NULL;
    
    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    
    if(type)
    {
        cmd = sd_str_create((uchar_t*)"_result", 7);
    }
    else
    {
        cmd = sd_str_create((uchar_t*)"_error", 6);
    }
    if(cmd == NULL)
    {
        goto EXIT;
    }

    rvalue = sd_value_create(SD_TYPE_STRING, cmd);
    if(rvalue == NULL)
    {
        goto EXIT;
    }
    
    walker += sd_value_write(rvalue, walker, end - walker);
    
    sd_dbl_t* id = sd_dbl_create(tid);
    if(id == NULL)
    {
        goto EXIT;
    }
    tvalue = sd_value_create(SD_TYPE_DOUBLE, id);
    if(tvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(tvalue, walker, end - walker);
    
    sd_obj_t* obj = sd_obj_create();
    if(obj == NULL)
    {
        goto EXIT;
    }
    {
        uchar_t* v = (uchar_t*)"FMS/3,5,1,525";
        sd_str_t* ver = sd_str_create((uchar_t*)"fmsVer", 6);
        sd_str_t* ver2 = sd_str_create(v, rns_strlen(v));
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            return -6;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        uchar_t* n = (uchar_t*)"capabilities";
        sd_str_t* name = sd_str_create(n, rns_strlen(n));
        sd_dbl_t* vstr = sd_dbl_create(31);
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_DOUBLE, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        uchar_t* n = (uchar_t*)"mode";
        sd_str_t* name = sd_str_create(n, rns_strlen(n));
        sd_dbl_t* vstr = sd_dbl_create(1);
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_DOUBLE, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    ovalue = sd_value_create(SD_TYPE_OBJECT, obj);
    if(ovalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(ovalue, walker, end - walker);
    
    sd_obj_t* info = sd_obj_create();
    if(info == NULL)
    {
        goto EXIT;
    }

    {
        sd_str_t* name = sd_str_create((uchar_t*)"level", 5);
        sd_str_t* vstr = sd_str_create((uchar_t*)"status", 6);
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_STRING, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(info, objppt);
    }
    {
        sd_str_t* name = sd_str_create((uchar_t*)"code", 4);
        sd_str_t* vstr = sd_str_create((uchar_t*)"NetConnection.Connect.Success", 29);
        
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_STRING, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(info, objppt);
    }
    {
        uchar_t* n = (uchar_t*)"description";
        uchar_t* v = (uchar_t*)"Connection succeeded.";
        sd_str_t* name = sd_str_create(n, rns_strlen(n));
        sd_str_t* vstr = sd_str_create(v, rns_strlen(v));
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_STRING, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(info, objppt);
    }
    {
        uchar_t* n = (uchar_t*)"objectEncoding";
        sd_str_t* name = sd_str_create(n, rns_strlen(n));
        sd_dbl_t* vstr = sd_dbl_create(0);
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_DOUBLE, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(info, objppt);
    }

    sd_obj_t* dobj = sd_obj_create();
    if(dobj == NULL)
    {
        goto EXIT;
    }
    {
        uchar_t* n = (uchar_t*)"version";
        uchar_t* m = (uchar_t*)"3,5,1,525";
        sd_str_t* name = sd_str_create(n, rns_strlen(n));
        sd_str_t* vstr = sd_str_create(m, rns_strlen(m));
        if(name == NULL || vstr == NULL)
        {
            goto EXIT;
        }
        sd_value_t* value = sd_value_create(SD_TYPE_STRING, vstr);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(dobj, objppt);
    }
    {
        uchar_t* n = (uchar_t*)"data";
        sd_str_t* name = sd_str_create(n, rns_strlen(n));
        
        if(name == NULL || dobj == NULL)
        {
            goto EXIT;
        }
        
        sd_value_t* value = sd_value_create(SD_TYPE_OBJECT, dobj);
        if(value == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(name, value);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(info, objppt);
    }
    ivalue = sd_value_create(SD_TYPE_OBJECT, info);
    if(ivalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(ivalue, walker, end - walker);
    
    int32_t ret = rtmp_chunkctrl_acksize(rconn, 2500000);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    ret = rtmp_chunkctrl_bw(rconn, 2500000);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    ret = rtmp_command_send(rconn, chunkstream->chunk.channel, chunkstream->chunk.streamid, buf, (uint32_t)(walker - buf), 1);
    if(ret < 0)
    {
        goto EXIT;
    }

    rtmp_resp_checkbw(rconn);
    
    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&ovalue);
    sd_value_destroy((void**)&tvalue);
    sd_value_destroy((void**)&ivalue);
    
    return 0;
    
EXIT:
    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&ovalue);
    sd_value_destroy((void**)&tvalue);
    sd_value_destroy((void**)&ivalue);

    return -1;
}

int32_t rtmp_command_call(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_obj_t* args = NULL;
    sd_obj_t* obj = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
   
    walker += bytes;

    walker += 1;
    obj = sd_obj_read(walker, end - walker, &bytes);
    if(obj == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    walker += 1;
    args = sd_obj_read(walker, end - walker, &bytes);
    if(args == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    int32_t ret = rtmp_resp_call(rconn, sd_dbl_value((sd_dbl_t*)id->value), 1);
    if(ret < 0)
    {
        walker = end;
        goto EXIT;
    }

EXIT:
    sd_value_destroy((void**)&id);
    sd_obj_destroy((void**)&obj);
    sd_obj_destroy((void**)&args);
    return walker - buf;
}

int32_t rtmp_resp_call(rns_rconn_t* rconn, double tid, uint32_t type)
{
    return 0;
}

int32_t rtmp_command_createStream(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    // this may need to modify
    
    ++rconn->rtmp->sindex;
    rconn->netstream = rconn->rtmp->sindex;
    
    int32_t ret = rtmp_resp_createStream(rconn, sd_dbl_value((sd_dbl_t*)id->value), 1, rconn->netstream);
    if(ret < 0)
    {
        walker = end;
        goto EXIT;
    }
    
EXIT:
    sd_value_destroy((void**)&id);
    return walker - buf;
}

int32_t rtmp_command_releaseStream(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_str_t* str = NULL;
    
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;
    
    str = sd_value_read(walker, end - walker, &bytes);
    if(str == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
EXIT:
    sd_value_destroy((void**)&id);
    sd_value_destroy((void**)&str);
    return walker - buf;
}


int32_t rtmp_resp_createStream(rns_rconn_t* rconn, double tid, uint32_t type, uint32_t streamid)
{
    uchar_t* buf = rns_malloc(1024);
    if(!buf)
    {
        return -1;
    }
    uchar_t* walker = buf;
    uchar_t* end = buf + 1024;
    sd_str_t* cmd = NULL;
    
    sd_value_t* rvalue = NULL;
    sd_value_t* tvalue = NULL;
    sd_value_t* svalue = NULL;

    
    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    
    if(type)
    {
        cmd = sd_str_create((uchar_t*)"_result", 7);
    }
    else
    {
        cmd = sd_str_create((uchar_t*)"_error", 6);
    }
    if(cmd == NULL)
    {
        goto EXIT;
    }
    
    rvalue = sd_value_create(SD_TYPE_STRING, cmd);
    if(rvalue == NULL)
    {
        goto EXIT;
    }
    
    walker += sd_value_write(rvalue, walker, end - walker);
    
    
    sd_dbl_t* id = sd_dbl_create(tid);
    if(id == NULL)
    {
        goto EXIT;
    }
    tvalue = sd_value_create(SD_TYPE_DOUBLE, id);
    if(tvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(tvalue, walker, end - walker);

    *walker = SD_TYPE_NULL;
    walker += 1;
    
    sd_dbl_t* sid = sd_dbl_create(streamid);
    if(id == NULL)
    {
        goto EXIT;
    }
    svalue = sd_value_create(SD_TYPE_DOUBLE, sid);
    if(svalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(svalue, walker, end - walker);
    
    int32_t ret = rtmp_command_send(rconn, chunkstream->chunk.channel, chunkstream->chunk.streamid, buf, (uint32_t)(walker - buf), 1);
    if(ret < 0)
    {
        goto EXIT;
    }

    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&svalue);
    sd_value_destroy((void**)&tvalue);
    return 0;
    
EXIT:
    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&svalue);
    sd_value_destroy((void**)&tvalue);

    return -1;
}

int32_t rtmp_command_getStreamLength(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_str_t* str = NULL;
    
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;
    
    str = sd_value_read(walker, end - walker, &bytes);
    if(str == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    int32_t ret = rtmp_resp_getStreamLength(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), 10);
    if(ret < 0)
    {
        goto EXIT;
    }
    
EXIT:
    sd_value_destroy((void**)&id);
    sd_value_destroy((void**)&str);
    return walker - buf;
}

int32_t rtmp_resp_getStreamLength(rns_rconn_t* rconn, uint32_t tid, uint32_t len)
{
    uchar_t* buf = rns_malloc(1024);
    if(!buf)
    {
        return -1;
    }
    uchar_t* walker = buf;
    uchar_t* end = buf + 1024;
    sd_str_t* cmd = NULL;
    
    sd_value_t* rvalue = NULL;
    sd_value_t* tvalue = NULL;
    sd_value_t* svalue = NULL;
    
    
    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    
    
    cmd = sd_str_create((uchar_t*)"_result", 7);
    if(cmd == NULL)
    {
        goto EXIT;
    }
    
    rvalue = sd_value_create(SD_TYPE_STRING, cmd);
    if(rvalue == NULL)
    {
        goto EXIT;
    }
    
    walker += sd_value_write(rvalue, walker, end - walker);
    
    
    sd_dbl_t* id = sd_dbl_create(tid);
    if(id == NULL)
    {
        goto EXIT;
    }
    tvalue = sd_value_create(SD_TYPE_DOUBLE, id);
    if(tvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(tvalue, walker, end - walker);
    
    *walker = SD_TYPE_NULL;
    walker += 1;
    
    sd_dbl_t* sid = sd_dbl_create(len);
    if(id == NULL)
    {
        goto EXIT;
    }
    svalue = sd_value_create(SD_TYPE_DOUBLE, sid);
    if(svalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(svalue, walker, end - walker);
    
    int32_t ret = rtmp_command_send(rconn, 3, chunkstream->chunk.streamid, buf, (uint32_t)(walker - buf), 1);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&svalue);
    sd_value_destroy((void**)&tvalue);
    return 0;
    
EXIT:
    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&svalue);
    sd_value_destroy((void**)&tvalue);
    
    return -1;
}

int32_t rtmp_command_checkbw(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;   
    }
    
    walker += 1;

EXIT:
    sd_value_destroy((void**)&id);
    return walker - buf;
    
}

int32_t rtmp_command_play(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_str_t* stream_name = NULL;
    sd_value_t* len = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;   
    }
    
    walker += 1;

    walker += 1; // value obj but not str
    stream_name = sd_str_read(walker, end - walker, &bytes);
    if(stream_name == NULL)
    {
        rtmp_resp_reset(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(stream_name));
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    len = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    
    rbt_node_t* node = rbt_search_str(&rconn->rtmp->mstreams, sd_str_value(stream_name));
    if(node == NULL)
    {
        rtmp_resp_reset(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(stream_name));
        walker = end;
        goto EXIT;
    }
    
    rns_rtmp_msg_stream_t* mstream = rbt_entry(node, rns_rtmp_msg_stream_t, node);
    rns_list_add(&rconn->player, &mstream->players);
    rconn->mstream = mstream;
    rconn->type = RNS_RCONN_TYPE_PLAY;

    rbt_node_t* cnode = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(cnode == NULL)
    {
        rtmp_resp_reset(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(stream_name));
        walker = end;
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(cnode, rns_rtmp_chunkstream_t, node);

    int32_t ret = rtmp_chunkctrl_chunksize(rconn, 1024);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    ret = rtmp_uc_sb(rconn, chunkstream->chunk.streamid);
    if(ret < 0)
    {
        rtmp_resp_reset(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(stream_name));
        walker = end;
        goto EXIT;
    }

    rtmp_resp_reset(rconn,  (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(stream_name));
    rtmp_resp_play(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(stream_name));
    
EXIT:
    sd_value_destroy((void**)&id);
    sd_str_destroy((void**)&stream_name);
    sd_value_destroy((void**)&len);
    return walker - buf;   
}

int32_t rtmp_command_play2(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    
    return walker - buf;
}

int32_t rtmp_command_deleteStream(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_dbl_t* streamid = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    walker += 1; // value obj but not str
    streamid = sd_dbl_read(walker, end - walker, &bytes);
    if(streamid == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    rbt_node_t* node = rbt_search_int(&rconn->rtmp->nstreams, (uint64_t)sd_dbl_value(streamid));
    if(node != NULL)
    {
        rns_rtmp_msg_stream_t* stream = rbt_entry(node, rns_rtmp_msg_stream_t, nnode);
        rbt_delete(&rconn->rtmp->nstreams, &stream->nnode);
        rbt_delete(&rconn->rtmp->mstreams, &stream->node);
        rns_rtmp_msg_stream_destroy(&stream);
    }

    --rconn->rtmp->sindex;
    
EXIT:
    sd_dbl_destroy((void**)&id);
    sd_dbl_destroy((void**)&streamid);
    return walker - buf;
}

int32_t rtmp_command_closeStream(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;
    
    rns_list_del(&rconn->player);
    rconn->mstream = NULL;

EXIT:
    sd_dbl_destroy((void**)&id);
    return walker - buf;  
}

int32_t rtmp_command_receiveAudio(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_ui8_t* flag = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    walker += 1; // value obj but not str
    flag = sd_ui8_read(walker, end - walker, &bytes);
    if(flag == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    rconn->raudio = sd_ui8_value(flag);

EXIT:
    sd_dbl_destroy((void**)&id);
    sd_ui8_destroy((void**)&flag);
    return walker - buf;
}

int32_t rtmp_command_receiveVideo(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_ui8_t* flag = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    walker += 1; // value obj but not str
    flag = sd_ui8_read(walker, end - walker, &bytes);
    if(flag == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    rconn->rvideo = sd_ui8_value(flag);

EXIT:
    sd_dbl_destroy((void**)&id);
    sd_ui8_destroy((void**)&flag);
    return walker - buf;
}

int32_t rtmp_command_publish(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_str_t* name = NULL;
    sd_str_t* type = NULL;
    rns_rtmp_msg_stream_t* mstream = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    walker += 1; // value obj but not str
    name = sd_str_read(walker, end - walker, &bytes);
    if(name == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    walker += 1;
    type = sd_str_read(walker, end - walker, &bytes);
    if(type == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        walker = end;
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    
    
    rbt_node_t* mnode = rbt_search_str(&rconn->rtmp->mstreams, sd_str_value(name));
    if(mnode == NULL)
    {
        mstream = rns_rtmp_msg_stream_create(rconn->rtmp, sd_str_value(name));
        if(mstream == NULL)
        {
            rtmp_resp_publish(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(name), 0);
            walker = end;
            goto EXIT;
        }
    }
    else
    {
        mstream = rbt_entry(node, rns_rtmp_msg_stream_t, node);
        rbt_delete(&rconn->rtmp->nstreams, &mstream->nnode);
    }

    mstream->netstreamid = chunkstream->chunk.streamid;
    rbt_set_key_int(&mstream->nnode, mstream->netstreamid);
    rbt_insert(&rconn->rtmp->nstreams, &mstream->nnode);

    
    mstream->type = RNS_RTMP_STREAM_TYPE_LIVE;
    mstream->state = RNS_RTMP_STREAM_STATE_PUBLISH;
    
    if(type != NULL)
    {
        if(!rns_strncmp(sd_str_value(type), "live", 4))
        {
            mstream->type = RNS_RTMP_STREAM_TYPE_LIVE;
        }
        else if(!rns_strncmp(sd_str_value(type), "append", 6))
        {
            mstream->type = RNS_RTMP_STREAM_TYPE_RECORD;
        }
        else if(!rns_strncmp(sd_str_value(type), "record", 6))
        {
            mstream->type = RNS_RTMP_STREAM_TYPE_APPEND;
        }
    }
    rtmp_resp_publish(rconn, (uint32_t)sd_dbl_value((sd_dbl_t*)id->value), sd_str_value(name), 1);

    rconn->type = RNS_RCONN_TYPE_PUBLISH;

EXIT:   
    sd_value_destroy((void**)&id);
    sd_str_destroy((void**)&name);
    sd_str_destroy((void**)&type);
    return walker - buf;   
}

int32_t rtmp_command_seek(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_dbl_t* playseek = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    walker += 1; // value obj but not str
    playseek = sd_dbl_read(walker, end - walker, &bytes);
    if(playseek == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    rconn->playseek = sd_dbl_value(playseek);
    
EXIT:
    sd_dbl_destroy((void**)&id);
    sd_dbl_destroy((void**)&playseek);
    return walker - buf;    
}

int32_t rtmp_command_pause(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    sd_ui8_t* p = NULL;
    sd_dbl_t* ptime = NULL;
    sd_value_t* id = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(id == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    if(end - walker < 1)
    {
        walker = buf;
        goto EXIT;
    }
    if(*walker != SD_TYPE_NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += 1;

    walker += 1; // value obj but not str
    p = sd_ui8_read(walker, end - walker, &bytes);
    if(p == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    walker += 1; // value obj but not str
    ptime = sd_dbl_read(walker, end - walker, &bytes);
    if(ptime == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
    rconn->pause = sd_ui8_value(p);
    rconn->ptime = sd_dbl_value(ptime);

EXIT:
    sd_dbl_destroy((void**)&id);
    sd_ui8_destroy((void**)&p);
    sd_dbl_destroy((void**)&ptime);
    return walker - buf;
}

void rtmp_resp_onstatus(rns_rconn_t* rconn, uint32_t tid, sd_value_t* ovalue)
{
    sd_value_t* cvalue = NULL;
    sd_value_t* tvalue = NULL;

    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        return;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);

    uchar_t* buf = rns_malloc(1024);
    if(!buf)
    {
        return;
    }
    uchar_t* walker = buf;
    uchar_t* end = buf + 1024;

    
    sd_str_t* cmd = sd_str_create((uchar_t*)"onStatus", 8);
    if(cmd == NULL)
    {
        goto EXIT;
    }
    
    cvalue = sd_value_create(SD_TYPE_STRING, cmd);
    if(cvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(cvalue, walker, end - walker);
    
    
    sd_dbl_t* id = sd_dbl_create(tid);
    if(id == NULL)
    {
        goto EXIT;
    }
    tvalue = sd_value_create(SD_TYPE_DOUBLE, id);
    if(tvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(tvalue, walker, end - walker);

    *walker = SD_TYPE_NULL;
    walker += 1;

    walker += sd_value_write(ovalue, walker, end - walker);
    
    int32_t ret = rtmp_command_send(rconn, 5, chunkstream->chunk.streamid, buf, (uint32_t)(walker - buf), 1);
    if(ret < 0)
    {
        goto EXIT;
    }
    
EXIT:
    
    sd_value_destroy((void**)&cvalue);
    sd_value_destroy((void**)&tvalue);
    
    return;
}

void rtmp_resp_publish(rns_rconn_t* rconn, uint32_t tid, uchar_t* name, uint32_t result)
{
    sd_value_t* ovalue = NULL;
    sd_obj_t* obj = sd_obj_create();
    if(obj == NULL)
    {
        return;
    }
    {
        sd_str_t* ver = sd_str_create((uchar_t*)"level", 5);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"status", 6);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        uchar_t* v = (uchar_t*)"code";
        uchar_t* v2 = (uchar_t*)"NetStream.Publish.Start";
        sd_str_t* ver = sd_str_create(v, rns_strlen(v));
        sd_str_t* ver2 = sd_str_create(v2, rns_strlen(v2));
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        sd_str_t* ver = sd_str_create((uchar_t*)"description", 11);
        uchar_t* s = rns_malloc(rns_strlen(name) + 64);
        if(s == NULL)
        {
            goto EXIT;
        }
        uint32_t len = snprintf((char_t*)s, rns_strlen(name) + 64, "%s is now published.", name);
        sd_str_t* ver2 = sd_str_create(s, len);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        
        sd_str_t* ver = sd_str_create((uchar_t*)"clientid", 8);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"clientid", 8);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    ovalue = sd_value_create(SD_TYPE_OBJECT, obj);
    if(ovalue == NULL)
    {
        goto EXIT;
    }

    int32_t ret = rtmp_chunkctrl_chunksize(rconn, 1024);
    if(ret < 0)
    {
        goto EXIT;
    }
    
    rtmp_resp_onstatus(rconn, tid, ovalue);
    
EXIT:
    sd_value_destroy((void**)&ovalue);
    
    return;
}

void rtmp_resp_reset(rns_rconn_t* rconn, uint32_t tid, uchar_t* name)
{
    sd_value_t* ovalue = NULL;
    sd_obj_t* obj = sd_obj_create();
    if(obj == NULL)
    {
        return;
    }
    {
        sd_str_t* ver = sd_str_create((uchar_t*)"level", 5);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"status", 6);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        uchar_t* v = (uchar_t*)"code";
        uchar_t* v2 = (uchar_t*)"NetStream.Play.Reset";
        sd_str_t* ver = sd_str_create(v, rns_strlen(v));
        sd_str_t* ver2 = sd_str_create(v2, rns_strlen(v2));
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        sd_str_t* ver = sd_str_create((uchar_t*)"description", 11);
        uchar_t* s = rns_malloc(rns_strlen(name) + 64);
        if(s == NULL)
        {
            goto EXIT;
        }
        uint32_t len = snprintf((char_t*)s, rns_strlen(name) + 64, "Playing and resetting %s.", name);
        sd_str_t* ver2 = sd_str_create(s, len);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        
        sd_str_t* ver = sd_str_create((uchar_t*)"details", 7);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"test", 4);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        
        sd_str_t* ver = sd_str_create((uchar_t*)"clientid", 8);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"clientid", 8);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    ovalue = sd_value_create(SD_TYPE_OBJECT, obj);
    if(ovalue == NULL)
    {
        goto EXIT;
    }
    
    rtmp_resp_onstatus(rconn, 0, ovalue);
    
EXIT:
    sd_value_destroy((void**)&ovalue);
    
    return;
}

void rtmp_resp_play(rns_rconn_t* rconn, uint32_t tid, uchar_t* name)
{
    sd_value_t* ovalue = NULL;
    sd_obj_t* obj = sd_obj_create();
    if(obj == NULL)
    {
        return;
    }
    {
        sd_str_t* ver = sd_str_create((uchar_t*)"level", 5);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"status", 6);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        uchar_t* v = (uchar_t*)"code";
        uchar_t* v2 = (uchar_t*)"NetStream.Play.Start";
        sd_str_t* ver = sd_str_create(v, rns_strlen(v));
        sd_str_t* ver2 = sd_str_create(v2, rns_strlen(v2));
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        sd_str_t* ver = sd_str_create((uchar_t*)"description", 11);
        uchar_t* s = rns_malloc(rns_strlen(name) + 64);
        if(s == NULL)
        {
            goto EXIT;
        }
        uint32_t len = snprintf((char_t*)s, rns_strlen(name) + 64, "Started playing %s.", name);
        sd_str_t* ver2 = sd_str_create(s, len);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        
        sd_str_t* ver = sd_str_create((uchar_t*)"details", 7);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"test", 4);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    {
        
        sd_str_t* ver = sd_str_create((uchar_t*)"clientid", 8);
        sd_str_t* ver2 = sd_str_create((uchar_t*)"clientid", 8);
        if(ver == NULL || ver2 == NULL)
        {
            goto EXIT;
        }
        sd_value_t* ver3 = sd_value_create(SD_TYPE_STRING, ver2);
        if(ver3 == NULL)
        {
            goto EXIT;
        }
        sd_objppt_t* objppt = sd_objppt_create(ver, ver3);
        if(objppt == NULL)
        {
            goto EXIT;
        }
        sd_obj_add(obj, objppt);
    }
    ovalue = sd_value_create(SD_TYPE_OBJECT, obj);
    if(ovalue == NULL)
    {
        goto EXIT;
    }
    

    
    rtmp_resp_onstatus(rconn, 0, ovalue);

    int32_t ret = rtmp_datamsg_send(rconn, name);
    if(ret < 0)
    {
        goto EXIT;
    }

    
EXIT:
    sd_value_destroy((void**)&ovalue);
    
    return;
}

void rtmp_resp_checkbw(rns_rconn_t* rconn)
{
    sd_value_t* cvalue = NULL;
    sd_value_t* tvalue = NULL;
    
    uchar_t* buf = rns_malloc(1024);
    if(!buf)
    {
        return;
    }
    uchar_t* walker = buf;
    uchar_t* end = buf + 1024;
    
    
    sd_str_t* cmd = sd_str_create((uchar_t*)"onBWDone", 8);
    if(cmd == NULL)
    {
        goto EXIT;
    }
    
    cvalue = sd_value_create(SD_TYPE_STRING, cmd);
    if(cvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(cvalue, walker, end - walker);
    
    
    sd_dbl_t* id = sd_dbl_create(0);
    if(id == NULL)
    {
        goto EXIT;
    }
    tvalue = sd_value_create(SD_TYPE_DOUBLE, id);
    if(tvalue == NULL)
    {
        goto EXIT;
    }
    walker += sd_value_write(tvalue, walker, end - walker);
    
    *walker = SD_TYPE_NULL;
    walker += 1;

    int32_t ret = rtmp_command_send(rconn, 3, 0, buf, (uint32_t)(walker - buf), 1);
    if(ret < 0)
    {
        goto EXIT;
    }

EXIT:
    sd_value_destroy((void**)&cvalue);
    sd_value_destroy((void**)&tvalue);

    return;
}

//-------------------------------------------------------------------------------

int32_t rtmp_command_parse(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    uint32_t psize = 0;
    uint32_t bytes= 0;

    walker += 1;
    sd_str_t* name = sd_str_read(walker, end - walker, &bytes);
    if(name == NULL)
    {
        walker = end;
        goto EXIT;
    }
    walker += bytes;

    debug("command : %s, size : %d\n", name->str, name->len);
    
    if(!rns_strncmp(name->str, "connect", name->len))
    {
        psize = rtmp_command_connect(rconn, walker, end - walker);
    }
    else if(!rns_strncmp(name->str, "call", name->len))
    {
        psize = rtmp_command_call(rconn, walker, end - walker);
    }
    else if(!rns_strncmp(name->str, "close", name->len))
    {
        /* psize = rtmp_command_close(rconn, walker, end - walker); */
    }
    else if(!rns_strncmp(name->str, "releaseStream", name->len))
    {
        psize = rtmp_command_releaseStream(rconn, walker, end - walker); 
    }
    else if(!rns_strncmp(name->str, "FCPublish", name->len))
    {
        psize = rtmp_command_releaseStream(rconn, walker, end - walker); 
    }
    else if(!rns_strncmp(name->str, "createStream", name->len))
    {
        psize = rtmp_command_createStream(rconn, walker, end - walker);
    }
    else if(!rns_strncmp(name->str, "getStreamLength", name->len))
    {
        psize = rtmp_command_getStreamLength(rconn, walker, end - walker);
    }
    else if(!rns_strncmp(name->str, "_checkbw", name->len))
    {
        psize = rtmp_command_checkbw(rconn, walker, end - walker);
    }
    else if(!rns_strncmp(name->str, "play", name->len))
    {
        psize = rtmp_command_play(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "play2", name->len))
    {
        psize = rtmp_command_play2(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "deleteStream", name->len))
    {
        psize = rtmp_command_deleteStream(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "closeStream", name->len))
    {
        psize = rtmp_command_closeStream(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "receiveAudio", name->len))
    {
        psize = rtmp_command_receiveAudio(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "receiveVideo", name->len))
    {
        psize = rtmp_command_receiveVideo(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "publish", name->len))
    {
        psize = rtmp_command_publish(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "seek", name->len))
    {
        psize = rtmp_command_seek(rconn, walker, end - walker);   
    }
    else if(!rns_strncmp(name->str, "pause", name->len))
    {
        psize = rtmp_command_pause(rconn, walker, end - walker);   
    }

    if(psize == 0)
    {
        walker = buf;
    }
    else
    {
        walker += psize;
    }
    
EXIT:
    return walker - buf;
}

int32_t rtmp_data_send(rns_rconn_t* rconn, uint32_t chunkid, uint32_t streamid, uint32_t msgtype, uchar_t* buf, uint32_t size, uint32_t clean)
{
    uint32_t sendbytes = 0;
    uint32_t len = 0;
    int32_t ret = 0;
    
    uchar_t* hbuf = rns_malloc(32);
    if(hbuf == NULL)
    {
        return -1;
    }
    
    rns_rtmp_chunk_t chunk;
    rns_memset(&chunk, sizeof(rns_rtmp_chunk_t));
    chunk.fmt = 0;
    chunk.channel = chunkid;
    chunk.timestamp = 0;
    chunk.msglen = size;
    chunk.msgtype = msgtype;
    chunk.streamid = streamid;
    
    uint32_t s = rtmp_chunk_write(&chunk, hbuf, 32);
    
    ret = rtmp_attach_buffer(rconn, hbuf, s, 1);
    if(ret < 0)
    {
        return -2;
    }
    
    while(sendbytes < size)
    {
        len = size - sendbytes < rconn->ochunksize ? size - sendbytes : rconn->ochunksize;
        
        ret = rtmp_attach_buffer(rconn, buf + sendbytes, len, 0);
        if(ret < 0)
        {
            return -3;
        }
        sendbytes += len;
        if(sendbytes < size)
        {
            uchar_t* c = rns_malloc(1);
            if(c == NULL)
            {
                return -3;
            }
            *c = 0xc0 | (uchar_t)chunkid; // need be modified, channel may be beyond one bytes
            ret = rtmp_attach_buffer(rconn, c, 1, 1);
            if(ret < 0)
            {
                return -4;
            }
        }
    }
    
    if(clean)
    {
        ret = rtmp_clean_buffer(rconn, buf);
        if(ret < 0)
        {
            return -5;
        }
    }
    
    return 0;
}

int32_t rtmp_command_send(rns_rconn_t* rconn, uint32_t channel, uint32_t streamid, uchar_t* buf, uint32_t size, uint32_t clean)
{
    return rtmp_data_send(rconn, channel, streamid, RNS_RTMP_MSGTYPE_COMMAND_AMF0, buf, size, clean);
}

int32_t rtmp_msg_header(rns_rtmp_chunk_t* chunk, rns_rtmp_msg_t* msg, uint32_t timestamp, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t state = RNS_RTMP_CHUNK_FMT;
    uint32_t fmt = chunk->fmt;
    
    while(walker < end)
    {
        switch (state)
        {
            case RNS_RTMP_CHUNK_FMT :
            {
                *walker = fmt << 6;
                state = RNS_RTMP_CHUNK_CHANNEL;
                if(fmt == 0)
                {
                    if(msg->type == RNS_RTMP_MSGTYPE_VIDEO)
                    {
                        chunk->fmt = 1;
                    }
                    else if(msg->type == RNS_RTMP_MSGTYPE_AUDIO)
                    {
                        chunk->fmt = 2;
                    }
                }
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL :
            {
                if(chunk->channel < 64)
                {
                    if(end - walker < 1)
                    {
                        goto ERR_EXIT;
                    }
                    *walker |= chunk->channel;
                    walker += 1;
                }
                else if(chunk->channel < 256 + 64)
                {
                    if(end - walker < 2)
                    {
                        goto ERR_EXIT;
                    }
                    *(walker + 1) = chunk->channel - 64;
                    walker += 2;
                }
                else if(chunk->channel > 256 + 64)
                {
                    if(end - walker < 3)
                    {
                        goto ERR_EXIT;
                    }
                    *walker += 1;
                    *(walker + 1) = 255;
                    *(walker + 2) = (chunk->channel - 256 - 64) >> 8;
                    walker += 3;
                }
                
                if(fmt != 3)
                {
                    state = RNS_RTMP_CHUNK_TIMESTAMP;
                }
                else
                {
                    state = RNS_RTMP_CHUNK_DONE;
                }
                break;
            }
            case RNS_RTMP_CHUNK_TIMESTAMP :
            {
                
                if(timestamp >= 16777215)
                {
                    rns_htob24(16777215, walker);
                }
                else
                {
                    rns_htob24(timestamp, walker);
                }
                walker += 3;

                if(fmt != 2)
                {
                    state = RNS_RTMP_CHUNK_MSG_LEN;
                }
                else
                {
                    state = RNS_RTMP_CHUNK_DONE;
                }
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_LEN :
            {
                if(end - walker < 3)
                {
                    goto ERR_EXIT;
                }
                rns_htob24(msg->len, walker);
                walker += 3;
                state = RNS_RTMP_CHUNK_MSG_TYPE;
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_TYPE :
            {
                if(end - walker < 1)
                {
                    goto ERR_EXIT;
                }
                *walker = msg->type;
                walker += 1;
                if(fmt == 0)
                {
                    state = RNS_RTMP_CHUNK_MSG_STREAMID;
                }
                else
                {
                    state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                }
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_STREAMID :
            {
                if(end - walker < 4)
                {
                    goto ERR_EXIT;
                }
                rns_htol32(chunk->streamid, walker);
                walker += 4;
                state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                break;
            }
            case RNS_RTMP_CHUNK_EXTRA_TIMESTAMP :
            {
                if(timestamp >= 0xFFFFFF)
                {
                    if(end - walker < 4)
                    {
                        goto ERR_EXIT;
                    }
                    
                    rns_htob32(timestamp, walker);
                    walker += 4;
                }
                state = RNS_RTMP_CHUNK_DONE;
                
                break;
            }
            case RNS_RTMP_CHUNK_DONE :
            {
                return walker - buf;
            }
        }
    }
    
    return walker - buf;
ERR_EXIT:
    return 0;
}

int32_t rtmp_mid_header(uint32_t chunkid, uint32_t timestamp, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;

    uint32_t state = RNS_RTMP_CHUNK_FMT;
    uint32_t e = 0;
    
    while(!e)
    {
        switch (state)
        {
            case RNS_RTMP_CHUNK_FMT :
            {
                *walker = 3 << 6;
                state = RNS_RTMP_CHUNK_CHANNEL;
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL :
            {
                if(chunkid < 64)
                {
                    if(end - walker < 1)
                    {
                        goto ERR_EXIT;
                    }
                    *walker |= chunkid;
                    walker += 1;
                }
                else if(chunkid < 256 + 64)
                {
                    if(end - walker < 2)
                    {
                        goto ERR_EXIT;
                    }
                    *(walker + 1) = chunkid - 64;
                    walker += 2;
                }
                else if(chunkid > 256 + 64)
                {
                    if(end - walker < 3)
                    {
                        goto ERR_EXIT;
                    }
                    *walker += 1;
                    *(walker + 1) = 255;
                    *(walker + 2) = (chunkid - 256 - 64) >> 8;
                    walker += 3;
                }

                state = RNS_RTMP_CHUNK_TIMESTAMP;
                break;
            }
            case RNS_RTMP_CHUNK_TIMESTAMP :
            {
                if(timestamp >= 0xFFFFFF)
                {
                    if(end - walker < 4)
                    {
                        goto ERR_EXIT;
                    }
                    
                    rns_htob32(timestamp, walker);
                    walker += 4;
                }
                e = true;
                break;
            }
        }
    }

    return walker - buf;
ERR_EXIT:
    return 0;
}

int32_t rtmp_msg_send(rns_rconn_t* rconn, rns_rtmp_msg_t* msg)
{
    uint32_t sendbytes = 0;
    uint32_t len = 0;
    int32_t ret = 0;
    uint32_t chunkid = rconn->videochunk;
    uint64_t offset = 0;
    
    if(msg->type == RNS_RTMP_MSGTYPE_AUDIO)
    {
        chunkid = rconn->audiochunk;
    }
    
    uchar_t* buf = rns_malloc(32);
    if(buf == NULL)
    {
        return -2;
    }
    
    if(rconn->pstate == RNS_RTMP_STREAM_PSTATE_KEYFRAME && !msg->key)
    {
        return 0;
    }
    
    rns_rtmp_chunkstream_t* chunkstream = NULL;
    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, chunkid);
    if(node == NULL)
    {
        chunkstream = (rns_rtmp_chunkstream_t*)rns_malloc(sizeof(rns_rtmp_chunkstream_t));
        if(chunkstream == NULL)
        {
            return -3;
        }
        rbt_set_key_int(&chunkstream->node, chunkid);
        rbt_insert(&rconn->chunkstreams, &chunkstream->node);
        chunkstream->chunk.fmt = 0;
        chunkstream->chunk.channel = chunkid;
        chunkstream->chunk.msgtype = msg->type;
        chunkstream->chunk.msglen = msg->size;
        chunkstream->chunk.streamid = rconn->netstream; 
        chunkstream->chunk.timestamp = msg->timestamp;
    }
    else
    {
        chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    }
    if((chunkstream->chunk.fmt == 0 && msg->type == RNS_RTMP_MSGTYPE_AUDIO) || rconn->pstate == RNS_RTMP_STREAM_PSTATE_KEYFRAME)
    {
        chunkstream->chunk.timestamp = msg->timestamp;
        chunkstream->chunk.fmt = 0;
    }

    rbt_node_t* mnode = rbt_search_int(&rconn->rtmp->nstreams, msg->streamid);
    if(mnode == NULL)
    {
        return -31;
    }
    rns_rtmp_msg_stream_t* stream = rbt_entry(mnode, rns_rtmp_msg_stream_t, nnode);

    uint32_t ochunklen = 0;
    uint32_t timestamp = msg->timestamp - chunkstream->chunk.timestamp;
    uint32_t size = rtmp_msg_header(&chunkstream->chunk, msg, timestamp, buf, 32);
    ret = rtmp_attach_buffer(rconn, buf, size, 0);
    if(ret < 0)
    {
        return -4;
    }
    chunkstream->chunk.timestamp = msg->timestamp;
    
    offset = msg->offset;

    while(sendbytes < msg->size) // size 代表着总的长度, len 代表当前的长度, 和 rtmp 协议有些出入
    {
        uint64_t offset_align = offset & RTMP_BLOCK_MASK;

        rbt_node_t* bnode = rbt_search_int(&stream->blocks, offset_align);
        if(bnode == NULL)
        {
            return -5;
        }
        rns_rtmp_block_t* block = rbt_entry(bnode, rns_rtmp_block_t, node);
        len = msg->size - sendbytes < rconn->ochunksize ? msg->size - sendbytes : rconn->ochunksize;
        len = len < RTMP_BLOCK_SIZE - offset + offset_align ? len : RTMP_BLOCK_SIZE - offset + offset_align;
        len = len < rconn->ochunksize - ochunklen ? len : rconn->ochunksize - ochunklen;

        
        ret = rtmp_attach_buffer(rconn,  block->data + offset - offset_align, len, 0);
        if(ret < 0)
        {
            return -5;
        }

        offset += len;
        sendbytes += len;
        ochunklen += len;
        if(sendbytes < msg->size && ochunklen >= rconn->ochunksize)
        {
            uchar_t* c = rns_malloc(16);
            uint32_t s = rtmp_mid_header(chunkid, timestamp, c, 16);
            if(s == 0)
            {
                return -51;
            }
            
            ret = rtmp_attach_buffer(rconn, c, s, 1);
            if(ret < 0)
            {
                return -6;
            }

            ochunklen = 0;
        }
    }

    if(rconn->pstate == RNS_RTMP_STREAM_PSTATE_KEYFRAME && msg->key)
    {
        rconn->pstate = RNS_RTMP_STREAM_PSTATE_FRAME;
    }
    
    return 0;
}

#define RTMP_MSG_INIT  0
#define RTMP_MSG_WRITE 1
#define RTMP_MSG_DONE  2
#define RTMP_MSG_ERROR 3

int32_t rtmp_msg_payload(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    
    uint32_t writebytes = 0;
    rns_rtmp_chunkstream_t* chunkstream = NULL;
    rns_rtmp_msg_stream_t* stream = NULL;
    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        chunkstream = (rns_rtmp_chunkstream_t*)rns_malloc(sizeof(rns_rtmp_chunkstream_t));
        if(chunkstream == NULL)
        {
            return size;
        }
        rbt_set_key_int(&chunkstream->node, rconn->cindex);
        ++rconn->cindex;
        rbt_insert(&rconn->chunkstreams, &chunkstream->node);
    }
    else
    {
        chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    }

    rbt_node_t* nnode = rbt_search_int(&rconn->rtmp->nstreams, chunkstream->chunk.streamid);
    if(nnode == NULL)
    {
        return size;
    }
    stream = rbt_entry(nnode, rns_rtmp_msg_stream_t, nnode);
    
    
    uint32_t state = RTMP_MSG_WRITE;
    rns_rtmp_msg_t* msg = NULL;

    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    
    uint32_t e = false;
    int32_t ret = 0;
    
    
    while(!e)
    {
        switch (state)
        {
            case RTMP_MSG_INIT :
            {
                if(msg && msg->len >= msg->size)
                {
                    if(msg->key && msg->packettype == AVC_PACKET_TYPE_SEQ)
                    {
                        stream->videometa = msg;
                        rns_list_del_safe(&msg->list);
                    }
                    else
                    {
                        list_head_t* p = NULL;
                        rns_list_for_each(p, &stream->players)
                        {
                            rns_rconn_t* player = rns_list_entry(p, rns_rconn_t, player);
                            
                            ret = rtmp_msg_send(player, msg);
                            if(ret < 0)
                            {
                                continue;
                            }
                            rns_list_add_safe(&player->list, &rconn->rtmp->rconns);
                        }
                    }
                }
                
                msg = (rns_rtmp_msg_t*)rns_malloc(sizeof(rns_rtmp_msg_t));
                if(msg == NULL)
                {
                    state = RTMP_MSG_ERROR;
                    break;
                }
                
                msg->size = chunkstream->chunk.msglen;
                msg->timestamp = chunkstream->chunk.timestamp;
                if(chunkstream->chunk.fmt == 3)
                {
                    msg->timestamp += 10;
                }
                msg->streamid = chunkstream->chunk.streamid;
                msg->type = chunkstream->chunk.msgtype;
                msg->len = 0;
                msg->offset = stream->offset;
                msg->packettype = AVC_PACKET_TYPE_NALU;
                if(msg->type == RNS_RTMP_MSGTYPE_VIDEO && *walker == 0x17)
                {
                    msg->key = 1;
                    if(*(walker + 1) == 0)
                    {
                        msg->packettype = AVC_PACKET_TYPE_SEQ;
                    }
                }
                rns_list_add(&msg->list, &stream->msgs);

                state = RTMP_MSG_WRITE;
                break;
            }
            case RTMP_MSG_WRITE :
            {
                msg = rns_list_last(&stream->msgs, rns_rtmp_msg_t, list);
                if(msg == NULL ||  msg->len >= msg->size)
                {
                    state = RTMP_MSG_INIT;
                    break;
                }
                
                while(writebytes < size && msg->len < msg->size)
                {
                    uint64_t offset_align = stream->offset & RTMP_BLOCK_MASK;
                    rns_rtmp_block_t* block = NULL;
                    rbt_node_t* bnode = rbt_search_int(&stream->blocks, offset_align);
                    if(bnode == NULL)
                    {
                        block = (rns_rtmp_block_t*)rns_malloc(sizeof(rns_rtmp_block_t));
                        if(block == NULL)
                        {
                            state = RTMP_MSG_ERROR;
                            break;
                        }
                        block->data = rns_mpool_alloc(stream->mp);
                        if(block->data == NULL)
                        {
                            state = RTMP_MSG_ERROR;
                            break;
                        }
                        block->size = RTMP_BLOCK_SIZE;
                        block->len = 0;
                        rbt_set_key_int(&block->node, offset_align);
                        rbt_insert(&stream->blocks, &block->node);
                    }
                    else
                    {
                        block = rbt_entry(bnode, rns_rtmp_block_t, node);
                    }
                    
                    uint32_t len = block->size - block->len < size - writebytes ? block->size - block->len : size - writebytes;
                    len = len < msg->size - msg->len ? len : msg->size - msg->len;
                    rns_memcpy(block->data + block->len, walker, len);
                    

                    walker += len;
                    block->len += len;
                    msg->len += len;
                    stream->offset += len;
                    writebytes += len;
                }
                if(msg->len < msg->size)
                {
                    walker = buf;
                }

                
                e = true;
                break;
            }
            case RTMP_MSG_ERROR :
            {
                walker = end;
                e = true;
                break;
            }
        }
    }

    return walker - buf;
}

int32_t rtmp_uc_parse(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t type = RNS_RTMP_UC_STREAM_BEGIN;

    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        walker = end;
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);

    uint32_t len = chunkstream->chunk.msglen - 2;
    
    if(end - walker < 2)
    {
        walker = buf;
        goto EXIT;
    }

    type = rns_btoh16(walker);
    walker += 2;
    if(end < len + walker)
    {
        walker = buf;
        goto EXIT;
    }
    
    switch(type)
    {
        case RNS_RTMP_UC_STREAM_BEGIN :
        {
            /* uint32_t streamid = rns_btoh32(walker); */
            walker += len;
            break;
        }
        case RNS_RTMP_UC_STREAM_EOF :
        {
            break;
        }
        case RNS_RTMP_UC_SET_BUFFER_LENGTH :
        {
            /* uint32_t streamid = rns_btoh32(walker); */
            walker += 4;
            /* uint32_t length = rns_btoh32(walker); */
            walker += len - 4;
            // what's the point? just parse the event, and do nothing
            break;
        }
    }

EXIT:
    return walker - buf;
}

int32_t rtmp_datamsg_send(rns_rconn_t* rconn, uchar_t* name)
{
    rbt_node_t* cnode = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(cnode == NULL)
    {
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(cnode, rns_rtmp_chunkstream_t, node);
    
    rbt_node_t* node = rbt_search_str(&rconn->rtmp->mstreams, name);
    if(node == NULL)
    {
        goto EXIT;
    }
    rns_rtmp_msg_stream_t* stream = rbt_entry(node, rns_rtmp_msg_stream_t, node);
    
    uchar_t* buf = rns_malloc(2048);
    if(buf == NULL)
    {
        goto EXIT;
    }

    uchar_t* walker = buf;
    uchar_t* end = buf + 2048;
    sd_str_t* cmd = NULL;
    sd_value_t* rvalue = NULL;
    sd_value_t* svalue = NULL;

    {
        uchar_t* s = (uchar_t*)"@setDataFrame";
        cmd = sd_str_create(s, rns_strlen(s));
        if(cmd == NULL)
        {
            goto EXIT;
        }
    
        rvalue = sd_value_create(SD_TYPE_STRING, cmd);
        if(rvalue == NULL)
        {
            goto EXIT;
        }
        walker += sd_value_write(rvalue, walker, end - walker);
    }
    {
        uchar_t* s = (uchar_t*)"onMetaData";
        cmd = sd_str_create(s, rns_strlen(s));
        if(cmd == NULL)
        {
            goto EXIT;
        }
        
        svalue = sd_value_create(SD_TYPE_STRING, cmd);
        if(svalue == NULL)
        {
            goto EXIT;
        }
        walker += sd_value_write(svalue, walker, end - walker);
    }

    *walker = SD_TYPE_ECMA;
    walker += 1;

    int32_t ret = sd_ecma_write(stream->meta, walker, end - walker);
    if(ret <= 0)
    {
        goto EXIT;
    }
    walker += ret;
    
    
    ret = rtmp_data_send(rconn, 5, chunkstream->chunk.streamid, RNS_RTMP_MSGTYPE_DATAMSG, buf, (uint32_t)(walker - buf), 1);
    if(ret < 0)
    {
        goto EXIT;
    }
    rconn->pstate = RNS_RTMP_STREAM_PSTATE_SPSPPS;
    
    
    if(stream->videometa != NULL)
    {
        ret = rtmp_msg_send(rconn, stream->videometa);
        if(ret < 0)
        {
            goto EXIT;
        }
        rconn->pstate = RNS_RTMP_STREAM_PSTATE_KEYFRAME;
    }

    
    sd_value_destroy((void**)&rvalue);
    sd_value_destroy((void**)&rvalue);
    return 0;
    
EXIT:
    sd_value_destroy((void**)&rvalue);
    return -1;
}

int32_t rtmp_datamsg_parse(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = buf + size;
    uint32_t bytes = 0;
    
    /* sd_obj_t* args = NULL; */
    sd_value_t* cmd = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(cmd == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    // 由sd系列保证bytes 的值不会大于 end - walker
    
    sd_value_t* str = (sd_value_t*)sd_value_read(walker, end - walker, &bytes);
    if(str == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;

    walker += 1;

    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        walker = end;
        goto EXIT;
    }
    rns_rtmp_chunkstream_t* chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
    rbt_node_t* mnode = rbt_search_int(&rconn->rtmp->nstreams, chunkstream->chunk.streamid);
    if(mnode == NULL)
    {
        walker = end;
        goto EXIT;
    }
    rns_rtmp_msg_stream_t* stream = rbt_entry(mnode, rns_rtmp_msg_stream_t, nnode);
    
    stream->meta = sd_ecma_read(walker, end - walker, &bytes);
    if(stream->meta == NULL)
    {
        walker = bytes ? end : buf;
        goto EXIT;
    }
    walker += bytes;
    
EXIT:
    sd_value_destroy((void**)&cmd);
    sd_value_destroy((void**)&str);
    return walker - buf;
    
}

int32_t rtmp_msg_parse(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{

    uchar_t* walker = buf;
    uchar_t* end = walker + size;

    rns_rtmp_chunkstream_t* chunkstream = NULL;
    rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
    if(node == NULL)
    {
        return size;
    }
    chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);

    switch (chunkstream->chunk.msgtype)
    {
        case RNS_RTMP_MSGTYPE_SIZE :
        {
            if(end - walker < 4)
            {
                break;
            }
            rconn->ichunksize = rns_btoh32(walker);
            walker += 4;
            break;
        }
        case RNS_RTMP_MSGTYPE_ABORT :
        {
            if(end - walker < 4)
            {
                break;
            }
            
            break;
        }
        case RNS_RTMP_MSGTYPE_ACK :
        {
            if(end - walker < 4)
            {
                break;
            }
            walker += 4;
            break;
        }
        case RNS_RTMP_MSGTYPE_UC :
        {
            uint32_t s = 0;
            uint32_t o = rconn->cbuflen;
            rns_memcpy(rconn->cbuf + rconn->cbuflen, walker, end - walker);
            rconn->cbuflen += end - walker;
            s = rtmp_uc_parse(rconn, rconn->cbuf, rconn->cbuflen);
            if(s > 0)
            {
                walker += s - o;
                rconn->cbuflen = 0;
            }
            break;
        }
        case RNS_RTMP_MSGTYPE_WACK :
        {
            if(end - walker < 4)
            {
                break;
            }
            
            walker += 4;
            break;
        }
        case RNS_RTMP_MSGTYPE_BW :
        {
            if(end - walker < 4)
            {
                break;
            }
            rconn->bw = rns_btoh32(walker);
            walker += 4;
            break;
        }
        case RNS_RTMP_MSGTYPE_AUDIO :
        case RNS_RTMP_MSGTYPE_VIDEO :
        {
            walker += rtmp_msg_payload(rconn, walker, end - walker);
            break;
        }
        case RNS_RTMP_MSGTYPE_DATAMSG :
        {
            uint32_t s = 0;
            uint32_t o = rconn->cbuflen;
            rns_memcpy(rconn->cbuf + rconn->cbuflen, walker, end - walker);
            rconn->cbuflen += end - walker;
            s = rtmp_datamsg_parse(rconn, rconn->cbuf, rconn->cbuflen);
            if(s > 0)
            {
                walker += s - o;
                rconn->cbuflen = 0;
            }
            break;
        }
        case RNS_RTMP_MSGTYPE_COMMAND_AMF3 :
        {
            walker += 1;
            uint32_t s = 0;
            uint32_t o = rconn->cbuflen;
            rns_memcpy(rconn->cbuf + rconn->cbuflen, walker, end - walker);
            rconn->cbuflen += end - walker;
            s = rtmp_command_parse(rconn, rconn->cbuf, rconn->cbuflen);
            if(s > 0)
            {
                walker += s - o;
                rconn->cbuflen = 0;
            }
            break;
        }
        case RNS_RTMP_MSGTYPE_COMMAND_AMF0 :
        {
            uint32_t s = 0;
            uint32_t o = rconn->cbuflen;
            rns_memcpy(rconn->cbuf + rconn->cbuflen, walker, end - walker);
            rconn->cbuflen += end - walker;
            s = rtmp_command_parse(rconn, rconn->cbuf, rconn->cbuflen);
            if(s > 0)
            {
                walker += s - o;
                rconn->cbuflen = 0;
            }
            
            break;
        }
    }

    return walker - buf;
}

int32_t rtmp_parse(rns_rconn_t* rconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = walker + size;
    int32_t ret = 0;
    rns_rtmp_chunkstream_t* chunkstream = (void*)1;
    
    rbt_node_t* onode = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);

    if(onode == NULL && rconn->state > RNS_RTMP_CHUNK_CHANNEL)
    {
        return size;
    }
    else if(onode != NULL)
    {
        chunkstream = rbt_entry(onode, rns_rtmp_chunkstream_t, node);
    }


    uint32_t e = false;
    
    while(walker < end && !e)
    {
        switch(rconn->state)
        {
            case RNS_RTMP_HANDSHAKE_C0 :
            {
                if(end < 1 + walker)
                {
                    goto EXIT;
                }
                walker += 1;
                rtmp_send_S0(rconn);
                rtmp_send_S1(rconn);
                rconn->state = RNS_RTMP_HANDSHAKE_C1;
                break;
            }
            case RNS_RTMP_HANDSHAKE_C1 :
            {
                if(end < walker + 1536)
                {
                    goto EXIT;
                }
                rns_memcpy(rconn->CS2, walker, 4);
                walker += 4;
                rns_memset(rconn->CS2 + 4, 4);
                walker += 4;
                rns_memcpy(rconn->CS2 + 8, walker, 1528);
                walker += 1528;
                rtmp_send_S2(rconn);
                
                rconn->state = RNS_RTMP_HANDSHAKE_C2;
                break;
            }
            case RNS_RTMP_HANDSHAKE_C2 :
            {
                if(end < walker + 1536)
                {
                    goto EXIT;
                }
                if(rns_memcmp(rconn->CS1, walker, 4))
                {
                    break;
                }
                walker += 4;

                walker += 4;
                
                ret = rns_memcmp(rconn->CS1 + 8, walker, 1528);
                if(ret != 0)
                {
                    break;
                }
                walker += 1528;

                rconn->state = RNS_RTMP_CHUNK_FMT;
                break;
            }
            case RNS_RTMP_HANDSHAKE_S0 :
            {
                rtmp_send_C1(rconn);
                rconn->state = RNS_RTMP_HANDSHAKE_S1;
                break;
            }
            case RNS_RTMP_HANDSHAKE_S1 :
            {
                rtmp_send_C2(rconn);
                rconn->state = RNS_RTMP_HANDSHAKE_S2;
                break;
            }
            case RNS_RTMP_HANDSHAKE_S2 :
            {
                rconn->state = RNS_RTMP_CHUNK_FMT;
                break;
            }
            case RNS_RTMP_CHUNK_FMT :
            {
                if(end < 1 + walker)
                {
                    goto EXIT;
                }
                rconn->curfmt = (*walker & 0xC0) >> 6;
                rconn->state = RNS_RTMP_CHUNK_CHANNEL_1;
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL_1 :
            {
                if(end < 1 + walker)
                {
                    goto EXIT;
                }
                rconn->curchunk = (*walker) & 0x3F;
                walker += 1;

                if(rconn->curchunk == 0)
                {
                    rconn->state = RNS_RTMP_CHUNK_CHANNEL_2;
                }
                else if(rconn->curchunk == 1)
                {
                    rconn->state = RNS_RTMP_CHUNK_CHANNEL_3;
                }
                else
                {
                    rconn->state = RNS_RTMP_CHUNK_CHANNEL;
                }
                
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL_2 :
            {
                if(end < 1 + walker)
                {
                    goto EXIT;
                }
                rconn->curchunk = *walker + 64;
                walker += 1;

                rconn->state = RNS_RTMP_CHUNK_CHANNEL;
                
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL_3 :
            {
                if(end < 1 + walker)
                {
                    goto EXIT;
                }
                
                rconn->curchunk = (*(walker + 1) << 8) + *walker + 64;
                walker += 2;
                rconn->state = RNS_RTMP_CHUNK_CHANNEL;
                
                break;
            }
            case RNS_RTMP_CHUNK_CHANNEL :
            {
                rbt_node_t* node = rbt_search_int(&rconn->chunkstreams, rconn->curchunk);
                if(node == NULL)
                {
                    chunkstream = rns_rtmp_chunkstream_create(rconn->curchunk);
                    if(chunkstream == NULL)
                    {
                        rconn->state = RNS_RTMP_CHUNK_FMT;
                        walker = end;
                        goto EXIT;
                    }
                    rbt_set_key_int(&chunkstream->node, rconn->curchunk);
                    rbt_insert(&rconn->chunkstreams, &chunkstream->node);
                }
                else
                {
                    chunkstream = rbt_entry(node, rns_rtmp_chunkstream_t, node);
                }


                chunkstream->chunk.fmt = rconn->curfmt;
                chunkstream->chunk.channel = rconn->curchunk;
                
                if(chunkstream->chunk.fmt != 3)
                {
                    rconn->state = RNS_RTMP_CHUNK_TIMESTAMP;
                }
                else
                {
                    rconn->state = RNS_RTMP_CHUNK_PAYLOAD;
                }
                break;
            }
            case RNS_RTMP_CHUNK_TIMESTAMP :
            {
                if(end < 3 + walker)
                {
                    goto EXIT;
                }
                uint32_t timestamp = rns_btoh24(walker);
                walker += 3;
                
                if(chunkstream->chunk.fmt != 0)
                {
                    chunkstream->chunk.timestamp += timestamp;
                }
                else
                {
                    chunkstream->chunk.timestamp = timestamp;
                }
                
                if(chunkstream->chunk.fmt == 2)
                {
                    rconn->state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                }
                else
                {
                    rconn->state = RNS_RTMP_CHUNK_MSG_LEN;
                }

                break;
            }
            case RNS_RTMP_CHUNK_MSG_LEN :
            {
                if(end < 3 + walker)
                {
                    goto EXIT;
                }
                chunkstream->chunk.msglen = rns_btoh24(walker);
                walker += 3;
                rconn->state = RNS_RTMP_CHUNK_MSG_TYPE;
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_TYPE :
            {
                if(end < 1 + walker)
                {
                    goto EXIT;
                }
                chunkstream->chunk.msgtype = *walker;
                walker += 1;
                if(chunkstream->chunk.fmt == 0)
                {
                    rconn->state = RNS_RTMP_CHUNK_MSG_STREAMID;
                }
                else
                {
                    rconn->state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                }
                
                break;
            }
            case RNS_RTMP_CHUNK_MSG_STREAMID :
            {
                if(end < 4 + walker)
                {
                    goto EXIT;
                }
                chunkstream->chunk.streamid = rns_ltoh32(walker);
                walker += 4;
                rconn->state = RNS_RTMP_CHUNK_EXTRA_TIMESTAMP;
                break;
            }
            case RNS_RTMP_CHUNK_EXTRA_TIMESTAMP :
            {
                if(chunkstream->chunk.timestamp == 0xFFFFFF)
                {
                    if(end < 4 + walker)
                    {
                        goto EXIT;
                    }
                    chunkstream->chunk.timestamp = rns_btoh32(walker);
                    walker += 4;
                }
                rconn->state = RNS_RTMP_CHUNK_PAYLOAD;
                break;
            }
            case RNS_RTMP_CHUNK_PAYLOAD :
            {

                uint32_t len = end > rconn->ichunksize + walker ? rconn->ichunksize : end - walker;
                uint32_t s = rtmp_msg_parse(rconn, walker, len);
                if(s > 0)
                {
                    walker += s;
                    rconn->curlen += s;
                }
                else if(s == 0)
                {
                    rconn->curlen += s;
                    walker += len;
                }
                if(s == 0 && len < rconn->ichunksize && rconn->curlen < rconn->ichunksize)
                {
                    
                }
                else
                {
                    rconn->state = RNS_RTMP_CHUNK_FMT;
                    rconn->curlen = 0;
                }

                break;
            }
            default :
            {
                return walker - buf;
            }
        }
    }
EXIT:
    return walker - buf;
}

static void rtmp_close_func(void* rconn, void* data)
{
    return;
}

static void rtmp_error_func(void* rconn, void* data)
{
    return;
}

static void accept_func(rns_epoll_t* ep, void* data)
{
    if(ep == NULL || data == NULL)
    {
        return;
    }

    rns_rtmp_t* rtmp = (rns_rtmp_t*)data;
    rtmp->ep = ep;
    
    while(true)
    {
        struct sockaddr_in cli_addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        int32_t fd = accept4(rtmp->listenfd, (struct sockaddr*)&cli_addr, &addr_len, SOCK_NONBLOCK);
        if(fd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            break;
        }
        else if(fd < 0)
        {
            continue;
        }

        rns_rconn_t* rconn = rns_rconnserver_create(rtmp, fd);
        if(rconn == NULL)
        {
            close(fd);
        }
    }
}

static void read_func(rns_epoll_t* ep, void* data)
{
    rns_rconn_t* rconn = (rns_rconn_t*)data;
    
    while(true)
    {
        int32_t size = rns_rconn_read(rconn, rconn->buf->buf + rconn->buf->write_pos, rconn->buf->size - rconn->buf->write_pos);
        /* debug("read, size : %d, errno : %d, read pos : %d, write pos : %d\n", size, errno, rconn->buf->read_pos, rconn->buf->write_pos); */
        if(size < 0 && errno == EAGAIN)
        {
            write_func(ep, rconn);
            list_head_t* p = NULL;
            list_head_t* n = NULL;
            rns_list_for_each_safe(p, n, &rconn->rtmp->rconns)
            {
                rns_rconn_t* c = rns_list_entry(p, rns_rconn_t, list);
                write_func(ep, c);
                if(rns_list_empty(&c->tasks))
                {
                    rns_list_del_safe(&c->list);
                }
            }
            break;
        }
        else if(size == 0)
        {
            if(rconn->scb.close != NULL)
            {
                rconn->scb.close(rconn, rconn->scb.data);
            }
            rns_rconn_destroy(&rconn);
            
            return;
        }
        else if(size < 0)
        {
            if(rconn->scb.error != NULL)
            {
                rconn->scb.error(rconn, rconn->scb.data);
            }
            rns_rconn_destroy(&rconn);
            return;
        }
        else
        {
            rconn->buf->write_pos += size;
            uint32_t psize = rtmp_parse(rconn, rconn->buf->buf + rconn->buf->read_pos, rconn->buf->write_pos - rconn->buf->read_pos);
            /* if(rconn->type == RNS_RCONN_TYPE_PUBLISH) */
            /* { */
            /*     rns_file_write(fin, rconn->buf->buf + rconn->buf->read_pos, rconn->buf->write_pos - rconn->buf->read_pos); */
            /* } */

            rconn->buf->read_pos += psize;
            
            /* debug("rtmp parse, psize : %d, state : %d\n", psize, rconn->state); */


            
            rconn->buf->read_pos = 0;
            rconn->buf->write_pos = 0;
        }
    }
    return;
}
static void write_func(rns_epoll_t* ep, void* data)
{
    rns_rconn_t* rconn = (rns_rconn_t*)data;
    rns_sock_task_t* task = NULL;
    int32_t size = 0;
    
    while((task = rns_list_first(&rconn->tasks, rns_sock_task_t, list)) != NULL)
    {
        rns_list_del(&task->list);
        if(task->cb.done != NULL)
        {
            task->cb.done(rconn);
            rns_free(task);
            break;
        }
        
        if(task->cb.work == NULL)
        {
            continue;
        }
        
        size = task->cb.work(rconn, task->cb.data);
        if(size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            rns_list_add_first(&task->list, &rconn->tasks);
            break;
        }
        else if(size < 0)
        {
            if(task->cb.error != NULL)
            {
                task->cb.error(rconn, task->cb.data);
            }
            rns_free(task);
        }
        else if(size == 0)
        {
            if(task->cb.clean != NULL)
            {
                task->cb.clean(task->cb.data);
            }
            rns_free(task);
        }
        else if(size > 0)
        {
            rns_list_add_first(&task->list, &rconn->tasks);
        }
    }


    
    return;
}

//-------------------------------------------------------------------------------
rns_rtmp_t* rns_rtmpserver_create(uchar_t* app, rns_addr_t* addr, rns_rtmp_cb_t* cb)
{
    rns_rtmp_t* rtmp = (rns_rtmp_t*)rns_malloc(sizeof(rns_rtmp_t));
    if(rtmp == NULL)
    {
        return NULL;
    }

    INIT_LIST_HEAD(&rtmp->rconns);
    
    rtmp->ecb.read = accept_func;
    rtmp->ecb.write = NULL;

    rtmp->type = RNS_RTMP_TYPE_SERVER;
    
    rns_memcpy(&rtmp->addr, addr, sizeof(rns_addr_t));
    rns_memcpy(&rtmp->cb, cb, sizeof(rns_rtmp_cb_t));

    rbt_init(&rtmp->mstreams, RBT_TYPE_STR);
    rbt_init(&rtmp->nstreams, RBT_TYPE_INT);
    
    rtmp->listenfd = rns_server_init(&rtmp->addr);
    if(rtmp->listenfd < 0)
    {
        rns_free(rtmp);
        return NULL;
    }

    rtmp->app = rns_malloc(rns_strlen(app) + 1);
    if(rtmp->app == NULL)
    {
        close(rtmp->listenfd);
        rns_free(rtmp);
        return NULL;
    }

    rns_memcpy(rtmp->app, app, rns_strlen(app));

    return rtmp;
}

void rns_rtmp_destroy(rns_rtmp_t** rtmp)
{
    if(*rtmp == NULL)
    {
        return;
    }

    close((*rtmp)->listenfd);
    (*rtmp)->listenfd = -1;
    if((*rtmp)->app != NULL)
    {
        rns_free((*rtmp)->app);
    }
    
    rns_free(*rtmp);
    *rtmp = NULL;

    return;
}

rns_rtmp_msg_stream_t* rns_rtmp_msg_stream_create(rns_rtmp_t* rtmp, uchar_t* name)
{
    rns_rtmp_msg_stream_t* stream = (rns_rtmp_msg_stream_t*)rns_malloc(sizeof(rns_rtmp_msg_stream_t));
    if(stream == NULL)
    {
        return NULL;
    }
    INIT_LIST_HEAD(&stream->msgs);
    INIT_LIST_HEAD(&stream->players);

    rbt_init(&stream->blocks, RBT_TYPE_INT);
    
    rbt_set_key_str(&stream->node, name);
    rbt_insert(&rtmp->mstreams, &stream->node);
    stream->name = rns_malloc(rns_strlen(name) + 1);
    if(stream->name == NULL)
    {
        rns_free(stream);
        return NULL;
    }
    rns_strncpy(stream->name, name, rns_strlen(name));

    
    uchar_t* ptr = NULL;
    int32_t ret = posix_memalign((void**)&ptr, 512, RTMP_BLOCK_SIZE * RTMP_BLOCK_NUM);
    if(ptr == NULL || ret < 0)
    {
        rns_free(stream);
        return NULL;
    }
    
    stream->mp = rns_mpool_init(ptr, RTMP_BLOCK_SIZE * RTMP_BLOCK_NUM, RTMP_BLOCK_SIZE);
    if(stream->mp == NULL)
    {
        rns_free(ptr);
        rns_free(stream);
        return NULL;
    }
    
    rns_strncpy(&stream->name, name, rns_strlen(name));
    
    return stream;
}

void rns_rtmp_msg_stream_destroy(rns_rtmp_msg_stream_t** stream)
{
    if(*stream == NULL)
    {
        return;
    }

    if((*stream)->mp != NULL)
    {
        rns_free((*stream)->mp);
    }
    
    if((*stream)->name != NULL)
    {
        rns_free((*stream)->name);
    }
    
    return;
}

rns_rtmp_chunkstream_t* rns_rtmp_chunkstream_create(uint32_t channel)
{
    rns_rtmp_chunkstream_t* chunkstream = (rns_rtmp_chunkstream_t*)rns_malloc(sizeof(rns_rtmp_chunkstream_t));
    if(chunkstream == NULL)
    {
        return NULL;
    }
    return chunkstream;
}

void rns_rtmp_chunkstream_destroy(rns_rtmp_chunkstream_t** chunkstream)
{
    if(*chunkstream == NULL)
    {
        return;
    }

    return;
}

//-----------------------------------------------------------------------------------------

rns_rconn_t* rns_rconnserver_create(rns_rtmp_t* rtmp, int32_t fd)
{
    rns_rconn_t* rconn = (rns_rconn_t*)rns_malloc(sizeof(rns_rconn_t));
    if(rconn == NULL)
    {
        return NULL;
    }

    rconn->ecb.read = read_func;
    rconn->ecb.write = write_func;
    rbt_init(&rconn->chunkstreams, RBT_TYPE_INT);
    
    rns_memcpy(&rconn->cb, &rtmp->cb, sizeof(rns_rtmp_cb_t));
    rconn->fd = fd;
    rconn->ep = rtmp->ep;
    rconn->rtmp = rtmp;

    rconn->ichunksize = 128;
    rconn->ochunksize = 128;
    INIT_LIST_HEAD(&rconn->tasks);
    
    rconn->state = RNS_RTMP_HANDSHAKE_C0;
    rconn->CS0 = 3;

    rconn->videochunk = 4;
    rconn->audiochunk = 6;
    
    rconn->buf = rns_buf_create(RNS_HTTP_BUF_SIZE);
    if(rconn->buf == NULL)
    {
        rns_free(rconn);
        return NULL;
    }

    rconn->scb.close = rtmp_close_func;
    rconn->scb.error = rtmp_error_func;

    int32_t ret = rns_epoll_add(rtmp->ep, rconn->fd, rconn, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        rns_buf_destroy(&rconn->buf);
        rns_free(rconn);
        return NULL;
    }
    return rconn;
}

rns_rconn_t* rns_rconnclient_create(rns_rtmp_t* rtmp)
{
    rns_rconn_t* rconn = (rns_rconn_t*)rns_malloc(sizeof(rns_rconn_t));
    if(rconn == NULL)
    {
        return NULL;
    }

    return rconn;
}


void rns_rconn_destroy(rns_rconn_t** rconn)
{
    if(*rconn == NULL)
    {
        return;
    }

    rns_list_del_safe(&(*rconn)->list);
    rns_list_del_safe(&(*rconn)->player);
    
    close((*rconn)->fd);
    rns_buf_destroy(&(*rconn)->buf);

    rns_free(*rconn);
    *rconn = NULL;
    
    return;
}

int32_t rns_rconn_read(rns_rconn_t* rconn, uchar_t* buf, int32_t size)
{
    return recv(rconn->fd, buf, size, 0);
}

int32_t rns_rconn_write(rns_rconn_t* rconn, uchar_t* buf, int32_t size)
{
    return send(rconn->fd, buf, size, 0);
}

//------------------------------------------------------------------


