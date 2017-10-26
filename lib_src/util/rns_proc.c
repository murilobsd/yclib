#include "rns_proc.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int32_t rns_proc_service()
{
    struct rlimit rl;
    pid_t pid;
    uint32_t i;
    int32_t ret = umask(0);
    if(ret < 0)
    {
        return -1;
    }
    
    ret = getrlimit(RLIMIT_NOFILE, &rl);
    if(ret < 0)
    {
        return -1;
    }

    if((pid = fork()) < 0)
    {
        return -2;
    }
    else if(pid != 0)
    {
        return 0;
    }
    
    ret = setsid();
    if(ret < 0)
    {
        return -3;
    }
    
    ret = chdir("/");
    if(ret < 0)
    {
        return -4;
    }
    
    if(rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }
    for(i = 0; i < rl.rlim_max; ++i)
    {
        close(i);
    }
    
#ifndef DEBUG    
    int32_t fd0 = open("/dev/null", O_RDWR);
    if(fd0 < 0)
    {
        return -5;
    }
    int32_t fd1 = dup(0);
    if(fd1 < 0)
    {
        return -6;
    }
    int32_t fd2 = dup(0);
    if(fd2 < 0)
    {
        return -7;
    }
#endif

    return 0;
}

int32_t rns_proc_daemon(uchar_t* filename)
{
    pid_t pid;
    if((pid = fork()) < 0)
    {
        return 0;
    }
    else if(pid == 0)
    {
        while(true)
        {
            execv((char_t*)filename, NULL);
        }
    }
    else
    {
        return pid;
    }
}
