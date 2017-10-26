#include "rns_signal.h"
#include "rns_memory.h"
#include <sys/signalfd.h>
#include <errno.h>
#include <unistd.h>

void sgncb(rns_epoll_t* ep, void* data)
{
    if(data == NULL)
    {
        return;
    }
    rns_sgn_t* sgn = (rns_sgn_t*)data;

    struct signalfd_siginfo fdsi;  

    while(true)
    {
        int32_t ret = read(sgn->fd, &fdsi, sizeof(struct signalfd_siginfo));
        if(ret < 0 && errno == EAGAIN)
        {
            break;
        }
        else if(ret != sizeof(struct signalfd_siginfo))
        {
            continue;
        }

        rns_cb_t* cb = (rns_cb_t*)fdsi.ssi_ptr;
        if(cb != NULL)
        {
            
            if(cb->func != NULL)
            {
                cb->func(cb->data);
            }
            rns_free(cb);
        }
    }
    
    return;
}

rns_sgn_t* rns_sgn_create(uint32_t sid)
{
    rns_sgn_t* sgn = (rns_sgn_t*)rns_malloc(sizeof(rns_sgn_t));
    if(sgn == NULL)
    {
        return NULL;
    }
    
    sigemptyset(&sgn->mask);  
    int32_t ret = sigaddset(&sgn->mask, sid);
    if(ret < 0)
    {
        rns_free(sgn);
        return NULL;
    }
    sigprocmask(SIG_BLOCK, &sgn->mask, NULL);

    sgn->fd = signalfd(-1, &sgn->mask, 0);  
    if(sgn->fd < 0)
    {
        rns_free(sgn);
        return NULL;
    }

    sgn->ecb.read = sgncb;
    
    return sgn;
}

void rns_sgn_destroy(rns_sgn_t** sgn)
{
    if(*sgn == NULL)
    {
        return;
    }

    close((*sgn)->fd);
    rns_free(*sgn);
    *sgn = NULL;
    
    return;
}
