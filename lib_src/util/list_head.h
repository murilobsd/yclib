#ifndef _LIST_HEAD_H
#define _LIST_HEAD_H

#include "comm.h"


typedef struct list_head {
         struct list_head *next, *prev;
}list_head_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

static inline void INIT_LIST_HEAD(struct list_head *list)
{
        list->next = list;
        list->prev = list;
}

static inline void __rns_list_add(struct list_head *lnew, struct list_head *prev,struct list_head *next)
{
        next->prev = lnew;
        lnew->next = next;
        lnew->prev = prev;
        prev->next = lnew;
}


static inline void rns_list_add(struct list_head *lnew, struct list_head *head)
{
        __rns_list_add(lnew, head, head->next);
}
 

static inline void rns_list_add_first(struct list_head *lnew, struct list_head *head)
{
    __rns_list_add(lnew, head->prev, head);
}



static inline void __rns_list_del(struct list_head *prev, struct list_head *next)
{
        next->prev = prev;
        prev->next = next;
}
 
static inline void rns_list_del(struct list_head *entry)
{
        __rns_list_del(entry->prev, entry->next);
        entry->next = NULL;
        entry->prev = NULL;
}

static inline void rns_list_del_safe(struct list_head* entry)
{
    if(entry->prev == 0 && entry->next == 0)
    {
        return;
    }

    rns_list_del(entry);
}

static inline void rns_list_add_safe(struct list_head *lnew, struct list_head *head)
{
    rns_list_del_safe(lnew);
    rns_list_add(lnew, head);
}


#define prefetch(x) __builtin_prefetch(x)

// the head is empty
#define rns_list_empty(list)                                       \
    (((list)->prev == (list)) && ((list)->next == (list))) ? 1 : 0

// whether node in a list
#define rns_list_free(list) \
    (((list)->prev == NULL) && ((list)->next == NULL)) ? 1 : 0

#define rns_rns_list_for_each_r(pos, head) \
         for (pos = (head)->next; pos != (head); \
                 pos = pos->next)

#define rns_list_for_each(pos, head)            \
    for (pos = (head)->prev; pos != (head);     \
         pos = pos->prev)

#define rns_list_for_each_safe(pos, n, head)                    \
    for (pos = (head)->prev, n = pos->prev; pos != (head); \
         pos = n, n = pos->prev)  
				 
#define rns_list_entry(ptr, type, member) \
    container_of(ptr, type, member)
		 
#define rns_list_first(head, type, member) \
    ((head)->prev == (head) ? NULL : container_of((head)->prev, type, member))
			
#define rns_list_last(head, type, member) \
    ((head)->next == (head) ? NULL : container_of((head)->next, type, member))

#define rns_list_prev(ptr, head, type, member)                               \
    ((ptr)->next == (head) ? NULL : container_of((ptr)->next, type, member))

#define rns_list_next(ptr, head, type, member)                               \
    ((ptr)->prev == (head) ? NULL : container_of((ptr)->prev, type, member))

typedef struct rns_list_s
{
    list_head_t list;
    uchar_t* data;
    uint32_t size;
}rns_list_t;

#endif
