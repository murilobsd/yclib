#include "foss.h"
#include "rns_log.h"
#include "stbMgr.h"
#include "phoneMgr.h"
#include "rns_json.h"
#include "rns_xml.h"
#include <stdio.h>


#define RELEASE_VERSION "1.0"

#define SOAP_ENV_START	     "<SOAP-ENV:Envelope>\r\n\t<SOAP-ENV:Body>\r\n"/* SOAP协议头 */
#define SOAP_ENV_END		"\t</SOAP-ENV:Body>\r\n</SOAP-ENV:Envelope>\r\n"/* SOAP协议尾 */

#define SOAP_ENV_NAME		"SOAP-ENV:Envelope"
#define SOAP_HEADER_NAME	"SOAP-ENV:Header"
#define SOAP_BODY_NAME		"SOAP-ENV:Body"

rns_log_t* lp = NULL;

void resp_error(rns_hconn_t* hconn)
{
    int32_t ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    return;
}


void free_mysql(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_free(mysql);
    return;
}

void error_func(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    
    int32_t ret = rns_hconn_resp2(hconn, 400, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }
    rns_ctx_destroy(&ctx);
    rns_dbpool_free(mysql);
    return;
}

void resp_server(rns_hconn_t* hconn, uchar_t* trsid, rns_addr_t* addr)
{
    rns_json_t* obj = rns_json_create_obj();
    if(obj == NULL)
    {
        return;
    }
    
    uchar_t ip[32];
    rns_json_add_str(obj, (uchar_t*)"ip", rns_ip_int2str(addr->ip, ip, 32));
    rns_json_add_int(obj, (uchar_t*)"port", addr->port);
    rns_json_add_int(obj, (uchar_t*)"isbind", 1);

    uchar_t* b = rns_json_write(obj);
    if(b == NULL)
    {
        rns_json_destroy(&obj);
        return;
    }

    LOG_DEBUG(lp, "trs server : \n%s", b);
    
    int32_t ret = rns_hconn_resp2(hconn, 200, b, rns_strlen(b), 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp server failed, ip : %s, port : %d", ip, addr->port);
    }
    rns_json_destroy(&obj);
    
    return;
}
//-----------------------------------------------------------------------------------
void server_get_work(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    foss_t* foss = rns_ctx_get(ctx, "foss");
    if(hconn == NULL)
    {
        return;
    }

    stb_dao_t dao;
    stb_dao_init(&dao);

    stbdev_t* stb = dao.parse(mysql);
    if(stb == NULL)
    {
        LOG_ERROR(lp, "parse stb failed");
        resp_error(hconn);
        return;
    }

    rbt_node_t* node = rbt_search_str(&foss->strsmgr, stb->trsid);
    if(node == NULL)
    {
        LOG_ERROR(lp, "trs is not online, trsid : %s", stb->trsid);
        resp_error(hconn);
        return;
    }
    trsmgr_t* trsmgr = rbt_entry(node, trsmgr_t, snode);

    resp_server(hconn, NULL, &trsmgr->addr);
    return;
}

void server_get_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    foss_t* foss = rns_ctx_get(ctx, "foss");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    uchar_t* stbid = rns_ctx_get(ctx, "stbid");
    if(hconn == NULL)
    {
        return;
    }

    if(mysql->rownum <= 0)
    {
        rbt_node_t* node = rbt_first(&foss->ntrsmgr);
        if(node == NULL)
        {
            LOG_ERROR(lp, "no trs is on line, stbid : %s",  stbid);
            resp_error(hconn);
            goto EXIT;
        }
        trsmgr_t* trsmgr = rbt_entry(node, trsmgr_t, nnode);
        
        stb_dao_t dao;
        stb_dao_init(&dao);

        rns_mysql_cb_t cb;
        cb.done = free_mysql;
        cb.data = NULL;
        
        rns_mysql_t* m = rns_dbpool_get(dbpool);
        if(m == NULL)
        {
            LOG_ERROR(lp, "ran out of mysql connection");
            resp_error(hconn);
            goto EXIT;
        }

        uint32_t number = trsmgr->nnode.key.idx;
        rbt_delete(&foss->ntrsmgr, &trsmgr->nnode);
        rbt_set_key_int(&trsmgr->nnode, number + 1);
        rbt_insert(&foss->ntrsmgr, &trsmgr->nnode);
        
        LOG_INFO(lp, "registry stb, stbid : %s, trsmgr id : %s", stbid, trsmgr->id);
        int32_t ret = dao.registry(m, stbid, trsmgr->id, &cb);
        if(ret < 0)
        {
            rns_dbpool_free(m);
            resp_error(hconn);
            goto EXIT;
        }
        resp_server(hconn, NULL, &trsmgr->addr);
    }

EXIT:
    rns_dbpool_free(mysql);
    rns_ctx_destroy(&ctx);
    rns_free(stbid);
    return;
}

void server_get_by_stb(rns_hconn_t* hconn, foss_t* foss, rns_dbpool_t* dbpool, uchar_t* stbid)
{
    int32_t  ret = 0;
    rns_ctx_t* ctx = NULL;
    rns_mysql_t *mysql = rns_dbpool_get(dbpool);
    uchar_t* sid = rns_dup(stbid);
    
    if(NULL == mysql)
    {
        LOG_ERROR(lp, "ran out of mysql connection");
        goto ERR_EXIT;
    }
    
    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        LOG_ERROR(lp, "create ctx failed");
        goto ERR_EXIT;
    }
    
    rns_ctx_add(ctx, "foss", foss);
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    rns_ctx_add(ctx, "stbid", sid);

    rns_mysql_cb_t cb;
    cb.work  = server_get_work;
    cb.done  = server_get_done;
    cb.error = error_func;
    cb.data  = ctx;
    
    stb_dao_t dao;
    stb_dao_init(&dao);
    
    ret = dao.search(mysql, stbid, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "search stb failed, stb id : %s", stbid);
        goto ERR_EXIT;
    }
    return;
ERR_EXIT:
    resp_error(hconn);
    rns_dbpool_free(mysql);
    if(sid != NULL)
    {
        rns_free(sid);
    }
    rns_ctx_destroy(&ctx);
    return;
}

//---------------------------------------------------------------------------
void stb_getstb_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    if(mysql->rownum <= 0)
    {
        resp_error(hconn);
    }
    rns_dbpool_free(mysql);
    rns_ctx_destroy(&ctx);
    return;
}

void stb_getstb_work(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }

    stb_dao_t dao;
    stb_dao_init(&dao);

    stbdev_t* stb = dao.parse(mysql);
    if(stb == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_json_t* obj = rns_json_create_obj();
    rns_json_add_str(obj, (uchar_t*)"stbid", stb->stbid);
    rns_json_add_str(obj, (uchar_t*)"StbUseId", stb->StbUserId);
    
    uchar_t* b = rns_json_write(obj);
    if(b == NULL)
    {
        LOG_ERROR(lp, "get stb id failed, stbid : %s", stb->stbid);
        resp_error(hconn);
        return;
    }

    LOG_DEBUG(lp, "get stb id : \n%s", b);
    
    int32_t ret = rns_hconn_resp2(hconn, 200, b, rns_strlen(b), 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp failed, buf : %s", b);
    }

    stb_destroy(&stb);
    
    return;
ERR_EXIT:
    resp_error(hconn);
    return;
    
}

void stb_get_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    if(mysql->rownum <= 0)
    {
        rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
        if(hconn == NULL)
        {
            return;
        }
        int32_t ret = rns_hconn_resp2(hconn, 402, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp2 failed ret : %d", ret);
        }
        rns_ctx_destroy(&ctx);
    }
    rns_dbpool_free(mysql);
    return;
}

void stb_get_work(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    if(hconn == NULL || dbpool == NULL)
    {
        return;
    }
    
    phone_dao_t dao;
    phone_dao_init(&dao);

    phonedev_t* phone = dao.parse(mysql);
    if(phone == NULL)
    {
        resp_error(hconn);
        return;
    }

    if(phone->stbid == NULL)
    {
        resp_error(hconn);
        return;
    }
    
    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        LOG_ERROR(lp, "get stb id failed, ran out of mysql connection, phone id : %s", phone->phoneid);
        resp_error(hconn);
        return;
    }

    stb_dao_t stbdao;
    stb_dao_init(&stbdao);

    rns_mysql_cb_t c;
    c.work = stb_getstb_work;
    c.done = stb_getstb_done;
    c.error = error_func;
    c.data = ctx;
    
    int32_t ret = stbdao.search(m, phone->stbid, &c);
    if(ret < 0)
    {
        LOG_ERROR(lp, "find stb by stbid failed, stbid : %s", phone->stbid);
        resp_error(hconn);
        return;
    }

    phone_destroy(&phone);
    return;
}

void stb_get_by_phoneid(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* phoneid)
{
    int  retcode = 0;
    int  ret = 0;
    rns_ctx_t* ctx = NULL;
    rns_mysql_t *mysql = rns_dbpool_get(dbpool);
    if(NULL == mysql)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
    
    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
    
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    
    rns_mysql_cb_t cb;
    cb.work  = stb_get_work;
    cb.done  = stb_get_done;
    cb.error = error_func;
    cb.data  = ctx;
    
    phone_dao_t dao;
    phone_dao_init(&dao);
    
    ret = dao.search(mysql, phoneid, &cb);
    if(ret < 0)
    {
        retcode = 400;
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
    return;
}

void stb_update_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    stbdev_t* stb = rns_ctx_get(ctx, "stb");
    int32_t ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }

    stb_destroy(&stb);
    rns_dbpool_free(mysql);
    rns_ctx_destroy(&ctx);
    return;
}

void phone_stb_unbind_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    if(hconn == NULL)
    {
        return;
    }
    
    int32_t ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp2 failed ret : %d", ret);
    }

    rns_ctx_destroy(&ctx);
    rns_dbpool_free(mysql);
    return;
}

void phone_stb_done(rns_mysql_t* mysql, void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_hconn_t* hconn = rns_ctx_get(ctx, "hconn");
    rns_dbpool_t* dbpool = rns_ctx_get(ctx, "dbpool");
    stbdev_t* stb = rns_ctx_get(ctx, "stb");
    uint32_t retcode = 200;
    if(hconn == NULL || dbpool == NULL || stb == NULL)
    {
        return;
    }

    rns_mysql_cb_t cb;
    cb.work  = NULL;
    cb.done  = stb_update_done;
    cb.error = error_func;
    cb.data  = ctx;

    rns_mysql_t* m = rns_dbpool_get(dbpool);
    if(m == NULL)
    {
        retcode = 400;
        goto EXIT;
    }
    
    stb_dao_t dao;
    stb_dao_init(&dao);
    
    int32_t ret = dao.update_info(m, stb, &cb);
    if(ret < 0)
    {
        retcode = 400;
        rns_dbpool_free(m);
        goto EXIT;
    }	
    
EXIT:
    if(retcode != 200)
    {
        ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp2 failed ret : %d", ret);
        }
        
    }
    rns_dbpool_free(mysql);
    return;
}

void phone_bindstb(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* phoneid, stbdev_t* stb)
{
    int  retcode = 0;
    int  ret = 0;
    rns_ctx_t* ctx = NULL;
    rns_mysql_t *mysql = rns_dbpool_get(dbpool);
    if(NULL == mysql)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
    
    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
    
    rns_ctx_add(ctx, "hconn", hconn);
    rns_ctx_add(ctx, "dbpool", dbpool);
    rns_ctx_add(ctx, "stb", stb);
    
    rns_mysql_cb_t cb;
    cb.work  = NULL;
    cb.done  = phone_stb_done;
    cb.error = error_func;
    cb.data  = ctx;
    
    phone_dao_t dao;
    phone_dao_init(&dao);
    
    ret = dao.insert2(mysql, phoneid, stb->stbid, &cb);
    if(ret < 0)
    {
        retcode = 400;
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
    return;
}

void phone_unbindstb(rns_hconn_t* hconn, rns_dbpool_t* dbpool, uchar_t* phoneid)
{
    int  retcode = 0;
    int  ret = 0;
    rns_ctx_t* ctx = NULL;
    rns_mysql_t *mysql = rns_dbpool_get(dbpool);
    if(NULL == mysql)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
    
    ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        retcode = 404;
        goto ERR_EXIT;
    }
    
    rns_ctx_add(ctx, "hconn", hconn);
    
    rns_mysql_cb_t cb;
    cb.work  = NULL;
    cb.done  = phone_stb_unbind_done;
    cb.error = error_func;
    cb.data  = ctx;
    
    phone_dao_t dao;
    phone_dao_init(&dao);
    
    ret = dao.delete_byid2(mysql, phoneid, &cb);
    if(ret < 0)
    {
        retcode = 400;
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
    return;
}

void trs_login(void* data)
{

    trsmgr_t* trsmgr = (trsmgr_t*)data;
    rbt_delete(&trsmgr->foss->strsmgr, &trsmgr->snode);
    rbt_delete(&trsmgr->foss->ntrsmgr, &trsmgr->nnode);
    LOG_WARN(lp, "trs : %s timeout", trsmgr->id);
    rns_timer_delete(trsmgr->foss->ep, trsmgr->timer);
    
    rns_free(trsmgr);

    return;
}

int32_t dispatch_cb(rns_hconn_t* hconn, void* data)
{
    foss_t* foss = (foss_t*)data;
    uint32_t retcode = 200;
    int32_t ret = 0;
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    
    LOG_DEBUG(lp, "uri : %s, size : %d, req : %s", info->uri, info->body_size, info->body);
    
    if(rns_strcmp(info->uri, "/GetServer/stb") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"stbid");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        LOG_DEBUG(lp, "GetServer stb : %s", arg->data);
        server_get_by_stb(hconn, foss, foss->dbpool, arg->data);
    }
    else if(rns_strcmp(info->uri, "/GetServer/phone") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"stbid");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        
        server_get_by_stb(hconn, foss, foss->dbpool, arg->data);
    }
    else if(rns_strcmp(info->uri, "/phone/GetStbid") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"phoneid");
        if(node == NULL)
        {
            printf("get phone id faield\n");
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        
        stb_get_by_phoneid(hconn, foss->dbpool, arg->data);
    }
    else if(rns_strcmp(info->uri, "/Foss/bind") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* phoneid = rns_json_node(req, "phoneid", NULL);
        stbdev_t* stb = stb_create2(info->body, info->body_size);
        if(phoneid == NULL || stb == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phone_bindstb(hconn, foss->dbpool, (uchar_t*)phoneid->valuestring, stb);
        rns_json_destroy(&req);
    }
    else if(rns_strcmp(info->uri, "/Foss/unbind") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* phoneid = rns_json_node(req, "phoneid", NULL);
        
        phone_unbindstb(hconn, foss->dbpool, (uchar_t*)phoneid->valuestring);
        rns_json_destroy(&req);
    }
    else if(rns_strcmp(info->uri, "/Foss/login") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* serverid = rns_json_node(req, "peer", NULL);
        rns_json_t* ip = rns_json_node(req, "ip", NULL);
        rns_json_t* port = rns_json_node(req, "port", NULL);

        if(serverid == NULL || ip == NULL || port == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }

        trsmgr_t* trsmgr = NULL;
        rbt_node_t* node = rbt_search_str(&foss->strsmgr, (uchar_t*)serverid->valuestring);
        if(node == NULL)
        {
            trsmgr = (trsmgr_t*)rns_malloc(sizeof(trsmgr_t));
            if(trsmgr == NULL)
            {
                retcode = 400;
                goto ERR_EXIT;
            }

            rbt_set_key_str(&trsmgr->snode, (uchar_t*)serverid->valuestring);
            rbt_insert(&foss->strsmgr, &trsmgr->snode);

            rbt_set_key_int(&trsmgr->nnode, 0);
            rbt_insert(&foss->ntrsmgr, &trsmgr->nnode);

            trsmgr->addr.ip = rns_ip_str2int((uchar_t*)ip->valuestring);
            trsmgr->addr.port = port->valueint;
            trsmgr->id = rns_dup((uchar_t*)serverid->valuestring);
            trsmgr->foss = foss;
            trsmgr->hconn = hconn;
            rns_cb_t cb;
            cb.func = trs_login;
            cb.data = trsmgr;
            trsmgr->timer = rns_timer_add(foss->ep, 120000, &cb);
            if(trsmgr->timer == NULL)
            {
                rns_free(trsmgr);
                retcode = 400;
                goto ERR_EXIT;
            }
        }
        else
        {
            trsmgr = rbt_entry(node, trsmgr_t, snode);
            trsmgr->hconn = hconn;
            rns_timer_delay(foss->ep, trsmgr->timer, 120000);
        }

        
        
        ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "");
        }
        rns_json_destroy(&req);
    }
    //--------------------------------stb playhistory------------------------------
    else
    {
        LOG_ERROR(lp, "uri is wrong, uri : %s", info->uri);
        retcode = 400;
        goto ERR_EXIT;
    }
    
    return 0;
ERR_EXIT:
    LOG_ERROR(lp, "req error, uri : %s, req : \n%s", info->uri, info->body);
    
    ret = rns_hconn_resp2(hconn, retcode, NULL, 0, 0);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp failed, ret : %d", ret);
    }
    return -1;
}

void foss_login(void* data)
{
    foss_t* foss = (foss_t*)data;

    uchar_t* b = rns_malloc(1024);
    if(b == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : 1024");
        return;
    }
    snprintf( (char_t*)b, 1024,
              SOAP_ENV_START
              "\t\t<foss peer_id=\"%s\" peer_type=\"%s\" version=\"3\"/>\r\n"
              SOAP_ENV_END,
              foss->id, foss->type);

    
    rns_hconn_t* hconn = rns_hconn_get(foss->ocs, NULL);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn failed");
        return;
    }
    int32_t ret = rns_hconn_req2(hconn, (uchar_t*)"POST", (uchar_t*)"/ois/server/login", b, rns_strlen(b), 1, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "req faield, ret : %d", ret);
        rns_free(b);
        rns_hconn_free(hconn);
        return;
    }
    
    rns_timer_delay(foss->ep, foss->timer, 30000);
    return;
}

int32_t main()
{
    int32_t retcode = 0;
    uchar_t* ptr = NULL;
    uint32_t level = 2;
    uchar_t* dlp = (uchar_t*)"./log/foss.log";
    uchar_t* logpath = NULL;
    
    rns_addr_t addr;
    
    
    foss_t* foss = (foss_t*)rns_malloc(sizeof(foss_t));
    if(foss == NULL)
    {
        retcode = -1;
        goto EXIT;
    }

    rbt_init(&foss->strsmgr, RBT_TYPE_STR);
    rbt_init(&foss->ntrsmgr, RBT_TYPE_INT);
    
    foss->ep = rns_epoll_init();
    if(foss->ep == NULL)
    {
        retcode = -2;
        goto EXIT;
    }
    
    rns_file_t* fp = rns_file_open((uchar_t*)"./conf/foss.xml", RNS_FILE_READ);
    if(fp == NULL)
    {
        retcode = -3;
        goto EXIT;
    }
    
    int32_t size = rns_file_size(fp);
    if(size < 0)
    {
        retcode = -4;
        goto EXIT;
    }
    
    uchar_t* cfgbuf = rns_malloc(size + 1);
    
    int32_t ret = rns_file_read(fp, cfgbuf, size);
    if(ret != size)
    {
        retcode = -5;
        goto EXIT;
    }
    
    rns_xml_t* xml = rns_xml_create(cfgbuf, size);
    if(xml == NULL)
    {
        retcode = -6;
        goto EXIT;
    }
    
    ptr = rns_xml_node_value(xml, "foss", "id", NULL);
    if(ptr == NULL)
    {
        retcode = -7;
        goto EXIT;
    }
    foss->id = rns_dup(ptr);

    ptr = rns_xml_node_value(xml, "foss", "type", NULL);
    if(ptr == NULL)
    {
        retcode = -7;
        goto EXIT;
    }
    foss->type = rns_dup(ptr);
    
    
    ptr = rns_xml_node_value(xml, "foss", "log", "level", NULL);
    if(ptr != NULL)
    {
        level = rns_atoi(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "foss", "log", "path", NULL);
    if(ptr != NULL)
    {
        logpath = rns_malloc(rns_strlen(ptr) + 1);
        if(logpath == NULL)
        {
            retcode = -7;
            goto EXIT;
        }
        rns_memcpy(logpath, ptr, rns_strlen(ptr));
        lp = rns_log_init(logpath, level, (uchar_t*)"foss");
    }
    else
    {
        lp = rns_log_init(dlp, level, (uchar_t*)"foss");
    }
    
    if(lp == NULL)
    {
        retcode = -2;
        goto EXIT;
    }
    
    ret = rns_epoll_add(foss->ep, lp->pipe->fd[0], lp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        retcode = -10;
        goto EXIT;
    }
    
    
    ptr = rns_xml_node_value(xml, "foss", "http", "ip", NULL);
    if(ptr == NULL)
    {
        addr.ip = 0;
    }
    else
    {
        addr.ip = rns_ip_str2int(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "foss", "http", "port", NULL);
    if(ptr == NULL)
    {
        addr.port = 9999;
    }
    else
    {
        addr.port = rns_atoi(ptr);
    }

    uint32_t expire = 60000;
    ptr = rns_xml_node_value(xml, "trs", "http", "expire", NULL);
    if(ptr != NULL)
    {
        expire = rns_atoi(ptr);
    }
    
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work  = dispatch_cb;
    cb.close = NULL;
    cb.clean = NULL;
    cb.error = NULL;
    cb.data  = foss;
    
    foss->http = rns_httpserver_create(&addr, &cb, expire);
    if(foss->http == NULL)
    {
        LOG_ERROR(lp, "http server create failed, ip : %u, port : %d", addr.ip, addr.port);
        retcode = -1;
        goto EXIT;
    }
    
    ret = rns_epoll_add(foss->ep, foss->http->listenfd, foss->http, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add http server to epoll failed");
        retcode = -2;
        goto EXIT;
    }
    
    rns_addr_t oaddr;
    ptr = rns_xml_node_value(xml, "foss", "ocs", "ip", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "ocs ip is null");   
        goto EXIT;
    }
    oaddr.ip = rns_ip_str2int(ptr);
    
    ptr = rns_xml_node_value(xml, "foss", "ocs", "port", NULL);
    if(ptr == NULL)
    {
        oaddr.port = 5000;
    }
    else
    {
        oaddr.port = rns_atoi(ptr);
    }

    foss->ocs = rns_httpclient_create(foss->ep, &oaddr, 1, NULL);
    if(foss->ocs == NULL)
    {
        LOG_ERROR(lp, "http server create failed, ip : %u, port : %d", oaddr.ip, oaddr.port);
        retcode = -1;
        goto EXIT;
    }

    rns_cb_t tcb;
    tcb.func = foss_login;
    tcb.data = foss;
    foss->timer = rns_timer_add(foss->ep, 4000, &tcb);
    if(foss->timer == NULL)
    {
        LOG_ERROR(lp, "");
        retcode = -2;
        goto EXIT;  
    }

    ptr = rns_xml_node_value(xml, "foss", "ocs", "postip", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "ocs ip is null");   
        goto EXIT;
    }
    foss->postaddr.ip = rns_ip_str2int(ptr);
    
    ptr = rns_xml_node_value(xml, "foss", "ocs", "postport", NULL);
    if(ptr == NULL)
    {
        oaddr.port = 9999;
    }
    else
    {
        foss->postaddr.port = rns_atoi(ptr);
    }
    
    
    rns_dbpara_t * db_param = (rns_dbpara_t*)rns_malloc(sizeof(rns_dbpara_t));
    if(db_param == NULL)
    {
        retcode = -3;
        goto EXIT;
    }
    ptr = rns_xml_node_value(xml, "foss", "db", "ip", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "doesn't config db ip");
        retcode = -3;
        goto EXIT;
        
    }
    db_param->hostip = rns_malloc(rns_strlen(ptr) + 1);
    if(db_param->hostip == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", rns_strlen(ptr) + 1);
        retcode = -7;
        goto EXIT;
    }
    rns_memcpy(db_param->hostip, ptr, rns_strlen(ptr));
    
    ptr = rns_xml_node_value(xml, "foss", "db", "port", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "doesn't config db port");
        
        retcode = -4;
        goto EXIT;
    }
    db_param->port = rns_atoi(ptr);
    
    ptr = rns_xml_node_value(xml, "foss", "db", "user", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "doesn't config db user");
        retcode = -5;
        goto EXIT;
    }
    db_param->username = rns_malloc(rns_strlen(ptr) + 1);
    if(db_param->username == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", rns_strlen(ptr) + 1);
        retcode = -7;
        goto EXIT;
    }
    rns_memcpy(db_param->username, ptr, rns_strlen(ptr));
    
    ptr = rns_xml_node_value(xml, "foss", "db", "passwd", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "doesn't config db passwd");
        retcode = -8;
        goto EXIT;
    }
    db_param->password = rns_malloc(rns_strlen(ptr) + 1);
    if(db_param->password == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", rns_strlen(ptr) + 1);
        retcode = -7;
        goto EXIT;
    }
    rns_memcpy(db_param->password, ptr, rns_strlen(ptr));
    
    
    ptr = rns_xml_node_value(xml, "foss", "db", "database", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "doesn't config db database");
        retcode = -9;
        goto EXIT;
    }
    db_param->database = rns_malloc(rns_strlen(ptr) + 1);
    if(db_param->database == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", rns_strlen(ptr) + 1);
        retcode = -7;
        goto EXIT;
    }
    rns_memcpy(db_param->database, ptr, rns_strlen(ptr));
    
    db_param->client_flag = 0;
    
    uint32_t min = 16;
    ptr = rns_xml_node_value(xml, "foss", "db", "min", NULL);
    if(ptr != NULL)
    {
        min = rns_atoi(ptr);
    }
    
    uint32_t max = 16;
    ptr = rns_xml_node_value(xml, "foss", "db", "max", NULL);
    if(ptr != NULL)
    {
        max = rns_atoi(ptr);
    }
    
    foss->dbpool = rns_dbpool_create(foss->ep, db_param, min, max, NULL);
    if(foss->dbpool == NULL)
    {
        LOG_ERROR(lp, "db pool create failed, db ip : %s, port : %d, user : %s", db_param->hostip, db_param->port, db_param->username);
        retcode = -4;
        goto EXIT;
    }

    uint64_t nofile = 655360;
    ptr = rns_xml_node_value(xml, "foss", "nofile", NULL);
    if(ptr != NULL)
    {
        nofile = rns_atoi(ptr);
    }
    
    rns_limit_set(RLIMIT_NOFILE, nofile, nofile);
    rns_limit_get(RLIMIT_NOFILE, &nofile, &nofile);
    LOG_INFO(lp, "nofile : %llu", nofile);
    
    rns_epoll_wait(foss->ep);
    
    return 0;	
    
EXIT:
    
    return retcode;
    
}
