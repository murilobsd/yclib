#include "rns.h"
#include "trs.h"
#include "rns_mysql.h"
#include "rns_json.h"
#include <errno.h>
#include <stdio.h>
#include "rns_xml.h"
#include "rns_log.h"

#define LOAD_STBDEV 0

void foss_keepavlie(void* data)
{
    trs_t* trs = (trs_t*)data;
    int32_t ret = 0;
    
    rns_json_t* body = rns_json_create_obj();
    ret = rns_json_add_str(body, (uchar_t*)"peer", trs->id);
    if(ret < 0)
    {
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    uchar_t ip[32];
    ret = rns_json_add_str(body, (uchar_t*)"ip", rns_ip_int2str(trs->postaddr.ip, ip, 32));
    if(ret < 0)
    {
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    ret = rns_json_add_int(body, (uchar_t*)"port", trs->postaddr.port);
    if(ret < 0)
    {
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    uchar_t* b = rns_json_write(body);
    if(b == NULL)
    {
        LOG_ERROR(lp, "");
        goto EXIT;
    }
    
    
    rns_hconn_t* hconn = rns_hconn_get(trs->foss, NULL);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn failed");
        goto EXIT;
    }
    ret = rns_hconn_req2(hconn, (uchar_t*)"POST", (uchar_t*)"/Foss/login", b, rns_strlen(b), 1, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "req faield, ret : %d", ret);
        goto EXIT;
    }
    
    rns_timer_delay(trs->ep, trs->timer, trs->interval * 1000);
    
EXIT:
    rns_json_destroy(&body);
    return;
}

void phone_fetch(rns_mysql_t* mysql, void* data)
{
    trs_t* trs = (trs_t*)data;
    phone_dao_t dao;
    phone_dao_init(&dao);
    
    phonedev_t* phone = dao.parse(mysql);
    if(phone == NULL)
    {
        return;
    }
    
    LOG_DEBUG(lp, "load phone : %s", phone->phoneid);
    rbt_set_key_str(&phone->node, phone->phoneid);
    rbt_insert(&trs->phoneroot, &phone->node);
    
    if(phone->stbid)
    {
        rbt_node_t* snode = rbt_search_str(&trs->stbroot, phone->stbid);
        if(snode != NULL)
        {
            stbdev_t* stb = rbt_entry(snode, stbdev_t, node);
            rns_list_add(&phone->list, &stb->phones);
            phone->stb = stb;
        }
    }
    
    return;
    
}

void phone_fetch_done(rns_mysql_t* mysql, void* data)
{
    LOG_DEBUG(lp, "phone number : %d", mysql->rownum);
    rns_dbpool_free(mysql);
}
void phone_fetch_error(rns_mysql_t* mysql, void* data)
{
    LOG_ERROR(lp, "load phone error, %s", rns_mysql_errstr(mysql));
    rns_dbpool_free(mysql);
}


void stb_fetch(rns_mysql_t* mysql, void* data)
{
    trs_t* trs = (trs_t*)data;
    if(trs == NULL)
    {
        return;
    }
    stb_dao_t stbdao;
    stb_dao_init(&stbdao);
    
    stbdev_t* stb = stbdao.parse(mysql);
    if(stb == NULL)
    {
        return;
    }
    LOG_DEBUG(lp, "load stb : %s", stb->stbid);
    rbt_set_key_str(&stb->node, stb->stbid);
    rbt_insert(&trs->stbroot, &stb->node);
    
    return;
}

void stb_fetch_done(rns_mysql_t* mysql, void* data)
{
    LOG_DEBUG(lp, "stb number : %d", mysql->rownum);
    rns_dbpool_free(mysql);
    
    trs_t* trs = (trs_t*)data;
    
    int32_t ret = 0;
    
    rns_mysql_t* m = rns_dbpool_get(trs->dbpool);
    if(m != NULL)
    {
        rns_mysql_cb_t cb;
        cb.work  = phone_fetch;
        cb.done  = phone_fetch_done;
        cb.error = phone_fetch_error;
        cb.data  = trs;
        
        phone_dao_t dao;
        phone_dao_init(&dao);
        
        ret = dao.search_all(m, &cb);
        if(ret < 0)
        {
            rns_dbpool_free(m);
            LOG_ERROR(lp, "phone find all failed");
            return;
        }
    }
    
    rns_cb_t tcb;
    tcb.func = foss_keepavlie;
    tcb.data = trs;
    trs->timer = rns_timer_add(trs->ep, 4000, &tcb);
    if(trs->timer == NULL)
    {
        LOG_ERROR(lp, "");
        return;
    }
    
    ret = rns_epoll_add(trs->ep, trs->http->listenfd, trs->http, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add http server to epoll failed");
    }
    
    return;
}

void stb_fetch_error(rns_mysql_t* mysql, void* data)
{
    LOG_ERROR(lp, "load stb error, %s", rns_mysql_errstr(mysql));
    rns_dbpool_free(mysql);
}
stbdev_t* stbdev_find(trs_t* trs, uchar_t* stbid)
{
    stbdev_t *stb = NULL;
    
    if(stbid == NULL)
    {
        return NULL;
    }
    
    rbt_node_t* node = rbt_search_str(&trs->stbroot, stbid);
    if(node != NULL)
    {
        stb = rbt_entry(node, stbdev_t, node);
        return stb;
    }
    
    return NULL;
    
}


phonedev_t* phonedev_find(trs_t* trs, uchar_t* phoneid)
{
    phonedev_t* phone = NULL;
    
    if(NULL == phoneid)
    {
        return NULL;
    }
    
    rbt_node_t* node = rbt_search_str(&trs->phoneroot, phoneid);
    if(node != NULL)
    {
        phone = rbt_entry(node, phonedev_t, node);
        return phone;
    }
    return NULL;
    
}


int32_t foss_dispatch_cb(rns_hconn_t* hconn, void* data)
{
    trs_t* trs = (trs_t*)data;
    uint32_t retcode = 200;
    int32_t ret = 0;
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    
    LOG_DEBUG(lp, "uri : %s, size : %d, req : \n%s", info->uri, info->body_size, info->body);

    ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
    if(ret < 0)
    {
        retcode = 400;
        goto ERR_EXIT;
    }
    
    if(rns_strcmp(info->uri, "/phone/bindStb") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* stbid = rns_json_node(req, "stbid", NULL);
        rns_json_t* phoneid = rns_json_node(req, "phoneid", NULL);
        if(stbid == NULL || phoneid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phonedev_t* phone = phonedev_find(trs, (uchar_t*)phoneid->valuestring);
        if(phone == NULL)
        {
            phone = phone_create((uchar_t*)phoneid->valuestring);
            if(phone == NULL)
            {
                retcode = 400;
                goto ERR_EXIT;
            }
            rbt_set_key_str(&phone->node, (uchar_t*)phoneid->valuestring);
            rbt_insert(&trs->phoneroot, &phone->node);
        }
        stbdev_t* stb = stbdev_find(trs, (uchar_t*)stbid->valuestring);
        if(stb == NULL)
        {
            stb = stb_create2(info->body, info->body_size);
            if(stb == NULL)
            {
                retcode = 400;
                goto ERR_EXIT;
            }
            rbt_set_key_str(&stb->node, (uchar_t*)stbid->valuestring);
            rbt_insert(&trs->stbroot, &stb->node);
            stb->trsid = rns_dup(trs->id);
        }
        phone_bindStb(hconn, trs->dbpool, trs->foss, phone, stb, 0);
    }
    else if(rns_strcmp(info->uri, "/phone/unbindStb") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* phoneid = rns_json_node(req, "phoneid", NULL);
        if(phoneid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phonedev_t* phone = phonedev_find(trs, (uchar_t*)phoneid->valuestring);
        if(phone == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phone_unbindStb(hconn, trs->dbpool, trs->foss, phone, 0);
    }
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
int32_t dispatch_cb(rns_hconn_t* hconn, void* data)
{
    trs_t* trs = (trs_t*)data;
    uint32_t retcode = 200;
    int32_t ret = 0;
    rns_http_info_t* info = rns_hconn_req_info(hconn);

    LOG_INFO(lp, "uri : %s, size : %d, req : %s", info->uri, info->body_size, info->body);
    
    if(rns_strcmp(info->uri, "/phone/bindStb") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* stbid = rns_json_node(req, "stbid", NULL);
        rns_json_t* phoneid = rns_json_node(req, "phoneid", NULL);
        if(stbid == NULL || phoneid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phonedev_t* phone = phonedev_find(trs, (uchar_t*)phoneid->valuestring);
        if(phone == NULL)
        {
            phone = phone_create((uchar_t*)phoneid->valuestring);
            if(phone == NULL)
            {
                retcode = 400;
                goto ERR_EXIT;
            }
            rbt_set_key_str(&phone->node, (uchar_t*)phoneid->valuestring);
            rbt_insert(&trs->phoneroot, &phone->node);
        }
        stbdev_t* stb = stbdev_find(trs, (uchar_t*)stbid->valuestring);
        if(stb == NULL)
        {
            stb = stb_create2(info->body, info->body_size);
            if(stb == NULL)
            {
                retcode = 400;
                goto ERR_EXIT;
            }
            stb->trsid = rns_dup(trs->id);
            rbt_set_key_str(&stb->node, (uchar_t*)stbid->valuestring);
            rbt_insert(&trs->stbroot, &stb->node);
        }
        phone_bindStb(hconn, trs->dbpool, trs->foss, phone, stb, 1);
        rns_json_destroy(&req);
    }
    else if(rns_strcmp(info->uri, "/phone/unbindStb") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* phoneid = rns_json_node(req, "phoneid", NULL);
        if(phoneid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phonedev_t* phone = phonedev_find(trs, (uchar_t*)phoneid->valuestring);
        if(phone == NULL)
        {
            LOG_ERROR(lp, "phone is not exist, phoneid : %s", phoneid->valuestring);
            retcode = 400;
            goto ERR_EXIT;
        }
        
        phone_unbindStb(hconn, trs->dbpool, trs->foss, phone, 1);
        rns_json_destroy(&req);
    }
    else if(rns_strcmp(info->uri, "/phone/screen") == 0 || rns_strcmp(info->uri, "/phone/control") == 0)
    {
        screen_info_t* sinfo = scree_info_create(info->body, info->body_size);
        if(sinfo == NULL)
        {
            retcode = 400;
            goto ERR_EXIT; 
        }
        
      
        phonedev_t* phonedev = phonedev_find(trs, sinfo->phoneid);
        if(phonedev == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        phone_screen(hconn, phonedev, sinfo, 0, 0);
        
        if(phonedev->stb != NULL)
        {
            LOG_INFO(lp, "phone control stb bind");
            rbt_node_t* node = rbt_search_str(&trs->stbs, phonedev->stb->stbid);
            if(node != NULL)
            {
                LOG_INFO(lp, "stb connected");
                stb_conn_t* n = rbt_entry(node, stb_conn_t, snode);
                stb_screen(n->hconn, trs, phonedev->stb);
            }
        }
    }
    else if(rns_strcmp(info->uri, "/stb/bindphonelist") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"stbid");
        if(node == NULL)
        {
            retcode = 401;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        stbdev_t* stb = stbdev_find(trs, arg->data);
        if(stb == NULL)
        {
            LOG_ERROR(lp, "stb is not found, stbid : %s", arg->data);
            retcode = 401;
            goto ERR_EXIT;
        }
        stb_bindphonelist(hconn, stb);
    }
    else if(rns_strcmp(info->uri, "/stb/screen") == 0 || rns_strcmp(info->uri, "/stb/getScreenInfo") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"stbid");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        stbdev_t* stb = stbdev_find(trs, arg->data);
        if(stb == NULL)
        {
            LOG_ERROR(lp, "stb is not exist stb id : %s", arg->data);
            retcode = 401;
            goto ERR_EXIT;
        }
        LOG_DEBUG(lp, "stb screen : %s", arg->data);
        stb_conn_t* stbconn = NULL;
        rbt_node_t* snode = rbt_search_str(&trs->stbs, arg->data);
        if(snode == NULL)
        {
            stbconn = (stb_conn_t*)rns_malloc(sizeof(stb_conn_t));
            if(stbconn == NULL)
            {
                retcode = 401;
                goto ERR_EXIT;
            }
            stbconn->hconn = hconn;
            rbt_set_key_str(&stbconn->snode, arg->data);
            rbt_insert(&trs->stbs, &stbconn->snode);

            rbt_set_key_int(&stbconn->hnode, (uint64_t)hconn);
            rbt_insert(&trs->hconns, &stbconn->hnode);
        }
        
        stb_screen(hconn, trs, stb);
    }
    else if(rns_strcmp(info->uri, "/stb/saveInfo") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* stbid = rns_json_node(req, "stbid", NULL);
        if(stbid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* volume = rns_json_node(req, "volume", NULL);
        if(volume == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        stbdev_t* stb = stbdev_find(trs, (uchar_t*)stbid->valuestring);
        if(stb == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }

        stb->StbVolume = volume->valueint;
        ret = rns_hconn_resp2(hconn, 200, NULL, 0, 0);
        if(ret < 0)
        {
            LOG_ERROR(lp, "resp failed, ret : %d", ret);
        }
        rns_json_destroy(&req);
    }
    else if(rns_strcmp(info->uri, "/stb/sendInfo") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* stbid = rns_json_node(req, "stbid", NULL);
        if(stbid == NULL)
        {
            LOG_DEBUG(lp, "stbid is not found in req");
            retcode = 400;
            goto ERR_EXIT;
        }
        stbdev_t* stb = stbdev_find(trs, (uchar_t*)stbid->valuestring);
        if(stb == NULL)
        {
            LOG_DEBUG(lp, "stb is not found in db, stbid : %s", stbid->valuestring);
            retcode = 400;
            goto ERR_EXIT;
        }
        stb_sendInfo(hconn, trs->dbpool, stb);
        rns_json_destroy(&req);
    }
    else if(rns_strcmp(info->uri, "/stb/switchuser") == 0)
    {
        rns_json_t* req = rns_json_read(info->body, info->body_size);
        if(req == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* stbid = rns_json_node(req, "stbid", NULL);
        if(stbid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_json_t* stbuserid = rns_json_node(req, "StbUserId", NULL);
        if(stbuserid == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        stbdev_t* stb = stbdev_find(trs, (uchar_t*)stbid->valuestring);
        if(stb == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        if(stb->StbUserId && rns_strcmp(stbuserid->valuestring, stb->StbUserId) == 0)
        {
            stb_switchUser(hconn, trs->dbpool, trs->id, stb, (uchar_t*)stbuserid->valuestring);
        }

        rns_json_destroy(&req);
    }
    //--------------------------------stb playhistory--------------------------------
    else if(rns_strcmp(info->uri, "/stb/playhistory/add") == 0)
    {
        stb_playhistory_t* playhistory = stb_playhistory_create(info->body, info->body_size);
        if(playhistory == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        playhistory->addtime = time(NULL);
        stb_playhistory_add(hconn, trs->dbpool, playhistory);
        stb_destroy_playhistory(&playhistory);
    }
    else if(rns_strcmp(info->uri, "/stb/playhistory/get") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"StbUserId");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        stb_ps_get(hconn, trs->dbpool, trs, (char_t*)arg->data, 0, 0);
    }
    //---------------------------------stb mark-------------------------------------
    else if(rns_strcmp(info->uri, "/stb/marks/get") == 0)
    {
        
        
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"StbUserId");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        stb_mark_get(hconn, trs->dbpool, trs, (char_t*)arg->data, 0, 0);
    }
    else if(rns_strcmp(info->uri, "/stb/marks/add") == 0)
    {
        stb_mark_t* mark = stb_mark_create(info->body, info->body_size);
        if(mark == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        mark->addtime = time(NULL);
        stb_mark_add(hconn, trs->dbpool, mark);
        stb_destroy_playhistory(&mark);
    }
    else if(rns_strcmp(info->uri, "/stb/marks/delete") == 0)
    {
        stb_playhistory_t* mark = stb_playhistory_create(info->body, info->body_size);
        if(mark == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        stb_mark_delete(hconn, trs->dbpool, mark);
        stb_destroy_playhistory(&mark);
    }
    //-----------------------------------phone playhistory-----------------------
    else if(rns_strcmp(info->uri, "/phone/playhistory/add") == 0)
    {
        phone_playhistory_t* phistory = phone_playhistory_create(info->body, info->body_size);
        if(phistory == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        phistory->addtime = time(NULL);
        phone_playhistory_add(hconn, trs->dbpool, phistory);
    }
    else if(rns_strcmp(info->uri, "/third/playhistory/get") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"StbUserId");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        phone_playhistory_get(hconn, trs->dbpool, (char_t*)arg->data, 0, 0);     
    }
    //----------------------------phone mark---------------------------------
    else if(rns_strcmp(info->uri, "/phone/marks/add") == 0)
    {
        phone_mark_t* pmark = phone_mark_create(info->body, info->body_size);
        if(pmark == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        pmark->addtime = time(NULL);
        phone_mark_add(hconn, trs->dbpool, pmark);   
    }
    else if(rns_strcmp(info->uri, "/third/marks/get") == 0)
    {
        rbt_node_t* node = rbt_search_str(&info->args, (uchar_t*)"StbUserId");
        if(node == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* arg = rbt_entry(node, rns_node_t, node);
        phone_mark_get(hconn, trs->dbpool, (char_t*)arg->data, 0, 0);   
    }
    else if(rns_strcmp(info->uri, "/phone/marks/delete") == 0)
    {
        rbt_node_t* pnode = rbt_search_str(&info->args, (uchar_t*)"phoneid");
        rbt_node_t* mnode = rbt_search_str(&info->args, (uchar_t*)"media_id");
        if(pnode == NULL || mnode == NULL)
        {
            retcode = 400;
            goto ERR_EXIT;
        }
        rns_node_t* parg = rbt_entry(pnode, rns_node_t, node);
        rns_node_t* marg = rbt_entry(mnode, rns_node_t, node);
        
        phone_mark_delete(hconn, trs->dbpool, parg->data, marg->data);
    }
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

void trs_report(void* data)
{
    trs_t* trs = (trs_t*)data;
    uint32_t len = 0;
    uchar_t buf[1024];
    while(true)
    {
        int32_t ret = rns_pipe_read(trs->pp, (uchar_t*)&len, sizeof(uint32_t));
        if(ret < 0 && errno == EAGAIN)
        {
            break;
        }
        else if(ret < 0)
        {
            continue;
        }
    
        ret = rns_pipe_read(trs->pp, buf, len);
        if(ret < 0 && errno == EAGAIN)
        {
            break;
        }
        else if(ret < 0)
        {
            continue;
        }

        rns_json_t* info = rns_json_read(buf, ret);
        if(info == NULL)
        {
            continue;
        }

        rns_json_t* type = rns_json_node(info, "type", NULL);
        switch (type->valueint)
        {
            case LOAD_STBDEV :
            {
                
                break;
            }
            default :
            {
                
            }
        }
        
    }
    
    return;
}

void dbpool_done(void* data)
{
    trs_t* trs = (trs_t*)data;

    int32_t ret = 0;
    
    rns_mysql_t* mysql = rns_dbpool_get(trs->dbpool);
    if(mysql != NULL)
    {
        stb_dao_t stbdao;
        stb_dao_init(&stbdao);
        
        rns_mysql_cb_t cb;
        cb.work  = stb_fetch;
        cb.done  = stb_fetch_done;
        cb.error = stb_fetch_error;
        cb.data  = trs;
        
        ret = stbdao.search_all(mysql, trs->id, &cb);
        if(ret < 0)
        {
            LOG_ERROR(lp, "stb find all failed");
            rns_dbpool_free(mysql);
            return;
        }
    }

    return;
}

int32_t main()
{
    int32_t retcode = 0;
    uchar_t* ptr = NULL;
    uint32_t level = 2;
    uchar_t* dlp = (uchar_t*)"./log/trs.log";
    uchar_t* logpath = NULL;
    
    rns_addr_t addr;
    

    trs_t* trs = (trs_t*)rns_malloc(sizeof(trs_t));
    if(trs == NULL)
    {
        retcode = -1;
        goto EXIT;
    }
    rbt_init(&trs->stbroot, RBT_TYPE_STR);
    rbt_init(&trs->phoneroot, RBT_TYPE_STR);
    rbt_init(&trs->stbs, RBT_TYPE_STR);
    rbt_init(&trs->hconns, RBT_TYPE_INT);
    
    trs->ep = rns_epoll_init();
    if(trs->ep == NULL)
    {
        retcode = -2;
        goto EXIT;
    }

    rns_file_t* fp = rns_file_open((uchar_t*)"./conf/trs.xml", RNS_FILE_READ);
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

    ptr = rns_xml_node_value(xml, "trs", "id", NULL);
    if(ptr == NULL)
    {
        retcode = -7;
        goto EXIT;
    }
    trs->id = rns_dup(ptr);
    
    
    ptr = rns_xml_node_value(xml, "trs", "log", "level", NULL);
    if(ptr != NULL)
    {
        level = rns_atoi(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "trs", "log", "path", NULL);
    if(ptr != NULL)
    {
        logpath = rns_malloc(rns_strlen(ptr) + 1);
        if(logpath == NULL)
        {
            retcode = -7;
            goto EXIT;
        }
        rns_memcpy(logpath, ptr, rns_strlen(ptr));
        lp = rns_log_init(logpath, level, (uchar_t*)"trs");
    }
    else
    {
        lp = rns_log_init(dlp, level, (uchar_t*)"trs");
    }

    if(lp == NULL)
    {
        retcode = -2;
        goto EXIT;
    }
    
    ret = rns_epoll_add(trs->ep, lp->pipe->fd[0], lp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        retcode = -10;
        goto EXIT;
    }


    ptr = rns_xml_node_value(xml, "trs", "http", "ip", NULL);
    if(ptr == NULL)
    {
        addr.ip = 0;
    }
    else
    {
        addr.ip = rns_ip_str2int(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "trs", "http", "port", NULL);
    if(ptr == NULL)
    {
        addr.port = 9000;
    }
    else
    {
        addr.port = rns_atoi(ptr);
    }
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work  = dispatch_cb;
    cb.close = NULL;
    cb.clean = NULL;
    cb.error = NULL;
    cb.data  = trs;

    uint32_t expire = 60000;
    ptr = rns_xml_node_value(xml, "trs", "http", "expire", NULL);
    if(ptr != NULL)
    {
        expire = rns_atoi(ptr);
    }
    
    trs->http = rns_httpserver_create(&addr, &cb, expire);
    if(trs->http == NULL)
    {
        LOG_ERROR(lp, "http server create failed, ip : %u, port : %d", addr.ip, addr.port);
        retcode = -1;
        goto EXIT;
    }
    
   

    
    ptr = rns_xml_node_value(xml, "trs", "foss", "ip", NULL);
    if(ptr == NULL)
    {
        addr.ip = 0;
    }
    else
    {
        addr.ip = rns_ip_str2int(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "trs", "foss", "port", NULL);
    if(ptr == NULL)
    {
        addr.port = 9000;
    }
    else
    {
        addr.port = rns_atoi(ptr);
    }
    
    trs->interval = 20;
    ptr = rns_xml_node_value(xml, "trs", "foss", "login_interval", NULL);
    if(ptr != NULL)
    {
        trs->interval = rns_atoi(ptr);
    }


    ptr = rns_xml_node_value(xml, "trs", "foss", "postip", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "add http server to epoll failed");
        retcode = -2;
        goto EXIT;
    }
    trs->postaddr.ip = rns_ip_str2int(ptr);
    
    ptr = rns_xml_node_value(xml, "trs", "foss", "postport", NULL);
    if(ptr == NULL)
    {
        trs->postaddr.port = 9000;
    }
    else
    {
        trs->postaddr.port = rns_atoi(ptr);
    }
    
    
    rns_http_cb_t cb2;
    rns_memset(&cb2, sizeof(rns_http_cb_t));
    cb2.work  = foss_dispatch_cb;
    cb2.close = NULL;
    cb2.clean = NULL;
    cb2.error = NULL;
    cb2.data  = trs;
    
    trs->foss = rns_httpclient_create(trs->ep, &addr, 1, &cb2);
    if(trs->foss == NULL)
    {
        LOG_ERROR(lp, "connect to foss failed, foss ip : %u, port : %d", addr.ip, addr.port);
        retcode = -1;
        goto EXIT;
    }

    rns_addr_t caddr;
    ptr = rns_xml_node_value(xml, "trs", "cms", "ip", NULL);
    if(ptr != NULL)
    {
        caddr.ip = rns_ip_str2int(ptr);
    }
    else
    {
        caddr.ip = rns_ip_str2int((uchar_t*)"117.71.39.8");
    }
    caddr.port = 80;
    ptr = rns_xml_node_value(xml, "trs", "cms", "port", NULL);
    if(ptr != NULL)
    {
        caddr.port = rns_atoi(ptr);
    }
    trs->cms = rns_httpclient_create(trs->ep, &caddr, 5, NULL);
    
    rns_dbpara_t * db_param = (rns_dbpara_t*)rns_malloc(sizeof(rns_dbpara_t));
    if(db_param == NULL)
    {
        retcode = -3;
        goto EXIT;
    }
    ptr = rns_xml_node_value(xml, "trs", "db", "ip", NULL);
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

    ptr = rns_xml_node_value(xml, "trs", "db", "port", NULL);
    if(ptr == NULL)
    {
        LOG_ERROR(lp, "doesn't config db port");

        retcode = -4;
        goto EXIT;
    }
    db_param->port = rns_atoi(ptr);
    
    ptr = rns_xml_node_value(xml, "trs", "db", "user", NULL);
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
    
    ptr = rns_xml_node_value(xml, "trs", "db", "passwd", NULL);
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

    
    ptr = rns_xml_node_value(xml, "trs", "db", "database", NULL);
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
    ptr = rns_xml_node_value(xml, "trs", "db", "min", NULL);
    if(ptr != NULL)
    {
        min = rns_atoi(ptr);
    }

    uint32_t max = 16;
    ptr = rns_xml_node_value(xml, "trs", "db", "max", NULL);
    if(ptr != NULL)
    {
        max = rns_atoi(ptr);
    }
    
    trs->pp = rns_pipe_open();
    if(trs->pp == NULL)
    {
        LOG_ERROR(lp, "open pipe failed");
        retcode = -3;
        goto EXIT;
    }

    trs->pp->cb.func = trs_report;
    trs->pp->cb.data = trs;
    
    ret = rns_epoll_add(trs->ep, trs->pp->fd[0], trs->pp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add pipe to http epoll failed");
        retcode = -14;
        goto EXIT;
    }
    

    rns_cb_t pcb;
    pcb.func = dbpool_done;
    pcb.data = trs;
    
    trs->dbpool = rns_dbpool_create(trs->ep, db_param, min, max, &pcb);
    if(trs->dbpool == NULL)
    {
        LOG_ERROR(lp, "db pool create failed, db ip : %s, port : %d, user : %s", db_param->hostip, db_param->port, db_param->username);
        retcode = -4;
        goto EXIT;
    }

    uint64_t nofile = 655360;
    ptr = rns_xml_node_value(xml, "trs", "nofile", NULL);
    if(ptr != NULL)
    {
        nofile = rns_atoi(ptr);
    }


    
    
    rns_limit_set(RLIMIT_NOFILE, nofile, nofile);
    rns_limit_get(RLIMIT_NOFILE, &nofile, &nofile);
    LOG_INFO(lp, "nofile : %llu", nofile);
    
    rns_epoll_wait(trs->ep);

    return 0;	

EXIT:

    return retcode;
    
}
