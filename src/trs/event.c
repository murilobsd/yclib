#include "event.h"
#include "rns_memory.h"
#include "rns_hash.h"
#include "stbMgr.h"
#include "phoneMgr.h"
#include "rns_dbpool.h"
#include "trs_comm.h"
#include "rns.h"
#include "rns_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rns_json.h"
#include <errno.h>


// 1. control 233 
// 2. getScreenInfo hold

void resp_error(rns_hconn_t* hconn)
{
    int32_t ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    return;
}


int32_t foss_bind_resp(rns_hconn_t* hconn, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    
    rns_hconn_t* c = (rns_hconn_t*)rns_ctx_get(ctx, "hconn");
    if(c == NULL)
    {
        goto ERR_EXIT;
    }
    phonedev_t* phone = (phonedev_t*)rns_ctx_get(ctx, "phone");
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }
    stbdev_t* stb = (stbdev_t*)rns_ctx_get(ctx, "stb");
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
    uint32_t* flag = (uint32_t*)rns_ctx_get(ctx, "flag");
    if(flag == NULL)
    {
        LOG_INFO(lp, "bind failed, stbid : %s, phoneid : %s", stb->stbid, phone->phoneid);
        goto ERR_EXIT;
    }
    
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    int32_t ret = 0;
    if(info->code != 200)
    {
        LOG_INFO(lp, "foss bind failed, stbid : %s, phoneid : %s", stb->stbid, phone->phoneid);
        goto ERR_EXIT;
    }

    time_t now = time(NULL);
    
    if(phone->stb != NULL)
    {
        rns_list_del_safe(&phone->list);
        phone->stb = NULL;
    }
    
    rns_list_add(&phone->list, &stb->phones);
    phone->stb = stb;
    phone->stbid = rns_dup(stb->stbid);
    phone->time = now;

    if(*flag)
    {
        LOG_INFO(lp, "bind success, stbid : %s, phoneid : %s", stb->stbid, phone->phoneid);
        ret = rns_hconn_resp2(c, 200, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp failed, ret : %d", ret);
        }
    }
    rns_ctx_destroy(&ctx);
    return 0;
    
ERR_EXIT:
    ret = rns_hconn_resp2(c, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp failed, ret : %d", ret);
    }
    rns_ctx_destroy(&ctx);
    return -1;
}

int32_t foss_unbind_resp(rns_hconn_t* hconn, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    
    rns_hconn_t* c = (rns_hconn_t*)rns_ctx_get(ctx, "hconn");
    if(c == NULL)
    {
        goto ERR_EXIT;
    }
    phonedev_t* phone = (phonedev_t*)rns_ctx_get(ctx, "phone");
    if(phone == NULL)
    {
        goto ERR_EXIT;
    }
    uint32_t* flag = (uint32_t*)rns_ctx_get(ctx, "flag");
    if(flag == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    int32_t ret = 0;
    if(info->code != 200)
    {
        LOG_INFO(lp, "foss unbind failed, phoneid : %s", phone->phoneid);
        goto ERR_EXIT;
    }
    
    time_t now = time(NULL);
    
    if(phone->stb != NULL)
    {
        rns_list_del_safe(&phone->list);
        phone->stb = NULL;
        if(phone->stbid != NULL)
        {
            rns_free(phone->stbid);
            phone->stbid = NULL;
        }
        
        phone->time = now;
    }


    if(*flag)
    {
        LOG_INFO(lp, "unbind success, phoneid : %s", phone->phoneid);
        ret = rns_hconn_resp2(c, 200, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp failed, ret : %d", ret);
        }
    }
    rns_ctx_destroy(&ctx);
    return 0;
    
ERR_EXIT:
    ret = rns_hconn_resp2(c, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp failed, ret : %d", ret);
    }
    rns_ctx_destroy(&ctx);
    return -1;
}

// update or insert cb
void done_func(rns_mysql_t* mysql, void* data)
{
    rns_hconn_t* hconn = (rns_hconn_t*)data;
    
    int32_t ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);
    return;
}
void error_func(rns_mysql_t* mysql, void* data)
{
    rns_hconn_t* hconn = (rns_hconn_t*)data;
    
    int32_t ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);
    return;
}

void error_ctx_func(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    uint32_t* v = rns_ctx_get(ctx, "volume");
    
    LOG_ERROR(lp, "save info failed");
    int32_t ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);
    if(v != NULL)
    {
        rns_free(v);
    }
    rns_ctx_destroy(&ctx);
    return;
}



void phone_bindStb(rns_hconn_t* hconn, rns_dbpool_t* dbpool, rns_http_t* foss, phonedev_t* phone, stbdev_t* stb, uint32_t flag)
{
    int32_t ret = 0;
    int32_t retcode = 0;
    rns_ctx_t* ctx = NULL;
    rns_json_t* body = rns_json_create_obj();
    rns_json_add_str(body, (uchar_t*)"stbid", stb->stbid);
    rns_json_add_str(body, (uchar_t*)"StbMac", stb->StbMac);
    rns_json_add_str(body, (uchar_t*)"StbModel", stb->StbModel);
    rns_json_add_str(body, (uchar_t*)"StbCom", stb->StbCom);
    rns_json_add_str(body, (uchar_t*)"StbUserId", stb->StbUserId);
    rns_json_add_str(body, (uchar_t*)"phoneid", phone->phoneid);
    
    uchar_t* b = rns_json_write(body);
    if(b == NULL)
    {
        retcode = -7;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        retcode = -7;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    ret = rns_ctx_add(ctx, "phone", phone);
    if(ret < 0)
    {
        retcode = -8;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    ret = rns_ctx_add(ctx, "stb", stb);
    if(ret < 0)
    {
        retcode = -8;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    ret = rns_ctx_add(ctx, "hconn", hconn);
    if(ret < 0)
    {
        retcode = -8;
        LOG_ERROR(lp, "");
        goto EXIT;
    }

    uint32_t* f = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(f == NULL)
    {
        retcode = -7;
        goto EXIT;
    }
    *f = flag;
    rns_ctx_add(ctx, "flag", f);
    
    rns_http_cb_t fcb;
    rns_memset(&fcb, sizeof(rns_http_cb_t));
    fcb.work = foss_bind_resp;
    fcb.close = NULL;
    fcb.error = NULL;
    fcb.data = ctx;
    
    rns_hconn_t* h = rns_hconn_get(foss, &fcb);
    if(hconn == NULL)
    {
        retcode = -9;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    ret = rns_hconn_req2(h, (uchar_t*)"POST", (uchar_t*)"/Foss/bind", b, rns_strlen(b), 1, 1);
    if(ret < 0)
    {
        retcode = -1;
        LOG_ERROR(lp, "");
        goto EXIT;
    }

    
EXIT:
    if(retcode < 0)
    {
        ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp failed, ret : %d", ret);
        }
        rns_free(b);
        if(ctx != NULL)
        {
            rns_free(ctx);
        }
    }
    rns_json_destroy(&body);
    return;
   
}

void phone_unbindStb(rns_hconn_t* hconn, rns_dbpool_t* dbpool, rns_http_t* foss, phonedev_t* phone, uint32_t flag)
{
    int32_t ret = 0;
    int32_t retcode = 0;
    rns_ctx_t* ctx = NULL;
    uchar_t* b = NULL;
    rns_json_t* body = rns_json_create_obj();
    
    if(phone->stb == NULL)
    {
        retcode = -1;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    ret = rns_json_add_str(body, (uchar_t*)"stbid", phone->stb->stbid);
    if(ret < 0)
    {
        retcode = -1;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    ret = rns_json_add_str(body, (uchar_t*)"phoneid", phone->phoneid);
    if(ret < 0)
    {
        retcode = -6;
        LOG_ERROR(lp, "");
        goto EXIT;
    }

    
    b = rns_json_write(body);
    if(b == NULL)
    {
        retcode = -7;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        retcode = -7;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    ret = rns_ctx_add(ctx, "phone", phone);
    if(ret < 0)
    {
        retcode = -8;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    ret = rns_ctx_add(ctx, "hconn", hconn);
    if(ret < 0)
    {
        retcode = -8;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    uint32_t* f = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(f == NULL)
    {
        retcode = -7;
        goto EXIT;
    }
    *f = flag;
    rns_ctx_add(ctx, "flag", f);
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work = foss_unbind_resp;
    cb.close = NULL;
    cb.error = NULL;
    cb.data = ctx;
    
    rns_hconn_t* h = rns_hconn_get(foss, &cb);
    if(hconn == NULL)
    {
        retcode = -9;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    ret = rns_hconn_req2(h, (uchar_t*)"POST", (uchar_t*)"/Foss/unbind", b, rns_strlen(b), 1, 1);
    if(ret < 0)
    {
        retcode = -1;
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
EXIT:
    if(retcode < 0)
    {
        ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp failed, ret : %d", ret);
        }
        rns_free(b);
    }
    rns_json_destroy(&body);
    return;
}

void stb_bindphonelist(rns_hconn_t* hconn, stbdev_t* stb)
{
    int32_t ret = 0;
    list_head_t *lh = NULL;
    phonedev_t *phone = NULL;

    rns_json_t* array = rns_json_create_array();
    rns_list_for_each(lh, &stb->phones)
    {
        phone = rns_list_entry(lh, phonedev_t, list);
        rns_json_t* p = rns_json_create_obj();
        if(p == NULL)
        {
            LOG_ERROR(lp, "json add str failed, phone id : %s", phone->phoneid);
        }
        ret = rns_json_add_str(p, (uchar_t*)"phoneid", phone->phoneid);
        if(ret < 0)
        {
            LOG_ERROR(lp, "json add str failed, phone id : %s", phone->phoneid);
        }
        rns_json_add(array, NULL, p);
    }

    uchar_t* buf = rns_json_write(array);
    
    if(buf == NULL || !rns_json_array_size(array))
    {
        LOG_INFO(lp, "the stb doesn't bind phone, stbid : %s", stb->stbid);
        ret = rns_hconn_resp2(hconn, 401, NULL, 0, 0);
    }
    else
    {
        LOG_INFO(lp, "the stb bind phone, stbid : %s", stb->stbid);
        ret = rns_hconn_resp2(hconn, 200, buf, rns_strlen(buf), 1);
    }
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }

    rns_json_destroy(&array);
    return;
}

void phone_screen(rns_hconn_t* hconn, phonedev_t* phone, screen_info_t *sinfo, int32_t status, int32_t t)
{
    uint32_t retcode = 200;
    int32_t  ret = 0;
	
    if(phone->stb == NULL)
    {
        retcode = 404;
        goto ERR_EXIT;
    }

    if(phone->stb->sinfo != NULL)
    {
        screen_info_destroy(&phone->stb->sinfo);
    }
    phone->stb->sinfo = sinfo;

    if(sinfo->keycode == 233)
    {
        phone->stb->StbVolume = rns_atoi(sinfo->content);
    }
   
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    return;
ERR_EXIT:
    retcode = 404;
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    return;
}

void stb_screen_resp(void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    stbdev_t* stb = rns_ctx_get(ctx, "stb");
    trs_t* trs = rns_ctx_get(ctx, "trs");
    if(hconn == NULL || stb == NULL)
    {
        return;
    }
    LOG_DEBUG(lp, "stb timeout : %s", stb->stbid);
    rns_timer_delete(trs->ep, stb->timer);
    stb->timer = NULL;
    
    rbt_node_t* hnode = rbt_search_int(&trs->hconns, (uint64_t)hconn);
    if(hnode != NULL)
    {
        stb_conn_t* stbconn = rbt_entry(hnode, stb_conn_t, hnode);
        rbt_delete(&trs->hconns, &stbconn->hnode);
        rbt_delete(&trs->stbs, &stbconn->snode);
        rns_free(stbconn);
    }
    int32_t ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2  failed");
    }
    rns_ctx_destroy(&stb->screenctx);
    return;
}

void stb_screen(rns_hconn_t* hconn, trs_t* trs, stbdev_t* stb)
{
    uint32_t retcode = 200;
    int32_t  ret = 0;
    rns_json_t* obj = NULL;
    uchar_t* b = NULL;
    
    stb->linktime = time(NULL);
    
    if(stb->sinfo == NULL)
    {
        if(stb->timer == NULL)
        {
            stb->screenctx = rns_ctx_create();
            if(stb->screenctx == NULL)
            {
                retcode = 404;
                goto ERR_EXIT;
            }
            
            rns_ctx_add(stb->screenctx, "hconn", hconn);
            rns_ctx_add(stb->screenctx, "stb", stb);
            rns_ctx_add(stb->screenctx, "trs", trs);
            
            rns_cb_t cb;
            cb.func = stb_screen_resp;
            cb.data = stb->screenctx;
            stb->timer = rns_timer_add(trs->ep, 20000, &cb);
            if(stb->timer == NULL)
            {
                LOG_ERROR(lp, "set timer to http failed, ret : %d, errno : %d", ret, errno);
                retcode = 404;
                goto ERR_EXIT;
            }
        }
        else
        {
            LOG_DEBUG(lp, "timer exist, %x", stb->timer);
        }
        return;
    }
    
    obj = rns_json_create_obj();
    if(obj == NULL)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
   
    
    rns_json_add_str(obj, (uchar_t*)"HDcontentid", stb->sinfo->HDcontentid);
    rns_json_add_str(obj, (uchar_t*)"SDcontentid", stb->sinfo->SDcontentid);
    rns_json_add_int(obj, (uchar_t*)"status", stb->sinfo->status);
    rns_json_add_int(obj, (uchar_t*)"time", stb->sinfo->time);
    rns_json_add_str(obj, (uchar_t*)"screenCode", stb->sinfo->screenCode);
    rns_json_add_int(obj, (uchar_t*)"serial", stb->sinfo->serial);
    rns_json_add_int(obj, (uchar_t*)"metaType", stb->sinfo->metatype);
    rns_json_add_str(obj, (uchar_t*)"name", stb->sinfo->name);
    rns_json_add_str(obj, (uchar_t*)"media_id", stb->sinfo->media_id);
    rns_json_add_str(obj, (uchar_t*)"provider_id", stb->sinfo->provider_id);
    //rns_json_add_str(obj, (uchar_t*)"remoteControlPhoneId", stb->sinfo->phoneid);
	//fuxaing start
	rns_json_add_str(obj, (uchar_t*)"remoteControlPhoneId", stb->sinfo->mobile);
	//fuxiang end
    rns_json_add_int(obj, (uchar_t*)"remoteControlKeycode", stb->sinfo->keycode);
    rns_json_add_int(obj, (uchar_t*)"remoteControlUtc", stb->sinfo->time);
    rns_json_add_str(obj, (uchar_t*)"remoteControlContent", stb->sinfo->content);
    
    
    b = rns_json_write(obj);
    if(b == NULL)
    {
        LOG_ERROR(lp, "stb screen resp json may be wrong %s", stb->stbid);
        retcode = 400;
        goto ERR_EXIT;
    }

    LOG_INFO(lp, "stb screen info %s", b);

    rbt_node_t* hnode = rbt_search_int(&trs->hconns, (uint64_t)hconn);
    if(hnode != NULL)
    {
        stb_conn_t* stbconn = rbt_entry(hnode, stb_conn_t, hnode);
        rbt_delete(&trs->hconns, &stbconn->hnode);
        rbt_delete(&trs->stbs, &stbconn->snode);
        rns_free(stbconn);
    }
    
    ret = rns_hconn_resp2(hconn, retcode, b, rns_strlen(b), 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2  failed");
    }
    
    rns_timer_delete(trs->ep, stb->timer);
    stb->timer = NULL;
    screen_info_destroy(&stb->sinfo);
    rns_json_destroy(&obj);
    rns_ctx_destroy(&stb->screenctx);
    
    return;
ERR_EXIT:
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2  failed");
    }
    if(b != NULL)
    {
        rns_free(b);
    }
    rns_ctx_destroy(&stb->screenctx);
    rns_timer_delete(trs->ep, stb->timer);
    stb->timer = NULL;
    rns_json_destroy(&obj);
    return;
}

void stb_saveInfo_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    uint32_t retcode = 200;
    uint32_t* v = NULL;
    stbdev_t* stb = NULL;
    int32_t ret = 0;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        retcode = 400;
        LOG_ERROR(lp, "get hconn from ctx failed");
        goto EXIT;
    }

    v = rns_ctx_get(ctx, "volume");
    if(v == NULL)
    {
        retcode = 400;
        LOG_ERROR(lp, "get hconn from ctx failed");
        goto EXIT;
    }

    stb = rns_ctx_get(ctx, "stb");
    if(stb == NULL)
    {
        retcode = 400;
        LOG_ERROR(lp, "get hconn from ctx failed");
        goto EXIT;
    }
    
    stb->StbVolume = *v;


EXIT:
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    if(v != NULL)
    {
        rns_free(v);
    }
    rns_ctx_destroy(&ctx);
    rns_dbpool_free(mysql);
    return;
}

void stb_saveInfo(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* trsid, stbdev_t* stb, uint32_t volume)
{
    int  retcode = 0;
    int  ret = 0;
    rns_ctx_t* ctx = NULL;
    uint32_t* v = NULL;
    rns_mysql_t *mysql = rns_dbpool_get(dbpool);
    if(NULL == mysql)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        retcode = 404;
        goto ERR_EXIT;
    }

    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        LOG_ERROR(lp, "ran out of memory");
        retcode = 404;
        goto ERR_EXIT;
    }
    v = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(v == NULL)
    {
        LOG_ERROR(lp, "ran out of memory");
        retcode = 404;
        goto ERR_EXIT;
    }
    *v = volume;
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "volume", v);
    rns_ctx_add(ctx, "stb", stb);
    
    rns_mysql_cb_t cb;
    cb.work  = NULL;
    cb.done  = stb_saveInfo_done;
    cb.error = error_ctx_func;
    cb.data  = ctx;
	
    stb_dao_t stbdao;
    stb_dao_init(&stbdao);

    ret = stbdao.update_bystbid(mysql, trsid, stb, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto ERR_EXIT;
    }	
    
    return;
ERR_EXIT:	
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);
    if(v != NULL)
    {
        rns_free(v);
    }
    rns_ctx_destroy(&ctx);

    return;

}

void stb_sendInfo(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stbdev_t* stb)
{
    int32_t retcode = 200;
    int32_t ret = 0;
    uchar_t* buf = NULL;
    uint32_t islink = 0;
    rns_json_t* obj = rns_json_create_obj();
    if(obj == NULL)
    {
        retcode = 404;
        goto EXIT;
    }

    ret = rns_json_add_str(obj, (uchar_t*)"stbid", stb->stbid);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add json str stbid : %s failed", stb->stbid);
        retcode = 404;
        goto EXIT;
    }
    ret = rns_json_add_int(obj, (uchar_t*)"volume", stb->StbVolume);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add json int volume : %d failed", stb->StbVolume);
        retcode = 404;
        goto EXIT;
    }
    if(time(NULL) - stb->linktime < 120)
    {
        islink = 1;
    }
    ret = rns_json_add_int(obj, (uchar_t*)"link", islink);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add json int link : %d failed", islink);
        retcode = 404;
        goto EXIT;
    }

    buf = rns_json_write(obj);
    if(buf == NULL)
    {
        LOG_ERROR(lp, "json write failed", islink);
        retcode = 404;
        goto EXIT;
    }

    LOG_DEBUG(lp, "stb status : \n%s", buf);

    ret = rns_hconn_resp2(hconn, retcode, buf, rns_strlen(buf), 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed, ret : %d", ret);
    }

    rns_json_destroy(&obj);
    return;
EXIT:
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed, ret : %d", ret);
    }
    if(buf != NULL)
    {
        rns_free(buf);
    }
    rns_json_destroy(&obj);
    return;
}

void stb_switchUser(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* trsid, stbdev_t* stb, uchar_t* StbUserId)
{
    int32_t ret = 0;
    
    if(stb->StbUserId != NULL)
    {
        rns_free(stb->StbUserId);
    }
    stb->StbUserId = rns_malloc(rns_strlen(StbUserId) + 1);
    memcpy(stb->StbUserId, StbUserId, rns_strlen(StbUserId));
    
    stb_dao_t stbdao;
    stb_dao_init(&stbdao);
    
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;
    
    ret = stbdao.update_bystbid(mysql, trsid, stb, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "update stb user failed, stbid : %s, new user : %s, mysql errstr : %s", stb->stbid, stb->StbUserId, rns_mysql_errstr(mysql));
        goto ERR_EXIT;
    }	
    
    return;
    
ERR_EXIT:
    
    ret = rns_hconn_resp2(hconn, 404, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed, ret : %d", ret);
    }

    rns_dbpool_free(mysql);
    return;
    
}
//------------------------------stb playhistory------------------------------------------------
void id_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    uint32_t* tn = rns_ctx_get(ctx, "tnumber");
    if(tn == NULL)
    {
        return;
    }
    
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    
    *tn = *n;
    
    return;	
}

void stb_playhistory_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stb_playhistory_t* phistory)
{
    int32_t retcode = 200;
    int32_t ret = 0;
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;
    
    stb_playhistory_dao_t dao;
    stb_playhistory_dao_init(&dao);
    
    ret = dao.insert(mysql, phistory, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "insert fialed");
    }
    return;
    
ERR_EXIT:
    retcode = 404;
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);
    return;
    
}

void get_error(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    uint32_t retcode = 400;
    int32_t ret = 0;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);

    stbdev_t* stb = rns_ctx_get(ctx, "stb");
    stb_destroy(&stb);

    rns_ctx_destroy(&ctx);
    
    return;
}

void free_mysql(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    return;
}

void ps_get_work(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    trs_t* trs = rns_ctx_get(ctx, "trs");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    rns_json_t* array = rns_ctx_get(ctx, "array");
    if(hconn == NULL || trs == NULL || dbpool == NULL || array == NULL)
    {
        return;
    }
    
    stb_mark_dao_t dao;
    stb_mark_dao_init(&dao);
    
    stb_playhistory_t* ps = dao.parse(mysql);
    if(ps == NULL)
    {
        return;
    }
    
    
    rns_json_t* h = rns_json_create_obj();
    rns_json_add_str(h, (uchar_t*)"user_id", ps->user_id);
    rns_json_add_str(h, (uchar_t*)"screencode", ps->screencode);
    rns_json_add_int(h, (uchar_t*)"metatype", ps->metatype);
    rns_json_add_int(h, (uchar_t*)"addtime", ps->addtime);
    rns_json_add_int(h, (uchar_t*)"totaltime", ps->totaltime);
    rns_json_add_int(h, (uchar_t*)"serial", ps->serial);
    rns_json_add_int(h, (uchar_t*)"title", ps->id);
    
    rns_json_add(array, NULL, h);
    
    return;
    
}

void ps_get_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    trs_t* trs = rns_ctx_get(ctx, "trs");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    rns_json_t* array = rns_ctx_get(ctx, "array");
    uint32_t* n = rns_ctx_get(ctx, "number");
    
    if(hconn == NULL || trs == NULL || dbpool == NULL || array == NULL || n == NULL)
    {
        return;
    }
    
    --(*n);
    if(*n <= 0)
    {
        rns_json_t* jsonobj = rns_json_create_obj();
        rns_json_add(jsonobj, (uchar_t*)"plays", array);
        rns_json_add_int(jsonobj, (uchar_t*)"pagecount", rns_json_array_size(array));
        rns_json_add_int(jsonobj, (uchar_t*)"totalcount", rns_json_array_size(array));
        
        uchar_t* b = rns_json_write(jsonobj);
        
        int32_t ret = rns_hconn_resp2(hconn, 200, b, rns_strlen(b), 1);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp2 failed ret : %d", ret);
        }
        
        rns_ctx_destroy(&ctx);
        rns_json_destroy(&jsonobj);
        rns_dbpool_free(mysql);
    }
    
    rns_dbpool_free(mysql);
    
    
    return;
}

void ps_get(rns_mysql_t* mysql, void* data)
{
    int ret = 0;
    
    stb_dao_t dao;
    stb_dao_init(&dao);

    stb_playhistory_dao_t pdao;
    stb_playhistory_dao_init(&pdao);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    uint32_t* n = rns_ctx_get(ctx, "number");
    
    stbdev_t* stb = dao.parse(mysql);
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work  = ps_get_work;
    cb.done  = ps_get_done;
    cb.error = get_error;
    cb.data  = ctx;
    
    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        goto ERR_EXIT;
    }

    *n += 1;
    ret = pdao.search(m, (char_t*)stb->stbid, &cb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }

    return;
    
ERR_EXIT:
    
    rns_dbpool_free(mysql);
    return; 
}
void stb_get_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* h = rns_ctx_get(ctx, "hconn");
    uint32_t* n = rns_ctx_get(ctx, "number");
    uchar_t* userid = rns_ctx_get(ctx, "StbUserId");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");

    stb_playhistory_dao_t dao;
    stb_playhistory_dao_init(&dao);
    
    rns_mysql_cb_t cb;
    cb.work = ps_get_work;
    cb.done = ps_get_done;
    cb.error = get_error;
    cb.data  = ctx;

    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m != NULL)
    {
        *n += 1;
        int32_t ret = dao.search(m, (char_t*)userid, &cb);
        if(ret < 0)
        {
        }	
        
    }
    
    *n -= 1;
    if(*n == 0)
    {
        int32_t ret = rns_hconn_resp2(h, 200, NULL, 0, 0);
        if(ret < 0)
        {
            
        }
        rns_ctx_destroy(&ctx);
    }
    
    rns_dbpool_free(mysql);
}

int32_t stb_ps_update(rns_hconn_t* hconn, void* data)
{
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    trs_t* trs = rns_ctx_get(ctx, "trs");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    uchar_t* userid = rns_ctx_get(ctx, "StbUserId");
    rns_json_t* r = NULL;
    uint32_t i = 0;
    if(trs == NULL || dbpool == NULL)
    {
        return 0;
    }
    
    stb_dao_t dao;
    stb_dao_init(&dao);
    
    r = rns_json_read(info->body, info->body_size);
    if(r != NULL)
    {
        rns_json_t* d = rns_json_node(r, "data", NULL);
        if(d != NULL)
        {
            uint32_t number = rns_json_array_size(d);
            for(i = 0; i < number; ++i)
            {
                rns_json_t* o = rns_json_array_node(d, i);
                
                rns_json_t* screenCode = rns_json_node(o, "seriescode", NULL);
                rns_json_t* t = rns_json_node(o, "time", NULL);
                rns_json_t* pt = rns_json_node(o, "playtime", NULL);
                rns_json_t* mt = rns_json_node(o, "mediatype", NULL);
                
                if(screenCode == NULL || t == NULL)
                {
                    continue;
                }
                
                stb_playhistory_t* playhistory = stb_create_playhistory(userid, userid, mt->valueint, (uchar_t*)screenCode->valuestring, t->valueint, pt->valueint, 0, 0);
                stb_playhistory_dao_t pdao;
                stb_playhistory_dao_init(&pdao);
                
                rns_mysql_t* m = rns_dbpool_get(dbpool);
                if(m == NULL)
                {
                    continue;
                }
                rns_mysql_cb_t cb;
                rns_memset(&cb, sizeof(rns_mysql_cb_t));
                cb.done = free_mysql;
                cb.error = free_mysql;
                int32_t ret = pdao.insert(m, playhistory, &cb);
                if(ret < 0)
                {
                    rns_dbpool_free(m);
                    LOG_ERROR(lp, "");
                }
            }
        }
    }
    
    rns_json_destroy(&r);
    rns_free(userid);
    rns_ctx_destroy(&ctx);
    return info->body_size;
}

void stb_ps_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, trs_t* trs, char_t* StbUserId, int limit1, int limit2 )
{
    rns_mysql_t* mysql = NULL;
    rns_ctx_t* ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        goto ERR_EXIT;
    }

    rns_ctx_t* ctxhttp = rns_ctx_create();
    if(ctxhttp == NULL)
    {
        goto ERR_EXIT;
    }

    uint32_t* n = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(n == NULL)
    {
        goto ERR_EXIT;
    }
    *n = 0;
    rns_json_t* array = rns_json_create_array();
    if(array == NULL)
    {
        goto ERR_EXIT;
    }

    
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    rns_ctx_add(ctx, "trs", trs);
    rns_ctx_add(ctx, "StbUserId", StbUserId);
    rns_ctx_add(ctx, "number", n);
    rns_ctx_add(ctx, "array", array);

    uchar_t* userid = rns_dup((uchar_t*)StbUserId);
    rns_ctx_add(ctxhttp, "dbpool", dbpool);
    rns_ctx_add(ctxhttp, "trs", trs);
    rns_ctx_add(ctxhttp, "StbUserId", userid);
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work = stb_ps_update;
    cb.data = ctxhttp;
    
    rns_hconn_t* hcms = rns_hconn_get(trs->cms, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn failed");
        goto ERR_EXIT;
    }
    
    uchar_t buf[128];
    snprintf((char_t*)buf, 128 ,"/itv-api2/api/vod/playlist.json?uid=%s", StbUserId);
    
    
    int32_t ret = 0;
    ret = rns_hconn_req_header_init(hcms);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_req_header_insert(hcms, (uchar_t*)"Connection", (uchar_t*)"keepalive");
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_req_header_insert(hcms, (uchar_t*)"HOST", (uchar_t*)"117.71.39.8");
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_req_header(hcms, (uchar_t*)"GET", buf);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    ret = rns_hconn_req(hcms, NULL, 0, 0);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }

    mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t mcb;
    rns_memset(&cb, sizeof(rns_mysql_cb_t));
    mcb.work = ps_get;
    mcb.done = stb_get_done;
    mcb.error = get_error;
    mcb.data = ctx;

    stb_dao_t dao;
    stb_dao_init(&dao);

    *n += 1;
    ret = dao.search_byuserid(mysql, StbUserId, &mcb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }

    
    return;
    
ERR_EXIT:
    return;
}

//------------------------------stb mark ------------------------------------------------------
void stb_mark_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stb_mark_t* mark)
{
    uint32_t retcode = 200;
    int32_t ret = 0;
    
    stb_mark_dao_t dao;
    stb_mark_dao_init(&dao);
    
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;
    
    ret = dao.insert(mysql, mark, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "insert failed");
    }
    
    return;
    
ERR_EXIT:
    retcode = 404;
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    return;
    
}

void mark_get_work(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    trs_t* trs = rns_ctx_get(ctx, "trs");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    rns_json_t* array = rns_ctx_get(ctx, "array");
    if(hconn == NULL || trs == NULL || dbpool == NULL || array == NULL)
    {
        return;
    }

    stb_mark_dao_t dao;
    stb_mark_dao_init(&dao);

    stb_mark_t* smark = dao.parse(mysql);
    if(smark == NULL)
    {
        return;
    }
    
    rns_json_t* h = rns_json_create_obj();
    rns_json_add_str(h, (uchar_t*)"user_id", smark->user_id);
    rns_json_add_str(h, (uchar_t*)"screencode", smark->screencode);
    rns_json_add_int(h, (uchar_t*)"metatype", smark->metatype);
    rns_json_add_int(h, (uchar_t*)"addtime", smark->addtime);
    rns_json_add_int(h, (uchar_t*)"totaltime", smark->totaltime);
    rns_json_add_int(h, (uchar_t*)"serial", smark->serial);
    rns_json_add_int(h, (uchar_t*)"title", smark->id);

    rns_json_add(array, NULL, h);

    return;

}

void mark_get_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    trs_t* trs = rns_ctx_get(ctx, "trs");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    rns_json_t* array = rns_ctx_get(ctx, "array");
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(hconn == NULL || trs == NULL || dbpool == NULL || array == NULL || n == NULL)
    {
        return;
    }
    
    --(*n);
    if(*n <= 0)
    {
        rns_json_t* jsonobj = rns_json_create_obj();
        rns_json_add(jsonobj, (uchar_t*)"marks", array);
        rns_json_add_int(jsonobj, (uchar_t*)"pagecount", rns_json_array_size(array));
        rns_json_add_int(jsonobj, (uchar_t*)"totalcount", rns_json_array_size(array));
        
        uchar_t* b = rns_json_write(jsonobj);
        
        int32_t ret = rns_hconn_resp2(hconn, 200, b, rns_strlen(b), 1);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp2 failed ret : %d", ret);
        }
        
        rns_ctx_destroy(&ctx);
        rns_json_destroy(&jsonobj);
    }
    
    rns_dbpool_free(mysql);
    
   
    return;
}

void mark_get(rns_mysql_t* mysql, void* data)
{
    int ret = 0;
    
    stb_dao_t dao;
    stb_dao_init(&dao);

    stb_mark_dao_t mdao;
    stb_mark_dao_init(&mdao);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    uint32_t* n = rns_ctx_get(ctx, "number");
    
    stbdev_t* stb = dao.parse(mysql);
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work  = mark_get_work;
    cb.done  = mark_get_done;
    cb.error = get_error;
    cb.data  = ctx;


    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        goto ERR_EXIT;
    }

    *n += 1;
    ret = mdao.search(m, (char_t*)stb->stbid, &cb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }

    return;
    
ERR_EXIT:
    
    rns_dbpool_free(mysql);
    return; 
}

void stb_get_done2(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* h = rns_ctx_get(ctx, "hconn");
    uint32_t* n = rns_ctx_get(ctx, "number");
    uchar_t* userid = rns_ctx_get(ctx, "StbUserId");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    
    stb_mark_dao_t dao;
    stb_mark_dao_init(&dao);
    
    rns_mysql_cb_t cb;
    cb.work = mark_get_work;
    cb.done = mark_get_done;
    cb.error = get_error;
    cb.data  = ctx;
    
    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m != NULL)
    {
        *n += 1;
        int32_t ret = dao.search(m, (char_t*)userid, &cb);
        if(ret < 0)
        {
        }

        
    }
    
    *n -= 1;
    if(*n == 0)
    {
        int32_t ret = rns_hconn_resp2(h, 200, NULL, 0, 0);
        if(ret < 0)
        {
            
        }
        rns_ctx_destroy(&ctx);
    }
    
    rns_dbpool_free(mysql);
}

int32_t stb_mark_update(rns_hconn_t* hconn, void* data)
{
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    trs_t* trs = rns_ctx_get(ctx, "trs");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    uchar_t* userid = rns_ctx_get(ctx, "StbUserId");
    rns_json_t* r = NULL;
    uint32_t i = 0;
    if(trs == NULL || dbpool == NULL)
    {
        return 0;
    }
    
    stb_dao_t dao;
    stb_dao_init(&dao);

    stb_mark_dao_t mdao;
    stb_mark_dao_init(&mdao);
    
    r = rns_json_read(info->body, info->body_size);
    if(r != NULL)
    {
        rns_json_t* d = rns_json_node(r, "data", NULL);
        if(d != NULL)
        {
            uint32_t number = rns_json_array_size(d);
            for(i = 0; i < number; ++i)
            {
                rns_json_t* o = rns_json_array_node(d, i);
                
                rns_json_t* screenCode = rns_json_node(o, "seriescode", NULL);
                rns_json_t* t = rns_json_node(o, "time", NULL);
                if(screenCode == NULL || t == NULL)
                {
                    continue;
                }
                
                stb_mark_t* mark = stb_mark_create2(userid, userid, 0, (uchar_t*)screenCode->valuestring, t->valueint, 0, 0, 0);
                
                rns_mysql_t* m = rns_dbpool_get(dbpool);
                if(m == NULL)
                {
                    continue;
                }
                rns_mysql_cb_t cb;
                rns_memset(&cb, sizeof(rns_mysql_cb_t));
                cb.done = free_mysql;
                cb.error = free_mysql;
                
                int32_t ret = mdao.insert(m, mark, &cb);
                if(ret < 0)
                {
                    rns_dbpool_free(m);
                    LOG_ERROR(lp, "");
                }
            }
        }
    }
    
    rns_json_destroy(&r);
    rns_free(userid);
    rns_ctx_destroy(&ctx);
    
    return info->body_size;
}

void stb_mark_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, trs_t* trs, char_t* StbUserId, int limit1, int limit2 )
{
    rns_ctx_t* ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        goto ERR_EXIT;
    }

    rns_ctx_t* ctxhttp = rns_ctx_create();
    if(ctxhttp == NULL)
    {
        goto ERR_EXIT;
    }
    

    uint32_t* n = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(n == NULL)
    {
        goto ERR_EXIT;
    }
    *n = 0;

    rns_json_t* array = rns_json_create_array();
    if(array == NULL)
    {
        goto ERR_EXIT;
    }
    rns_ctx_add(ctx, "array", array);
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    rns_ctx_add(ctx, "trs", trs);
    rns_ctx_add(ctx, "StbUserId", StbUserId);
    rns_ctx_add(ctx, "number", n);

    uchar_t* userid = rns_dup((uchar_t*)StbUserId);
    rns_ctx_add(ctxhttp, "trs", trs);
    rns_ctx_add(ctxhttp, "dbpool", dbpool);
    rns_ctx_add(ctxhttp, "StbUserId", userid);
    
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work = stb_mark_update;
    cb.data = ctxhttp;
    
    rns_hconn_t* hcms = rns_hconn_get(trs->cms, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn failed");
        goto ERR_EXIT;
    }
    
    uchar_t buf[128];
    snprintf((char_t*)buf, 128 ,"/itv-api2/api/vod/collect.json?uid=%s", StbUserId);
    
    
    int32_t ret = 0;
    ret = rns_hconn_req_header_init(hcms);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_req_header_insert(hcms, (uchar_t*)"Connection", (uchar_t*)"keepalive");
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_req_header_insert(hcms, (uchar_t*)"HOST", (uchar_t*)"117.71.39.8");
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_req_header(hcms, (uchar_t*)"GET", buf);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    ret = rns_hconn_req(hcms, NULL, 0, 0);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        goto ERR_EXIT;
    }

    stb_dao_t dao;
    stb_dao_init(&dao);
    
    rns_mysql_cb_t mcb;
    rns_memset(&mcb, sizeof(rns_mysql_cb_t));
    mcb.work = mark_get;
    mcb.done = stb_get_done2;
    mcb.error = get_error;
    mcb.data = ctx;
    
    *n += 1;
    ret = dao.search_byuserid(mysql, StbUserId, &mcb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }

    
    return;
ERR_EXIT:
    return;
}

void stb_mark_delete(rns_hconn_t* hconn, rns_dbpool_t* dbpool, stb_mark_t* mark)
{
    uint32_t retcode = 200;
    int32_t ret = 0;
    
    stb_mark_dao_t dao;
    stb_mark_dao_init(&dao);
    
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        return;
    }
    
    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;
    
    ret = dao.delete(mysql, mark, &cb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }	
    
    
    return;
ERR_EXIT:
    retcode = 404;
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    
    
    return;
}
//-------------------------------phone playhistory------------------------------------------------------------

void phone_playhistory_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, phone_playhistory_t* phistory)
{
    uint32_t retcode = 200;
    int32_t ret = 0;
    
    phone_playhistory_dao_t dao;
    phone_playhistory_dao_init(&dao);
    
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;
    
    ret = dao.insert(mysql, phistory, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "insert failed");
    }

    phone_playhistory_destroy(&phistory);
    return;
    
ERR_EXIT:
    retcode = 404;
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    phone_playhistory_destroy(&phistory);
    return;
}

void third_error(rns_mysql_t* mysql, void* data)
{
    LOG_ERROR(lp, "third error : %s, %s", mysql->sql, rns_mysql_errstr(mysql));
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    uint32_t* n = rns_ctx_get(ctx, "number");

    if(n == NULL || hconn == NULL)
    {
        return;
    }
    
    --(*n);
    if(*n == 0)
    {
        rns_free(n);
        rns_ctx_destroy(&ctx);
        resp_error(hconn);
    }
    return;
}

void fphone_playhistory_get_work(rns_mysql_t* mysql, void* data)
{
    phone_playhistory_dao_t dao;
    phone_playhistory_dao_init(&dao);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    rns_json_t* array = rns_ctx_get(ctx, "array");
    if(array == NULL)
    {
        return;
    }
    
    phone_playhistory_t* ph = dao.parse(mysql);
    if(ph == NULL)
    {
        return;
    }
    
    rns_json_t* h = rns_json_create_obj();
    rns_json_add_str(h, (uchar_t*)"phoneid", ph->phoneid);
    rns_json_add_str(h, (uchar_t*)"media_id", ph->media_id);
    rns_json_add_int(h, (uchar_t*)"metatype", ph->metatype);
    rns_json_add_int(h, (uchar_t*)"addtime", ph->addtime);
    rns_json_add_int(h, (uchar_t*)"playsecond", ph->playsecond);
    rns_json_add_int(h, (uchar_t*)"totaltime", ph->totaltime);
    rns_json_add_int(h, (uchar_t*)"serial", ph->serial);
    rns_json_add_str(h, (uchar_t*)"title", ph->title);
    
    rns_json_add(array, NULL, h);
    phone_playhistory_destroy(&ph);
    return;
    
}

void fphone_playhistory_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    rns_json_t* array = rns_ctx_get(ctx, "array");
    if(array == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    --(*n);
    if(*n <= 0)
    {
        rns_json_t* jsonobj = rns_json_create_obj();
        rns_json_add(jsonobj, (uchar_t*)"plays", array);
        rns_json_add_int(jsonobj, (uchar_t*)"pagecount", rns_json_array_size(array));
        rns_json_add_int(jsonobj, (uchar_t*)"totalcount", rns_json_array_size(array));
        
        uchar_t* b = rns_json_write(jsonobj);
        
        int32_t ret = rns_hconn_resp2(hconn, 200, b, rns_strlen(b), 1);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp2 failed ret : %d", ret);
        }

        rns_free(n);
        rns_ctx_destroy(&ctx);
        rns_json_destroy(&jsonobj);
    }
    
    return;
}

void fphone_get(rns_mysql_t* mysql, void* data)
{
    int ret = 0;
    phone_playhistory_dao_t dao;
    phone_playhistory_dao_init(&dao);
    
    phone_dao_t pdao;
    phone_dao_init(&pdao);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    if(dbpool == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    
    phonedev_t* phone = pdao.parse(mysql);
    if(phone == NULL)
    {
        return;
    }
    
    rns_mysql_cb_t cb;
    cb.work  = fphone_playhistory_get_work;
    cb.done  = fphone_playhistory_get_done;
    cb.error = third_error;
    cb.data  = ctx;
    
    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto EXIT;
    }

    ++(*n);
    ret = dao.search(m, phone, &cb);
    if(ret < 0)
    {
        rns_dbpool_free(m);
        goto EXIT;
    }

EXIT:
    phone_destroy(&phone);
    return; 
    
}


void fphone_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    
    --(*n);
    
    if(*n <= 0)
    {
        resp_error(hconn);
        rns_free(n);
        rns_ctx_destroy(&ctx);
    }
    
    return;
}

void fstb_get(rns_mysql_t* mysql, void* data)
{
    int ret = 0;
    stb_dao_t dao;
    stb_dao_init(&dao);
    
    phone_dao_t pdao;
    phone_dao_init(&pdao);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    if(dbpool == NULL)
    {
        return;
    }
    
    stbdev_t* stb = dao.parse(mysql);
    if(stb == NULL)
    {
        return;
    }
    
    rns_mysql_cb_t cb;
    cb.work  = fphone_get;
    cb.done  = fphone_get_done;
    cb.error = third_error;
    cb.data  = ctx;
    
    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto EXIT;
    }

    ++(*n);
    ret = pdao.search_bystbid(m, (char_t*)stb->stbid, &cb);
    if(ret < 0)
    {
        rns_dbpool_free(m);
        goto EXIT;
    }

EXIT:
    stb_destroy(&stb);
    return;	
    
}

void fstb_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    
    --(*n);
    
    if(*n <= 0)
    {
        resp_error(hconn);
        rns_free(n);
        rns_ctx_destroy(&ctx);
    }
    
    return;
}


void phone_playhistory_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, char_t* StbUserId, int limit1, int limit2 )
{
    int ret = 0;
    stb_dao_t dao;
    stb_dao_init(&dao);

    uint32_t* n = NULL;
    rns_json_t* array = NULL;
    rns_ctx_t* ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        goto ERR_EXIT;
    }

    n = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(n == NULL)
    {
        goto ERR_EXIT;
    }
    *n = 0;
        
    array = rns_json_create_array();
	
    rns_ctx_add(ctx, "array", array);
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    rns_ctx_add(ctx, "number", n);

    rns_mysql_cb_t cb;
    cb.work  = fstb_get;
    cb.done  = fstb_get_done;
    cb.error = third_error;
    cb.data  = ctx;
	
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        return;
    }

    ++(*n);
    ret = dao.search_byuserid(mysql, StbUserId, &cb);
    if(ret < 0)
    {
        rns_dbpool_free(mysql);
        LOG_ERROR(lp, "get stb failed, stbuserid : %s", StbUserId);
        goto ERR_EXIT;
    }

    return;
	
	
ERR_EXIT:
    if(n != NULL)
    {
        rns_free(n);
    }
    rns_ctx_destroy(&ctx);
    ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_dbpool_free(mysql);
    rns_json_destroy(&array);
    return;

}

//---------------------------------phone mark----------------------------------------------------------
void phone_mark_add(rns_hconn_t* hconn, rns_dbpool_t* dbpool, phone_mark_t* mark)
{
    uint32_t retcode = 200;
    int32_t ret = 0;
    
    phone_mark_dao_t dao;
    phone_mark_dao_init(&dao);
    
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;
    
    ret = dao.insert(mysql, mark, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "insert failed");
    }

    phone_mark_destroy(&mark);
    return;
    
ERR_EXIT:
    retcode = 404;
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    phone_mark_destroy(&mark);
    return;
    
}

void phone_mark_delete(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* phoneid, uchar_t* media_id)
{
    uint32_t retcode = 200;
    int32_t ret = 0;

    phone_mark_dao_t dao;
    phone_mark_dao_init(&dao);

    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        return;
    }

    rns_mysql_cb_t cb;
    cb.work = NULL;
    cb.done = done_func;
    cb.error = error_func;
    cb.data = hconn;

    ret = dao.delete(mysql, phoneid, media_id, &cb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }


    return;
ERR_EXIT:
    retcode = 404;

    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }


    return;
}

void phone_mark_get_work(rns_mysql_t* mysql, void* data)
{
    phone_mark_dao_t dao;
    phone_mark_dao_init(&dao);
    
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    rns_json_t* array = rns_ctx_get(ctx, "array");
    if(array == NULL)
    {
        return;
    }
    
    phone_mark_t* pmark = dao.parse(mysql);
    if(pmark == NULL)
    {
        return;
    }
    
    rns_json_t* h = rns_json_create_obj();
    rns_json_add_str(h, (uchar_t*)"phoneid", pmark->phoneid);
    rns_json_add_str(h, (uchar_t*)"media_id", pmark->media_id);
    rns_json_add_int(h, (uchar_t*)"metatype", pmark->metatype);
    rns_json_add_int(h, (uchar_t*)"addtime", pmark->addtime);
    rns_json_add_int(h, (uchar_t*)"totaltime", pmark->totaltime);
    rns_json_add_int(h, (uchar_t*)"serial", pmark->serial);
    rns_json_add_str(h, (uchar_t*)"title", pmark->title);
    
    rns_json_add(array, NULL, h);

    phone_playhistory_destroy(&pmark);
    return;

}

void phone_mark_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    rns_json_t* array = rns_ctx_get(ctx, "array");
    if(array == NULL)
    {
        return;
    }
    
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    --(*n);
    if(*n <= 0)
    {
        rns_json_t* jsonobj = rns_json_create_obj();
        rns_json_add(jsonobj, (uchar_t*)"marks", array);
        rns_json_add_int(jsonobj, (uchar_t*)"pagecount", rns_json_array_size(array));
        rns_json_add_int(jsonobj, (uchar_t*)"totalcount", rns_json_array_size(array));
        
        uchar_t* b = rns_json_write(jsonobj);
        
        int32_t ret = rns_hconn_resp2(hconn, 200, b, rns_strlen(b), 1);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp2 failed ret : %d", ret);
        }
        rns_free(n);
        rns_ctx_destroy(&ctx);
        rns_json_destroy(&jsonobj);
    }
    
    return;
}

void phone_get(rns_mysql_t* mysql, void* data)
{
    int ret = 0;
    phone_mark_dao_t dao;
    phone_mark_dao_init(&dao);

    phone_dao_t pdao;
    phone_dao_init(&pdao);

    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    if(dbpool == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }

    phonedev_t* phone = pdao.parse(mysql);
    if(phone == NULL)
    {
        LOG_ERROR(lp, "parse phone failed");
        return;
    }
	
    rns_mysql_cb_t cb;
    cb.work  = phone_mark_get_work;
    cb.done  = phone_mark_get_done;
    cb.error = third_error;
    cb.data  = ctx;

    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto EXIT;
    }

    ++(*n);
    ret = dao.search(m, phone, &cb);
    if(ret < 0)
    {
        rns_dbpool_free(m);
        LOG_ERROR(lp, "get phone mark failed, phoneid : %s", phone->phoneid);
        goto EXIT;
    }

EXIT:
    phone_destroy(&phone);
    return; 
}


void phone_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }
    
    --(*n);
    
    if(*n <= 0)
    {
        resp_error(hconn);
        rns_free(n);
        rns_ctx_destroy(&ctx);
    }
    
    return;
}

void stb_get(rns_mysql_t* mysql, void* data)
{
    int ret = 0;
    stb_dao_t dao;
    stb_dao_init(&dao);

    phone_dao_t pdao;
    phone_dao_init(&pdao);

    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    uint32_t* n = rns_ctx_get(ctx, "number");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    
    if(hconn == NULL || n == NULL || dbpool == NULL)
    {
        return;
    }
    
    stbdev_t* stb = dao.parse(mysql);
    if(stb == NULL)
    {
        LOG_ERROR(lp, "parse stb failed");
        return;
    }
	
    rns_mysql_cb_t cb;
    cb.work  = phone_get;
    cb.done  = phone_get_done;
    cb.error = third_error;
    cb.data  = ctx;

    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto EXIT;
    }
    ++(*n);
    ret = pdao.search_bystbid(m, (char_t*)stb->stbid, &cb);
    if(ret < 0)
    {
        rns_dbpool_free(m);
        LOG_ERROR(lp, "get phone by stbid failed, stbid : %s", stb->stbid);
        goto EXIT;
    }
    
EXIT:
    stb_destroy(&stb);
    return;	

}

void mstb_get_done(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    uint32_t* n = rns_ctx_get(ctx, "number");
    if(n == NULL)
    {
        return;
    }

    --(*n);
    if(*n <= 0)
    {
        resp_error(hconn);
        rns_free(n);
        rns_ctx_destroy(&ctx);
    }

    return;
}

void phone_mark_get(rns_hconn_t* hconn, rns_dbpool_t* dbpool, char_t* StbUserId, int limit1, int limit2 )
{
    int ret = 0;
    stb_dao_t dao;
    stb_dao_init(&dao);

    uint32_t* n = NULL;
    rns_json_t* array = NULL;
    rns_ctx_t* ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        goto ERR_EXIT;
    }
	
    n = (uint32_t*)rns_malloc(sizeof(uint32_t));
    if(n == NULL)
    {
        goto ERR_EXIT;
    }
    *n = 0;

    array = rns_json_create_array();
    if(array == NULL)
    {
        goto ERR_EXIT;
    }

    
    rns_ctx_add(ctx, "array", array);
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    rns_ctx_add(ctx, "number", n);
        
    rns_mysql_cb_t cb;
    cb.work  = stb_get;
    cb.done  = mstb_get_done;
    cb.error  = third_error;
    cb.data  = ctx;
	
	
    rns_mysql_t* mysql = rns_dbpool_get(dbpool);
    if(mysql == NULL)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto ERR_EXIT;
    }

    ++(*n);
    ret = dao.search_byuserid(mysql, StbUserId, &cb);
    if(ret < 0)
    {
        rns_dbpool_free(mysql);
        LOG_ERROR(lp, "search stb by stbuserid failed, StbUserId : %s", StbUserId);
        goto ERR_EXIT;
    }

    return;
	
	
ERR_EXIT:
    if(n != NULL)
    {
        rns_free(n);
    }
    rns_ctx_destroy(&ctx);
    ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_json_destroy(&array);
    return;

}
