#include "rns_tpool.h"
#include "rns_thread.h"
#include "rns_memory.h"

int do_task(void* data)
{
	rns_tpool_thread_t* tpool_thread = (rns_tpool_thread_t*)data;
	
	rns_sem_wait(tpool_thread->tpool->task_sem);
	
	rns_cas_lock(&tpool_thread->tpool->task_lock);
	list_head_t* p = &tpool_thread->tpool->task_list;
	rns_task_t* task = rns_list_first(p, rns_task_t, list);
	rns_cas_unlock(&tpool_thread->tpool->task_lock);
	if(task != NULL)
	{
		task->func(task->data);
	}
        
	return 0;
}

rns_tpool_t* rns_tpool_init(uint32_t thread_num)
{
    int32_t ret = 0; 
    rns_tpool_t* tpool = (rns_tpool_t*)rns_malloc(sizeof(rns_tpool_t));
    if(tpool == NULL)
    {
        return NULL;
    }
    INIT_LIST_HEAD(&tpool->thread_list);
    INIT_LIST_HEAD(&tpool->task_list);
    tpool->task_sem = rns_sem_init();
	
    uint32_t i = 0;
    for(i = 0; i < thread_num; ++i)
    {
        rns_tpool_thread_t* tpool_thread = (rns_tpool_thread_t*)rns_malloc(sizeof(rns_tpool_thread_t));
        if(tpool_thread == NULL)
        {
            continue;
        }
        tpool_thread->tpool = tpool;
        tpool_thread->thread = rns_thread_create(do_task, tpool_thread);
        rns_list_add(&tpool_thread->list, &tpool->thread_list);
        ret = rns_thread_start(tpool_thread->thread);
        if(ret < 0)
        {
            continue;
        }
    }
	
    return tpool;
}


int32_t rns_task_add(rns_tpool_t* tpool, void (*func)(void*), void* data)
{
    rns_task_t* task = (rns_task_t*)rns_malloc(sizeof(rns_task_t));
    if(task == NULL)
    {
        return -1;
    }
    task->func = func;
    task->data = data;
	
    rns_cas_lock(&tpool->task_lock);
    rns_list_add(&task->list, &tpool->task_list);
    rns_cas_unlock(&tpool->task_lock);
	
    rns_sem_post(tpool->task_sem);

    return 0;
}

void rns_tpool_destroy(rns_tpool_t** tpool)
{
    if(*tpool == NULL)
    {
        return;
    }
    rns_free(*tpool);
    *tpool = NULL;
    return;
}
