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
    uint32_t index; // cpu�ı��
    uint32_t load;  // ���������
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
RLIMIT_AS //���̵�������ڴ�ռ䣬�ֽ�Ϊ��λ��
RLIMIT_CORE //�ں�ת���ļ�����󳤶ȡ�
RLIMIT_CPU //��������CPUʹ��ʱ�䣬��Ϊ��λ�������̴ﵽ�����ƣ��ں˽����䷢��SIGXCPU�źţ���һ�źŵ�Ĭ����Ϊ����ֹ���̵�ִ�С�Ȼ�������Բ�׽�źţ��������ɽ����Ʒ��ظ�������������̼����ķ�CPUʱ�䣬���Ļ���ÿ��һ�ε�Ƶ�ʸ��䷢��SIGXCPU�źţ�ֱ���ﵽӲ���ƣ���ʱ�������̷��� SIGKILL�ź���ֹ��ִ�С�
RLIMIT_DATA //�������ݶε����ֵ��
RLIMIT_FSIZE //���̿ɽ������ļ�����󳤶ȡ����������ͼ������һ����ʱ�����Ļ���䷢��SIGXFSZ�źţ�Ĭ������½���ֹ���̵�ִ�С�
RLIMIT_LOCKS //���̿ɽ������������޵����ֵ��
RLIMIT_MEMLOCK //���̿��������ڴ��е�������������ֽ�Ϊ��λ��
RLIMIT_MSGQUEUE //���̿�ΪPOSIX��Ϣ���з��������ֽ�����
RLIMIT_NICE //���̿�ͨ��setpriority() �� nice()�������õ��������ֵ��
RLIMIT_NOFILE //ָ���Ƚ��̿ɴ򿪵�����ļ������ʴ�һ��ֵ��������ֵ���������EMFILE����
RLIMIT_NPROC //�û���ӵ�е�����������
RLIMIT_RTPRIO //���̿�ͨ��sched_setscheduler �� sched_setparam���õ����ʵʱ���ȼ���
RLIMIT_SIGPENDING //�û���ӵ�е��������ź�����
RLIMIT_STACK //���Ľ��̶�ջ�����ֽ�Ϊ��λ��

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
