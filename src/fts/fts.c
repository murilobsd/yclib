#include "fts.h"
#include "rns_json.h"
#include "rns_xml.h"
#include "rns_log.h"


#define FTS_TASK_TYPE_VIDEOINFO 0
#define FTS_TASK_TYPE_FILE2IMG  1


rns_log_t* lp = NULL;




int32_t dispatch(rns_hconn_t* hconn, void* data)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    
    LOG_INFO(lp, "req info, uri : %s, client ip : %u, port : %u", info->uri, hconn->addr.ip, hconn->addr.port);

    rns_json_t* json = rns_json_read(info->body, rns_strlen(info->body));
    if(json == NULL)
    {
        LOG_ERROR(lp, "parse req body failed, body : %s\n", info->body);
        goto ERR_EXIT;
    }

    
    rns_json_t* task = rns_json_node(json, "task", NULL);
    if(task != NULL)
    {
        
    }

    rns_json_t* resp = rns_json_create_obj();
    rns_json_t* rarray = rns_json_create_array();
    if(resp == NULL || rarray == NULL)
    {
        goto ERR_EXIT;
    }
    rns_json_add(resp, (uchar_t*)"response", rarray);
    
    uint32_t size = rns_json_array_size(task);
    for(uint32_t i = 0; i < size; ++i)
    {
        rns_json_t* t = rns_json_array_node(task, i);
        if(t == NULL)
        {
            continue;
        }

        rns_json_t* tasktype = rns_json_node(t, "tasktype", NULL);
        if(tasktype == NULL)
        {
            continue;
        }
        rns_json_t* taskindex = rns_json_node(t, "taskindex", NULL);
        if(taskindex == NULL)
        {
            continue;
        }

        switch (tasktype->valueint)
        {
            case FTS_TASK_TYPE_VIDEOINFO :
            {
                rns_json_t* ipath = rns_json_node(t, "input", "path", NULL);
                if(ipath == NULL)
                {
                    break;
                }

                uint64_t duration = 0;
                uint64_t s = 0;
                int32_t ret = fts_videoinfo((uchar_t*)ipath->valuestring, &duration, &s);
                if(ret < 0)
                {
                    
                }

                rns_json_t* r = rns_json_create_obj();

                ret = rns_json_add_int(r, (uchar_t*)"tasktype", tasktype->valueint);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }
                ret = rns_json_add_int(r, (uchar_t*)"taskindex", taskindex->valueint);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }
                ret = rns_json_add_int(r, (uchar_t*)"duration", duration);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }
                ret = rns_json_add_int(r, (uchar_t*)"size", s);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }

                rns_json_add(rarray, NULL, r);
                
                break;
                
            }
            case FTS_TASK_TYPE_FILE2IMG :
            {
                rns_json_t* ipath = rns_json_node(t, "input", "path", NULL);
                if(ipath == NULL)
                {
                    break;
                }
                
                rns_json_t* otype = rns_json_node(t, "output", "outtype", NULL);
                if(otype == NULL)
                {
                    break;
                }
                rns_json_t* start = rns_json_node(t, "output", "start", NULL);
                if(start == NULL)
                {
                    break;
                }
                rns_json_t* interval = rns_json_node(t, "output", "interval", NULL);
                if(interval == NULL)
                {
                    break;
                }
                rns_json_t* number = rns_json_node(t, "output", "number", NULL);
                if(number == NULL)
                {
                    break;
                }

                list_head_t* head = fts_file2img((uchar_t*)ipath->valuestring,  (uchar_t*)otype->valuestring, start->valueint, interval->valueint, number->valueint);
                if(head == NULL)
                {
                    LOG_ERROR(lp, "fts file 2 img failed, input path : %s,  start time : %d, interval : %d, number : %d, ret : %d", ipath->valuestring, start->valueint, interval->valueint, number->valueint);
                    goto ERR_EXIT;
                }

                rns_json_t* r = rns_json_create_obj();
                rns_json_t* uarray = rns_json_create_array();
                if(r == NULL || uarray == NULL)
                {
                    goto ERR_EXIT;
                }
                rns_json_add(rarray, NULL, r);
                
                int32_t ret = rns_json_add_int(r, (uchar_t*)"tasktype", tasktype->valueint);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }
                ret = rns_json_add_int(r, (uchar_t*)"taskindex", taskindex->valueint);
                if(ret < 0)
                {
                    goto ERR_EXIT;
                }
                rns_json_add(r, (uchar_t*)"path", uarray);
                
                list_head_t* p = NULL;
                rns_list_for_each(p, head)
                {
                    rns_list_t* rlist = rns_list_entry(p, rns_list_t, list);
                    ret = rns_json_add_str(uarray, (uchar_t*)"uri", rlist->data);
                    if(ret < 0)
                    {
                        
                    }
                }

                list_head_t* q = NULL;
                rns_list_for_each_safe(p, q, head)
                {
                    rns_list_t* rlist = rns_list_entry(p, rns_list_t, list);
                    rns_free(rlist->data);
                    rns_free(rlist);
                }
                
                break;
            }
        }
        
    }
    
    uint32_t respsize = 0;
    uchar_t* buf = rns_json_write(resp);
    if(buf != NULL)
    {
        respsize = rns_strlen(buf);
    }
    
    int32_t ret = rns_hconn_resp_header_init(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "init resp header failed, uri : %s, ret : %d", info->uri, ret);
        goto ERR_EXIT;
    }

    uchar_t str[32];
    snprintf((char_t*)str, 32, "%d", respsize);
    ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", str);
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

    ret = rns_http_attach_buffer(hconn, buf, respsize, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "attach buf to client failed, body size : %d", respsize);
    }
    
    ret = rns_hconn_resp(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp done failed, ret : %d", ret);
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
    return info->body_size;
}

int32_t main()
{
    int32_t retcode = 0;
    uchar_t* ptr = NULL;
    uint32_t level = 2;
    uchar_t* logpath = NULL;
    uchar_t* dlp = (uchar_t*)"./log/fts.log";
    mfts_t* mfts = (mfts_t*)rns_malloc(sizeof(mfts_t));
    if(mfts == NULL)
    {
        goto EXIT;
    }
    
    mfts->ep = rns_epoll_init();
    if(mfts->ep == NULL)
    {
        retcode = -2;
        goto EXIT;
    }

    rns_file_t* fp = rns_file_open((uchar_t*)"./conf/fts.xml", RNS_FILE_READ);
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

    ptr = rns_xml_node_value(xml, "fts", "log", "level", NULL);
    if(ptr != NULL)
    {
        level = rns_atoi(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "fts", "log", "path", NULL);
    if(ptr != NULL)
    {
        logpath = rns_malloc(rns_strlen(ptr) + 1);
        if(logpath == NULL)
        {
            retcode = -7;
            goto EXIT;
        }
        rns_memcpy(logpath, ptr, rns_strlen(ptr));
        lp = rns_log_init(logpath, level, (uchar_t*)"fts");
    }
    else
    {
        lp = rns_log_init(dlp, level, (uchar_t*)"fts");
    }

    if(lp == NULL)
    {
        retcode = -2;
        goto EXIT;
    }
    
    ret = rns_epoll_add(mfts->ep, lp->pipe->fd[0], lp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        retcode = -10;
        goto EXIT;
    }

    rns_addr_t addr;
    ptr = rns_xml_node_value(xml, "fts", "http", "ip", NULL);
    if(ptr == NULL)
    {
        addr.ip = 0;
    }
    else
    {
        addr.ip = rns_ip_str2int(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "fts", "http", "port", NULL);
    if(ptr == NULL)
    {
        addr.port = 6510;
    }
    else
    {
        addr.port = rns_atoi(ptr);
    }
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.work = dispatch;
    cb.close = NULL;
    cb.error = NULL;
    cb.data = mfts;

    mfts->http = rns_httpserver_create(&addr, &cb);
    if(mfts->http == NULL)
    {
        retcode = -3;
        goto EXIT;
    }

    ret = rns_epoll_add(mfts->ep, mfts->http->listenfd, mfts->http, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add listen port to epoll failed, ip : %d, port : %d", addr.ip, addr.port);
        retcode = -4;
        goto EXIT;
    }

    av_register_all();
    rns_epoll_wait(mfts->ep);

EXIT:
    if(mfts != NULL)
    {
        if(logpath != NULL)
        {
            rns_free(logpath);
        }
        rns_log_destroy(&lp);
        rns_http_destroy(&mfts->http);
        rns_free(mfts);
    }
    return retcode;
}

