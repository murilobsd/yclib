#include "rns_pipe.h"
#include "rns_memory.h"

#include <unistd.h>
#include <fcntl.h>

void pipecb(rns_epoll_t* ep, void* data)
{
    if(data == NULL)
    {
        return;
    }
    rns_pipe_t* pp = (rns_pipe_t*)data;

    if(pp->cb.func != NULL)
    {
        pp->cb.func(pp->cb.data);
    }

    return;
}

rns_pipe_t* rns_pipe_open()
{
    rns_pipe_t* p = (rns_pipe_t*)rns_malloc(sizeof(rns_pipe_t));
    if(p == NULL)
    {
        return NULL;
    }
    
    int32_t ret = pipe(p->fd);
    if(ret < 0)
    {
        rns_free(p);
        return NULL;
    }

    int32_t flags = fcntl(p->fd[0], F_GETFL, 0);
    ret = fcntl(p->fd[0], F_SETFL, flags | O_NONBLOCK);
    if(ret < 0)
    {
        rns_free(p);
        return NULL;
    }
    flags = fcntl(p->fd[1], F_GETFL, 0);
    ret = fcntl(p->fd[1], F_SETFL, flags | O_NONBLOCK);
    if(ret < 0)
    {
        rns_free(p);
        return NULL;
    }

    p->ecb.read = pipecb;
    
    return p;
}

int32_t rns_pipe_read(rns_pipe_t* p, uchar_t* buf, uint32_t size)
{
    return read(p->fd[0], buf, size);
}

int32_t rns_pipe_write(rns_pipe_t* p, uchar_t* buf, uint32_t size)
{
    return write(p->fd[1], buf, size);
}

void rns_pipe_close(rns_pipe_t** p)
{
    if(*p == NULL)
    {
        return;
    }
    close((*p)->fd[0]);
    close((*p)->fd[1]);

    rns_free(*p);
    *p = NULL;

    return;
}

int32_t rns_pipe_rfd(rns_pipe_t* p)
{
    return p->fd[0];
}
