#ifndef _STB_MRG_
#define _STB_MRG_

#include "list_head.h"
#include "rns_mysql.h"
#include "rns_http.h"
#define  MAX_STBHASH_NUM 4096
#define  STBDEV_MEMSIZE  256*2
#define  STBDEV_MEMNUM   512*1024

typedef struct screen_info_s
{
    uchar_t* SDcontentid;
    uchar_t* HDcontentid;
    uchar_t* content;
    uchar_t* screenCode;
    uchar_t* name;     //媒资名字
    uchar_t* media_id;    //媒资id
    uchar_t* provider_id;   //媒资provider id
    int32_t status;
    uchar_t* phoneid;
    int32_t time;

    int32_t keycode;
    int32_t delay; 	
    int32_t metatype;
    int32_t serial;
    int32_t totaltime;

	//fuxiang start
	uchar_t * mobile;
	//fuxiang end
    
}screen_info_t;

typedef struct stbdev
{
    rbt_node_t node;   		//加链结点
    list_head_t phones;  //绑定的手机列表
    screen_info_t* sinfo;
    uint32_t linktime;
    void* timer;
    uchar_t* version;
    uchar_t* StbUserId; 	//iptv 业务账号
    uchar_t* StbModel;		//机顶盒型号
    uchar_t* StbCom; 	//机顶盒厂商
    uchar_t* StbMac;  	//机顶盒mac地址
    uchar_t* stbid;    	// 机顶盒ID
    int32_t StbVolume;					//机顶盒音量
    uchar_t* trsid;
    rns_ctx_t* screenctx;
}stbdev_t;

typedef struct stb_conn_s
{
    rbt_node_t snode;
    rbt_node_t hnode;
    rns_hconn_t* hconn;
}stb_conn_t;

struct trs_s;

typedef struct stb_playhistory
{
    uchar_t* stb_id;
    uchar_t* user_id; 	
    int32_t metatype;		
    uchar_t* screencode; 	
    int32_t addtime;  
    int32_t playsecond;    	
    int32_t  totaltime;					
    int32_t  serial;
    int32_t id;

	//fuxiang start
	uchar_t  * mobile;  //手机号
	//fuxiang end
}stb_playhistory_t;

typedef stb_playhistory_t stb_mark_t;


typedef struct stb_mark_dao_s
{
    int32_t (*search)(rns_mysql_t* mysql, char_t* StbUserId, rns_mysql_cb_t* cb);
    stb_mark_t* (*parse)(rns_mysql_t* mysql);
    int32_t (*insert)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb);
    int32_t (*update)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb);
    int32_t (*delete)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb);
    int32_t (*fetch)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb, int32_t limit1, int32_t limit2);
}stb_mark_dao_t;

typedef struct stb_playhistory_dao_s
{
    int32_t (*search)(rns_mysql_t* mysql, char_t* StbUserId, rns_mysql_cb_t* cb);
    stb_playhistory_t* (*parse)(rns_mysql_t* mysql);
    int32_t (*insert)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb);
    int32_t (*update)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb);
    int32_t (*fetch)(rns_mysql_t* mysql, stb_playhistory_t* stb, rns_mysql_cb_t* cb, int32_t limit1, int32_t limit2);
}stb_playhistory_dao_t;

typedef struct stb_dao_s
{
    int32_t (*search)(rns_mysql_t* mysql, uchar_t* stbid, rns_mysql_cb_t* cb);
    int32_t (*search_all)(rns_mysql_t* mysql, uchar_t* trsid, rns_mysql_cb_t* cb);
    int32_t (*search_bystbid)(rns_mysql_t* mysql, stbdev_t* stb, rns_mysql_cb_t* cb);
    int32_t (*search_byuserid)(rns_mysql_t* mysql, char_t* stb, rns_mysql_cb_t* cb);
    stbdev_t* (*parse)(rns_mysql_t* mysql);
    int32_t (*update_bystbid)(rns_mysql_t* mysql, uchar_t* trsid, stbdev_t* stb, rns_mysql_cb_t* cb);
    int32_t (*update_info)(rns_mysql_t* mysql, stbdev_t* stb, rns_mysql_cb_t* cb);
    int32_t (*insert)(rns_mysql_t* mysql, uchar_t* trsid, stbdev_t* stb, rns_mysql_cb_t* cb);
    int32_t (*registry)(rns_mysql_t* mysql, uchar_t* stbid, uchar_t* trsid, rns_mysql_cb_t* cb);
    void (*destroy)(stbdev_t** stb);
    
}stb_dao_t;

stbdev_t* stb_create(uchar_t* stbid, uchar_t* version, uchar_t* StbModel, uchar_t* StbCom, uchar_t* StbMac, uchar_t* StbUserId, uint32_t StbVolume, uchar_t* trsid);
stbdev_t* stb_create2(uchar_t* buf, uint32_t size);
void stb_destroy(stbdev_t** stb);
void stb_dao_init(stb_dao_t* dao);
void stb_playhistory_dao_init(stb_playhistory_dao_t* dao);
void stb_mark_dao_init(stb_mark_dao_t* dao);
stb_playhistory_t* stb_playhistory_create(uchar_t* buf, uint32_t size);
stb_mark_t* stb_mark_create(uchar_t* buf, uint32_t size);
stb_mark_t* stb_mark_create2(uchar_t* stb_id, uchar_t* user_id, int32_t metatype, uchar_t* screencode, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial);
stb_playhistory_t* stb_create_playhistory(uchar_t* stb_id, uchar_t* user_id, int32_t metatype, uchar_t* screencode, int32_t addtime, int32_t  playsecond, int32_t  totaltime, int32_t  serial);
void stb_destroy_playhistory(stb_playhistory_t **stb);
screen_info_t* scree_info_create(uchar_t* buf, uint32_t size);
void screen_info_destroy(screen_info_t** si);

#endif



