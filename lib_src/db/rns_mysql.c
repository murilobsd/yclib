#include <stdio.h>
#include "rns_mysql.h"

void rns_mysql_init_cont(void* data);
void rns_mysql_connect_cont(void* data);
void rns_mysql_query_cont(void* data);
void rns_mysql_fetch_cont(void* data);
void rns_mysql_reconnect_cont(void* data);
void rns_mysql_fetch(rns_mysql_t* mysql);
void rns_mysql_free(rns_mysql_t* mysql);
void rns_mysql_free_cont(void* data);

#define RNS_MYSQL_STATE_INIT      0
#define RNS_MYSQL_STATE_CONNECT   1
#define RNS_MYSQL_STATE_QUERY     2
#define RNS_MYSQL_STATE_FETCH     3
#define RNS_MYSQL_STATE_RECONNECT   4
#define RNS_MYSQL_STATE_FREE       5


static void read_func(rns_epoll_t* ep, void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    printf("read func, state : %d\n", mysql->state);
    switch (mysql->state)
    {
        case RNS_MYSQL_STATE_CONNECT :
        {
            rns_mysql_connect_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_QUERY :
        {
            rns_mysql_query_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_FETCH :
        {
            rns_mysql_fetch_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_RECONNECT :
        {
            rns_mysql_reconnect_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_FREE :
        {
            rns_mysql_free_cont(mysql);
            break;
        }
    }

    return;
}

static void write_func(rns_epoll_t* ep, void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    switch (mysql->state)
    {
        case RNS_MYSQL_STATE_CONNECT :
        {
            rns_mysql_connect_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_QUERY :
        {
            rns_mysql_query_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_FETCH :
        {
            rns_mysql_fetch_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_RECONNECT :
        {
            rns_mysql_reconnect_cont(mysql);
            break;
        }
        case RNS_MYSQL_STATE_FREE :
        {
            rns_mysql_free_cont(mysql);
            break;
        }
    }
    return;
}

void rns_mysql_init_cont(void* data)
{
    return;
}

void rns_mysql_reconnect_cont(void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    mysql->status = mysql_real_connect_cont(&mysql->mysql, mysql->mysql, mysql->status);
    if(!mysql->status)
    {
        if(mysql->mysql == NULL)
        {
            if(mysql->qcb.error != NULL)
            {
                mysql->qcb.error(mysql, mysql->qcb.data);
            }
        }
        else
        {
            mysql->state = RNS_MYSQL_STATE_QUERY;
            int32_t ret = rns_mysql_query(mysql, mysql->sql, rns_strlen(mysql->sql), &mysql->qcb);
            if(ret < 0)
            {
                return;
            }
            
        }
    }
    
    return;
}

void rns_mysql_connect_cont(void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    MYSQL* m = NULL;
    mysql->status = mysql_real_connect_cont(&m, mysql->mysql, mysql->status);
    if(!mysql->status)
    {
        if(m == NULL)
        {
            if(mysql->ccb.error != NULL)
            {

                mysql->ccb.error(mysql, mysql->ccb.data);
            }
        }
        else
        {
            mysql->state = RNS_MYSQL_STATE_INIT;
            if(mysql->ccb.work != NULL)
            {
                mysql->ccb.work(mysql, mysql->ccb.data);
            }
        }
    }

    return;
}

void rns_mysql_query_cont(void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    int32_t ret = 0;
    mysql->status = mysql_real_query_cont(&ret, mysql->mysql, mysql->status);
    if(ret != 0)
    {
        if(mysql->qcb.error != NULL)
        {
            mysql->qcb.error(mysql, mysql->qcb.data);
        }
        return;
    }
    if(!mysql->status)
    {
        mysql->res = mysql_use_result(mysql->mysql);
        if(mysql->res)
        {
            rns_mysql_fetch(mysql);

        }
        else
        {
            rns_mysql_free(mysql);
        }
    }
    return;
}

void rns_mysql_fetch_cont(void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    mysql->status = mysql_fetch_row_cont(&mysql->row, mysql->res, mysql->status);
    if(!mysql->status)
    {
        if(mysql->row)
        {
            ++mysql->rownum;
            if(mysql->qcb.work != NULL)
            {
                mysql->qcb.work(mysql, mysql->qcb.data);
            }
            rns_mysql_fetch(mysql);
        }
        else
        {
            rns_mysql_free(mysql);
        }
    }
    return;
}

void rns_mysql_free_cont(void* data)
{
    rns_mysql_t* mysql = (rns_mysql_t*)data;
    mysql->status = mysql_free_result_cont(mysql->res, mysql->status);
    if(!mysql->status)
    {
        mysql->state = RNS_MYSQL_STATE_INIT;
        if(mysql->qcb.done != NULL)
        {
            mysql->qcb.done(mysql, mysql->qcb.data);
        }
    }
    return;
}

rns_mysql_t* rns_mysql_init(const uchar_t *user, const uchar_t *passwd, const uchar_t *db, uchar_t* ip, uint32_t port, const uchar_t* unix_socket,  unsigned long client_flags)
{
    rns_mysql_t* mysql = (rns_mysql_t*)rns_malloc(sizeof(rns_mysql_t));
    if(mysql == NULL)
    {
        return NULL;
    }

    mysql->mysql = mysql_init(NULL);
    if(mysql->mysql == NULL)
    {
        rns_free(mysql);
        return NULL;
    }
    
    int32_t ret = mysql_options(mysql->mysql, MYSQL_OPT_NONBLOCK, 0);
    if(ret < 0)
    {
        mysql_close(mysql->mysql);
        rns_free(mysql);
        return NULL;
    }


    mysql->user = rns_malloc(rns_strlen(user) + 1);
    if(mysql->user == NULL)
    {
        goto ERR_EXIT;
    }
    rns_strncpy(mysql->user, user, rns_strlen(user));
    
    mysql->passwd = rns_malloc(rns_strlen(passwd) + 1);
    if(mysql->passwd == NULL)
    {
        goto ERR_EXIT;
    }
    rns_strncpy(mysql->passwd, passwd, rns_strlen(passwd));
    
    mysql->db = rns_malloc(rns_strlen(db) + 1);
    if(mysql->db == NULL)
    {
        goto ERR_EXIT;
    }
    rns_strncpy(mysql->db, db, rns_strlen(db));
    
    mysql->ip = rns_malloc(rns_strlen(ip) + 1);
    if(mysql->ip == NULL)
    {
        goto ERR_EXIT;
    }
    rns_strncpy(mysql->ip, ip, rns_strlen(ip));
    
    mysql->port = port;

    if(unix_socket != NULL)
    {
        mysql->unix_socket = rns_malloc(rns_strlen(unix_socket) + 1);
        if(mysql->unix_socket == NULL)
        {
            goto ERR_EXIT;
        }
        rns_strncpy(mysql->unix_socket, unix_socket, rns_strlen(unix_socket));
    }

    mysql->client_flags = client_flags;
    
    mysql->ecb.read = read_func;
    mysql->ecb.read = write_func;
    
    return mysql;

ERR_EXIT:

    if(mysql != NULL)
    {
        if(mysql->user != NULL)
        {
            rns_free(mysql->user);
        }
        if(mysql->passwd != NULL)
        {
            rns_free(mysql->passwd);
        }
        if(mysql->ip != NULL)
        {
            rns_free(mysql->ip);
        }
        if(mysql->db != NULL)
        {
            rns_free(mysql->db);
        }
        mysql_close(mysql->mysql);
        rns_free(mysql);
    }
    return NULL;
}

int32_t rns_mysql_reconnect(rns_mysql_t* mysql, rns_mysql_cb_t* cb)
{
    int32_t retcode = 0;
    
    mysql->status = mysql_real_connect_start(&mysql->mysql, mysql->mysql, (char_t*)mysql->ip, (char_t*)mysql->user, (char_t*)mysql->passwd, (char_t*)mysql->db, mysql->port, (char_t*)mysql->unix_socket, mysql->client_flags);
    if(mysql->mysql == NULL)
    {
        retcode = -5;
        goto ERR_EXIT;
    }
    
    mysql->state = RNS_MYSQL_STATE_RECONNECT;
    if(!mysql->status)
    {
        rns_mysql_connect_cont(mysql);
    }
    
    return 0;
    
ERR_EXIT:
    
    return retcode;
}

int32_t rns_mysql_connect(rns_mysql_t* mysql, rns_mysql_cb_t* cb)
{
    int32_t retcode = 0;

    mysql->status = mysql_real_connect_start(&mysql->mysql, mysql->mysql, (char_t*)mysql->ip, (char_t*)mysql->user, (char_t*)mysql->passwd, (char_t*)mysql->db, mysql->port, (char_t*)mysql->unix_socket, mysql->client_flags);
    if(mysql->mysql == NULL)
    {
        retcode = -5;
        goto ERR_EXIT;
    }

    mysql->state = RNS_MYSQL_STATE_CONNECT;
    if(!mysql->status)
    {
        rns_mysql_connect_cont(mysql);
    }

    rns_memcpy(&mysql->ccb, cb, sizeof(rns_mysql_cb_t));
    
    return 0;

ERR_EXIT:
    
    return retcode;
}

void rns_mysql_destroy(rns_mysql_t** mysql)
{
    if(*mysql == NULL)
    {
        return;
    }

    if((*mysql)->user != NULL)
    {
        rns_free((*mysql)->user);
    }
    if((*mysql)->passwd != NULL)
    {
        rns_free((*mysql)->passwd);
    }
    if((*mysql)->db != NULL)
    {
        rns_free((*mysql)->db);
    }
    if((*mysql)->ip != NULL)
    {
        rns_free((*mysql)->ip);
    }

    mysql_close((*mysql)->mysql);

    if((*mysql)->sql != NULL)
    {
        rns_free((*mysql)->sql);
    }
    
    rns_free(*mysql);
    *mysql = NULL;

    return;
}

int32_t rns_mysql_query(rns_mysql_t* mysql, uchar_t* sql, uint32_t size, rns_mysql_cb_t* cb)
{
    int32_t ret = 0;

    if(cb != NULL)
    {
        rns_memcpy(&mysql->qcb, cb, sizeof(rns_mysql_cb_t));
    }
    else
    {
        rns_memset(&mysql->qcb, sizeof(rns_mysql_cb_t));
    }

    mysql->rownum = 0;
    
    mysql->status = mysql_real_query_start(&ret, mysql->mysql, (char_t*)sql, size);
    if(ret < 0)
    {
        return -1;
    }

    mysql->sql = rns_dup(sql);
    mysql->state = RNS_MYSQL_STATE_QUERY;
    
    if(!mysql->status)
    {
        mysql->state = RNS_MYSQL_STATE_INIT;
        mysql->res = mysql_use_result(mysql->mysql);
        if(mysql->res)
        {
            rns_mysql_fetch(mysql);
            
        }
        else
        {
            rns_mysql_free(mysql);
        }
    }
    return 0;
}

void rns_mysql_fetch(rns_mysql_t* mysql)
{
BEGIN:
    
    mysql->status = mysql_fetch_row_start(&mysql->row, mysql->res);
    mysql->state = RNS_MYSQL_STATE_FETCH;
    if(!mysql->status)
    {
        if(mysql->row)
        {
            ++mysql->rownum;
            if(mysql->qcb.work != NULL)
            {
                mysql->qcb.work(mysql, mysql->qcb.data);
            }
            goto BEGIN;
        }
        else
        {

            rns_mysql_free(mysql);
        }
    }
    return;
}

void rns_mysql_free(rns_mysql_t* mysql)
{
    mysql->state = RNS_MYSQL_STATE_FREE;
    mysql->status = mysql_free_result_start(mysql->res);
    if(!mysql->status)
    {
        mysql->state = RNS_MYSQL_STATE_INIT;
        if(mysql->qcb.done != NULL)
        {
            mysql->qcb.done(mysql, mysql->qcb.data);
        }
    }
    return;
}

//----------------------------------------------------------------------

int32_t rns_mysql_sock(rns_mysql_t* mysql)
{
    return mysql_get_socket(mysql->mysql);
}


int32_t rns_mysql_errno(rns_mysql_t* mysql)
{
    return mysql_errno(mysql->mysql);
}

uchar_t* rns_mysql_errstr(rns_mysql_t* mysql)
{
    return (uchar_t*)mysql_error(mysql->mysql);
}
