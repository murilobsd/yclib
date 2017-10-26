#ifndef _RNS_TPOOL_H
#define _RNS_TPOOL_H

#include "rns_thread.h"
#include "list_head.h"
#include "comm.h"

typedef struct rns_tpool_s
{
	list_head_t thread_list;
	list_head_t task_list;
	uint32_t thread_lock;
	uint32_t task_lock;
	rns_sem_t* task_sem;
	
}rns_tpool_t;

typedef struct rns_tpool_thread_s
{
	list_head_t list;
	rns_thread_t* thread;
	rns_tpool_t* tpool;
}rns_tpool_thread_t;

typedef struct rns_task_s
{
	list_head_t list;
	void (*func)(void*);
	void* data;
}rns_task_t;


rns_tpool_t* rns_tpool_init(uint32_t thread_num);
int32_t rns_task_add(rns_tpool_t* tpool, void (*func)(void*), void* data);
void rns_tpool_destroy(rns_tpool_t** tpool);


#endif
