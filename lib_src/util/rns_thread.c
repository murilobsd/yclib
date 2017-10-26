#include "rns_thread.h"
#include "rns_memory.h"
#include <stdio.h>
#include <unistd.h>

rns_thread_t* rns_thread_create(int32_t (*func)(void*),  void* data)
{
    rns_thread_t* rns_thread = NULL;
    rns_thread = (rns_thread_t*)rns_malloc(sizeof(rns_thread_t));
    if(rns_thread == NULL)
    {
        return NULL;
    }
    
    rns_thread->semctl = rns_sem_init();
    if(rns_thread->semctl == NULL)
    {
        rns_free(rns_thread);
        return NULL;
    }
    rns_thread->semend = rns_sem_init();
    if(rns_thread->semend == NULL)
    {
        rns_sem_destroy(&rns_thread->semctl);
        rns_free(rns_thread);
        return NULL;
    }
    
    rns_thread->func = func;
    rns_thread->data = data;
	
    return rns_thread;
}

void rns_thread_destroy(rns_thread_t** thread)
{
	rns_thread_stop(*thread);
	
	rns_sem_destroy(&(*thread)->semctl);
	rns_sem_destroy(&(*thread)->semend);
	rns_free(*thread);
        *thread = NULL;

        return;
}

void* rns_thread_server(void* data)
{
	rns_thread_t* thread = (rns_thread_t*)data;
	struct timespec timeout;                                                 
	timeout.tv_sec = time(NULL) + 1;                      
	timeout.tv_nsec = 0;
	while(rns_sem_timedwait(thread->semctl, &timeout))
	{
            timeout.tv_sec = thread->func(thread->data);
	}
        
	int32_t ret = rns_sem_post(thread->semend);
        if(ret < 0)
        {
            return NULL;
        }

        return NULL;
}

int32_t rns_thread_start(rns_thread_t* thread)
{
    int32_t ret = pthread_create(&thread->tid, NULL, rns_thread_server, (void*)thread);
    if(ret < 0)
    {
        return -1;
    }
    
    return 0;
}

void rns_thread_stop(rns_thread_t* thread)
{
    rns_sem_post(thread->semctl);
	
    rns_sem_wait(thread->semend);

    return;
}

rns_thread_key_t* rns_thread_key_create()
{
    rns_thread_key_t* k = (rns_thread_key_t*)rns_malloc(sizeof(rns_thread_key_t));
    if(k == NULL)
    {
        return NULL;
    }
    
    int32_t ret = pthread_key_create(&k->key, NULL);
    if(ret < 0)
    {
        rns_free(k);
        return NULL;
    }

    return k;
}

void* rns_thread_key_get(rns_thread_key_t* key)
{
    return pthread_getspecific(key->key);
}

int32_t rns_thread_key_set(rns_thread_key_t* key, void* data)
{
    int32_t ret = pthread_setspecific(key->key, data);
    if(ret < 0)
    {
        return -1;
    }
    return 0;
}

