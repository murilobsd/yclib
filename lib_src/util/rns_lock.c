#include "rns_lock.h"
#include "rns_memory.h"
#include <semaphore.h>

extern int sem_timedwait(sem_t *, const struct timespec *);
int32_t rns_spin_lock()
{
    return 0;
}

int32_t rns_spin_unlock()
{
    return 0;
}

void rns_cas_lock(uint32_t *ptr)
{
    while(!__sync_val_compare_and_swap(ptr, 0, 1));
    return;
}

void rns_cas_unlock(uint32_t *ptr)
{
    while(!__sync_val_compare_and_swap(ptr, 1, 0));
    return;
}

rns_sem_t* rns_sem_init()
{
    rns_sem_t* sem = (rns_sem_t*)rns_malloc(sizeof(rns_sem_t));
    if(sem == NULL)
    {
        return NULL;
    }
    
    int32_t ret = sem_init(&sem->sem, 0, 0);
    if(ret < 0)
    {
        rns_free(sem);
        return NULL;
    }
    
    return sem;
}

void rns_sem_destroy(rns_sem_t** sem)
{
    sem_destroy(&(*sem)->sem);
    rns_free(*sem);
    *sem = NULL;
}

int32_t rns_sem_wait(rns_sem_t* sem)
{
    return sem_wait(&sem->sem);
}

int32_t rns_sem_timedwait(rns_sem_t* sem, const struct timespec * timeout)
{
    return sem_timedwait(&sem->sem, timeout);
}

int32_t rns_sem_post(rns_sem_t* sem)
{
    return sem_post(&sem->sem);
}


int32_t rns_mutex_lock(rns_mutex_t* mutex)
{
    int32_t ret = pthread_mutex_lock(&mutex->mutex);	
    return ret;
}

int32_t rns_mutex_unlock(rns_mutex_t* mutex)
{
    int32_t ret = pthread_mutex_unlock(&mutex->mutex);
    return ret;
}

int32_t rns_cond_wait(rns_cond_t* rns_cond)
{
    rns_mutex_lock(&rns_cond->lock);
    pthread_cond_wait(&rns_cond->cond, &rns_cond->lock.mutex);
    rns_mutex_unlock(&rns_cond->lock);

    return 0;
}

int32_t rns_cond_signal(rns_cond_t* rns_cond)
{
    rns_mutex_lock(&rns_cond->lock);
    pthread_cond_signal(&rns_cond->cond);
    rns_mutex_unlock(&rns_cond->lock);

    return 0;
}
