#include "rns_network.h"
#include "rns_memory.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include "rns_signal.h"


uint32_t is_le()
{
    uint32_t a = 1;
    if(*((uchar_t*)&a) == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint32_t rns_ip_type(uchar_t* ip, uint32_t size)
{
    uchar_t* walker = ip;
    uchar_t* end = ip + size;

    uint32_t len = 0;
    uint32_t count = 0;
   
    while(walker < end)
    {
        if(*walker != '.' && !isdigit(*walker))
        {
            return RNS_URL_TYPE_DOMAIN;
        }
        
        if(*walker == '.')
        {
            if(len > 3)
            {
                return RNS_URL_TYPE_DOMAIN;
            }
            ++count;
            if(count > 3)
            {
                return RNS_URL_TYPE_DOMAIN;
            }
            len = 0;
        }
        else
        {
            ++len;
            if(len > 3)
            {
                return RNS_URL_TYPE_DOMAIN;
            }
        }
        
        ++walker;
    }
    
    return RNS_URL_TYPE_IP;
}

uchar_t* rns_ip_int2str(uint32_t ip, uchar_t* buf, uint32_t size)
{
    if(size < 20 || buf == NULL)
    {
        return NULL;
    }
    
    snprintf((char_t*)buf, 20, "%d.%d.%d.%d", (ip >> 24 & 0x000000FF), (ip >> 16 & 0x000000FF), (ip >> 8 & 0x000000FF), (ip & 0x000000FF));
    
    return buf;
}


uint32_t rns_ip_str2int(uchar_t* ip)
{

    if(ip == NULL)
    {
        return 0;
    }
    uint32_t ip_num = 0;
    uchar_t buf[64];
    rns_memset(buf, 64);
    rns_strncpy(buf, ip, 63);

    uchar_t* walker = buf;

    walker = rns_strtok(buf, ".");
    if(walker == NULL)
    {
        return 0;
    }
    
    ip_num += rns_atoi(walker);
    
    while((walker = rns_strtok(NULL, ".")) != NULL)
    {
        ip_num = ip_num << 8;
        ip_num += rns_atoi(walker);
    }
    return ip_num;
}

uchar_t* rns_reverse(uchar_t* buf, uint32_t size)
{
    uchar_t* left = buf;
    uchar_t* right = buf + size - 1;
    
    while (right > left)
    {
        *right = *right ^ *left ;
        *left = *right ^ *left ;
        *right-- ^= *left++ ;
    }
    
    return buf ;
}

//这些都是大端序
uint64_t rns_btoh64(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 8;
    
    
    while(size > 0)
    {
        result += *ptr;
        --size;
        if(size <= 0)
        {
            break;
        }
        result = result << 8;
    }
    
    return result;
}

uchar_t* rns_htob64(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 8;
    while(size > 0)
    {
        *ptr++ = (data >> (56 - ((8 - size) << 3))) & 0x00000000000000FF;
        --size;
    }
    
    return buf;
}


uint32_t rns_btoh32(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 4;

    
    while(size > 0)
    {
        result += *ptr++;
        --size;
        if(size <= 0)
        {
            break;
        }
        result = result << 8;
    }

    return result;
}

uchar_t* rns_htob32(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 4;
    while(size > 0)
    {
        *ptr++ = (data >> (24 - ((4 - size) << 3))) & 0x000000FF;
        --size;
    }

    return buf;
}

uint32_t rns_ltoh32(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 4;
    
    
    while(size > 0)
    {
        result += (*ptr++) << ((4 - size) * 8) ;
        --size;
    }
    
    return result;
}

uchar_t* rns_htol32(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 4;
    while(size > 0)
    {
        *ptr++ = (data >> (32 - (size << 3))) & 0x000000FF;
        --size;
    }
    
    return buf;
}

uint32_t rns_btoh24(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 3;
    
    
    while(size > 0)
    {
        result += *ptr++;
        --size;
        if(size <= 0)
        {
            break;
        }
        result = result << 8;
    }
    
    return result;
}


uchar_t* rns_htob24(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 3;
    while(size > 0)
    {
        *ptr++ = (data >> (24 - ((4 - size) << 3))) & 0x000000FF;
        --size;
    }
    
    return buf;
}

uint32_t rns_ltoh24(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 3;
    
    
    while(size > 0)
    {
        result += (*ptr++) << ((3 - size) * 8) ;
        --size;
    }
    
    return result;
}


uchar_t* rns_htol24(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 3;
    while(size > 0)
    {
        *ptr++ = (data >> (24 - (size << 3))) & 0x000000FF;
        --size;
    }
    
    return buf;
}



uint32_t rns_btoh16(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 2;
    
    
    while(size > 0)
    {
        result += *ptr++;
        --size;
        if(size <= 0)
        {
            break;
        }
        result = result << 8;
    }
    
    return result;
}

uchar_t* rns_htob16(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 2;
    while(size > 0)
    {
        *ptr++ = (data >> (24 - ((4 - size) << 3))) & 0x000000FF;
        --size;
    }
    
    return buf;
}

uint32_t rns_ltoh16(uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t result = 0;
    uint32_t size = 2;
    
    while(size > 0)
    {
        result += (*ptr++) << ((2 - size) * 8) ;
        --size;
    }
    
    return result;
}

uchar_t* rns_htol16(uint32_t data, uchar_t* buf)
{
    uchar_t* ptr = buf;
    uint32_t size = 2;
    while(size > 0)
    {
        *ptr++ = (data >> (16 - (size << 3))) & 0x000000FF;
        --size;
    }
    
    return buf;
}


uchar_t* rns_htobd(double data, uchar_t* buf)
{
    rns_reverse((uchar_t*)&data, sizeof(double));
    rns_memcpy(buf, (uchar_t*)&data, sizeof(double));

    return buf;
}

double rns_btohd(uchar_t* buf)
{
    double data;
    rns_memcpy((uchar_t*)&data, buf, sizeof(double));
    rns_reverse((uchar_t*)&data, sizeof(double));

    return data;
}


uchar_t* rns_htold(double data, uchar_t* buf)
{
    rns_reverse((uchar_t*)&data, sizeof(double));
    rns_memcpy(buf, (uchar_t*)&data, sizeof(double));
    
    return buf;
}

double rns_ltohd(uchar_t* buf)
{
    double data;
    rns_reverse(buf, sizeof(double));
    rns_memcpy((uchar_t*)&data, buf, sizeof(double));
    
    return data;
}


int32_t rns_server_init(rns_addr_t* addr)
{
    int32_t listenfd;
    struct sockaddr_in servaddr;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        return -1;
    }
    
    int32_t on = 1;
    int32_t ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
    {
        close(listenfd);
        return -2;
    }
    
    int32_t flags = fcntl(listenfd, F_GETFL, 0);
    ret = fcntl(listenfd, F_SETFL, flags|O_NONBLOCK);
    if(ret < 0)
    {
        close(listenfd);
        return -3;
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = rns_htonl(addr->ip);
    servaddr.sin_port = rns_htons(addr->port);
    
    ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(ret < 0)
    {
        close(listenfd);
        return -3;
    }
    
    ret = listen(listenfd, 1000);
    if(ret < 0)
    {
        close(listenfd);
        return -4;
    }
    
    return listenfd;
}


void addrcb(void* data)
{
    rns_ctx_t* ctx = (rns_ctx_t*)data;
    rns_addr_cb_t* cb = rns_ctx_get(ctx, "cb");
    struct gaicb* host = rns_ctx_get(ctx, "host");

    if(cb->func != NULL)
    {
        cb->func(cb->data, ((struct sockaddr_in*)(host->ar_result->ai_addr))->sin_addr.s_addr);
    }

    rns_free(host);
    rns_free(cb);
    rns_free(ctx);
    return;
}


int32_t rns_ip_getaddr(uchar_t* domain, rns_addr_cb_t* cb)
{
    struct sigevent sev;
    rns_cb_t* scb = (rns_cb_t*)rns_malloc(sizeof(rns_cb_t));
    if(scb == NULL)
    {
        return -1;
    }
    
    struct gaicb* host = (struct gaicb*)rns_malloc(sizeof(struct gaicb));
    if(host == NULL)
    {
        rns_free(scb);
        return -2;
    }
    
    rns_addr_cb_t* c = (rns_addr_cb_t*)rns_malloc(sizeof(rns_addr_cb_t));
    if(c == NULL)
    {
        rns_free(host);
        rns_free(scb);
        return -3;
    }

    rns_ctx_t* ctx = rns_ctx_create();
    if(ctx == NULL)
    {
        rns_free(c);
        rns_free(host);
        rns_free(scb);
        return -4;
    }
    rns_ctx_add(ctx, "cb", c);
    rns_ctx_add(ctx, "host", host);
    
    rns_memcpy(c, cb, sizeof(rns_addr_cb_t));

    
    scb->func = addrcb;
    scb->data = ctx;
    
    host->ar_name = (char_t*)rns_dup(domain);
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_value.sival_ptr = scb;
    sev.sigev_signo = SIGRTMIN;

    return getaddrinfo_a(GAI_NOWAIT, &host, 1, &sev);
}

uchar_t rns_char2value(uchar_t c)
{
    if(c <= '9' && c >= '0')
    {
        return c - '0';
    }

    if(c <= 'f' && c >= 'a')
    {
        return c - 'a' + 10;
    }

    if(c <= 'F' && c >= 'A')
    {
        return c - 'A' + 10;
    }

    return 0;
}

uint32_t rns_hex2dec(uchar_t* hex, uint32_t size)
{
    uint32_t i = 0;
    uint32_t value = 0;
    if(size > 8)
    {
        size = 8;
    }
    for(i = 0; i < size; ++i)
    {
        if(i > 0)
        {
            value = value << 4;
        }
        value += rns_char2value(hex[i]);
    }

    return value;
}

uchar_t* rns_dec2hex(uint32_t value)
{
    return NULL;
}
