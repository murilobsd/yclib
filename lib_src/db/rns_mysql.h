/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_mysql.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-06-05 10:31:35
 * -------------------------
**************************************************************************/

#ifndef _RNS_MYSQL_H_
#define _RNS_MYSQL_H_

#include "rns.h"
#include <mysql.h>

struct rns_mysql_s;
struct rns_dbpool_s;
typedef struct rns_mysql_cb_s
{
    void (*work)(struct rns_mysql_s* mysql, void*);
    void (*done)(struct rns_mysql_s* mysql, void*);
    void (*error)(struct rns_mysql_s* mysql, void*);
    void* data;
}rns_mysql_cb_t;

typedef struct rns_mysql_s
{
    rns_epoll_cb_t ecb;
    rns_mysql_cb_t ccb;
    rns_mysql_cb_t qcb;
    list_head_t list;
    struct rns_dbpool_s* dbpool;
    MYSQL* mysql;
    MYSQL_RES* res;
    MYSQL_ROW row;
    uchar_t* user;
    uchar_t* passwd;
    uchar_t* db;
    uchar_t* ip;
    uint32_t port;
    uchar_t* unix_socket;
    uint32_t client_flags;
    uint32_t state; // different op
    uint32_t status; // different status of op
    uint32_t rownum;     		 // 查询结果返回的行数
    uint32_t colnum;       		 // 查询结果返回的列数
    uchar_t* sql;
}rns_mysql_t;


//-------------------------------------------------
rns_mysql_t* rns_mysql_init(const uchar_t *user, const uchar_t *passwd, const uchar_t *db, uchar_t* ip, uint32_t port, const uchar_t* unix_socket,  unsigned long client_flags);
int32_t rns_mysql_connect(rns_mysql_t* mysql, rns_mysql_cb_t* cb);
int32_t rns_mysql_reconnect(rns_mysql_t* mysql, rns_mysql_cb_t* cb);
void rns_mysql_destroy(rns_mysql_t** mysql);

int32_t rns_mysql_query(rns_mysql_t* mysql, uchar_t* buf, uint32_t size, rns_mysql_cb_t* cb);
//-----------------------------------------------

int32_t rns_mysql_opt(rns_mysql_t* mysql);
int32_t rns_mysql_sock(rns_mysql_t* mysql);
uchar_t* rns_mysql_errstr(rns_mysql_t* mysql);
int32_t rns_mysql_errno(rns_mysql_t* mysql);

#endif



