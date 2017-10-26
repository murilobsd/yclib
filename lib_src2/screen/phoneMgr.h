/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : phoneMgr.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-07-24 15:02:32
 * -------------------------
**************************************************************************/

#ifndef _PHONEMGR_H_
#define _PHONEMGR_H_

#include "list_head.h"
#include "rns_mysql.h"
#include "rns_http.h"
#include "stbMgr.h"

#define  MAX_PHONEHASH_NUM  1024
#define  PHONEDEV_MEMSIZE  256
#define  PHONEDEV_MEMNUM   128*1024

typedef struct phonedev
{
    rbt_node_t node;  // 加 绑定机顶盒的节点
    list_head_t list;
    uchar_t* phoneid;
    stbdev_t* stb;
    uchar_t* stbid;
    uint32_t time;    		// 绑定时间
}phonedev_t;

typedef struct phone_playhistory
{
    uchar_t* phoneid;
    uchar_t* media_id; 	
    int32_t  metatype;		
    uchar_t* title; 	
    int32_t  addtime;  
    int32_t  playsecond;    	
    int32_t  totaltime;					
    int32_t  serial;

	//fuxiang start
	uchar_t  * mobile;
	//fuxiang end

}phone_playhistory_t;
typedef phone_playhistory_t phone_mark_t;

typedef struct phone_mark_dao_s
{
    int32_t (*search)(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb);
    phone_mark_t* (*parse)(rns_mysql_t* mysql);
    int32_t (*insert)(rns_mysql_t* mysql, phone_mark_t* phone, rns_mysql_cb_t* cb);
    int32_t (*update)(rns_mysql_t* mysql, phone_mark_t* phone, rns_mysql_cb_t* cb);
    
    int32_t (*delete)(rns_mysql_t* mysql, uchar_t* phoneid, uchar_t* media_id, rns_mysql_cb_t* cb);
    
}phone_mark_dao_t;

typedef struct phone_playhistory_dao_s
{
    int32_t (*search)(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb);
    phone_playhistory_t* (*parse)(rns_mysql_t* mysql);
    int32_t (*insert)(rns_mysql_t* mysql, phone_playhistory_t* phone, rns_mysql_cb_t* cb);		
    int32_t (*update)(rns_mysql_t* mysql, phone_playhistory_t* phone, rns_mysql_cb_t* cb);
}phone_playhistory_dao_t;

typedef struct phone_dao_s
{
    int32_t (*search)(rns_mysql_t* mysql, uchar_t* phoneid, rns_mysql_cb_t* cb);
    int32_t (*search_all)(rns_mysql_t* mysql, rns_mysql_cb_t* cb);
    int32_t (*search_byid)(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb);
	int32_t (*search_byuserid)(rns_mysql_t* mysql, char_t* userid, rns_mysql_cb_t* cb);
	int32_t (*search_bystbid)(rns_mysql_t* mysql, char_t* stbid, rns_mysql_cb_t* cb);
    
    phonedev_t* (*parse)(rns_mysql_t* mysql);
    int32_t (*delete_byid)(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb);
    int32_t (*delete_byid2)(rns_mysql_t* mysql, uchar_t* phoneid, rns_mysql_cb_t* cb);
    int32_t (*update_byid)(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb);
    int32_t (*insert)(rns_mysql_t* mysql, phonedev_t* phone, rns_mysql_cb_t* cb);
    int32_t (*insert2)(rns_mysql_t* mysql, uchar_t* phoneid, uchar_t* stbid, rns_mysql_cb_t* cb);
    void (*destroy)(phonedev_t** phone);
}phone_dao_t;

phonedev_t* phone_create(uchar_t* phoneid);
void phone_destroy(phonedev_t** phone);
void phone_dao_init(phone_dao_t* dao);
void phone_playhistory_dao_init(phone_playhistory_dao_t* dao);
void phone_mark_dao_init(phone_mark_dao_t* dao);

phone_playhistory_t* phone_playhistory_create(uchar_t* buf, uint32_t size);
phone_mark_t* phone_mark_create(uchar_t* buf, uint32_t size);
phone_playhistory_t* phone_create_playhistory(uchar_t* phoneid, uchar_t* media_id, int32_t metatype, uchar_t* title, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial);
//fuxaing start
phone_playhistory_t* phone_create_playhistory1(uchar_t* phoneid, uchar_t* media_id,uchar_t * mobile, int32_t metatype, uchar_t* title, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial);

//fuxiang end
void phone_playhistory_destroy(phone_playhistory_t** phistory);
void phone_mark_destroy(phone_mark_t** mark);


#endif




