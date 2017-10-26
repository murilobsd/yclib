#include "rns_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>

uchar_t* levelstr[] = {(uchar_t*)"DEBUG", (uchar_t*)"INFO", (uchar_t*)"WARN", (uchar_t*)"ERROR"};

void do_log(rns_epoll_t* ep, void* data)
{
    if(data == NULL)
    {
        return;
    }
    rns_log_t* log = (rns_log_t*)data;

    uint32_t size = 0;

    while(true) 
    {
    
        int32_t ret = rns_pipe_read(log->pipe, (uchar_t*)&size, sizeof(uint32_t));
        if(ret <= 0)
        {
            return;
        }

        size = size < log->size ? size : log->size;
        ret = rns_pipe_read(log->pipe, log->read_buf, size);
        if(ret <= 0)
        {
            return;
        }
    
        ret = rns_file_write(log->fp, log->read_buf, size);
        if(ret < 0)
        {
            return;
        }
    }
}

rns_log_t* rns_log_init(uchar_t* file, uint32_t level, uchar_t* module)
{
    if(file == NULL || module == NULL)
    {
        goto ERR_EXIT;
    }
    
    rns_log_t* log = (rns_log_t*)rns_malloc(sizeof(rns_log_t));
    if(log == NULL)
    {
       goto ERR_EXIT;
    }

    log->pipe = rns_pipe_open();
    if(log->pipe == NULL)
    {
        
        goto ERR_EXIT;
    }

    log->fp = rns_file_open(file, RNS_FILE_OPEN | RNS_FILE_RW);
    if(log->fp == NULL)
    {
       
        goto ERR_EXIT;
    }

    log->size = 10240;
    log->read_buf = rns_malloc(log->size);
    if(log->read_buf == NULL)
    {
        goto ERR_EXIT;
    }

    log->write_buf = rns_malloc(log->size);
    if(log->write_buf == NULL)
    {
        goto ERR_EXIT;
    }

    log->level = level;
    rns_strncpy(log->module, module, sizeof(log->module) - 1);

    log->ecb.read = do_log;
    log->ecb.write = NULL;
    
    return log;

ERR_EXIT:
    if(log->read_buf != NULL)
    {
        rns_free(log->read_buf);
    }
    
    if(log->write_buf != NULL)
    {
        rns_free(log->write_buf);
    }
    rns_file_close(&log->fp);
    rns_pipe_close(&log->pipe);
    
    if(log != NULL)
    {
        rns_free(log);
    }

    return NULL;
}

void rns_log_destroy(rns_log_t** log)
{
    if(*log == NULL)
    {
        return;
    }

    if((*log)->read_buf != NULL)
    {
        rns_free((*log)->read_buf);
    }
    
    if((*log)->write_buf != NULL)
    {
        rns_free((*log)->write_buf);
    }
    rns_file_close(&(*log)->fp);
    rns_pipe_close(&(*log)->pipe);
    
    rns_free(*log);
    *log = NULL;
    return;
}

void rns_log(rns_log_t* log, uint32_t level, char_t* file, const char_t* func, uint32_t line, char_t* format, ...)
{
    if(log == NULL)
    {
        return;
    }
    if(level < log->level)
    {
        return;
    }
    uint32_t total = 0;
    va_list list;
    va_start(list, format);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm t;
    localtime_r(&tv.tv_sec, &t);
    
    uint32_t size = 0;

    if(level > RNS_LOG_LEVEL_ERROR)
    {
        return;
    }
    
    size = snprintf((char_t*)(log->write_buf), log->size, "[%s][%s][%d-%d-%d %d:%d:%d:%llu][%s][%s:%d]:", log->module, levelstr[level], t.tm_year + 1990, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec, func, file, line);
    
    total = size + vsnprintf((char_t*)log->write_buf + size, log->size - size, format, list);
    va_end(list);

    total += snprintf((char_t*)log->write_buf + total, log->size - total, "\n");
    log->write_buf[total] = 0;

    int32_t ret = rns_pipe_write(log->pipe, (uchar_t*)&total, sizeof(total));
    if(ret < 0)
    {
        return;
    }
    ret = rns_pipe_write(log->pipe, log->write_buf, total);
    if(ret <= 0)
    {
        return;
    }
    return;
}
