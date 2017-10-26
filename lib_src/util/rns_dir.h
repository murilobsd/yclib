/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_dir.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-18 15:58:13
 * -------------------------
**************************************************************************/

#ifndef _RNS_DIR_H_
#define _RNS_DIR_H_

#include "comm.h"

#include <sys/stat.h>
#include <sys/types.h>

// uchar_t* rns_dir_find(uchar_t* dir, uchar_t* name);
int32_t rns_dir_create(uchar_t* path, uint32_t len, mode_t mode);

#endif
