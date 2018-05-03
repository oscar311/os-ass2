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

void update_dup_offsets(int fd, int l) 
{

    node *curr = getfile(fd);


    if(curr->dup != NULL) {
        node *t = curr->dup;

        while(t != NULL) {
            
            if(t->isDup == 1) {
                t->offset += l;
                t = t->dup;

            } else 
                break;

            
        }
    }
    
}



list getfile(int fd) 
{
    node *curr = filetable->rest;

    kprintf("hey!->>>>>>>>> %d\n", fd);

    while(curr != NULL)
    {
        kprintf(">> %d\n", curr->fd);
        if(curr->fd == fd)
            return curr;

        curr = curr->next;
    }

    return NULL;
}

void init_node(node *curr, node *parent, int fd, int isDup, node *dup,int offset)
{
    
    curr->fd = fd;
    curr->isDup = isDup;
    curr->offset = offset;
    curr->vn = kmalloc(sizeof(struct vnode));
    curr->parent = parent;
    curr->next = NULL;
    curr->dup = dup;
}

int open(const char *filename, int flags)
{

    int ret = 0; // return value


    // ignore - stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) 

    // if filetable is empty 
    // file the first 3 slot with std
    // then use as normal
    // should work as open is always called first


    /*if (filetable == NULL) {
        
        filetable = kmalloc(sizeof(struct head));
        filetable->rest = kmalloc(sizeof(struct node));

        node *stdin = kmalloc(sizeof(struct node));
        node *stdout = kmalloc(sizeof(struct node));
        node *stderr = kmalloc(sizeof(struct node));

        

        init_node(stdin, NULL,0,0,NULL,0);
        init_node(stdout, stdin,1,0,NULL,0);
        init_node(stderr, stdout,2,0,NULL,0);
        
        filetable->rest = stdin;
        stdin->next = stdout;
        stdout->next = stderr;
    } */


    node *t = filetable->rest;

    kprintf("open->>>>>>>>>\n");

    while(t != NULL)
    {
        
        kprintf("Node: %d\n",t->fd);

        t = t->next;
    }


    node *curr = filetable->rest;
    node *prev = NULL;
    int i = 0;

    while(curr != NULL)
    {
        prev = curr;
        curr = curr->next;
        i++;
    }

    node *newnode = kmalloc(sizeof(struct node));

    init_node(newnode, prev ,i,0,NULL,0);

    curr = newnode;


    char* f = (char*) filename;

    error_num = vfs_open(f, flags, 0, &newnode->vn);

    kprintf("opened file ->>>>>>>>> %d\n", i);

    if(error_num)
        return -1;
    else 
        ret = i;



    return ret;
}

ssize_t read(int fd, void *buf, size_t count) 
{

    int ret = 0;

    // set up unique i/o (uio)
    struct uio unique_io;
    struct iovec iov;
    // set up buf to read into
    void *kerBuf = kmalloc(sizeof(count));


    node *t = filetable->rest;

    kprintf("read->>>>>>>>>\n");

    while(t != NULL)
    {
        
        kprintf("Node: %d\n",t->fd);

        t = t->next;
    }

    node *file = getfile(fd);

    // ensure file is not null or stdout or stderr
    if(file == NULL || fd == 1 || fd == 2 ) {
        error_num = EBADF;
        ret = -1;
    } else {

        // get stats for out file
        struct stat stats;
        VOP_STAT(file->vn, &stats);

        // check if we have bytes to read

        if(file->offset < stats.st_size) {
            // ensure we are not reading over too much, if so we adjust to fit
            int l = 0;
            if(count > stats.st_size - file->offset) {
                l = stats.st_size - file->offset;
            } else {
                l = count;
            }


            uio_kinit(&iov, &unique_io, kerBuf, l, file->offset, UIO_READ);
            
            error_num = VOP_READ(file->vn, &unique_io);

            if(error_num)
                return -1;

            file->offset += l;

            update_dup_offsets(fd, l);

            copyout(kerBuf, (userptr_t) buf, l);

            ret = l;

        } else {
            ret = 0;
            copyout(kerBuf, (userptr_t) buf, count);

        }
    }


    return ret;
}

ssize_t write(int fd, const void *buf, size_t count)
{

    int ret = 0;

    // set up unique i/o (uio)
    struct uio unique_io;
    struct iovec iov;
    // set up buf to write into
    void *kerBuf = kmalloc(sizeof(count));



    node *t = filetable->rest;

    kprintf("write->>>>>>>>>\n");

    while(t != NULL)
    {
        
        kprintf("Node: %d\n",t->fd);

        t = t->next;
    }



    node *file = getfile(fd);

    // stdin is read only

    if(fd == 0) {
        error_num = EBADF;
        ret = -1;
    } else if (file != NULL) {

        // copy date from userland to kernel

        error_num = copyin((const_userptr_t)buf, kerBuf, count);

        if(error_num)
            return -1;

        // set up uio for kernel io

        uio_kinit(&iov, &unique_io, kerBuf, count, file->offset, UIO_WRITE);
            
        error_num = VOP_WRITE(file->vn, &unique_io);
        if(error_num)
            return -1;

        // update offset

        file->offset += count;

        kprintf("start\n");

        update_dup_offsets(fd,count);

        kprintf("end\n");

        ret = count;
    } else {
        error_num = EBADF;
        ret = -1;
    }

    return ret;
}

off_t lseek(int fd, off_t offset, int whence)
{

    int ret = 0;
    int old_offset;


    node *t = filetable->rest;

    kprintf("lseek->>>>>>>>>\n");

    while(t != NULL)
    {
        
        kprintf("Node: %d\n",t->fd);

        t = t->next;
    }



    node *file = getfile(fd);

    

    // std's dont support lseek
    if(fd == 0 || fd == 1 || fd == 2) {
        error_num = ESPIPE;
        ret = -1;
    } else if (file == NULL) {
        error_num = EBADF;
        ret = -1;
    } else {
        old_offset = file->offset;

        if(whence == SEEK_SET) {
            file->offset = offset;
        } else if (whence == SEEK_CUR) {
            file->offset += offset;
        } else if (whence == SEEK_END) {
            // need stats here 
            struct stat stats;
            VOP_STAT(file->vn, &stats);

            file->offset = (stats.st_size -1) + offset;

        } else {
            // whence was not a correct value
            error_num = EINVAL;
            ret = -1;
        }

        // now check the legally of the new offset
        error_num = VOP_ISSEEKABLE(file->vn);
        if(error_num) {
            file->offset = old_offset;
            ret = -1;
        }
    }


    // if theres been no error
    if(ret == 0) {
        // check if offsets negative
        if(file->offset < 0) {
            file->offset = old_offset;
            error_num = EINVAL;
            ret = -1;
        } else {
            ret = file->offset;
        }
    }

    return ret;
}

int close(int fd)
{
    int ret = 0;





    node *t = filetable->rest;

    kprintf("close->>>>>>>>>\n");

    while(t != NULL)
    {
        
        kprintf("Node: %d\n",t->fd);

        t = t->next;
    }





    node *file = getfile(fd);


    

    // can't close std's
    if(fd == 0 || fd == 1 || fd == 2) {
        error_num = EBADF;
        ret = -1;
    } else if (file != NULL && file->isDup == 0) {

        // close all dups

        node * curr = filetable->rest;


        while(curr != NULL) {
            if(curr->fd == fd && curr->dup != NULL) {
                node *t = curr->dup;
                node *y = NULL;
                while(t != NULL) {
                    
                    if(t->isDup == 1) {
                        vfs_close(t->vn);
                        kfree(t->vn); 
                    }
                    y = t;
                    t = t->dup;
                    kfree(y);
                }
            }
            curr = curr->next;
        }


        vfs_close(file->vn);
        kfree(file->vn);
        kfree(file);

        ret = 0;
        
    } else {
        error_num = EBADF;
        ret = -1;
    }


    return ret;
}
int dup2(int oldfd, int newfd) 
{

    int ret = 0;
    //int saved_oldfd = oldfd;

    node *t = filetable->rest;

    kprintf("dup2->>>>>>>>>\n");

    while(t != NULL)
    {
        
        kprintf("Node: %d\n",t->fd);

        t = t->next;
    }

    node *oldfile = getfile(oldfd);
    node *newfile = getfile(newfd);

    if(oldfile == NULL || newfile == NULL) {
        error_num = EBADF;
        ret = -1;
    } else {
        // close if already opened
        if(newfile != NULL) {
            error_num = close(newfd);
            if(error_num)
                return -1;
        }

        // there must be some data in oldfd
        if(oldfile == NULL) {
            error_num = EBADF;
            ret = -1;
        } else {

            if(oldfile->dup != NULL) {
                node *curr = oldfile;
                while(curr->dup != NULL) {
                    curr = curr->dup;
                }

                // once free spot is the linked list of dups is found
                newfile = oldfile;
                curr->dup = newfile;
                newfile->isDup = 1;

                ret = 0;
            }
        }
    }
    return ret;
}

