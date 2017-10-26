/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_memory.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2016-12-18 21:08:04
 * -------------------------
**************************************************************************/


#ifndef _RNS_MEMORY_H
#define _RNS_MEMORY_H

#include "comm.h"
#include "list_head.h"
#include "rns_rbt.h"
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#define rns_strncpy(dst, src, size)     strncpy((char_t*)(dst), (char_t*)(src), size)
#define rns_strlen(str)  strlen((char_t*)(str))
#define rns_strncmp(src, dst, size)   strncmp((char_t*)(src), (char_t*)(dst), size)
#define rns_strcmp(src, dst)   strcmp((char_t*)(src), (char_t*)(dst))
#define rns_strncasecmp(src, dst, size) strncasecmp((char_t*)(src), (char_t*)(dst), size)
#define rns_strstr(str, sub)  (uchar_t*)strstr((char_t*)(str), (char_t*)(sub))
#define rns_strchr(str, delim)   (uchar_t*)strchr((char_t*)(str), (char_t)delim)
#define rns_strrchr(str, delim)   (uchar_t*)strrchr((char_t*)(str), (char_t)delim)
#define rns_memset(dst, size)  memset((uchar_t*)(dst), 0, size)
#define rns_memcmp(addr1, addr2, size)  memcmp((void*)(addr1), (void*)(addr2), size)
#define rns_malloc(size)       (uchar_t*)calloc(1, size)
#define rns_calloc(num, size)       (void*)calloc(num, size)
#define rns_free(ptr)          free((void*)(ptr))        
#define rns_memcpy(dst, src, size) memcpy((void*)(dst), (void*)(src), size)
#define rns_memmove(dst, src, size) memmove((void*)(dst), (void*)(src), size)
#define rns_atol(str)           atol((char_t*)(str))
#define rns_atoi(str)           atoi((char_t*)(str))
#define rns_strtok(str, delim)  (uchar_t*)strtok((char_t*)(str), (char_t*)(delim))
    
#define rns_mem_forwad(curr, offset) ((uchar_t*)(curr) + offset)

rns_node_t* rns_split_key_value(uchar_t* str, uchar_t delim);
uchar_t* rns_remalloc(uchar_t** ptr, uint32_t size);
uchar_t* rns_dup(uchar_t* str);

#endif
