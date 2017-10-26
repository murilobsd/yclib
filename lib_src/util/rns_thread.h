#ifndef _RNS_THREAD_H
#define _RNS_THREAD_H

#include <pthread.h>
#include "rns_lock.h"
#include "list_head.h"
typedef struct rns_thread_s
{
	list_head_t list;
	pthread_t tid;
	rns_sem_t*   semctl;
	rns_sem_t*   semend;
	int        (*func)(void*);
	void*        data;
}rns_thread_t;

typedef struct rns_thread_key_s
{
    pthread_key_t key;
}rns_thread_key_t;

rns_thread_t* rns_thread_create(int (*func)(void*),  void* data);
void rns_thread_destroy(rns_thread_t** thread);
int32_t rns_thread_start(rns_thread_t* thread);
void rns_thread_stop(rns_thread_t* thread);
rns_thread_key_t* rns_thread_key_create();
void* rns_thread_key_get(rns_thread_key_t* key);
int32_t rns_thread_key_set(rns_thread_key_t* key, void* data);


#endif
