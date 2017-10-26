#ifndef _RNS_FILE_H
#define _RNS_FILE_H

#include <sys/types.h>       
#include <sys/stat.h> 
#include <unistd.h>      
#include <fcntl.h> 
#include <sys/vfs.h>
#include <stdint.h>
#include "comm.h"

#include <libaio.h>
#include <sys/eventfd.h>

#include "rns_epoll.h"

#define RNS_FILE_READ       O_RDONLY
#define RNS_FILE_WRITE      O_WRONLY | O_APPEND 
#define RNS_FILE_RW         O_RDWR | O_APPEND

#define RNS_FILE_OPEN       O_CREAT 
#define RNS_FILE_CREAT      O_CREAT | O_TRUNC

typedef struct rns_file_s
{
    rns_epoll_cb_t ecb;
    int32_t wfd;
    int32_t rfd;
    int32_t sfd;

    io_context_t ctx;
    int32_t eventfd;
    
    uint32_t size;
    uchar_t* name;
}rns_file_t;

rns_file_t* rns_file_open(uchar_t* file_name, int32_t flag);
rns_file_t* rns_file_open2(uchar_t* file, int32_t flag);

void rns_file_close(rns_file_t** file);
int32_t rns_file_read2(rns_file_t* file, uint32_t offset, void* buf, uint32_t buf_size, void (*func)(void* data, uchar_t* buf, uint32_t size), void* data);
void rns_file_read_cb(rns_epoll_t* ep, void* data);

int32_t rns_file_read(rns_file_t* file, uchar_t* buf, uint32_t buf_size);
int32_t rns_file_write(rns_file_t* file, uchar_t* buf, uint32_t buf_size);

int32_t rns_file_info(rns_file_t* file);

int32_t rns_file_size(rns_file_t* file);
int32_t rns_file_size2(uchar_t* file);

time_t rns_file_mtime(rns_file_t* fp);

uint32_t rns_file_rfd(rns_file_t* file);
uint32_t rns_file_wfd(rns_file_t* file);
uint32_t rns_file_sfd(rns_file_t* file);

#define rns_file_wseek(file, pos) lseek(file->wfd, (off_t)(pos), SEEK_SET)

int32_t rns_file_wseek_forward(rns_file_t* file, uint32_t offset);
int32_t rns_file_wseek_backward(rns_file_t* file, uint32_t offset);
int32_t rns_file_sseek_forward(rns_file_t* file, uint32_t offset);
int32_t rns_file_sseek_backward(rns_file_t* file, uint32_t offset);


int32_t rns_file_rename(uchar_t* oldpath, uchar_t* newpath);
// short rns_fs_use_rate(uchar_t* path);
#endif
