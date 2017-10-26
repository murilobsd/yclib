/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_network.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-18 15:52:08
 * -------------------------
**************************************************************************/

#ifndef _RNS_NETWORK_H_
#define _RNS_NETWORK_H_

#include "comm.h"
#include <arpa/inet.h>

#define RNS_URL_TYPE_IP       0
#define RNS_URL_TYPE_DOMAIN   1

typedef struct rns_addr_cb_s
{
    void (*func)(void*, uint32_t);
    void* data;
}rns_addr_cb_t;

int32_t rns_ip_getaddr(uchar_t* domain, rns_addr_cb_t* cb);

#define rns_htonl(num)  htonl(num)
#define rns_ntohl(num)  ntohl(num)

#define rns_ntohs(num)  ntohs(num)
#define rns_htons(num)  htons(num)

uchar_t* rns_ip_int2str(uint32_t ip, uchar_t* buf, uint32_t size);
uint32_t rns_ip_str2int(uchar_t* ip);
uint32_t rns_ip_type(uchar_t* ip, uint32_t size);

uchar_t* rns_reverse(uchar_t* buf, uint32_t size);

uchar_t* rns_htob64(uint32_t data, uchar_t* buf);
uint64_t rns_btoh64(uchar_t* buf);

uchar_t* rns_htob32(uint32_t data, uchar_t* buf);
uint32_t rns_btoh32(uchar_t* buf);

uchar_t* rns_htol32(uint32_t data, uchar_t* buf);
uint32_t rns_ltoh32(uchar_t* buf);

uchar_t* rns_htob24(uint32_t data, uchar_t* buf);
uint32_t rns_btoh24(uchar_t* buf);

uchar_t* rns_htol24(uint32_t data, uchar_t* buf);
uint32_t rns_ltoh24(uchar_t* buf);

uchar_t* rns_htob16(uint32_t data, uchar_t* buf);
uint32_t rns_btoh16(uchar_t* buf);

uchar_t* rns_htol16(uint32_t data, uchar_t* buf);
uint32_t rns_ltoh16(uchar_t* buf);

uchar_t* rns_htobd(double data, uchar_t* buf);
double rns_btohd(uchar_t* buf);

uchar_t* rns_htold(double data, uchar_t* buf);
double rns_ltohd(uchar_t* buf);

int32_t rns_server_init(rns_addr_t* addr);


uint32_t rns_hex2dec(uchar_t* hex, uint32_t size);
#endif
