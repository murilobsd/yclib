#include "rns_dbpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void errorcb(rns_mysql_t* mysql, void* data)
{
    printf("connect error, %s\n", rns_mysql_errstr(mysql));
    return;
}

void connectcb(rns_mysql_t* mysql, void* data)
{
    rns_dbpool_t* dbpool = (rns_dbpool_t*)data;
    
    rns_list_add(&mysql->list, &dbpool->mysqls);
    ++dbpool->num;
    if(dbpool->num < dbpool->max)
    {
        rns_mysql_t* m = rns_mysql_init(mysql->user, mysql->passwd, mysql->db, mysql->ip, mysql->port, NULL, 0);
        
        rns_mysql_cb_t cb;
        cb.work  = connectcb;
        cb.done  = NULL;
        cb.error = errorcb;
        cb.data  = dbpool;
        
        m->dbpool = dbpool;

        int32_t ret = rns_mysql_connect(m, &cb);
        if(ret < 0)
        {
            return;
        }
        
        ret = rns_epoll_add(dbpool->ep, rns_mysql_sock(m), m, RNS_EPOLL_IN | RNS_EPOLL_OUT);
        if(ret < 0)
        {
            return;
        }
       
    }
    else
    {
        if(dbpool->cb.func != NULL)
        {
            dbpool->cb.func(dbpool->cb.data);
        }
    }
    
    return;
}

rns_dbpool_t* rns_dbpool_create(rns_epoll_t* ep, rns_dbpara_t* param, uint32_t min, uint32_t max, rns_cb_t* pcb)
{
    if(NULL == param || ep == NULL)
    {
        goto ERR_EXIT;
    }

    int32_t ret = 0;
    rns_dbpool_t* dbpool = (rns_dbpool_t*)rns_malloc(sizeof(rns_dbpool_t));
    if(dbpool == NULL)
    {
        goto ERR_EXIT;
    }
    dbpool->min = min;
    dbpool->max = max;
    dbpool->ep = ep;
    if(pcb != NULL)
    {
        rns_memcpy(&dbpool->cb, pcb, sizeof(rns_cb_t));
    }

    
    INIT_LIST_HEAD(&dbpool->mysqls);


    rns_mysql_t* mysql = rns_mysql_init((uchar_t*)(param->username), (uchar_t*)(param->password), (uchar_t*)(param->database), (uchar_t*)(param->hostip), param->port, NULL, 0);
    rns_mysql_cb_t cb;
    cb.work  = connectcb;
    cb.done  = NULL;
    cb.error = NULL;
    cb.data  = dbpool;

    mysql->dbpool = dbpool;
    
    ret = rns_mysql_connect(mysql, &cb);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }

    ret = rns_epoll_add(ep, rns_mysql_sock(mysql), mysql, RNS_EPOLL_IN | RNS_EPOLL_OUT);
    if(ret < 0)
    {
        goto ERR_EXIT;
    }
    
    return dbpool;
ERR_EXIT:
    if(dbpool != NULL)
    {
        list_head_t* p = NULL;
        list_head_t* q = NULL;
        rns_list_for_each_safe(p, q, &dbpool->mysqls)
        {
            rns_mysql_t* m = rns_list_entry(p, rns_mysql_t, list);
            rns_mysql_destroy(&m);
        }
        rns_free(dbpool);
    }
    return NULL;
}

void rns_dbpool_destroy(rns_dbpool_t** dbpool)
{
    if(*dbpool == NULL)
    {
        return;
    }
    list_head_t* p = NULL;
    list_head_t* q = NULL;
    rns_list_for_each_safe(p, q, &(*dbpool)->mysqls)
    {
        rns_mysql_t* m = rns_list_entry(p, rns_mysql_t, list);
        rns_mysql_destroy(&m);
    }
    
    rns_free(*dbpool);
    *dbpool = NULL;
    return;
}


rns_mysql_t* rns_dbpool_get(rns_dbpool_t* dbpool)
{
    rns_mysql_t* mysql = rns_list_first(&dbpool->mysqls, rns_mysql_t, list);
    if(mysql == NULL)
    {
        return NULL;
    }
    rns_list_del(&mysql->list);
    
    return mysql;
}

void rns_dbpool_free(rns_mysql_t* mysql)
{
    if(mysql != NULL)
    {
        rns_list_add_safe(&mysql->list, &mysql->dbpool->mysqls);
        if(mysql->sql != NULL)
        {
            rns_free(mysql->sql);
            mysql->sql = NULL;
        }
    }
    return;
}

