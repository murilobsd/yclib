/* #include "rns_timer.h" */
/* #include "rns_memory.h" */
/* #include <unistd.h> */

/* void timercb(rns_epoll_t* ep, void* data) */
/* { */
/*     rns_timer_t* timer = (rns_timer_t*)data; */
/*     if(timer->cb.func != NULL) */
/*     { */
/*         timer->cb.func(timer->cb.data); */
/*     } */
/*     return; */
/* } */

/* rns_timer_t* rns_timer_create() */
/* { */
/*     rns_timer_t* timer = (rns_timer_t*)rns_malloc(sizeof(rns_timer_t)); */
/*     if(timer == NULL) */
/*     { */
/*         return NULL; */
/*     } */

/*     timer->ecb.read = timercb; */
/*     timer->ecb.write = NULL; */
    
/*     timer->timerfd = timerfd_create(CLOCK_MONOTONIC, 0); */
/*     if(timer->timerfd < 0) */
/*     { */
/*         rns_free(timer); */
/*         return NULL; */
/*     } */
    
/*     return timer; */
/* } */

/* void rns_timer_destroy(rns_timer_t** timer) */
/* { */
/*     if(*timer == NULL) */
/*     { */
/*         return; */
/*     } */
    
/*     close((*timer)->timerfd); */
/*     rns_free(*timer); */
/*     *timer = NULL; */
/*     return; */
/* } */

/* int32_t rns_timer_set(rns_timer_t* timer, uint32_t delay, rns_cb_t* cb) */
/* { */
/*     if(timer == NULL || cb == NULL) */
/*     { */
/*         return -1; */
/*     } */
    
/*     rns_memcpy(&timer->cb, cb, sizeof(rns_cb_t)); */

/*     timer->t.it_value.tv_sec = delay / 1000; */
/*     timer->t.it_value.tv_nsec = (delay % 1000) * 1000000; */
        
/*     int32_t ret = timerfd_settime(timer->timerfd, 0, &timer->t, NULL); */
/*     if(ret < 0) */
/*     { */
/*         return -2; */
/*     } */
        
/*     return 0; */
    
/* } */

/* int32_t rns_timer_get(rns_timer_t* timer) */
/* { */
/*     if(timer == NULL) */
/*     { */
/*         return -1; */
/*     } */

/*     struct itimerspec t; */
    
    
/*     int32_t ret = timerfd_gettime(timer->timerfd, &t); */
/*     if(ret < 0) */
/*     { */
/*         return -2; */
/*     } */
    
    
/*     return t.it_value.tv_sec * 1000 + t.it_value.tv_nsec; */
    
/* } */

/* int32_t rns_timer_setdelay(rns_timer_t* timer, uint32_t delay) */
/* { */
/*     if(timer == NULL) */
/*     { */
/*         return -1; */
/*     } */
    
/*     timer->t.it_value.tv_sec = delay / 1000; */
/*     timer->t.it_value.tv_nsec = (delay % 1000) * 1000000; */
    
/*     int32_t ret = timerfd_settime(timer->timerfd, 0, &timer->t, NULL); */
/*     if(ret < 0) */
/*     { */
/*         return -2; */
/*     } */
    
/*     return 0; */
    
/* } */

/* void rns_timer_cancel(rns_timer_t* timer) */
/* { */
/*     if(timer == NULL) */
/*     { */
/*         return; */
/*     } */

/*     timer->t.it_value.tv_sec = 0; */
/*     timer->t.it_value.tv_nsec = 0; */

/*     int32_t ret = timerfd_settime(timer->timerfd, 0, &timer->t, NULL); */
/*     if(ret < 0) */
/*     { */
/*         return; */
/*     } */
    
/*     return; */
/* } */
