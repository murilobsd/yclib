/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : comm.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-18 15:16:51
 * -------------------------
**************************************************************************/

#ifndef _COMM_H_
#define _COMM_H_

#include <stdint.h>
#include <stddef.h>


#define RNS_STATE_INVALID 0
#define RNS_STATE_VALID   1

#ifndef char_t 
#define char_t char
#endif
#ifndef uchar_t
#define uchar_t unsigned char
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                              \
            const __typeof__( ((type *)0)->member ) *__mptr = (ptr);    \
            (type *)( (char *)__mptr - offsetof(type,member) );})
#endif


#undef NULL  
#if defined(__cplusplus)  
#define NULL 0  
#else  
#define NULL ((void *)0)  
#endif  

#undef true
#define true 1
#undef false
#define false 0


typedef struct rns_addr_s
{
    uint32_t ip;
    uint32_t port;
}rns_addr_t;

typedef struct rns_data_s
{
    void* data;
    uint32_t size;
    uint32_t offset;
}rns_data_t;
#endif

