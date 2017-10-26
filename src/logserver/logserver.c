#include "logserver.h"
#include <time.h>
#include <stdio.h>
#include "rns_xml.h"

rns_log_t* lp = NULL;

logfile_t* logfile_open(uchar_t* path)
{
    logfile_t* lf = (logfile_t*)rns_malloc(sizeof(logfile_t));
    if(lf == NULL)
    {
        return NULL;
    }

    lf->fp = rns_file_open2(path, RNS_FILE_OPEN | RNS_FILE_RW);
    if(lf->fp == NULL)
    {
        rns_free(lf);
        return NULL;
    }

    return lf;
}

void logfile_close(logfile_t** lf)
{
    if(*lf == NULL)
    {
        return;
    }

    time_t t = time(NULL);
    struct tm tm;
    uchar_t buf[64];
    
    rns_time_time2tml(&t, &tm);
    rns_time_tm2str(&tm, buf, 64, (uchar_t*)"%Y%m%d%H%M%S");

    
    uint32_t len = rns_strlen((*lf)->fp->name);
    uchar_t* oldpath = rns_malloc(len + 1);
    uchar_t* newpath = rns_malloc(len + 16);
    if(oldpath != NULL)
    {
        rns_memcpy(oldpath, (*lf)->fp->name, len);
    }

    
    rns_file_close(&(*lf)->fp);


    if(newpath != NULL && oldpath != NULL)
    {
        snprintf((char_t*)newpath, len + 16, "%s-%s", oldpath, buf);
        int32_t ret = rns_file_rename(oldpath, newpath);
        if(ret < 0)
        {
            
        }
    }
    
    rns_free(*lf);
    
    return;
}

int32_t logserver_req_dispatch(rns_hconn_t* hconn, void* data)
{
    logserver_t* ls = (logserver_t*)data;
    uchar_t* path = NULL;
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    
    LOG_INFO(lp, "req info, uri : %s, client ip : %u, port : %u", info->uri, hconn->addr.ip, hconn->addr.port);
    
    int32_t ret = rns_hconn_resp_header_init(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "init resp header failed, uri : %s, ret : %d", info->uri, ret);
        goto ERR_EXIT;
    }
    
    
    logfile_t* lf = NULL;
    time_t utc = time(NULL);
    struct tm tm;
    uchar_t tbuf[64];
    
    rns_time_time2tml(&utc, &tm);
    rns_time_tm2str(&tm, tbuf, 64, (uchar_t*)"%Y%m%d%H%M%S");

    
    path = rns_malloc(rns_strlen(info->uri) + rns_strlen(ls->root) + 2);
    if(path == NULL)
    {
        goto ERR_EXIT;
    }
    snprintf((char_t*)path, rns_strlen(info->uri) + rns_strlen(ls->root) + 2, "%s/%s", ls->root, info->uri);

    uint32_t l = rns_strlen(path);
    
    while(l > 0 && path[l - 1] == '/')
    {
        path[l - 1] = 0;
        --l;
    }
    if(rns_strlen(path) == 0)
    {
        goto ERR_EXIT;
    }
    
    rbt_node_t* node = rbt_search_str(&ls->fds, path);
    if(node == NULL)
    {
        lf = logfile_open(path);
        if(lf == NULL)
        {
            goto ERR_EXIT;
        }

        rbt_set_key_str(&lf->node, path);
        rbt_insert(&ls->fds, &lf->node);
    }
    else
    {
        lf = rbt_entry(node, logfile_t, node);
        uint32_t size = rns_file_size(lf->fp);
        if(size >= ls->size)
        {
            rbt_delete(&ls->fds, &lf->node);
            logfile_close(&lf);
            lf = logfile_open(path);
            if(lf == NULL)
            {
                goto ERR_EXIT;
            }
            rbt_set_key_str(&lf->node, path);
            rbt_insert(&ls->fds, &lf->node);
        }
    }

    
    uchar_t t[128];
    uint32_t len = snprintf((char_t*)t, 128, "%u|%s|", utc, tbuf);
    int32_t size = rns_file_write(lf->fp, t, len);
    if(size < (int32_t)len || len >= 128)
    {
        goto ERR_EXIT;
    }
    size = rns_file_write(lf->fp, info->body, info->body_size);
    if(size < (int32_t)info->body_size)
    {
        goto ERR_EXIT;
    }
    if(info->curr_size >= info->body_size)
    {
        size = rns_file_write(lf->fp, (uchar_t*)"\n", 1);
        if(size != 1)
        {
            goto ERR_EXIT;
        }
    }

    ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", (uchar_t*)"0");
    if(ret < 0)
    {
        LOG_ERROR(lp, "insert respose header  failed, content length : 0");
        goto ERR_EXIT;
    }
    
    ret = rns_hconn_resp_header(hconn, 200);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp failed, ret : %d", ret);
    }

    ret = rns_hconn_resp(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp done failed, ret : %d", ret);
    }
    if(path != NULL)
    {
        rns_free(path);
    }

    return info->body_size;
    
ERR_EXIT:
    ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", (uchar_t*)"0");
    if(ret < 0)
    {
        LOG_ERROR(lp, "insert respose header  failed, content length : 0");
    }
    ret = rns_hconn_resp_header(hconn, 500);
    if(ret < 0)
    {
        LOG_ERROR(lp, "response to client failed, ret : %d", ret);
    }
    ret = rns_hconn_resp(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp done failed, ret : %d", ret);
    }
    if(path != NULL)
    {
        rns_free(path);
    }

    return info->body_size;
}

int32_t main()
{
    int32_t retcode = 0;
    uint32_t level = 0;
    uchar_t* ptr = NULL;
    
    logserver_t* ls = (logserver_t*)rns_malloc(sizeof(logserver_t));
    if(ls == NULL)
    {
        retcode = -1;
        goto ERR_EXIT;
    }

    rbt_init(&ls->fds, RBT_TYPE_STR);
    
    rns_file_t* fp = rns_file_open((uchar_t*)"./conf/logserver.xml", RNS_FILE_READ);
    if(fp == NULL)
    {
        retcode = -3;
        goto ERR_EXIT;
    }
    
    int32_t size = rns_file_size(fp);
    if(size < 0)
    {
        retcode = -4;
        goto ERR_EXIT;
    }
    
    uchar_t* cfgbuf = rns_malloc(size + 1);
    
    int32_t ret = rns_file_read(fp, cfgbuf, size);
    if(ret != size)
    {
        retcode = -5;
        goto ERR_EXIT;
    }
    
    rns_xml_t* xml = rns_xml_create(cfgbuf, size);
    if(xml == NULL)
    {
        retcode = -6;
        goto ERR_EXIT;
    }
    
    ls->ep = rns_epoll_init();
    if(ls->ep == NULL)
    {
        retcode = -4;
        goto ERR_EXIT;
    }

    ls->size = 100000000;
    ptr = rns_xml_node_value(xml, "logserver", "size", NULL);
    if(ptr != NULL)
    {
        ls->size = rns_atoi(ptr);
    }

    ptr = rns_xml_node_value(xml, "logserver", "rootdir", NULL);
    if(ptr != NULL)
    {
        ls->root = rns_malloc(rns_strlen(ptr) + 1);
        if(ls->root == NULL)
        {
            retcode = -41;
            goto ERR_EXIT;
        }

        rns_memcpy(ls->root, ptr, rns_strlen(ptr));
    }
    else
    {
        retcode = -5;
        goto ERR_EXIT;
    }
    
    ptr = rns_xml_node_value(xml, "logserver", "log", "level", NULL);
    if(ptr != NULL)
    {
        level = rns_atoi(ptr);
    }
    
    
    lp = rns_log_init((uchar_t*)"./log/logserver.txt", level, (uchar_t*)"logserver");
    if(lp == NULL)
    {
        retcode = -2;
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(ls->ep, lp->pipe->fd[0], lp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        retcode = -10;
        goto ERR_EXIT;
    }
    
    rns_addr_t addr;
    ptr = rns_xml_node_value(xml, "logserver", "http", "ip", NULL);
    if(ptr == NULL)
    {
        addr.ip = rns_ip_str2int((uchar_t*)"127.0.0.1");
    }
    else
    {
        addr.ip = rns_ip_str2int(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "logserver", "http", "port", NULL);
    if(ptr == NULL)
    {
        addr.port = 6600;
    }
    else
    {
        addr.port = rns_atoi(ptr);
    }

    uint32_t expire = 60000;
    ptr = rns_xml_node_value(xml, "logserver", "http", "expire", NULL);
    if(ptr != NULL)
    {
        expire = rns_atoi(ptr);
    }
    
    
    rns_http_cb_t lscb;
    rns_memset(&lscb, sizeof(rns_http_cb_t));
    lscb.work = logserver_req_dispatch;
    lscb.close = NULL;
    lscb.error = NULL;
    lscb.data = ls;
    
    ls->http = rns_httpserver_create(&addr, &lscb, expire);
    if(ls->http == NULL)
    {
        retcode = -3;
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(ls->ep, ls->http->listenfd, ls->http, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add listen port to epoll failed, ip : %d, port : %d", addr.ip, addr.port);
        retcode = -17;
        goto ERR_EXIT;
    }

    uint64_t nofile = 655360;
    ptr = rns_xml_node_value(xml, "logserver", "nofile", NULL);
    if(ptr != NULL)
    {
        nofile = rns_atoi(ptr);
    }
    
    rns_limit_set(RLIMIT_NOFILE, nofile, nofile);
    rns_limit_get(RLIMIT_NOFILE, &nofile, &nofile);
    LOG_INFO(lp, "nofile : %llu", nofile);
    
    rns_epoll_wait(ls->ep);
    
    
ERR_EXIT:

    if(ls != NULL)
    {
        rns_epoll_destroy(&ls->ep);
        rns_http_destroy(&ls->http);
        rns_log_destroy(&lp);
        rns_free(ls);
    }
    return retcode;
}
