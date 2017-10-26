#include "rns_epoll.h"
#include "rns_memory.h"
#include "rns_os.h"
#include <stdio.h>
#define EPOLL_DELETE(epfd, fd) epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL)

rns_epoll_t* rns_epoll_init()
{
    rns_epoll_t* ep = (rns_epoll_t*)rns_malloc(sizeof(rns_epoll_t));
    if(ep == NULL)
    {
        return NULL;
    }
    
    ep->epfd = epoll_create(100);
    if(ep->epfd < 0)
    {
        rns_free(ep);
        return NULL;
    }

    rbt_init(&ep->t, RBT_TYPE_INT);
    rbt_init(&ep->i, RBT_TYPE_INT);
    
    return ep;
}

void rns_epoll_destroy(rns_epoll_t** ep)
{
    if(*ep == NULL)
    {
        return;
    }
    
    rns_free(*ep);
    *ep = NULL;
    return;
}
#define RNS_EVENT_NUMBER 100
void rns_epoll_wait(rns_epoll_t* ep)
{
    int32_t i = 0;
    int32_t nfds = 0;
    
    struct epoll_event events[RNS_EVENT_NUMBER];
    rns_memset(events, sizeof(struct epoll_event) * RNS_EVENT_NUMBER);
    uint64_t t = rns_time_ms();

    
    while(!ep->exit)
    {
        uint32_t expire = 500;
        rbt_node_t* node = rbt_first(&ep->t);
        if(node != NULL)
        {
            if(node->key.idx <= t)
            {
                expire = 1;
            }
            else
            {
                expire = node->key.idx - t;
            }
            
        }
        nfds = epoll_wait(ep->epfd, events, RNS_EVENT_NUMBER, expire);
        t = rns_time_ms();
        if(nfds == 0)
        {
            rbt_node_t* n = NULL;
            while((n = rbt_first(&ep->t)) != NULL && n->key.idx <= t)
            {
                rns_cbs_t* cbs = rbt_entry(n, rns_cbs_t, t);
                if(cbs->cb.func)
                {
                    cbs->cb.func(cbs->cb.data);
                }
            }
        }
        else
        {
            for(i = 0; i < nfds; ++i)
            {
                rns_epoll_cb_t* ecb = (rns_epoll_cb_t*)events[i].data.ptr;
                if(events[i].events & EPOLLIN)
                {
                    if( ecb->read != NULL)
                    {
                        ecb->read(ep, events[i].data.ptr);
                    }
                }
                else if(events[i].events & EPOLLOUT)
                {
                    if( ecb->write != NULL)
                    {
                        ecb->write(ep, events[i].data.ptr);
                    }

                }
            }
        }
    }
    
}

int32_t rns_epoll_add(rns_epoll_t* ep, int32_t fd, void* data, int32_t flag)
{
    if(ep == NULL || data == NULL)
    {
        return -1;
    }
    struct epoll_event event;
    
    event.data.ptr = data;
    event.events = flag | EPOLLET;

    int32_t ret = epoll_ctl(ep->epfd, EPOLL_CTL_ADD, fd, &event);
    if(ret < 0)
    {
        return -2;
    }

    return 0;
}

void rns_epoll_delete(rns_epoll_t* ep, int32_t fd)
{
    if(ep == NULL)
    {
        return;
    }
    
    EPOLL_DELETE(ep->epfd, fd);
    return;
}

void* rns_timer_add(rns_epoll_t* ep, uint32_t delay, rns_cb_t* cb)
{
    if(ep == NULL || cb == NULL || delay == 0)
    {
        return NULL;
    }

    rns_cbs_t* cbs = (rns_cbs_t*)rns_malloc(sizeof(rns_cbs_t));
    if(cbs == NULL)
    {
        return NULL;
    }
    uint64_t curt = rns_time_ms();
    
    rbt_set_key_int(&cbs->t, curt + delay);
    rbt_insert(&ep->t, &cbs->t);

    rbt_set_key_int(&cbs->i, (uint64_t)cbs);
    rbt_insert(&ep->i, &cbs->i);

    rns_memcpy(&cbs->cb, cb, sizeof(rns_cb_t));
    
    return cbs;
}

void rns_timer_delete(rns_epoll_t* ep, void* id)
{
    rbt_node_t* node = rbt_search_int(&ep->i, (uint64_t)id);
    if(node == NULL)
    {
        return;
    }
    rns_cbs_t* cbs = rbt_entry(node, rns_cbs_t, i);

    rbt_delete(&ep->i, &cbs->i);
    rbt_delete(&ep->t, &cbs->t);

    rns_free(cbs);
    
    return;
}

void rns_timer_delay(rns_epoll_t* ep, void* id, uint32_t delay)
{
    if(delay == 0)
    {
        return;
    }

    rbt_node_t* node = rbt_search_int(&ep->i, (uint64_t)id);
    if(node == NULL)
    {
        return;
    }
    rns_cbs_t* cbs = rbt_entry(node, rns_cbs_t, i);
    
    uint64_t curt = rns_time_ms();
    rbt_delete(&ep->t, &cbs->t);
    rbt_set_key_int(&cbs->t, curt + delay);
    rbt_insert(&ep->t, &cbs->t);
    
    return;
}
