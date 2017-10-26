#include "rns_http.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
/* #include <sys/timerfd.h> */
#include <sys/sendfile.h>
#include <netdb.h>

#define RNS_HTTP_BUF_SIZE 8192

void rns_hconn_resp_done(rns_hconn_t* hconn, void* data);
void rns_htune_init(rns_htune_t* tune, rns_http_cb_t* cb);
void rns_http_info_init(rns_http_info_t* info);
int32_t rns_http_info_read(rns_hconn_t* hconn, uchar_t* buf, uint32_t size);
static void read_func(rns_epoll_t* ep, void* data);
static void write_func(rns_epoll_t* ep, void* data);

int32_t rns_hconn_read(rns_hconn_t* hconn, uchar_t* buf, int32_t size);
int32_t rns_hconn_write(rns_hconn_t* hconn, uchar_t* buf, int32_t size);
int32_t rns_hconn_connect(rns_hconn_t* hconn);
int32_t rns_hconn_sock(rns_hconn_t* hconn);
void rns_hconn_req_done(rns_hconn_t* hconn);


static void debug(char_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    /* vprintf(fmt, list); */
    va_end(list);

    return;
}

void rns_hconn_secure(void* data)
{
    rns_hconn_t* hconn = (rns_hconn_t*)data;
    hconn->phase = 0;
    rns_hconn_destroy(&hconn);
    return;
}

static void accept_func(rns_epoll_t* ep, void* data)
{
    if(ep == NULL || data == NULL)
    {
        return;
    }
    
    rns_http_t* http = (rns_http_t*)data;
    
    while(true)
    {
        struct sockaddr_in cli_addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        int32_t fd = accept4(http->listenfd, (struct sockaddr*)&cli_addr, &addr_len, SOCK_NONBLOCK);
        if(fd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            break;
        }
        else if(fd < 0)
        {
            debug("fd : %d, errno : %d\n", fd, errno);
            return;
        }
        
        rns_hconn_t* hconn = (rns_hconn_t*)rns_malloc(sizeof(rns_hconn_t));
        if(hconn == NULL)
        {
            continue;
        }
        rns_memcpy(&hconn->addr, &cli_addr, sizeof(rns_addr_t));
        
        hconn->fd = fd;
        hconn->type = http->type;
        uint32_t send_buf_size = 1 * 1024 * 1024;
        
        setsockopt(hconn->fd, SOL_SOCKET, SO_SNDBUF, ( const char* )&send_buf_size, sizeof(uint32_t));

        rns_htune_init(&hconn->tune[0], &http->cb);
        
        hconn->ecb.read = read_func;
        hconn->ecb.write = write_func;
        hconn->http = http;
        hconn->ep = ep;
        hconn->expire = http->expire;


        rns_cb_t cb;
        cb.func = rns_hconn_secure;
        cb.data = hconn;
        hconn->timer = rns_timer_add(ep, hconn->expire, &cb);
        if(hconn->timer == NULL)
        {
            rns_hconn_close(hconn);
            rns_free(hconn);
            continue;
        }
        
        INIT_LIST_HEAD(&hconn->tasks);
        hconn->buf = rns_buf_create(RNS_HTTP_BUF_SIZE);
        if(hconn->buf == NULL)
        {
            rns_hconn_close(hconn);
            rns_free(hconn);
            continue;
        }
        
        int32_t ret = rns_epoll_add(ep, hconn->fd, hconn, RNS_EPOLL_IN | RNS_EPOLL_OUT);
        if(ret < 0)
        {
            rns_hconn_close(hconn);
            rns_free(hconn);
            continue;
        }
    
        debug("accept : fd : %d, remote ip : %x, remote port : %d\n", fd, hconn->addr.ip, hconn->addr.port);
    }
    return;
}    
void rns_htune_init(rns_htune_t* tune, rns_http_cb_t* cb)
{
    rns_http_info_init(&tune->read_info);
    rns_http_info_init(&tune->write_info);
    
    
    if(cb != NULL)
    {
        rns_memcpy(&tune->cb, cb, sizeof(rns_http_cb_t));
    }
    else
    {
        rns_memset(&tune->cb, sizeof(rns_http_cb_t));
    }

    return;
}
void rns_htune_destroy(rns_htune_t* tune)
{
    rns_http_info_init(&tune->read_info);
    rns_http_info_init(&tune->write_info);

    return;
}
void rns_http_info_init(rns_http_info_t* info)
{
    info->code = 0;
    info->body = NULL;
    info->body_size = 0;
    info->total_size = 0;
    info->curr_size = 0;
    info->state = RNS_HTTP_INFO_STATE_INIT;
    info->chunk = 0;
    info->chunksize = 0;
    
    rbt_node_t* node = NULL;
    while((node = rbt_first(&info->headers)) != NULL)
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        rbt_delete(&info->headers, node);
        if(item->data != NULL)
        {
            rns_free(item->data);
            item->data = NULL;
        }
        rns_free(item);
    }
    
    rbt_init(&info->headers, RBT_TYPE_STR);

    while((node = rbt_first(&info->args)) != NULL)
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        rbt_delete(&info->args, node);
        if(item->data != NULL)
        {
            rns_free(item->data);
            item->data = NULL;
        }
        rns_free(item);
    }
    rbt_init(&info->args, RBT_TYPE_STR);
    
    return;
}
int32_t rns_http_info_read(rns_hconn_t* hconn, uchar_t* buf, uint32_t size)
{
    uchar_t* walker = buf;
    uchar_t* end = walker + size;
    rns_http_info_t* info = &hconn->tune[hconn->read_type].read_info;
    uchar_t* line = (void*)1;

    
    while(walker < end)
    {
        switch(info->state)
        {
            case RNS_HTTP_INFO_STATE_INIT :
            {
                
                line = rns_strstr(walker, "\r\n");
                if(line == NULL || line > end)
                {
                    return walker - buf;
                }
                
                while(*walker == ' ')++walker;
                uchar_t* ptr = rns_strstr(walker, " ");
                if(ptr == NULL || ptr > line)
                {
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }

                
                if(!rns_strncmp(walker, "HTTP", 4))
                {
                    info = &hconn->tune[1].read_info;
                    hconn->read_type = 1;
                    info->state = RNS_HTTP_INFO_STATE_CODE;
                    walker = ptr + 1;
                }
                else
                {
                    info = &hconn->tune[0].read_info;
                    hconn->read_type = 0;
                    info->state = RNS_HTTP_INFO_STATE_METHOD;
                }

                break;
            }
            case RNS_HTTP_INFO_STATE_CODE :
            {
                while(*walker == ' ')++walker;
                uchar_t* ptr = rns_strstr(walker, " ");
                if(ptr == NULL || ptr > line)
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                info->code = rns_atoi(walker);
                walker = line + 2;
                
                info->state = RNS_HTTP_INFO_STATE_HEADER;
                break;
            }
            case RNS_HTTP_INFO_STATE_METHOD :
            {
                while(*walker == ' ')++walker;
                uchar_t* ptr = rns_strstr(walker, " ");
                if(ptr == NULL || ptr > line)
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                if(!rns_strncmp(walker, "POST", ptr - walker))
                {
                    info->method = RNS_HTTP_METHOD_POST;
                    info->state = RNS_HTTP_INFO_STATE_URI;
                }
                else if(!rns_strncmp(walker, "GET", ptr - walker))
                {
                    info->method = RNS_HTTP_METHOD_POST;
                    info->state = RNS_HTTP_INFO_STATE_URI;
                }
                else
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                
                
                
                walker = ptr + 1;
                break;
            }
            case RNS_HTTP_INFO_STATE_URI :
            {
                while(*walker == ' ')++walker;
                if(walker >= line)
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                
                uchar_t* ptr = rns_strstr(walker, "?");
                if(ptr == NULL || ptr > line)
                {
                    ptr = rns_strstr(walker, "HTTP");
                    if(ptr == NULL || ptr > line)
                    {
                        ptr = line;
                    }
                    while(*(ptr - 1) == ' ')--ptr;
                }

                if(info->uri != NULL)
                {
                    rns_free(info->uri);
                    info->uri = NULL;
                }
                info->uri = rns_malloc(ptr - walker + 1);
                if(info->uri == NULL)
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                rns_strncpy(info->uri, walker, ptr - walker);
                
                walker = ptr;
                
                if(*walker == '?')
                {
                    info->state = RNS_HTTP_INFO_STATE_ARGS;
                    ++walker;
                }
                else
                {
                    info->state = RNS_HTTP_INFO_STATE_HEADER;
                    walker = line + 2;
                }
                
                break;
            }
            case RNS_HTTP_INFO_STATE_ARGS :
            {
                uchar_t* ptr = rns_strstr(walker, "&");
                if(ptr == NULL || ptr > line)
                {
                    ptr = rns_strstr(walker, "HTTP");
                    if(ptr == NULL || ptr > line)
                    {
                        ptr = line;
                    }
                    while(*(ptr - 1) == ' ')--ptr;
                    
                }
                else
                {
                    info->state = RNS_HTTP_INFO_STATE_ARGS;
                }
                
                uchar_t* buff = rns_malloc(ptr - walker + 1);
                if(buff == NULL)
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                rns_strncpy(buff, walker, ptr - walker);
                rns_node_t* item = rns_split_key_value(buff, '=');
                if(item != NULL)
                {
                    rbt_insert(&info->args, &item->node);
                }
                rns_free(buff);
                walker = ptr;
                if(*walker == '&')
                {
                    info->state = RNS_HTTP_INFO_STATE_ARGS;
                    ++walker;
                }
                else
                {
                    info->state = RNS_HTTP_INFO_STATE_HEADER;
                    walker = line + 2;
                }
                
                break;
            }
            case RNS_HTTP_INFO_STATE_HEADER :
            {
                line = rns_strstr(walker, "\r\n");
                if(line == NULL)
                {
                    return walker - buf;
                }
                
                uchar_t* buff = rns_malloc(line - walker + 1);
                if(buff == NULL)
                {
                    printf("%d - %s\n", info->state, walker);
                    info->state = RNS_HTTP_INFO_STATE_ERROR;
                    break;
                }
                rns_strncpy(buff, walker, line - walker);
                rns_node_t* item = rns_split_key_value(buff, ':');
                if(item != NULL)
                {
                    if(!rns_strncmp(item->node.key.str, "Content-Length", 14))
                    {
                        info->total_size = rns_atoi(item->data);
                    }
                    if(!rns_strncmp(item->node.key.str, "Transfer-Encoding", 14))
                    {
                        info->chunk = 1;
                    }
                    rbt_insert(&info->headers, &item->node);
                }
                rns_free(buff);
                walker = line + 2;

                while(*walker == ' ')++walker;
                if(*(walker) == '\r' && *(walker + 1) == '\n')
                {
                    walker += 2;

                    if(info->chunk)
                    {
                        info->state = RNS_HTTP_INFO_STATE_CHUNK;
                    }
                    else
                    {
                        info->state = RNS_HTTP_INFO_STATE_BODY;
                    }
                }
                else
                {
                    info->state = RNS_HTTP_INFO_STATE_HEADER;
                }
            
                break;
            }
            case RNS_HTTP_INFO_STATE_CHUNK :
            {
                while(*walker == ' ')++walker;
                line = rns_strstr(walker, "\r\n");
                if(line == NULL || line > end)
                {
                    return walker - buf;
                }
                //for the second .. chunk
                if(line == walker)
                {
                    walker += 2;
                    line = rns_strstr(walker, "\r\n");
                    if(line == NULL || line > end)
                    {
                        return walker - buf;
                    }
                }
                
                
                info->chunksize = rns_hex2dec(walker, line - walker);
                walker = line + 2;
                if(info->chunksize == 0)
                {
                    info->state = RNS_HTTP_INFO_STATE_DONE;
                    break;
                }
                info->total_size += info->chunksize;
                info->state = RNS_HTTP_INFO_STATE_BODY;
                break;
            }
            case RNS_HTTP_INFO_STATE_BODY :
            {
                if(info->body_size == 0)
                {
                    info->body = walker;
                }
                
                if(!info->chunk && (uint32_t)(end - walker) >= info->total_size - info->curr_size)
                {
                    info->state = RNS_HTTP_INFO_STATE_DONE;
                }
                
                uint32_t len = (uint32_t)(end - walker) < info->total_size - info->curr_size ? end - walker : info->total_size - info->curr_size;
               
                info->curr_size += len;
                info->body_size += len;
                walker += len;
                
                return walker - buf;
            }
            case RNS_HTTP_INFO_STATE_DONE :
            {
                return walker - buf;
            }
            case RNS_HTTP_INFO_STATE_ERROR :
            {
                return end - buf;
            }
            default :
            {
                return end - buf;
            }
        }
    }

    if(walker == end && info->state == RNS_HTTP_INFO_STATE_BODY && info->total_size == 0)
    {
        info->state = RNS_HTTP_INFO_STATE_DONE;
    }
    
    return walker - buf;
}
#define RNS_HCONN_STATE_READ 0
#define RNS_HCONN_STATE_PARSE 1
#define RNS_HCONN_STATE_EXEC 2
#define RNS_HCONN_STATE_DONE 3
static void read_func(rns_epoll_t* ep, void* data)
{
    rns_hconn_t* hconn = (rns_hconn_t*)data;
    uint32_t state = 0;
    uint32_t e = 0;
    int32_t ret = 0;
    while(!e)
    {
        switch (state)
        {
            case RNS_HCONN_STATE_READ :
            {
                int32_t size = rns_hconn_read(hconn, hconn->buf->buf + hconn->buf->write_pos, hconn->buf->size - hconn->buf->write_pos);
                debug("size : %d\n", size);
                if(size < 0 && errno == EAGAIN)
                {
                    if(hconn->tune[hconn->read_type].read_info.state == RNS_HTTP_INFO_STATE_DONE)
                    {
                        hconn->buf->write_pos = 0;
                        hconn->buf->read_pos = 0;

                        if(hconn->read_type == 1)
                        {
                            rns_hconn_req_done(hconn);
                        }
                        hconn->write = 1;
                        write_func(ep, hconn);
                    }

                    e = 1;
                    break;
                }
                else if(size == 0)
                {
                    if(hconn->tune[0].cb.close != NULL)
                    {
                        hconn->tune[0].cb.close(hconn, hconn->tune[0].cb.data);
                    }
                    if(hconn->tune[1].cb.close != NULL)
                    {
                        hconn->tune[1].cb.close(hconn, hconn->tune[1].cb.data);
                    }
                    rns_hconn_destroy(&hconn);
                    e = 1;
                    break;
                }
                else if(size < 0)
                {
                    if(hconn->tune[0].cb.error != NULL)
                    {
                        hconn->tune[0].cb.error(hconn, hconn->tune[0].cb.data);
                    }
                    if(hconn->tune[1].cb.error != NULL)
                    {
                        hconn->tune[1].cb.error(hconn, hconn->tune[1].cb.data);
                    }
                    rns_hconn_destroy(&hconn);
                    e = 1;
                    break;
                }
                hconn->buf->write_pos += size;
                if(hconn->buf->write_pos < hconn->buf->size)
                {
                    hconn->buf->buf[hconn->buf->write_pos] = 0;
                }

                state = RNS_HCONN_STATE_PARSE;
                break;
            }
            case RNS_HCONN_STATE_PARSE :
            {

                uint32_t psize = rns_http_info_read(hconn, hconn->buf->buf + hconn->buf->read_pos, hconn->buf->write_pos - hconn->buf->read_pos);
                hconn->buf->read_pos += psize;

                rns_http_info_t* info = &hconn->tune[hconn->read_type].read_info;
                if(info->state == RNS_HTTP_INFO_STATE_BODY && hconn->buf->size == hconn->buf->write_pos)
                {
                    rns_timer_delay(ep, hconn->timer, hconn->expire);
                    state = RNS_HCONN_STATE_EXEC;
                    hconn->buf->write_pos = 0;
                    hconn->buf->read_pos = 0;
                }
                else if(info->chunk && info->state == RNS_HTTP_INFO_STATE_BODY && info->total_size <= info->curr_size)
                {
                    rns_timer_delay(ep, hconn->timer, hconn->expire);
                    state = RNS_HCONN_STATE_EXEC;
                    info->state = RNS_HTTP_INFO_STATE_CHUNK;
                }
                else if(hconn->tune[hconn->read_type].read_info.state == RNS_HTTP_INFO_STATE_DONE)
                {
                    rns_timer_delay(ep, hconn->timer, hconn->expire);
                    state = RNS_HCONN_STATE_EXEC;
                }
                else if(hconn->tune[hconn->read_type].read_info.state == RNS_HTTP_INFO_STATE_ERROR)
                {
                    rns_http_info_init(&hconn->tune[hconn->read_type].read_info);
                    state = RNS_HCONN_STATE_READ;
                }
                else
                {
                    state = RNS_HCONN_STATE_READ;
                }
            
                break;
            }
            case RNS_HCONN_STATE_EXEC : // 有两种形式的执行, 一种是缓冲满了, 另外一种是请求完成了
            {
                if((hconn->tune[hconn->read_type].read_info.chunk == 0 || hconn->tune[hconn->read_type].read_info.state == RNS_HTTP_INFO_STATE_CHUNK) && hconn->tune[hconn->read_type].cb.work != NULL)
                {
                    if(!hconn->read_type)
                    {
                        hconn->phase = 1;
                    }
                    ret = hconn->tune[hconn->read_type].cb.work(hconn, hconn->tune[hconn->read_type].cb.data);
                    if(ret < 0)
                    {
                        
                    }
                }
                
                hconn->tune[hconn->read_type].read_info.body_size = 0;
                hconn->tune[hconn->read_type].read_info.body = NULL;
                
                if(hconn->tune[hconn->read_type].read_info.state == RNS_HTTP_INFO_STATE_DONE)
                {
                    if(hconn->tune[hconn->read_type].cb.done != NULL)
                    {
                        if(!hconn->read_type)
                        {
                            hconn->phase = 1;
                        }
                        hconn->tune[hconn->read_type].cb.done(hconn, NULL);
                    }
                }

                if(hconn->buf->read_pos >= hconn->buf->write_pos)
                {
                    state = RNS_HCONN_STATE_READ;
                }
                else 
                {
                    if(hconn->tune[hconn->read_type].read_info.state == RNS_HTTP_INFO_STATE_DONE)
                    {
                        // for http chunk model
                        rns_http_info_init(&hconn->tune[hconn->read_type].read_info);
                    }
                    state = RNS_HCONN_STATE_PARSE;
                }
                
                break;
            }
        }
    }

    return;
}


static void write_func(rns_epoll_t* ep, void* data)
{
    rns_hconn_t* hconn = (rns_hconn_t*)data;
    rns_http_task_t* task = NULL;
    int32_t size = 0;
    
    while((task = rns_list_first(&hconn->tasks, rns_http_task_t, list)) != NULL)
    {
        rns_list_del(&task->list);
        if(task->cb.work)
        {
            size = task->cb.work(hconn, task->cb.data);
        }
        if(size < 0 &&  (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            rns_list_add_first(&task->list, &hconn->tasks);
            break;
        }
        else if(size < 0)
        {
            if(task->cb.error)
            {
                task->cb.error(hconn, task->cb.data);
            }
            rns_free(task);
        }
        else if(size == 0)
        {
            if(task->cb.done)
            {
                task->cb.done(hconn, task->cb.data);
            }
            if(task->cb.close)
            {
                task->cb.close(hconn, task->cb.data);
            }
            if(task->cb.clean)
            {
                task->cb.clean(task->cb.data);
            }
            if(task->cb.close || task->cb.clean)
            {
                rns_free(task);
                break;
            }
            rns_free(task);
        }
        else
        {
            rns_list_add_first(&task->list, &hconn->tasks);
        }
    }
    
    return;
}

//---------------------------------------------------------------------------------

void rns_http_destroy(rns_http_t** http)
{
    if(*http == NULL)
    {
        return;
    }
    
    rns_hconn_t* hconn = NULL;
    while((hconn = rns_list_first(&(*http)->hconns, rns_hconn_t, list)) != NULL)
    {
        rns_list_del(&hconn->list);
        rns_hconn_destroy(&hconn);
    }

    close((*http)->listenfd);
    
    rns_free(*http);
    *http = NULL;
    return;
}

rns_http_t* rns_httpserver_create(rns_addr_t* addr, rns_http_cb_t* cb, uint32_t expire)
{
    rns_http_t* http = (rns_http_t*)rns_malloc(sizeof(rns_http_t));
    if(http == NULL)
    {
        return NULL;
    }
    
    INIT_LIST_HEAD(&http->hconns);
    
    rns_memcpy(&http->addr, addr, sizeof(rns_addr_t));

    rns_memcpy(&http->cb, cb, sizeof(rns_http_cb_t));
    
    http->listenfd = rns_server_init(addr);
    if(http->listenfd < 0)
    {
        rns_free(http);
        return NULL;
    }

    http->ecb.read = accept_func;
    http->ecb.write = NULL;
    http->type = RNS_HTTP_TYPE_SERVER;
    http->expire = expire;
    
    return http;
}

rns_hconn_t* rns_hconn_create(rns_http_t* http)
{
    rns_hconn_t* hconn = (rns_hconn_t*)rns_malloc(sizeof(rns_hconn_t));
    if(hconn == NULL)
    {
        return NULL;
    }

    rns_memcpy(&hconn->addr, &http->addr, sizeof(rns_addr_t));

    hconn->type = http->type;
    hconn->http = http;
    hconn->ep = http->ep;
    rns_htune_init(&hconn->tune[0], &http->cb);
    rns_htune_init(&hconn->tune[1], NULL);

    hconn->buf = rns_buf_create(RNS_HTTP_BUF_SIZE);
    if(hconn->buf == NULL)
    {
        rns_hconn_close(hconn);
        rns_free(hconn);
        return NULL;
    }
    
    INIT_LIST_HEAD(&hconn->tasks);
    
    hconn->ecb.read = read_func;
    hconn->ecb.write = write_func;
    
    int32_t ret = rns_hconn_sock(hconn);
    if(ret < 0)
    {
        rns_free(hconn);
        return NULL;
    }

    ret = rns_epoll_add(http->ep, hconn->fd, hconn, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        rns_buf_destroy(&hconn->buf);
        rns_hconn_close(hconn);
        rns_free(hconn);
        return NULL;
    }
    ret = rns_hconn_connect(hconn);
    if(ret < 0 && errno != EINPROGRESS)
    {
        rns_hconn_close(hconn);
        rns_free(hconn);
        return NULL;
    }
    
    return hconn;
}

rns_http_t* rns_httpclient_create(rns_epoll_t* ep, rns_addr_t* addr, uint32_t num, rns_http_cb_t* cbpush)
{
    if(ep == NULL || addr == NULL)
    {
        return NULL;
    }
    rns_http_t* http = (rns_http_t*)rns_malloc(sizeof(rns_http_t));
    if(http == NULL)
    {
        return NULL;
    }
    
    INIT_LIST_HEAD(&http->hconns);
    
    rns_memcpy(&http->addr, addr, sizeof(rns_addr_t));
    if(cbpush != NULL)
    {
        rns_memcpy(&http->cb, cbpush, sizeof(rns_http_cb_t));
    }

    http->type = RNS_HTTP_TYPE_CLIENT;
    http->listenfd = -1;
    http->ep = ep;
    for(uint32_t i = 0; i < num; ++i)
    {
        rns_hconn_t* hconn = rns_hconn_create(http);
        if(hconn != NULL)
        {
            rns_list_add(&hconn->list, &http->hconns);
        }
    }
    
    return http;
}

//-------------------------------------------------------------

int32_t rns_hconn_read(rns_hconn_t* hconn, uchar_t* buf, int32_t size)
{
    return recv(hconn->fd, buf, size, 0);
}

int32_t rns_hconn_write(rns_hconn_t* hconn, uchar_t* buf, int32_t size)
{
    return send(hconn->fd, buf, size, 0);
}

int32_t rns_hconn_sendfile(rns_hconn_t* hconn, rns_file_t* fp, uint64_t offset, uint32_t size)
{
    off_t off = offset;
    return sendfile(hconn->fd, fp->sfd, &off, size);
}

rns_http_info_t* rns_hconn_resp_info(rns_hconn_t* hconn)
{
    return &hconn->tune[1].read_info;
}

rns_http_info_t* rns_hconn_req_info(rns_hconn_t* hconn)
{
    return &hconn->tune[0].read_info;
}

int32_t rns_hconn_header_range(rns_hconn_t* hconn, uint32_t* start, uint32_t* end)
{
    rns_node_t* item = NULL;
    rbt_node_t* node = rbt_search_str(&hconn->tune[0].read_info.headers, (uchar_t*)"Range");
    if(node == NULL)
    {
        return -1;
    }
    
    item = rbt_entry(node, rns_node_t, node);

    uchar_t* endptr = item->data + rns_strlen(item->data);
    
    uchar_t* walker = rns_strchr(item->data, '=');
    if(walker == NULL || walker + 1 >= endptr)
    {
        return -2;
    }

    uchar_t* ptr = walker + 1;
    walker = rns_strchr(walker, '-');
    if(walker == NULL)
    {
        return -3;
    }

    uchar_t* buf = rns_malloc(walker - ptr + 1);
    if(buf == NULL)
    {
        return -4;
    }
    rns_strncpy(buf, ptr, walker - ptr);
    
    *start = rns_atoi(buf);
    rns_free(buf);
    
    ptr = walker + 1;
    
    if(ptr < endptr)
    {
        *end = rns_atoi(ptr);
    }

    return 0;
}

int32_t rns_hconn_sock(rns_hconn_t* hconn)
{
    int32_t ret = 0;
    if(hconn->fd != -1 )
    {
        hconn->fd = -1;
    }
    
    hconn->fd = socket(AF_INET, SOCK_STREAM, 0);
    if( hconn->fd == -1 )
    {
        return -1;
    }
    
    struct linger val;
    val.l_onoff = 0;
    val.l_linger = 0;
    
    int32_t flag = 1;
    ret = setsockopt(hconn->fd, SOL_SOCKET, SO_LINGER, (char*)&val, sizeof(struct linger) );
    if(ret < 0)
    {
        return -1;
    }
    ret = setsockopt(hconn->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag) );
    if(ret < 0)
    {
        return -2;
    }
    
    int32_t sflags = fcntl(hconn->fd, F_GETFL, 0);
    ret = fcntl(hconn->fd, F_SETFL, sflags | O_NONBLOCK);
    if(ret < 0)
    {
        return -4;
    }
    
    return 0;
}

int32_t rns_hconn_connect(rns_hconn_t* hconn)
{
    struct sockaddr_in addr;
    rns_memset(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = rns_htons(hconn->addr.port);
    addr.sin_addr.s_addr = rns_htonl(hconn->addr.ip);
    return connect(hconn->fd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
}

int32_t rns_hconn_open(rns_hconn_t* hconn)
{
    if(hconn == NULL)
    {
        return -1;
    }
    if(hconn->fd >= 0)
    {
        rns_hconn_close(hconn);
    }

    rns_memcpy(&hconn->addr, &hconn->http->addr, sizeof(rns_addr_t));

    int32_t ret = rns_hconn_sock(hconn);
    if(ret < 0)
    {
        rns_hconn_close(hconn);
        return -2;
    }

    ret = rns_epoll_add(hconn->ep, hconn->fd, hconn, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        rns_hconn_close(hconn);
        return -3;
    }
    
    ret = rns_hconn_connect(hconn);
    if(ret < 0 && errno != EINPROGRESS)
    {
        rns_hconn_close(hconn);
        return -4;
    }
    
    return 0;
}

void rns_hconn_close(rns_hconn_t* hconn)
{
    if(hconn == NULL)
    {
        return;
    }
    debug("close hconn\n");
    rns_epoll_delete(hconn->ep, hconn->fd);
    rns_timer_delete(hconn->ep, hconn->timer);
    hconn->timer = NULL;
    
    close(hconn->fd);
    hconn->fd = -1;
    rns_htune_destroy(&hconn->tune[0]);
    rns_htune_destroy(&hconn->tune[1]);
    
    return;
}

void rns_hconn_destroy(rns_hconn_t** hconn)
{
    if(*hconn == NULL)
    {
        return;
    }

    if((*hconn)->phase)
    {
        (*hconn)->destroy = 1;
        return;
    }
    
    rns_http_task_t* task = NULL;
    while((task = rns_list_first(&(*hconn)->tasks, rns_http_task_t, list)) != NULL)
    {
        if(task->cb.clean != NULL)
        {
            task->cb.clean(task->cb.data);
        }
        if(!rns_list_free(&task->list))
        {
            rns_list_del(&task->list);
        }
        rns_free(task);
    }
    
    rns_hconn_close(*hconn);
    if(!rns_list_free(&(*hconn)->list))
    {
        rns_list_del(&(*hconn)->list);
    }
    rns_buf_destroy(&(*hconn)->buf);
    
    rns_free(*hconn);
    *hconn = NULL;
    return;
}

rns_hconn_t* rns_hconn_get(rns_http_t* http, rns_http_cb_t* cb)
{
    rns_hconn_t* hconn = rns_list_first(&http->hconns, rns_hconn_t, list);
    if(hconn != NULL)
    {
        if(hconn->fd < 0 && hconn->type == RNS_HTTP_TYPE_CLIENT)
        {
            int32_t ret = rns_hconn_open(hconn);
            if(ret < 0)
            {
                return NULL;
            }
        }
        rns_list_del(&hconn->list);
    }
    else if(hconn == NULL)
    {
        hconn = rns_hconn_create(http);
        if(hconn == NULL)
        {
            rns_hconn_destroy(&hconn);
            return NULL;
        }
    }
    
    rns_htune_init(&hconn->tune[1], cb);
    
    return hconn;
}

void rns_hconn_free(rns_hconn_t* hconn)
{
    if(rns_list_free(&hconn->list))
    {
        rns_list_add(&hconn->list, &hconn->http->hconns);
    }
    return;
}

int32_t rns_hconn_req(rns_hconn_t* hconn, uchar_t* buf, uint32_t size, uint32_t clean)
{
    if(buf != NULL && size > 0)
    {
        int32_t ret = rns_http_attach_buffer(hconn, buf, size, clean);
        if(ret < 0)
        {
            return -1;
        }
    }
    /* rns_list_add_safe(&hconn->send, &hconn->http->sends); */
    write_func(hconn->ep, hconn);
    return 0;
}

void rns_hconn_req_cb(rns_hconn_t* hconn, rns_http_cb_t* cb)
{
    rns_htune_init(&hconn->tune[1], cb);
    return;
}

int32_t rns_hconn_req2(rns_hconn_t* hconn, uchar_t* method, uchar_t* uri, uchar_t* buf, uint32_t size, uint32_t clean, uint32_t keepalive)
{
    int32_t ret = 0;
    ret = rns_hconn_req_header_init(hconn);
    if(ret < 0)
    {
        return -1;
    }
    uchar_t str[32];
    snprintf((char_t*)str, 32, "%d", size);
    ret = rns_hconn_req_header_insert(hconn, (uchar_t*)"Content-Length", str);
    if(ret < 0)
    {
        return -2;
    }

    if(keepalive)
    {
        ret = rns_hconn_req_header_insert(hconn, (uchar_t*)"Connection", (uchar_t*)"keepalive");
        if(ret < 0)
        {
            return -2;
        }
    }
    else
    {
        ret = rns_hconn_req_header_insert(hconn, (uchar_t*)"Connection", (uchar_t*)"Close");
        if(ret < 0)
        {
            return -2;
        }
    }
    
    ret = rns_hconn_req_header(hconn, method, uri);
    if(ret < 0)
    {
        return -3;
    }
    ret = rns_hconn_req(hconn, buf, size, clean);
    if(ret < 0)
    {
        return -4;
    }

    return 0;
}

void rns_hconn_req_done(rns_hconn_t* hconn)
{
    rbt_node_t* node = rbt_search_str(&hconn->tune[1].read_info.headers, (uchar_t*)"Connection");
    if(node == NULL)
    {
        node = rbt_search_str(&hconn->tune[1].write_info.headers, (uchar_t*)"Connection");
        if(node == NULL)
        {
            rns_hconn_destroy(&hconn);
            return;
        }
    }
    
    rns_node_t* item = rbt_entry(node, rns_node_t, node);
    if(!rns_strncasecmp(item->data, "Close", 5))
    {
        rns_hconn_destroy(&hconn);
    }
    else
    {
        rns_htune_destroy(&hconn->tune[1]);
        rns_hconn_free(hconn);
    }
    
    return;
}

void rns_hconn_resp_close(rns_hconn_t* hconn, void* data)
{
    rbt_node_t* node = rbt_search_str(&hconn->tune[0].write_info.headers, (uchar_t*)"Connection");
    hconn->write = 0;
    if(node == NULL || hconn->destroy == 1)
    {
        rns_hconn_destroy(&hconn);
    }
    else
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        if(!rns_strncasecmp(item->data, "Close", 5))
        {
            rns_hconn_destroy(&hconn);
        }
        else
        {
            rns_htune_destroy(&hconn->tune[0]);
        }

    }
    return;
}
void rns_hconn_resp_done(rns_hconn_t* hconn, void* data)
{
    hconn->phase = 0;
    return;
}

int32_t rns_hconn_resp(rns_hconn_t* hconn)
{
    rns_http_task_t* task = (rns_http_task_t*)rns_malloc(sizeof(rns_http_task_t));
    if(task == NULL)
    {
        return -1;
    }
    task->cb.done = rns_hconn_resp_done;
    task->cb.work = NULL;
    task->cb.close = rns_hconn_resp_close;
    task->cb.clean = NULL;
    task->cb.error = NULL;
    task->cb.data = NULL;

    rns_list_add(&task->list, &hconn->tasks);
    
    if(hconn->write)
    {
        write_func(hconn->ep, hconn);	
    }

    return 0;
}

int32_t rns_hconn_resp2(rns_hconn_t* hconn, uint32_t code, uchar_t* buf, uint32_t len, uint32_t clean)
{
    int32_t ret = rns_hconn_resp_header_init(hconn);
    if(ret < 0)
    {
        return -1;
    }
    
    uchar_t str[16];
    snprintf((char_t*)str, 16, "%d", len);
    ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Length", str);
    if(ret < 0)
    {
        return -2;
    }
    
    ret = rns_hconn_resp_header(hconn, code);
    if(ret < 0)
    {
        return -3;
    }

    if(len > 0 && buf != NULL)
    {
        ret = rns_http_attach_buffer(hconn, buf, len, clean);
        if(ret < 0)
        {
            return -5;
        }
    }

    
    ret = rns_hconn_resp(hconn);
    if(ret < 0)
    {
        return -4;
    }
    
    return 0;
}


int32_t rns_hconn_req_header_init(rns_hconn_t* hconn)
{
    int32_t ret = 0;
    
    ret = rns_hconn_req_header_insert(hconn, (uchar_t*)"Accept", (uchar_t*)"*/*");
    if(ret < 0)
    {
        return -3;
    }
    
    return 0;
}

int32_t rns_hconn_req_header_insert(rns_hconn_t* hconn, uchar_t* key, uchar_t* value)
{
    rns_node_t* item = NULL;
    rbt_node_t* node = rbt_search_str(&hconn->tune[1].write_info.headers, key);
    if(node != NULL)
    {
        item = rbt_entry(node, rns_node_t, node);
        rbt_delete(&hconn->tune[1].write_info.headers, node);
        if(item->data != NULL)
        {
            rns_free(item->data);
            item->data = NULL;
        }
        rns_free(item);
        item = NULL;
    }
    
    item = (rns_node_t*)rns_malloc(sizeof(rns_node_t));
    if(item == NULL)
    {
        return -1;
    }
    
    item->data = rns_malloc(rns_strlen(value) + 1);
    if(item->data == NULL)
    {
        rns_free(item);
        return -2;
    }
    
    rbt_set_key_str(&item->node, key);
    item->size = rns_strlen(value);
    rns_strncpy(item->data, value, item->size);
    rbt_insert(&hconn->tune[1].write_info.headers, &item->node);
    
    return 0;
}

int32_t rns_hconn_req_header(rns_hconn_t* hconn, uchar_t* method, uchar_t* uri)
{
    uchar_t* buf = rns_malloc(1024);
    if(buf == NULL)
    {
        return -1;
    }
    
    uint32_t send_len = 0;
    
    send_len += snprintf((char_t*)buf + send_len, 1024 - send_len, "%s %s HTTP/1.1\r\n", method, uri);
    rbt_node_t* node = NULL;
    rbt_for_each(&hconn->tune[1].write_info.headers, node)
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        send_len += snprintf((char_t*)buf + send_len, 1024 - send_len, "%s: %s\r\n", item->node.key.str, item->data);
    }
    send_len += snprintf((char_t*)buf + send_len, 1024 - send_len, "\r\n");
    
    return rns_http_attach_buffer(hconn, buf, send_len, 1);
}

int32_t rns_hconn_resp_header_init(rns_hconn_t* hconn)
{
    int32_t ret = 0;
    rbt_node_t* node = rbt_search_str(&hconn->tune[0].read_info.headers, (uchar_t*)"Connection");
    if(node == NULL)
    {
        ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Connection", (uchar_t*)"close");
        if(ret < 0)
        {
            return -1;
        }
    }
    else
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Connection", item->data);
        if(ret < 0)
        {
            return -2;
        }
        
    }
    
    ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Accept", (uchar_t*)"*/*");
    if(ret < 0)
    {
        return -3;
    }

    uchar_t* ext = rns_strrchr(hconn->tune[0].read_info.uri, '.');
    if(ext != NULL)
    {
        if(!rns_strncmp(ext + 1, "ts", 2))
        {
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"video/mp2t");
        }
        else if(!rns_strncmp(ext + 1, "m3u8", 4))
        {
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"application/vnd.apple.mpegurl");
        }
        else if(!rns_strncmp(ext + 1, "flv", 3))
        {
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"video/x-flv");
        }
        else if(!rns_strncmp(ext + 1, "mp4", 3))
        {
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"video/mp4");
        }
        else if(!rns_strncmp(ext + 1, "jpg", 3))
        {
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"image/jpeg");
        }
        else if(!rns_strncmp(ext + 1, "html", 4))
        {
            ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"text/html");
        }
        if(ret < 0)
        {
            return -4;
        }
    }
    else
    {
        ret = rns_hconn_resp_header_insert(hconn, (uchar_t*)"Content-Type", (uchar_t*)"text/html");
        if(ret < 0)
        {
            return -5;
        }
    }

    return 0;
}

int32_t rns_hconn_resp_header_insert(rns_hconn_t* hconn, uchar_t* key, uchar_t* value)
{
    rns_node_t* item = NULL;
    rbt_node_t* node = rbt_search_str(&hconn->tune[0].write_info.headers, key);
    if(node != NULL)
    {
        item = rbt_entry(node, rns_node_t, node);
        rbt_delete(&hconn->tune[0].write_info.headers, node);
        if(item->data != NULL)
        {
            rns_free(item->data);
            item->data = NULL;
        }
        rns_free(item);
        item = NULL;
    }
    
    item = (rns_node_t*)rns_malloc(sizeof(rns_node_t));
    if(item == NULL)
    {
        return -1;
    }

    item->data = rns_malloc(rns_strlen(value) + 1);
    if(item->data == NULL)
    {
        rns_free(item);
        return -2;
    }
    
    rbt_set_key_str(&item->node, key);
    item->size = rns_strlen(value);
    rns_strncpy(item->data, value, item->size);
    rbt_insert(&hconn->tune[0].write_info.headers, &item->node);

    return 0;
}

int32_t rns_hconn_resp_header(rns_hconn_t* hconn, uint32_t ret_code)
{
    uchar_t* buf = rns_malloc(1024);
    if(buf == NULL)
    {
        return -1;
    }
    uchar_t ret_str[64];
    rns_memset(ret_str, 64);
    
    switch (ret_code)
    {
        case 100 :
        {
            rns_strncpy(ret_str, "Continue", 63);
            break;
        }
        case 101 :
        {
            rns_strncpy(ret_str, "Switching Protocols", 63);
            break;
        }
        case 200 :
        {
            rns_strncpy(ret_str, "OK", 63);
            break;
        }
        case 201 :
        {
            rns_strncpy(ret_str, "Created", 63);
            break;
        }
        case 202 :
        {
            rns_strncpy(ret_str, "Accepted", 63);
            break;
        }
        case 203 :
        {
            rns_strncpy(ret_str, "Non-Authoritative Information", 63);
            break;
        }
        case 204 :
        {
            rns_strncpy(ret_str, "No Content", 63);
            break;
        }
        case 205 :
        {
            rns_strncpy(ret_str, "Reset Content", 63);
            break;
        }
        case 206 :
        {
            rns_strncpy(ret_str, "Partial Content", 63);
            break;
        }
        case 300 :
        {
            rns_strncpy(ret_str, "Multiple Choices", 63);
            break;
        }
        case 301 :
        {
            rns_strncpy(ret_str, "Moved Permanently", 63);
            break;
        }
        case 302 :
        {
            rns_strncpy(ret_str, "Found", 63);
            break;
        }
        case 303 :
        {
            rns_strncpy(ret_str, "See Other", 63);
            break;
        }
        case 304 :
        {
            rns_strncpy(ret_str, "Not Modified", 63);
            break;
        }
        case 305 :
        {
            rns_strncpy(ret_str, "Use Proxy", 63);
            break;
        }
        case 306 :
        {
            break;
        }
        case 307 :
        {
            rns_strncpy(ret_str, "Temporary Redirect", 63);
            break;
        }
        case 400 :
        {
            rns_strncpy(ret_str, "Bad Request", 63);
            break;
        }
        case 401 :
        {
            rns_strncpy(ret_str, "Unauthorized", 63);
            break;
        }
        case 402 :
        {
            rns_strncpy(ret_str, "Payment Required", 63);
            break;
        }
        case 403 :
        {
            rns_strncpy(ret_str, "Forbidden", 63);
            break;
        }
        case 404 :
        {
            rns_strncpy(ret_str, "Not Found", 63);
            break;
        }
        case 405 :
        {
            rns_strncpy(ret_str, "Method Not Allowed", 63);
            break;
        }
        case 406 :
        {
            rns_strncpy(ret_str, "Not Acceptable", 63);
            break;
        }
        case 407 :
        {
            rns_strncpy(ret_str, "Proxy Authentication Required", 63);
            break;
        }
        case 408 :
        {
            rns_strncpy(ret_str, "Request Timeout", 63);
            break;
        }
        case 409 :
        {
            rns_strncpy(ret_str, "Conflict", 63);
            break;
        }
        case 410 :
        {
            rns_strncpy(ret_str, "Gone", 63);
            break;
        }
        case 411 :
        {
            rns_strncpy(ret_str, "Length Required", 63);
            break;
        }
        case 412 :
        {
            rns_strncpy(ret_str, "Precondition Failed", 63);
            break;
        }
        case 413 :
        {
            rns_strncpy(ret_str, "Request Entity Too Large", 63);
            break;
        }
        case 414 :
        {
            rns_strncpy(ret_str, "Request-URI Too Long", 63);
            break;
        }
        case 415 :
        {
            rns_strncpy(ret_str, "Unsupported Media Type", 63);
            break;
        }
        case 416 :
        {
            rns_strncpy(ret_str, "Requested Range Not Satisfiable", 63);
            break;
        }
        case 417 :
        {
            rns_strncpy(ret_str, "Expectation Failed", 63);
            break;
        }
        case 500 :
        {
            rns_strncpy(ret_str, "Internal Server Error", 63);
            break;
        }
        case 501 :
        {
            rns_strncpy(ret_str, "Not Implemented", 63);
            break;
        }
        case 502 :
        {
            rns_strncpy(ret_str, "Bad Gateway", 63);
            break;
        }
        case 503 :
        {
            rns_strncpy(ret_str, "Service Unavailable", 63);
            break;
        }
        case 504 :
        {
            rns_strncpy(ret_str, "Gateway Timeout", 63);
            break;
        }
        case 505 :
        {
            rns_strncpy(ret_str, "HTTP Version Not Supported", 63);
            break;
        }
    }

    uint32_t send_len = 0;
    
    send_len += snprintf((char_t*)buf + send_len, 1024 - send_len, "HTTP/1.1 %d %s\r\n", ret_code, ret_str);
    rbt_node_t* node = NULL;
    rbt_for_each(&hconn->tune[0].write_info.headers, node)
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        send_len += snprintf((char_t*)buf + send_len, 1024 - send_len, "%s: %s\r\n", item->node.key.str, item->data);
    }
    send_len += snprintf((char_t*)buf + send_len, 1024 - send_len, "\r\n");

    return rns_http_attach_buffer(hconn, buf, send_len, 1);
}

//------------------------------------------------------------------------------

typedef struct rns_http_download_s
{
    rns_file_t* fp;
    uint32_t offset;
    uint32_t size;
    uint32_t len;
}rns_http_download_t;

static int32_t file_download_work(rns_hconn_t* hconn, void* data)
{
    rns_http_download_t* task = (rns_http_download_t*)data;

    if(task->len >= task->size)
    {
        return 0;
    }
    
    int32_t ret = rns_hconn_sendfile(hconn, task->fp, task->offset, task->size - task->len);
    if(ret < 0)
    {
        return ret;
    }

    task->offset += ret;
    task->len += ret;
    
    return ret;
}

static void file_download_error(rns_hconn_t* hconn, void* data)
{
    rns_http_download_t* task = (rns_http_download_t*)data;
    
    rns_file_close(&task->fp);
    
    return;
}

static void file_download_done(rns_hconn_t* hconn, void* data)
{
    rns_http_download_t* task = (rns_http_download_t*)data;
    
    rns_file_close(&task->fp);
    
    return;
}

int32_t rns_http_download(rns_hconn_t* hconn, rns_file_t* fp, uint32_t start, uint32_t size)
{
    if(hconn == NULL || fp == NULL)
    {
        return -1;
    }
    rns_http_download_t* task = (rns_http_download_t*)rns_malloc(sizeof(rns_http_download_t));
    if(task == NULL)
    {
        return -2;
    }

    rns_http_task_t* t = (rns_http_task_t*)rns_malloc(sizeof(rns_http_task_t));
    if(t == NULL)
    {
        rns_free(task);
        return -3;
    }
    
    task->fp = fp;
    task->offset = start;
    task->size = size;
    
    uint32_t send_buf_size = 2 * 1024 * 1024;
    int32_t ret = setsockopt(hconn->fd, SOL_SOCKET, SO_SNDBUF, (const char*)&send_buf_size, sizeof(uint32_t));
    if(ret < 0)
    {
        rns_free(task);
        return -4;
    }
    
    t->cb.work = file_download_work;
    t->cb.error = file_download_error;
    t->cb.done = file_download_done;
    t->cb.data = task;

    rns_list_add(&t->list, &hconn->tasks);
    
    return 0;
}

//-------------------------------------------------------------------------------
int32_t rns_http_send_buffer_work(rns_hconn_t* hconn, void* data)
{

    rns_data_t* rdata = (rns_data_t*)data;
    /* debug("[write]send buffer, size : %d, offset : %d\n", rdata->size, rdata->offset); */

    if(rdata->size <= rdata->offset)
    {
        return 0;
    }
    
    int32_t ret = rns_hconn_write(hconn, (uchar_t*)rdata->data + rdata->offset, rdata->size - rdata->offset);
    if(ret < 0)
    {
        return ret;
    }
    rdata->offset += ret;
    
    return ret;
}
    
void rns_http_send_buffer_done(rns_hconn_t* hconn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    /* debug("[write]send buffer close, size : %d, offset : %d\n", rdata->size, rdata->offset); */
    rns_free(rdata);
    return;
}

void rns_http_send_buffer_error(rns_hconn_t* hconn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    /* debug("[write]send buffer close, size : %d, offset : %d\n", rdata->size, rdata->offset); */
    rns_free(rdata);
    return;
}

void rns_http_send_buffer_done_deep(rns_hconn_t* hconn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    /* debug("[write]send buffer close clean, size : %d, offset : %d\n", rdata->size, rdata->offset); */
    if(rdata->data != NULL)
    {
        rns_free(rdata->data);
    }
    rns_free(rdata);
    return;
}


void rns_http_send_buffer_error_deep(rns_hconn_t* hconn, void* data)
{
    rns_data_t* rdata = (rns_data_t*)data;
    /* debug("[write]send buffer close clean, size : %d, offset : %d\n", rdata->size, rdata->offset); */
    if(rdata->data != NULL)
    {
        rns_free(rdata->data);
    }
    rns_free(rdata);
    return;
}

int32_t rns_http_attach_buffer(rns_hconn_t* hconn, uchar_t* buf, uint32_t size, uint32_t clean)
{
    rns_http_task_t* task = (rns_http_task_t*)rns_malloc(sizeof(rns_http_task_t));
    if(task == NULL)
    {
        return -1;
    }

    rns_data_t* rdata = (rns_data_t*)rns_malloc(sizeof(rns_data_t));
    if(rdata == NULL)
    {
        rns_free(task);
        return -2;
    }
    rdata->data = buf;
    rdata->size = size;

    task->cb.work = rns_http_send_buffer_work;
    if(clean)
    {
        task->cb.done = rns_http_send_buffer_done_deep;
        task->cb.error = rns_http_send_buffer_error_deep;
    }
    else
    {
        task->cb.done = rns_http_send_buffer_done;
        task->cb.error = rns_http_send_buffer_error;
    }
    task->cb.data = rdata;

    rns_list_add(&task->list, &hconn->tasks);
    return 0;
}


//-------------------------------------
rns_http_url_t* rns_http_url_create(uchar_t* url)
{
    uchar_t* walker = url;
    uchar_t* ptr = url;
    uchar_t port[8];
    
    rns_http_url_t* http_url = (rns_http_url_t*)rns_malloc(sizeof(rns_http_url_t));
    if(http_url == NULL)
    {
        return NULL;
    }
    
    walker = rns_strstr(url, "//");
    if(walker == NULL)
    {
        rns_free(http_url);
        return NULL;
    }
    
    walker += 2;
    ptr = walker;
    
    
    while(*walker != ':' && *walker != '/')++walker;
    if(walker > ptr + 128)
    {
        rns_free(http_url);
        return NULL;
    }

    http_url->addr = rns_malloc(walker - ptr + 1);
    if(http_url->addr == NULL)
    {
        rns_free(http_url);
        return NULL;
    }
    rns_memcpy(http_url->addr, ptr, walker - ptr);

    
    if(*walker == ':')
    {
        ptr = walker + 1;
        walker = rns_strchr(walker, '/');
        if(walker == NULL || walker > ptr + 8)
        {
            rns_free(http_url);
            return NULL;
        }
        
        rns_strncpy(port, ptr, walker - ptr);
        http_url->port = rns_atoi(port);
    }
    else
    {
        http_url->port = 80;
    }
    
    ptr = walker;
    walker = rns_strchr(walker, '?');
    if(walker != NULL)
    {
        http_url->uri = rns_malloc(walker - ptr + 1);
        if(http_url->uri == NULL)
        {
            rns_free(http_url);
            return NULL;
        }
        rns_strncpy(http_url->uri, ptr, walker - ptr);
    }
    else
    {
        http_url->uri = rns_malloc(rns_strlen(ptr) + 1);
        if(http_url->uri == NULL)
        {
            rns_free(http_url);
            return NULL;
        }
        rns_strncpy(http_url->uri, ptr, rns_strlen(ptr));
        
    }
    
    //need handle args
    return http_url;
}

void rns_http_url_destroy(rns_http_url_t** http_url)
{
    if(*http_url == NULL)
    {
        return;
    }
    
    rbt_node_t* node = NULL;
    while((node = rbt_first(&(*http_url)->args)) != NULL)
    {
        rns_node_t* item = rbt_entry(node, rns_node_t, node);
        if(item->data != NULL)
        {
            rns_free(item->data);
            item->data = NULL;
        }
        rns_free(item);
    }
    
    if((*http_url)->addr != NULL)
    {
        rns_free((*http_url)->addr);
    }
    rns_free((*http_url)->uri);
    rns_free(*http_url);
    *http_url = NULL;
    return;
}
