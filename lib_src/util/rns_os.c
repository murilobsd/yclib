#include "rns_os.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/select.h>

#include <sys/resource.h>
#include "rns_memory.h"
#include "list_head.h"
#include <sched.h>
#include <pthread.h>


uchar_t* rns_time_time2str(time_t t)
{
    return (uchar_t*)ctime(&t);
}

void rns_time_time2tm(time_t* t, struct tm* tm)
{

    return;
}

void rns_time_time2tml(time_t* t, struct tm* tm)
{
    localtime_r(t, tm);
    return;
}

time_t rns_time_tm2time(struct tm* tm)
{
    time_t t = 0;;
    return t;
}

uchar_t* rns_time_tm2str(struct tm* tm, uchar_t* buf, uint32_t size, uchar_t* format)
{
    strftime((char_t*)buf, size, (char_t*)format, tm);
    return buf;
}

time_t rns_time_str2time(uchar_t* buf, uint32_t size, uchar_t* format)
{
    time_t t = 0;
    return t;
}

void rns_time_str2tm(uchar_t* buf, uint32_t size, uchar_t* format, struct tm* tm)
{
    return;
}

void rns_sleep_us(uint32_t us)
{
	struct timeval tv;
	tv.tv_sec = us / 1000000;
	tv.tv_usec = us % 1000000;
	select(0, NULL, NULL, NULL, &tv);

        return;
}

uint64_t rns_time_utc()
{
	return time(NULL);
}

uint64_t rns_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)((uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void rns_time_us(struct timeval *tv)
{
	gettimeofday(tv, NULL);
}

int32_t rns_os_mac(uchar_t* hdware, uchar_t* buf, uint32_t size)
{
    if(size < 6)
    {
        return -1;
    }
    
    int32_t sock = 0;
    struct ifreq ifreq;
    rns_memset(&ifreq, sizeof(ifreq));
    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
    { 
        return -2; 
    } 
    rns_strncpy(ifreq.ifr_name, hdware, sizeof(ifreq.ifr_name));

    int32_t ret = ioctl(sock,SIOCGIFHWADDR,&ifreq);
    if(ret < 0) 
    { 
        return -3; 
    }

    rns_memcpy(buf, ifreq.ifr_hwaddr.sa_data, 6);
    return   0; 
}

typedef struct rns_cpu_s
{
    list_head_t cpu_list;
    uint32_t index; // cpu的编号
    uint32_t load;  // 任务的数量
    list_head_t task_list;
}rns_cpu_t;

typedef struct rns_cpus_t
{
    list_head_t cpu_list;
}rns_cpus_t;

rns_cpus_t* rns_cpus()
{
    return NULL;
}

int32_t rns_process_bind_cpu(rns_cpus_t* cpus, uint32_t pid)
{
    cpu_set_t mask;
    rns_cpu_t* cpu = rns_list_first(&cpus->cpu_list, rns_cpu_t, cpu_list);
    if(cpu == NULL)
    {
        return -1;
    }
    
    CPU_ZERO(&mask);
    CPU_SET(cpu->index, &mask);
    
    rns_list_del(&cpu->cpu_list);
    rns_list_add(&cpu->cpu_list, &cpus->cpu_list);
    
    return sched_setaffinity(pid, sizeof(mask), &mask);
    
}

int32_t rns_thread_bind_cpu(rns_cpus_t* cpus, uint32_t tid)
{
    cpu_set_t mask;
    rns_cpu_t* cpu = rns_list_first(&cpus->cpu_list, rns_cpu_t, cpu_list);
    if(cpu == NULL)
    {
        return -1;
    }
    
    CPU_ZERO(&mask);
    CPU_SET(cpu->index, &mask);
    
    rns_list_del(&cpu->cpu_list);
    rns_list_add(&cpu->cpu_list, &cpus->cpu_list);
    
    return pthread_setaffinity_np(tid, sizeof(mask), &mask);
}

/*
RLIMIT_AS //进程的最大虚内存空间，字节为单位。
RLIMIT_CORE //内核转存文件的最大长度。
RLIMIT_CPU //最大允许的CPU使用时间，秒为单位。当进程达到软限制，内核将给其发送SIGXCPU信号，这一信号的默认行为是终止进程的执行。然而，可以捕捉信号，处理句柄可将控制返回给主程序。如果进程继续耗费CPU时间，核心会以每秒一次的频率给其发送SIGXCPU信号，直到达到硬限制，那时将给进程发送 SIGKILL信号终止其执行。
RLIMIT_DATA //进程数据段的最大值。
RLIMIT_FSIZE //进程可建立的文件的最大长度。如果进程试图超出这一限制时，核心会给其发送SIGXFSZ信号，默认情况下将终止进程的执行。
RLIMIT_LOCKS //进程可建立的锁和租赁的最大值。
RLIMIT_MEMLOCK //进程可锁定在内存中的最大数据量，字节为单位。
RLIMIT_MSGQUEUE //进程可为POSIX消息队列分配的最大字节数。
RLIMIT_NICE //进程可通过setpriority() 或 nice()调用设置的最大完美值。
RLIMIT_NOFILE //指定比进程可打开的最大文件描述词大一的值，超出此值，将会产生EMFILE错误。
RLIMIT_NPROC //用户可拥有的最大进程数。
RLIMIT_RTPRIO //进程可通过sched_setscheduler 和 sched_setparam设置的最大实时优先级。
RLIMIT_SIGPENDING //用户可拥有的最大挂起信号数。
RLIMIT_STACK //最大的进程堆栈，以字节为单位。

*/

int32_t rns_limit_set(uint32_t type, uint64_t soft, uint64_t hard)
{
    struct rlimit limit = {soft, hard};
    return setrlimit((__rlimit_resource_t)type, &limit);
}

int32_t rns_limit_get(uint32_t type, uint64_t* soft, uint64_t* hard)
{
    struct rlimit limit;
    int32_t ret = getrlimit((__rlimit_resource_t)type, &limit);
    if(ret < 0)
    {
        return -1;
    }
    *soft = limit.rlim_cur;
    *hard = limit.rlim_max;

    return 0;
}
