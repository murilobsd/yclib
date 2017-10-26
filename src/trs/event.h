/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : event.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-07-24 12:42:43
 * -------------------------
**************************************************************************/

#ifndef _EVENT_H_
#define _EVENT_H_

#include "rns.h"
#include "rns_http.h"
#include "stbMgr.h"
#include "phoneMgr.h"
#include "trs_comm.h"
#include "rns_dbpool.h"

void phone_bindStb(rns_hconn_t* hconn, rns_dbpool_t* dbpool, rns_http_t* foss, phonedev_t* phone, stbdev_t* stb, uint32_t flag);

void phone_unbindStb(rns_hconn_t* hconn, rns_dbpool_t* dbpool, rns_http_t* foss, phonedev_t* phone, uint32_t flag);

void stb_bindphonelist(rns_hconn_t* hconn, stbdev_t* stb);

void phone_screen(rns_hconn_t* hconn, phonedev_t* phone, screen_info_t* si, int32_t status, int32_t t);
void phone_control(rns_hconn_t* hconn, stbdev_t* stb);
void phone_playhistory_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, phone_playhistory_t* phistory);

void stb_screen(rns_hconn_t* hconn, trs_t* trs, stbdev_t* stb);

void stb_mark_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stb_mark_t* mark);
void stb_mark_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, trs_t* trs, char_t* StbUserId, int limit1, int limit2 );
void stb_mark_delete(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stb_mark_t* mark);
void stb_playhistory_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stb_playhistory_t* shistory);
void stb_ps_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, trs_t* trs, char_t* StbUserId, int limit1, int limit2 );

void phone_mark_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, phone_mark_t* mark);
void phone_mark_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, char_t* StbUserId, int limit1, int limit2 );
void phone_mark_delete(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* phoneid, uchar_t* media_id);
void phone_playhistory_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, phone_playhistory_t* phistory);
void phone_playhistory_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, char_t* StbUserId, int limit1, int limit2 );


void stb_saveInfo(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* trsid, stbdev_t* stb, uint32_t volume);
void stb_sendInfo(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stbdev_t* stb);
void stb_screeninfo(rns_hconn_t* hconn, stbdev_t* stb);
void stb_switchUser(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* trsid, stbdev_t* stb, uchar_t* StbUserId);


#endif
