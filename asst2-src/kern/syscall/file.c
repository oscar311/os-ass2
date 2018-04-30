#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>

/*
 * Add your file-related functions here ...
 */

int open(const char *filename, int flags)
{

    int ret = 0; // return value



    const char *t = filename;
    ret = flags;

    if(t == NULL)
    return ret;
    else 
    return 0;
}

ssize_t read(int fd, void *buf, size_t count) 
{

    void *t = buf;
    int ret = fd+ count;


    if(t == NULL)
    return ret;
    else 
    return 0;
}

ssize_t write(int fd, const void *buf, size_t count)
{



    const void *t = buf;
    int ret = fd+ count;

    if(t == NULL)
    return ret;
    else 
    return 0;
}

off_t lseek(int fd, off_t offset, int whence)
{

    int ret = fd+ whence + offset;


    return ret;
}
int close(int fd)
{




    return fd;
}
int dup2(int oldfd, int newfd) 
{


    return oldfd +newfd;
}

