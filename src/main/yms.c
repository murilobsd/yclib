#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "rns.h"
#include "rns_xml.h"
#include "rns_log.h"
#include "rns_http.h"
#include "yms_live.h"
#include "yms_vod.h"
#include "yms.h"
#include <sys/timerfd.h>
#include <errno.h>



#define RELEASE_VERSION "1.0"

#define SOAP_ENV_START	     "<SOAP-ENV:Envelope>\r\n\t<SOAP-ENV:Body>\r\n"/* SOAP协议头 */
#define SOAP_ENV_END		"\t</SOAP-ENV:Body>\r\n</SOAP-ENV:Envelope>\r\n"/* SOAP协议尾 */

#define SOAP_ENV_NAME		"SOAP-ENV:Envelope"
#define SOAP_HEADER_NAME	"SOAP-ENV:Header"
#define SOAP_BODY_NAME		"SOAP-ENV:Body"

int32_t yms_write(yms_t* yms, char_t* method, uchar_t* uri, uchar_t* buf, uint32_t size, int32_t (*func)(rns_hconn_t* hconn, void* data), void* data)
{
    uchar_t* req_buf = rns_malloc(1024);
    if(req_buf == NULL)
    {
        return -1;
    }
    uint32_t len = snprintf((char_t*)req_buf, 1024,
                            "%s %s HTTP/1.1\r\n"
                            "Accept: */*\r\n"
                            "Accept-Language: zh-cn\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: keepalive\r\n\r\n%s", method, uri, size, buf);

    LOG_DEBUG(lp, "yms req : \n%s", req_buf);
    
    rns_http_cb_t cb;
    cb.work = func;
    cb.close = NULL;
    cb.error = NULL;
    cb.data = data;

    rns_hconn_t* hconn = rns_hconn_get(yms->ocs->ohttp, &cb);
    if(hconn == NULL)
    {
        LOG_ERROR(lp, "get hconn falied, size : %d,  req : \n%s", len, req_buf);
        return -2;
    }
    
    int32_t ret = rns_hconn_req(hconn, req_buf, len, 1);
    if(ret < 0)
    {
        LOG_ERROR(lp, "yms req failed, ret : %d, size : %d", ret, len);
        return -4;
    }
    return 0;
}

int32_t yms_report_channel_state(yms_t* yms, channel_t* channel)
{
    char_t xml_buf[1024];
    int32_t xml_len;
    
    rns_memset(xml_buf, 1024);
    
    xml_len = snprintf( xml_buf, sizeof(xml_buf),
              SOAP_ENV_START
              "<chnl cds_id=\"%s\"  chnl_id=\"%s\"  status=\"%d\" stream_status=\"%d\" bitrate=\"13336233\" remark=\"\"/>"
              SOAP_ENV_END,
              "yms", channel->content_id, !channel->state, channel->state);
    
    return yms_write(yms, "post", yms->ocs->report_channel_uri, (uchar_t*)xml_buf, xml_len, NULL, NULL);
}

static int32_t yms_channel_list(rns_hconn_t* hconn, void* data)
{
    printf("channel list\n");
    yms_t* yms = (yms_t*)data;
    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    
    rns_xml_t* xml = rns_xml_create(info->body, info->body_size);
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return info->body_size;
    }
    
    LOG_INFO(lp, "channel list resp : %s\n", info->body);

    rns_xml_node_t* xml_node = rns_xml_node(xml, SOAP_ENV_NAME, SOAP_BODY_NAME, "channel", NULL);
    while(xml_node)
    {
        uchar_t* content_id = rns_xml_subnode_attr(xml_node, (uchar_t*)"content_id");
        if(content_id == NULL)
        {
            LOG_WARN(lp, "parse xml failed, no channel id found");
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        
        uchar_t* surl = rns_xml_subnode_attr(xml_node, (uchar_t*)"url");
        if(surl == NULL)
        {
            LOG_WARN(lp, "parse xml failed, no content url found, channel id : %s", content_id);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }

        rbt_node_t* cnode = rbt_search_str(&yms->channels, content_id);
        if(cnode != NULL)
        {
            LOG_INFO(lp, "the channel %s is already in the list", content_id);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        
        channel_t* channel = yms_channel_create(yms->ep, yms->pipe, content_id, surl, yms->recddir, yms->recdsize, yms->gopsize, yms->tfragsize);
        if(channel == NULL)
        {
            LOG_ERROR(lp, "create channel failed, channel id : %s, surl : %s, recddir : %s, gop size : %d, frag size : %d\n", content_id, surl, yms->recddir, yms->gopsize, yms->tfragsize);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        int32_t ret = yms_channel_start(channel);
        if(ret < 0)
        {
            LOG_ERROR(lp, "start channel failed, channel id : %s, uri : %s, recddir : %s, ret : %d\n", content_id, surl, yms->recddir, ret);
            yms_channel_destroy(&channel);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        rbt_set_key_str(&channel->node, channel->content_id);
        rbt_insert(&yms->channels, &channel->node);
        
        xml_node = rns_xml_node_next(xml_node);
    }

    rns_xml_destroy(&xml);
    
    return info->body_size;
}

static int32_t yms_resource_list(rns_hconn_t* hconn, void* data)
{
    yms_t* yms = (yms_t*)data;

    rns_http_info_t* info = rns_hconn_resp_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, info->body_size);
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return info->body_size;
    }
    LOG_INFO(lp, "%s\n", info->body);
    
    resource_t* resource = (resource_t*)rns_malloc(sizeof(resource_t));
    if(resource == NULL)
    {
        /* LOG_ERROR */
        return info->body_size;
    }
    
    /* rns_strncpy(resource->content_id, rns_xml_node_value(xml, "resource", "content_id", NULL), sizeof(resource->content_id)); */
    /* rns_strncpy(resource->state, rns_xml_node_value(xml, "resource", "status", NULL), sizeof(resource->state)); */
    /* rns_strncpy(resource->path, rns_xml_node_value(xml, "resource", "path", NULL), sizeof(resource->path)); */

    rbt_set_key_str(&resource->node, resource->content_id);
    rbt_insert(&yms->resources, &resource->node);
    return info->body_size;
}

static int32_t yms_login(yms_t* yms)
{
    uchar_t xml_buf[1024];
    int32_t xml_len;
    
    rns_memset(xml_buf, 1024);
    
    xml_len = snprintf( (char_t*)xml_buf, sizeof(xml_buf),
              SOAP_ENV_START
              "\t\t<cds peer_id=\"%s\" peer_type=\"%s\" mac=\"00:00:00:00:00:00\"  p2p_ip=\"0\" p2p_port=\"0\"  hls_port=\"0\" httpx_port=\"0\" httpinput_port=\"0\" ftp_port=\"0\" rtmp_port=\"0\" ftp_path=\"1.0\"/>\r\n"
              SOAP_ENV_END,
              yms->id, yms->type);
    
    return yms_write(yms, "GET", yms->ocs->login_uri, xml_buf, xml_len, NULL, NULL);
}

static int32_t yms_req_channels(yms_t* yms)
{
    uchar_t xml_buf[1024];
    uint32_t xml_len = 0;
    
    rns_memset(xml_buf, 1024);
    
    xml_len = snprintf( (char_t*)xml_buf, sizeof(xml_buf),
                        SOAP_ENV_START
                        "\t\t<channel peer_id=\"%s\"  page_no=\"%d\" page_size=\"20\" />\r\n"
                        SOAP_ENV_END,
                        yms->id, 0);
    
    return yms_write(yms, "GET", yms->ocs->channels_uri, xml_buf, xml_len, yms_channel_list, yms);
}

static int32_t yms_req_resources(yms_t* yms)
{
    uchar_t xml_buf[1024];
    uint32_t xml_len;
    
    rns_memset(xml_buf, 1024);
    
    xml_len = snprintf( (char_t*)xml_buf, sizeof(xml_buf),
                        SOAP_ENV_START
                        "\t\t <file peer_id=\"%s\" page_no=\"%d\" page_size=\"30\" />\r\n"
                        SOAP_ENV_END,
                        yms->id, 0);
    
    return yms_write(yms, "GET", yms->ocs->resources_uri, xml_buf, xml_len, yms_resource_list, yms);
}

static void yms_keepalive(void* data)
{
    yms_t* yms = (yms_t*)data;
    int32_t ret =yms_login(yms);
    if(ret < 0)
    {
        LOG_ERROR(lp, "login to ocs failed, yms id : %s, ret : %d", yms->id, ret);
        goto EXIT;
    }

    ret = yms_req_channels(yms);
    if(ret < 0)
    {
        LOG_ERROR(lp, "req channel list failed, yms id : %s, ret : %d", yms->id, ret);
        goto EXIT;
    }

    
    ret = yms_req_resources(yms);
    if(ret < 0)
    {
        LOG_ERROR(lp, "req resource list failed, yms id : %s, ret : %d", yms->id, ret);
        goto EXIT;
    }

EXIT:
    ret = rns_timer_setdelay(yms->timer, 10000);
    if(ret < 0)
    {
        LOG_ERROR(lp, "set keepalive timer failed, ret : %d", ret);
    }

    return;
    
}

int32_t channel_add(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }

    rns_xml_node_t* xml_node = rns_xml_node(xml, SOAP_ENV_NAME, SOAP_BODY_NAME, "channel", NULL);
    while(xml_node)
    {

        uchar_t* content_id = rns_xml_subnode_attr(xml_node, (uchar_t*)"content_id");
        if(content_id == NULL)
        {
            LOG_WARN(lp, "parse xml failed, no channel id found");
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }

        rbt_node_t* cnode = rbt_search_str(&yms->channels, content_id);
        if(cnode != NULL)
        {
            LOG_INFO(lp, "the channel %s is already in the list, channel id : %s", content_id);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        
        uchar_t* surl = rns_xml_subnode_attr(xml_node, (uchar_t*)"url");
        if(surl == NULL)
        {
            LOG_WARN(lp, "parse xml failed, no content url found, channel id : %s", content_id);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        
        channel_t* channel = yms_channel_create(yms->ep, yms->pipe, content_id, surl, yms->recddir, yms->recdsize, yms->gopsize, yms->tfragsize);
        if(channel == NULL)
        {
            LOG_ERROR(lp, "create channel failed, channel id : %s, surl : %s, recddir : %s, gop size : %d, tfragsize : %d\n", content_id, surl, yms->recddir, yms->gopsize, yms->tfragsize);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        int32_t ret = yms_channel_start(channel);
        if(ret < 0)
        {
            LOG_ERROR(lp, "start channel failed, channel id : %s, uri : %s, recddir : %s, ret : %d\n", content_id, surl, yms->recddir, ret);
            yms_channel_destroy(&channel);
            xml_node = rns_xml_node_next(xml_node);
            continue;
        }
        rbt_set_key_str(&channel->node, channel->content_id);
        rbt_insert(&yms->channels, &channel->node);
        
        xml_node = rns_xml_node_next(xml_node);
    }
    
    rns_xml_destroy(&xml);
    return 0;
}

int32_t channel_mdf(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }
    /* uchar_t content_id[128]; */
    
    /* rns_strncpy(content_id, rns_xml_node_value(xml, SOAP_ENV_NAME, SOAP_BODY_NAME, "channel", "content_id", NULL), sizeof(content_id)); */
    

    
    /* rbt_node_t* node = rbt_search_str(&yms->channels, content_id); */
    /* if(node == NULL) */
    /* { */
    /*     LOG_ERROR(lp, "search channel failed, contetn id : %s", content_id); */
    /*     rns_xml_destroy(&xml); */
    /*     return -2; */
    /* } */
    /* channel_t* channel = rbt_entry(node, channel_t, node); */
    /* if(channel == NULL) */
    /* { */
    /*     rns_xml_destroy(&xml); */
    /*     return -3; */
    /* } */
    
    /* rns_strncpy(channel->suri, rns_xml_node_value(xml, "channel", "url", NULL), sizeof(channel->suri)); */
    /* yms_channel_stop(channel); */
    
    /* int32_t ret = yms_channel_start(channel); */
    /* if(ret < 0) */
    /* { */
    /*     LOG_ERROR(lp, "start channel failed, channel id : %s, ret : %d", channel->content_id, ret); */
    /*     rns_xml_destroy(&xml); */
    /*     return -4; */
    /* } */

    rns_xml_destroy(&xml);
    return 0;
}

int32_t channel_del(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }

    /* uchar_t content_id[128]; */
    
    /* rns_strncpy(content_id, rns_xml_node_value(xml, SOAP_ENV_NAME, SOAP_BODY_NAME, "channel", "content_id", NULL), sizeof(content_id)); */
    
    /* rbt_node_t* node = rbt_search_str(&yms->channels, content_id); */
    /* if(node == NULL) */
    /* { */
    /*     LOG_ERROR(lp, "search channel failed, contetn id : %s", content_id); */
    /*     rns_xml_destroy(&xml); */
    /*     return -2; */
    /* } */
    
    /* channel_t* channel = rbt_entry(node, channel_t, node); */
    /* if(channel == NULL) */
    /* { */
    /*     rns_xml_destroy(&xml); */
    /*     return -3; */
    /* } */

    /* rns_http_cpool_destroy(&channel->cpool); */

    /* rbt_delete(&yms->channels, &channel->node); */
    /* channel->state = 0; */
    
    /* yms_channel_stop(channel); */

    rns_xml_destroy(&xml);
    /* rns_free(channel); */

    return 0;
}

int32_t resource_dld(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }
    
    resource_t* resource = (resource_t*)rns_malloc(sizeof(resource_t));
    if(resource == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", sizeof(resource_t));
        rns_xml_destroy(&xml);
        return -2;
    }
    
    /* rns_strncpy(resource->content_id, rns_xml_node_value(xml, "resource", "content_id", NULL), sizeof(channel->content_id)); */
    /* rns_strncpy(resource->url, rns_xml_node_value(xml, "resource", "url", NULL), sizeof(channel->url)); */
    /* rns_strncpy(resource->status, rns_xml_node_value(xml, "resource", "status", NULL), sizeof(channel->status)); */
    /* rns_strncpy(resource->path, rns_xml_node_value(xml, "resource", "path", NULL), sizeof(channel->path)); */

    
    rbt_insert(&yms->resources, &resource->node);

    rns_xml_destroy(&xml);
    return 0;
}

int32_t resource_add(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }
    
    resource_t* resource = (resource_t*)rns_malloc(sizeof(resource_t));
    if(resource == NULL)
    {
        rns_xml_destroy(&xml);
        /* LOG_ERROR() */
        return -2;
    }
    
    /* rns_strncpy(resource->content_id, rns_xml_node_value(xml, "resource", "content_id", NULL), sizeof(channel->content_id)); */
    /* rns_strncpy(resource->url, rns_xml_node_value(xml, "resource", "url", NULL), sizeof(channel->url)); */
    /* rns_strncpy(resource->status, rns_xml_node_value(xml, "resource", "status", NULL), sizeof(channel->status)); */
    /* rns_strncpy(resource->path, rns_xml_node_value(xml, "resource", "path", NULL), sizeof(channel->path)); */

    rbt_insert(&yms->resources, &resource->node);

    rns_xml_destroy(&xml);
    return 0;
}

int32_t resource_del(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }
    
    /* resource_t* resource = (resource_t*)rns_malloc(sizeof(resource_t)); */
    
    /* rns_strncpy(resource->content_id, rns_xml_node_value(xml, "resource", "content_id", NULL), sizeof(channel->content_id)); */
    /* rns_strncpy(resource->url, rns_xml_node_value(xml, "resource", "url", NULL), sizeof(channel->url)); */
    /* rns_strncpy(resource->status, rns_xml_node_value(xml, "resource", "status", NULL), sizeof(channel->status)); */
    /* rns_strncpy(resource->path, rns_xml_node_value(xml, "resource", "path", NULL), sizeof(channel->path)); */
    
    rns_xml_destroy(&xml);
    return 0;
}

int32_t resource_scan(rns_hconn_t* hconn, yms_t* yms)
{
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    rns_xml_t* xml = rns_xml_create(info->body, rns_strlen(info->body));
    if(xml == NULL)
    {
        LOG_ERROR(lp, "xml parse failed, xml buf : %s", info->body);
        return -1;
    }
    
    /* resource_t* resource = (resource_t*)rns_malloc(sizeof(resource_t)); */
    
    /* rns_strncpy(resource->content_id, rns_xml_node_value(xml, "resource", "content_id", NULL), sizeof(channel->content_id)); */
    /* rns_strncpy(resource->url, rns_xml_node_value(xml, "resource", "url", NULL), sizeof(channel->url)); */
    /* rns_strncpy(resource->status, rns_xml_node_value(xml, "resource", "status", NULL), sizeof(channel->status)); */
    /* rns_strncpy(resource->path, rns_xml_node_value(xml, "resource", "path", NULL), sizeof(channel->path)); */

    rns_xml_destroy(&xml);
    return 0;
}

int32_t yms_dispatch(rns_hconn_t *hconn, void* data)
{
    yms_t* yms = (yms_t*)data;
    int32_t ret = 0;
    uint32_t http_code = 200;
    
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    LOG_INFO(lp, "methond : %d, uri : %s", info->method, info->uri);

    ret = rns_hconn_resp_header_init(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "init resp header failed, uri : %s, ret : %d", info->uri, ret);
        http_code = 500;
        goto EXIT;
        
    }
    
    if(!rns_strncmp(info->uri, "/ois/channel/resource/add", rns_strlen("/ois/channel/resource/add")))
    {
        ret = channel_add(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }
    }
    else if(!rns_strncmp(info->uri, "/ois/channel/resource/modify", rns_strlen("/ois/channel/resource/modify")))
    {
        ret = channel_mdf(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }
    }
    else if(!rns_strncmp(info->uri, "/ois/channel/resource/delete", rns_strlen("/ois/channel/resource/delete")))
    {
        ret = channel_del(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }
    }
    
    else if(!rns_strncmp(info->uri, "/ois/file/resource/download", rns_strlen("/ois/file/resource/download")))
    {
        ret = resource_dld(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }
    }
    else if(!rns_strncmp(info->uri, "/ois/file/resource/add", rns_strlen("/ois/file/resource/add")))
    {
        ret = resource_add(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }
    }
    else if(!rns_strncmp(info->uri, "/ois/file/resource/delete", rns_strlen("/ois/file/resource/delete")))
    {
        ret = resource_del(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }

    }
    else if(!rns_strncmp(info->uri, "/ois/file/resource/scan", rns_strlen("/ois/file/resource/scan")))
    {
        ret = resource_scan(hconn, yms);
        if(ret < 0)
        {
            http_code = 500;
        }
    }

EXIT:
    
    ret = rns_hconn_resp_header(hconn, http_code);
    if(ret < 0)
    {
        LOG_ERROR(lp, "resp header failed, ret code : %d, size : %d", http_code, 0);
    }
    return info->body_size;
    
}

int32_t yms_req_dispatch(rns_hconn_t* hconn, void* data)
{
    yms_t* yms = (yms_t*)data;

    
    rns_http_info_t* info = rns_hconn_req_info(hconn);
    
    LOG_INFO(lp, "req info, uri : %s, client ip : %u, port : %u", info->uri, hconn->addr.ip, hconn->addr.port);

    int32_t ret = rns_hconn_resp_header_init(hconn);
    if(ret < 0)
    {
        LOG_ERROR(lp, "init resp header failed, uri : %s, ret : %d", info->uri, ret);
        goto ERR_EXIT;
    }
    
    uchar_t* p = rns_strrchr(info->uri, '/');
    uchar_t* q = rns_strrchr(info->uri, '.');
    if(p == NULL || q == NULL)
    {
        LOG_ERROR(lp, "req uri may be wrong, uri : %s", info->uri);
        goto ERR_EXIT;
    }
    
    uchar_t* content_id = NULL;
    uchar_t* o = rns_strstr(p, "-frag");
    if(o == NULL)
    {
        content_id = rns_malloc(q - p);
        if(content_id == NULL)
        {
            LOG_ERROR(lp, "malloc failed, size : %d", q - p);
            goto ERR_EXIT;   
        }
        rns_strncpy(content_id, p + 1, q - p - 1);
    }
    else
    {
        content_id = rns_malloc(o - p);
        if(content_id == NULL)
        {
            LOG_ERROR(lp, "malloc failed, size : %d", q - p);
            goto ERR_EXIT;   
        }
        rns_strncpy(content_id, p + 1, o - p - 1);
    }
    
   
    rbt_node_t* node = rbt_search_str(&yms->channels, content_id);
    if(node == NULL)
    {
        LOG_WARN(lp, "client req a wrong content, req content id : %s, uri : %s", content_id, info->uri);
        rns_free(content_id);
        goto ERR_EXIT;
    }
    
    channel_t* channel = rbt_entry(node, channel_t, node);
    ret = yms_live_req(hconn, channel);
    if(ret < 0)
    {
        LOG_ERROR(lp, "live req failed, channel id : %s, uri : %s", channel->content_id, info->uri);
    }

    rns_free(content_id);
    return info->body_size;
    
ERR_EXIT:
    ret = rns_hconn_resp_header(hconn, 501);
    if(ret < 0)
    {
        LOG_ERROR(lp, "response to client failed, ret : %d", ret);
    }
    return info->body_size;
}

void yms_report(void* data)
{
    yms_t* yms = (yms_t*)data;
    uint32_t len = 0;
    int32_t ret = rns_pipe_read(yms->pipe, (uchar_t*)&len, sizeof(uint32_t));
    if(ret < 0)
    {
        LOG_ERROR(lp, "read inform pipe failed, ret : %d", ret);
        return;
    }
    ret = rns_pipe_read(yms->pipe, yms->report_buf, len);
    if(ret < 0)
    {
        LOG_ERROR(lp, "read inform pipe failed, ret : %d", ret);
        return;
    }

    return;
}

int32_t main(int32_t argc, char_t** argv)
{
    int32_t ret_code = 0;
    uchar_t* buf = NULL;
    int32_t ret = 0;
    rns_xml_t* xml = NULL;
    uchar_t* ptr = NULL;
    rns_addr_t addr;
    uint32_t level = 2;
    rns_file_t* fp = NULL;
    

    yms_t* yms = (yms_t*)rns_malloc(sizeof(yms_t));
    if(yms == NULL)
    {
        ret_code = -1;
        goto ERR_EXIT;
    }

    yms->ep = rns_epoll_init();
    if(yms->ep == NULL)
    {
        LOG_ERROR(lp, "create http failed");
        ret_code = -11;
        goto ERR_EXIT;
        
    }
    
    fp = rns_file_open((uchar_t*)"./conf/config.xml", RNS_FILE_RW);
    if(fp == NULL)
    {
        ret_code = -2;
        goto ERR_EXIT;
    }
    int32_t size = rns_file_size(fp);
    if(size < 0)
    {
        ret_code = -3;
        goto ERR_EXIT;
    }
    buf = rns_malloc(size + 1);
    if(buf == NULL)
    {
        ret_code = -4;
        goto ERR_EXIT;
    }
    ret = rns_file_read(fp, buf, size);
    if(ret != size)
    {
        ret_code = -5;
        goto ERR_EXIT;
    }

    xml = rns_xml_create(buf, size);
    if(xml == NULL)
    {
        ret_code = -6;
        goto ERR_EXIT;
    }

    ptr = rns_xml_node_value(xml, "yms", "id", NULL);
    if(ptr == NULL)
    {
        ret_code = -7;
        goto ERR_EXIT;
    }
    rns_strncpy(yms->id, ptr, sizeof(yms->id) - 1);

    ptr = rns_xml_node_value(xml, "yms", "type", NULL);
    if(ptr == NULL)
    {
        ret_code = -8;
        goto ERR_EXIT;
    }
    rns_strncpy(yms->type, ptr, sizeof(yms->type) - 1);

    ptr = rns_xml_node_value(xml, "yms", "recddir", NULL);
    if(ptr == NULL)
    {
        rns_strncpy(yms->recddir, "/usr/local/yms/media/live/", sizeof(yms->recddir) - 1);
    }
    else
    {
        rns_strncpy(yms->recddir, ptr, sizeof(yms->recddir) - 1);
    }

    ptr = rns_xml_node_value(xml, "yms", "recdsize", NULL);
    if(ptr != NULL)
    {
        yms->recdsize = rns_atol(ptr);
    }
    else
    {
        yms->recdsize = 1000000000;
    }
    
    
    ptr = rns_xml_node_value(xml, "yms", "log", "level", NULL);
    if(ptr != NULL)
    {
        level = rns_atoi(ptr);
    }

    lp = rns_log_init((uchar_t*)"./log/log.txt", level, (uchar_t*)"yms");
    if(lp == NULL)
    {
        ret_code = -9;
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(yms->ep, lp->pipe->fd[0], lp, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add lp to ep failed, ret : %d", ret);
        ret_code = -10;
        goto ERR_EXIT;
    }
    
    LOG_INFO(lp, "xml config : %s", buf);

    yms->ocs = (ocs_t*)rns_malloc(sizeof(ocs_t));
    if(yms->ocs == NULL)
    {
        LOG_ERROR(lp, "malloc failed, size : %d", sizeof(ocs_t));
        ret_code = -10;
        goto ERR_EXIT;
    }
    ptr = rns_xml_node_value(xml, "yms", "ocs", "channels_uri", NULL);
    if(ptr != NULL)
    {
        rns_strncpy(yms->ocs->channels_uri, ptr, sizeof(yms->ocs->channels_uri) - 1);
    }
    else
    {
        rns_strncpy(yms->ocs->channels_uri, "/ois/channel/resource/list", sizeof(yms->ocs->channels_uri));
    }

    ptr = rns_xml_node_value(xml, "yms", "ocs", "resources_uri", NULL);
    if(ptr != NULL)
    {
        rns_strncpy(yms->ocs->resources_uri, ptr, sizeof(yms->ocs->resources_uri) - 1);
    }
    else
    {
        rns_strncpy(yms->ocs->resources_uri, "/ois/file/resource/list", sizeof(yms->ocs->resources_uri));
    }

    ptr = rns_xml_node_value(xml, "yms", "ocs", "login_uri", NULL);
    if(ptr != NULL)
    {
        rns_strncpy(yms->ocs->login_uri, ptr, sizeof(yms->ocs->login_uri) - 1);
    }
    else
    {
        rns_strncpy(yms->ocs->login_uri, "/ois/user/login", sizeof(yms->ocs->login_uri) - 1);
    }
    
    ptr = rns_xml_node_value(xml, "yms", "ocs", "report_channel_uri", NULL);
    if(ptr != NULL)
    {
        rns_strncpy(yms->ocs->report_channel_uri, ptr, sizeof(yms->ocs->report_channel_uri) - 1);
    }
    else
    {
        rns_strncpy(yms->ocs->report_channel_uri, "/ois/post/channel/status", sizeof(yms->ocs->report_channel_uri) - 1);
    }

    uint32_t num =  5;
    ptr = rns_xml_node_value(xml, "yms", "ocs", "max_conns", NULL);
    if(ptr != NULL)
    {
        num = rns_ip_str2int(ptr);
    }

    ptr = rns_xml_node_value(xml, "yms", "ocs", "server", "ip", NULL);
    if(ptr == NULL)
    {
        yms->ocs->addr.ip = 0;
    }
    else
    {
        yms->ocs->addr.ip = rns_ip_str2int(ptr);
    }
    ptr = rns_xml_node_value(xml, "yms", "ocs", "server", "port", NULL);
    if(ptr == NULL)
    {
        yms->ocs->addr.port = 5000;
    }
    else
    {
        yms->ocs->addr.port = rns_atoi(ptr);
    }
        
    yms->ocs->ohttp = rns_httpclient_create(yms->ep, &yms->ocs->addr, num, NULL);
    if(yms->ocs->ohttp == NULL)
    {
        uchar_t b[32];
        LOG_ERROR(lp, "create ocs cpool failed, ocs ip : %s, port : %u\n", rns_ip_int2str(yms->ocs->addr.ip, b, 32), yms->ocs->addr.port);
    }
    LOG_INFO(lp, "ocs info, ip : %u, port : %d", yms->ocs->addr.ip, yms->ocs->addr.port);
    
    yms->timer = rns_timer_create();
    if(yms->timer == NULL)
    {
        LOG_ERROR(lp, "create timer failed");
        ret_code = -11;
        goto ERR_EXIT;
    }

    rns_cb_t cb;
    cb.func = yms_keepalive;
    cb.data = yms;
    ret = rns_timer_set(yms->timer, 1000, &cb);
    if(ret < 0)
    {
        LOG_ERROR(lp, "set timer to http failed");
        ret_code = -15;
        goto ERR_EXIT;
    }
    
    ret = rns_epoll_add(yms->ep, yms->timer->timerfd, yms->timer, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add timer to http failed");
        ret_code = -15;
        goto ERR_EXIT;
    }
    
    yms->pipe = rns_pipe_open();
    if(yms->pipe == NULL)
    {
        LOG_ERROR(lp, "open pipe failed");
        ret_code = -13;
        goto ERR_EXIT;
    }

    
    yms->pipe->cb.func = yms_report;
    yms->pipe->cb.data = yms;
    
    ret = rns_epoll_add(yms->ep, yms->pipe->fd[0], yms->pipe, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add pipe to http epoll failed");
        ret_code = -14;
        goto ERR_EXIT;
    }


    rbt_init(&yms->channels, RBT_TYPE_STR);
    rbt_init(&yms->resources, RBT_TYPE_STR);

    ptr = rns_xml_node_value(xml, "yms", "hls", "gopsize", NULL);
    if(ptr == NULL)
    {
        yms->gopsize = 4;
    }
    else
    {
        yms->gopsize = rns_atoi(ptr);
    }

    ptr = rns_xml_node_value(xml, "yms", "hls", "tfragsize", NULL);
    if(ptr == NULL)
    {
        yms->tfragsize = 10;
    }
    else
    {
        yms->tfragsize = rns_atoi(ptr);
    }
    
    ptr = rns_xml_node_value(xml, "yms", "hls", "ip", NULL);
    if(ptr == NULL)
    {
        addr.ip = rns_ip_str2int((uchar_t*)"127.0.0.1");
    }
    else
    {
        addr.ip = rns_ip_str2int(ptr);
    }

    ptr = rns_xml_node_value(xml, "yms", "hls", "port", NULL);
    if(ptr == NULL)
    {
        addr.port = 5002;
    }
    else
    {
        addr.port = rns_atoi(ptr);
    }

    rns_http_cb_t hlscb;
    rns_memset(&hlscb, sizeof(rns_http_cb_t));
    hlscb.work = yms_req_dispatch;
    hlscb.close = NULL;
    hlscb.error = NULL;
    hlscb.data = yms;
    
    yms->hls = rns_httpserver_create(&addr, &hlscb);
    if(yms->hls == NULL)
    {
        LOG_ERROR(lp, "add listen port to http failed, ip : %d, port : %d", addr.ip, addr.port);
        ret_code = -16;
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(yms->ep, yms->hls->listenfd, yms->hls, RNS_EPOLL_IN);
    if(ret < 0)
    {
        LOG_ERROR(lp, "add listen port to epoll failed, ip : %d, port : %d", addr.ip, addr.port);
        ret_code = -17;
        goto ERR_EXIT;
    }
    
    rns_epoll_wait(yms->ep);
    return 0;

ERR_EXIT:
    printf("ret : %d\n", ret_code);
    if(yms != NULL)
    {
        rns_free(yms->ocs);
        rns_pipe_close(&yms->pipe);
        rns_timer_destroy(&yms->timer);
        rns_http_destroy(&yms->hls);
        rns_log_destroy(&lp);

        rns_xml_destroy(&xml);
        rns_free(buf);
        rns_file_close(&fp);
        rns_free(yms);
    }
    return ret_code;

}
