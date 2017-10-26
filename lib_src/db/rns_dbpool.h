/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_dbpool.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-07-25 09:44:55
 * -------------------------
**************************************************************************/

#ifndef _RNS_DBPOOL_H_
#define _RNS_DBPOOL_H_

#include "mysql.h"
#include "rns.h"
#include "rns_mysql.h"

#define MYSQL_MIN_CONNECT_NUM  3
#define MYSQL_MAX_CONNECT_NUM  16

typedef struct rns_dbpara_s
{
    uchar_t* hostip;    
    uchar_t* username;    
    uchar_t* password;
    uchar_t* database;
    int  port;  
    unsigned int client_flag;  //<Ò»°ãÎª0

}rns_dbpara_t;

typedef struct rns_dbpool_s
{
    rns_dbpara_t param;
    list_head_t  mysqls;
    uint32_t num;
    uint32_t min;
    uint32_t max;
    rns_epoll_t* ep;
    rns_cb_t cb;
}rns_dbpool_t;

rns_dbpool_t* rns_dbpool_create(rns_epoll_t* ep, rns_dbpara_t* param, uint32_t min, uint32_t max, rns_cb_t* cb);
void rns_dbpool_destroy(rns_dbpool_t** dbpool);
rns_mysql_t* rns_dbpool_get(rns_dbpool_t* dbpool);
void rns_dbpool_free(rns_mysql_t* mysql);

#endif

