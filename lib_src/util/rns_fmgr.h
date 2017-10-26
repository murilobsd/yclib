/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_fmgr.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-15 10:47:50
 * -------------------------
**************************************************************************/

#ifndef _RNS_FMGR_H_
#define _RNS_FMGR_H_

#include "comm.h"
#include "rns_rbt.h"
#include "rns_file.h"

typedef struct rns_finfo_s
{
    rbt_node_t node;
    uchar_t* file;
    rns_file_t* fp;
}rns_finfo_t;

typedef struct rns_fmgr_s
{
    rbt_root_t root;
    rns_file_t* fp;
}rns_fmgr_t;

rns_fmgr_t* rns_fmgr_load(uchar_t* mgr_file);
void rns_fmgr_close(rns_fmgr_t** fmgr);
rns_finfo_t* rns_fmgr_add(rns_fmgr_t* fmgr, uchar_t* file, uint64_t key);
int32_t rns_fmgr_del(rns_fmgr_t* fmgr, uint64_t key);
rns_finfo_t* rns_fmgr_search(rns_fmgr_t* fmgr, uint64_t key);
rns_finfo_t* rns_fmgr_next(rns_finfo_t* finfo);
uint64_t rns_fmgr_key(rns_finfo_t* finfo);

#endif

