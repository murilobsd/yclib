#include "rtl.h"
#include "rns_xml.h"
#include "rns_json.h"
#include <errno.h>
#include <stdio.h>

#define RTL_EVENT_TYPE_STERAM 0
#define RTL_EVENT_TYPE_RECORD 1


rns_log_t* lp;

uint32_t rtl_report(uchar_t* uri, rns_json_t* info, uchar_t* buf, uint32_t size)
{
    uint32_t bodysize = 0;
    
    uint32_t len = snprintf((char_t*)buf, size,
                            "POST %s HTTP/1.1\r\n"
                            "Accept: */*\r\n"
                            "Accept-Language: zh-cn\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: keepalive\r\n\r\n%s", uri, bodysize, buf);
    
    
    return len;
}

mgr_t* rtl_mgr_create(rns_epoll_t* ep, rns_addr_t* addr)
{
    mgr_t* mgr = (mgr_t*)rns_malloc(sizeof(mgr_t));
    if(mgr == NULL)
    {
        return NULL;
    }

    
    mgr->hc = rns_httpclient_create(ep, addr, 1, NULL);
    if(mgr->hc == NULL)
    {
        rns_free(mgr);
        return NULL;
    }

    return mgr;
}

void rtl_report_done(rns_hconn_t* hconn)
{
    rns_hconn_free(hconn);
    return;
}

void rtl_report_stream(rtl_t* rtl)
{
    rns_json_t* info = NULL;
    uchar_t* buf = rns_malloc(1024);
    if(buf == NULL)
    {
        LOG_ERROR(lp, "");
        return;
    }
    uint32_t size = rtl_report(rtl->mgr->stream_publish_uri, info, buf, 1024);

    rns_json_destroy(&info);
    
    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.done = rtl_report_done;
    
    rns_hconn_t* hconn = rns_hconn_get(rtl->mgr->hc, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn falied, size : %d,  req : \n%s", size, buf);
        return;
    }
    
    int32_t ret = rns_hconn_req(hconn, buf, size, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "yms req failed, ret : %d, size : %d", ret, size);
        return;
    }
    
    return;
}

void rtl_report_record(rtl_t* rtl)
{
    rns_json_t* info = NULL;
    uchar_t* buf = rns_malloc(1024);
    if(buf == NULL)
    {
        LOG_ERROR(lp, "");
        return;
    }
    uint32_t size = rtl_report(rtl->mgr->stream_record_uri, info, buf, 1024);
    rns_json_destroy(&info);

    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.done = rtl_report_done;
    
    rns_hconn_t* hconn = rns_hconn_get(rtl->mgr->hc, NULL);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn falied, size : %d,  req : \n%s", size, buf);
        return;
    }
    
    int32_t ret = rns_hconn_req(hconn, buf, size, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "yms req failed, ret : %d, size : %d", ret, size);
        return;
    }
    
    return;
}

void rtl_ctl(void* data)
{
    rtl_t* rtl = (rtl_t*)data;
    uint32_t len = 0;
    while(true)
    {
        int32_t ret = rns_pipe_read(rtl->rp, (uchar_t*)&len, sizeof(uint32_t));
        if(ret < 0 && errno == EAGAIN)
        {
            break;
        }
        else if(ret < 0)
        {
            LOG_ERROR(lp, "read inform pipe failed, ret : %d", ret);
            break;
        }
        ret = rns_pipe_read(rtl->rp, rtl->reportbuf, len);
        if(ret < 0 && errno == EAGAIN)
        {
            break;
        }
        else if(ret < 0)
        {
            LOG_ERROR(lp, "read inform pipe failed, ret : %d", ret);
            break;
        }

        rns_json_t* info = rns_json_read(rtl->reportbuf, len);
        if(info == NULL)
        {
            LOG_ERROR(lp, "json create failed");
            continue;
        }

        uint32_t type = 0;
        switch (type)
        {
            case RTL_EVENT_TYPE_STERAM:
            {
                rtl_report_stream(rtl);
                break;
            }
            case RTL_EVENT_TYPE_RECORD :
            {
                rtl_report_record(rtl);
                break;
            }
            default :
            {
                LOG_ERROR(lp, "report info may be wrong, info : \n%s", rtl->reportbuf);
            }
            
        }
        
    }
    
    return;
}

void rtl_login(mgr_t* mgr)
{
    rns_json_t* info = NULL;
    uchar_t* buf = rns_malloc(1024);
    if(buf == NULL)
    {
        LOG_ERROR(lp, "");
        return;
    }
    uint32_t size = rtl_report(mgr->login_uri, info, buf, 1024);
    rns_json_destroy(&info);

    rns_http_cb_t cb;
    rns_memset(&cb, sizeof(rns_http_cb_t));
    cb.done = rtl_report_done;
    
    rns_hconn_t* hconn = rns_hconn_get(mgr->hc, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn falied, size : %d,  req : \n%s", size, buf);
        return;
    }
    
    int32_t ret = rns_hconn_req(hconn, buf, size, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "yms req failed, ret : %d, size : %d", ret, size);
        return;
    }
    
    return;
}

void rtl_keepalive(void* data)
{
    rtl_t* rtl = (rtl_t*)data;
    rtl_login(rtl->mgr);
    
    int32_t ret = rns_timer_setdelay(rtl->timer, 10000);
    if(ret < 0)
    {
        LOG_ERROR(lp, "set keepalive timer failed, ret : %d", ret);
    }
    
    return;
}


int32_t main()
{
    int32_t retcode = 0;
    int32_t ret = 0;
    uchar_t* ptr = NULL;
    
    rtl_t* rtl = (rtl_t*)rns_malloc(sizeof(rtl_t));
    if(rtl == NULL)
    {
        retcode = -1;
        goto ERR_EXIT;
    }

    rtl->ep = rns_epoll_init();
    if(rtl->ep == NULL)
    {
        retcode = -2;
        goto ERR_EXIT;
    }

    rns_file_t* fp = rns_file_open((uchar_t*)"./conf/rtl.xml", RNS_FILE_READ);
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

    ret = rns_file_read(fp, cfgbuf, size);
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

    uint32_t loglevel = 2;
    ptr = rns_xml_node_value(xml, "rtl", "log", "level", NULL);
    if(ptr != NULL)
    {
        loglevel = rns_atoi(ptr);
    }

    ptr = rns_xml_node_value(xml, "rtl", "log", "path", NULL);
    if(ptr == NULL)
    {
        lp = rns_log_init((uchar_t*)"./log/rtl.log", loglevel, (uchar_t*)"rtl");
    }
    else
    {
        lp = rns_log_init(ptr, loglevel, (uchar_t*)"rtl");
    }

    if(lp == NULL)
    {
        retcode = -7;
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(rtl->ep, lp->pipe->fd[0], lp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        retcode = -8;
        goto ERR_EXIT;
    }

    rns_addr_t addr;
    rns_memset(&addr, sizeof(rns_addr_t));
    ptr = rns_xml_node_value(xml, "rtl", "rtmp", "ip", NULL);
    if(ptr != NULL)
    {
        addr.ip = rns_ip_str2int(ptr);
    }

    addr.port = 1935;
    ptr = rns_xml_node_value(xml, "rtl", "rtmp", "port", NULL);
    if(ptr != NULL)
    {
        addr.port = rns_atoi(ptr);
    }

    ptr = rns_xml_node_value(xml, "rtl", "app", NULL);
    if(ptr == NULL)
    {
        retcode = -81;
        goto ERR_EXIT;
    }
    rns_rtmp_cb_t cb;
    
    rtl->rtmp = rns_rtmpserver_create(ptr, &addr, &cb);
    if(rtl->rtmp == NULL)
    {
        retcode = -9;
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(rtl->ep, rtl->rtmp->listenfd, rtl->rtmp, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        retcode = -10;
        goto ERR_EXIT;
    }

    rtl->timer = rns_timer_create();
    if(rtl->timer == NULL)
    {
        retcode = -11;
        goto ERR_EXIT;
    }

    rns_cb_t tcb;
    tcb.func = rtl_keepalive;
    tcb.data = rtl;
    ret = rns_timer_set(rtl->timer, 1000, &tcb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "set timer to http failed");
        retcode = -12;
        goto ERR_EXIT;
    }
    
    ret = rns_epoll_add(rtl->ep, rtl->timer->timerfd, rtl->timer, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        retcode = -13;
        goto ERR_EXIT;
    }

    rns_addr_t mgraddr;
    
    rtl->mgr = rtl_mgr_create(rtl->ep, &mgraddr);
    if(rtl->mgr ==  NULL)
    {
        retcode = -14;
        goto ERR_EXIT;
    }

    rtl->rp = rns_pipe_open();
    if(rtl->rp == NULL)
    {
        retcode = -15;
        goto ERR_EXIT;
    }

    rtl->rp->cb.func = rtl_ctl;
    rtl->rp->cb.data = rtl;
    
    ret = rns_epoll_add(rtl->ep, rtl->rp->fd[0], rtl->rp, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        retcode = -16;
        goto ERR_EXIT;
    }
    
    rns_epoll_wait(rtl->ep);
    
    return 0;
    
ERR_EXIT:

    return retcode;
}
