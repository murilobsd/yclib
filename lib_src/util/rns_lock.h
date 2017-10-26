/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_lock.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-02-18 15:15:26
 * -------------------------
**************************************************************************/

#ifndef _RNS_LOCK_H_
#define _RNS_LOCK_H_

#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>

#include "comm.h"

int32_t rns_spin_lock();
int32_t rns_spin_unlock();

void rns_cas_lock(uint32_t *ptr);
void rns_cas_unlock(uint32_t *ptr);


typedef struct rns_sem_s
{
	sem_t sem;
}rns_sem_t;

rns_sem_t* rns_sem_init();
void rns_sem_destroy(rns_sem_t** sem);
int32_t rns_sem_wait(rns_sem_t* sem);
int32_t rns_sem_timedwait(rns_sem_t* sem, const struct timespec * timeout);
int32_t rns_sem_post(rns_sem_t* sem);


typedef struct rns_mutex_s
{
	pthread_mutex_t mutex;
}rns_mutex_t;
int32_t rns_mutex_lock(rns_mutex_t* mutex);
int32_t rns_mutex_unlock();


typedef struct rns_cond_s
{
	rns_mutex_t	lock;
	pthread_cond_t  cond;
}rns_cond_t;

int32_t rns_cond_wait(rns_cond_t* rns_cond);
int32_t rns_cond_signal(rns_cond_t* rns_cond);

#endif
