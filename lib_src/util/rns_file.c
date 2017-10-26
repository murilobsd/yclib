#include "rns_file.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>

#include <sys/vfs.h>
#include "rns_memory.h"
#include <stdio.h>
#include "rns_dir.h"

typedef struct rns_iocb_s
{
    struct iocb iocb;
    void (*func)(void* data, uchar_t* buf, uint32_t size);
    void* data;
}rns_iocb_t;

void rns_file_read_cb(rns_epoll_t* ep, void* data)
{
    rns_file_t* fp = (rns_file_t*)data;

    uint64_t finished_aio = 0;
    int32_t ret = read(fp->eventfd, &finished_aio, sizeof(finished_aio));
    if(ret != sizeof(finished_aio))
    {
        return;
    }

    struct io_event ioevent;
    ret = io_getevents(fp->ctx, 1, 1, &ioevent, NULL);
    if(ret < 0)
    {
        return;
    }

    rns_iocb_t* iocb = (rns_iocb_t*)ioevent.obj;
    iocb->func(data, (uchar_t*)iocb->iocb.u.c.buf, iocb->iocb.u.c.nbytes);

    rns_free(iocb);

    return;
}

rns_file_t* rns_file_open2(uchar_t* file, int32_t flag)
{
    uchar_t* walker = NULL;
    int32_t ret = 0;
    
    walker = rns_strrchr(file, '/');
    if(walker == NULL)
    {
        return rns_file_open(file, flag);
    }
    else
    {
        ret = rns_dir_create(file, walker - file, 0755);
        if(ret < 0)
        {
            return NULL;
        }
        return rns_file_open(file, flag);
    }
}

rns_file_t* rns_file_open(uchar_t* file_name, int32_t flag)
{
    if(file_name == NULL)
    {
        return NULL;
    }
    rns_file_t* file = (rns_file_t*)rns_malloc(sizeof(rns_file_t));
    if(file == NULL)
    {
        return NULL;
    }

    file->sfd = -1;
    file->wfd = -1;
    file->rfd = -1;

    if((flag & O_RDWR) != 0 || (flag & O_WRONLY) != 0)
    {
        int32_t wflag = flag & ~O_RDWR | O_WRONLY;
        file->wfd = open((const char_t*)file_name, wflag, 0644);
        if(file->wfd < 0)
        {
            rns_free(file);
            return NULL;
        }
    }

    if((flag & O_RDWR) != 0 || (flag & 0x1) == 0)
    {
        int32_t rflag = flag & ~O_RDWR | O_RDONLY | O_DIRECT;
        file->rfd = open((const char_t*)file_name, rflag);
        if(file->rfd < 0)
        {
            close(file->wfd);
            rns_free(file);
            return NULL;
        }
        
        int32_t sflag =  flag & ~O_RDWR | O_RDONLY & ~O_DIRECT;
        file->sfd = open((const char_t*)file_name, sflag);
        if(file->sfd < 0)
        {
            close(file->wfd);
            close(file->rfd);
            rns_free(file);
            return NULL;
        }
    }

    int32_t ret = io_setup(100, &file->ctx);
    if(ret < 0)
    {
        rns_free(file);
        return NULL;
    }
    file->eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    file->name = rns_malloc(rns_strlen(file_name) + 1);
    if(file->name == NULL)
    {
        rns_free(file);
        return NULL;
    }
    rns_strncpy(file->name, file_name, rns_strlen(file_name));

    file->ecb.read = rns_file_read_cb;
    file->ecb.write = NULL;
    
    return file;
		
}

void rns_file_close(rns_file_t** file)
{
    if(*file == NULL)
    {
        return;
    }

    close(rns_file_wfd(*file));
    close(rns_file_rfd(*file));
    close(rns_file_sfd(*file));

    close((*file)->eventfd);
    io_destroy((*file)->ctx);
    
    if((*file)->name != NULL)
    {
        rns_free((*file)->name);
    }
    
    free(*file);
    *file = NULL;
}

int32_t rns_file_read2(rns_file_t* file, uint32_t offset, void* buf, uint32_t buf_size, void (*func)(void* data, uchar_t* buf, uint32_t size), void* data)
{
    struct iocb* array[1];
    
    rns_iocb_t* cb = (rns_iocb_t*)rns_malloc(sizeof(rns_iocb_t));
    if(cb == NULL)
    {
        return -1;
    }

    cb->func = func;
    cb->data = data;

    array[0] = &cb->iocb;
    io_prep_pread(&cb->iocb, file->rfd, buf, buf_size, offset);
    io_set_eventfd(&cb->iocb, file->eventfd);
    
    int32_t ret = io_submit(file->ctx, 1, array);
    if(ret < 0)
    {
        rns_free(cb);
        return -2;
    }
    
    return 0;
}

int32_t rns_file_read(rns_file_t* file, uchar_t* buf, uint32_t buf_size)
{
    uint32_t total_bytes = buf_size;
    uint32_t read_bytes = 0;
    int32_t ret = 0;
    uint32_t bytes = 1048576;
	
    do
    {
        uint32_t need = bytes < total_bytes - read_bytes ? bytes : total_bytes - read_bytes; 
        ret = read(file->sfd, buf + read_bytes, need);
        if(ret < 0)
        {
            return -1;
        }
        else if(ret == 0)
        {
            break;
        }
        read_bytes += ret;
    }while(read_bytes < total_bytes);
	
    return read_bytes;
}


int32_t rns_file_write(rns_file_t* file, uchar_t* buf, uint32_t buf_size)
{
    return write(rns_file_wfd(file), buf, buf_size);
}

int32_t rns_file_info(rns_file_t* file)
{
    struct stat buf;
    int32_t ret = stat((const char_t*)file->name, &buf);
    if(ret < 0)
    {
        return ret;
    }

    file->size = buf.st_size;
    return 0;
}

int32_t rns_file_size(rns_file_t* file)
{
    if(file == NULL)
    {
        return -1;
    }
    struct stat buf;
    int32_t ret = stat((const char_t*)file->name, &buf);
    if(ret < 0)
    {
        return -2;
    }

    return buf.st_size;
	
}


int32_t rns_file_size2(uchar_t* file)
{
    if(file == NULL)
    {
        return -1;
    }
    struct stat buf;
    int32_t ret = stat((const char_t*)file, &buf);
    if(ret < 0)
    {
        return -2;
    }
    
    return buf.st_size;
    
}

time_t rns_file_mtime(rns_file_t* fp)
{
    if(fp == NULL)
    {
        return -1;
    }
    struct stat buf;
    int32_t ret = stat((const char_t*)fp->name, &buf);
    if(ret < 0)
    {
        return -2;
    }
    
    return buf.st_mtime;
}

uint32_t rns_file_wfd(rns_file_t* file)
{
    return file->wfd;
}

uint32_t rns_file_rfd(rns_file_t* file)
{
    return file->rfd;
}

uint32_t rns_file_sfd(rns_file_t* file)
{
    return file->sfd;
}



uint32_t rns_file_lock(uchar_t* filename)
{
    rns_file_t* file = rns_file_open(filename, RNS_FILE_CREAT | RNS_FILE_RW);
	
    struct flock f;
    f.l_type =  F_WRLCK;
    f.l_whence = SEEK_SET;
    f.l_start = 0;
    f.l_len = 10;

    return fcntl(rns_file_wfd(file), F_SETLK, &f);
}

uint32_t rns_file_unlock(uchar_t* filename)
{
    rns_file_t* file = rns_file_open(filename, RNS_FILE_CREAT | RNS_FILE_RW);
	
    struct flock f;
    f.l_type =  F_UNLCK;
    f.l_whence = SEEK_SET;
    f.l_start = 0;
    f.l_len = 10;

    return fcntl(rns_file_wfd(file), F_SETLK, &f);
}

typedef struct rns_fs_info_s
{
    uint32_t fs_type;
    uint32_t block_size;	
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t avail_bytes;
    uint64_t max_namelen;
}rns_fs_info_t;

rns_fs_info_t* rns_fs_info(uchar_t* path)
{
    rns_fs_info_t* fs_info = (rns_fs_info_t*)rns_malloc(sizeof(rns_fs_info_t));
    if(fs_info == NULL)
    {
        return NULL;
    }
    
    struct statfs buf;
    int32_t ret = statfs((char_t*)path, &buf);
    if(ret < 0)
    {
        rns_free(fs_info);
        return NULL;
    }
    fs_info->fs_type = buf.f_type;
    fs_info->block_size = buf.f_bsize;
    /* fs_info->total_bytes = fs_info->block_size * buf.f_blocks; */
    /* fs_info->free_bytes = fs_info->block_size * buf.f_bfree; */
    /* fs_info->avail_bytes = fs_info->block_size * buf.f_bavail; */
    fs_info->max_namelen = buf.f_namelen;
	
    return fs_info;
}

int32_t rns_file_wseek_forward(rns_file_t* file, uint32_t offset)
{
    return lseek(rns_file_wfd(file), offset, SEEK_CUR);
}
int32_t rns_file_wseek_backward(rns_file_t* file, uint32_t offset)
{
    return lseek(rns_file_wfd(file), -1 * offset, SEEK_CUR);
}

int32_t rns_file_sseek_forward(rns_file_t* file, uint32_t offset)
{
    return lseek(rns_file_sfd(file), offset, SEEK_CUR);
}
int32_t rns_file_sseek_backward(rns_file_t* file, uint32_t offset)
{
    return lseek(rns_file_sfd(file), -1 * offset, SEEK_CUR);
}

/* short rns_fs_use_rate(uchar_t* path) */
/* { */
/*     struct statfs buf; */
/*     statfs(path, &buf); */
    
/*     return 100 - (buf.f_bavail * 100)/ buf.f_blocks; */
/* } */

int32_t rns_file_rename(uchar_t* oldpath, uchar_t* newpath)
{
    return rename((char_t*)oldpath, (char_t*)newpath);
}
