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

void update_dup_offsets(node * curr, int l) 
{

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



node * getfile(int fd) 
{

    if(fd >= 0 && fd < FILETABLE_SIZE)
        return filetable[fd];
    else 
        return NULL;
}



node *init_node(int fd, int isDup, node *dup,int offset)
{
    
    struct node *curr = kmalloc(sizeof(struct node));
    if(curr == NULL)
        return NULL;

    curr->fd = fd;
    curr->isDup = isDup;
    curr->offset = offset;
    curr->vn = NULL; //kmalloc(sizeof(struct vnode));

    curr->dup = dup;

    curr->f_lock = lock_create("lock");

    curr->type = NONE;

    return curr;
}

int open( char *filename, int flags)
{

    int ret = 0; // return value

    int i = 0;

    for(i = 0; i < FILETABLE_SIZE; i++) {

        // check for dup?
        if(filetable[i] == NULL) {

            filetable[i] = init_node(i,0,NULL,0);

            lock_acquire(filetable[i]->f_lock);
            error_num = vfs_open(filename, flags, i, &filetable[i]->vn);
            lock_release(filetable[i]->f_lock);
            
            if(error_num) return -1;
            
            lock_acquire(filetable[i]->f_lock);
            error_num = VOP_EACHOPEN(filetable[i]->vn, flags);
            lock_release(filetable[i]->f_lock);
            
            if(error_num) return -1;
            
            ret = i;
            break;
        }

    }

    if(i >= FILETABLE_SIZE) {
        error_num = EMFILE;
        ret = -1;
    }
    //kprintf("Hekko %d\n", ret);
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

    node *file = getfile(fd);

    // ensure file is not null or stdout or stderr
    lock_acquire(file->f_lock);
    if(file == NULL || file->type == 1 || file->type == 2 ) {
        error_num = EBADF;
        ret = -1;
    } else {

        // get stats for out file
        struct stat stats;
        
        VOP_STAT(file->vn, &stats);


        // check if we have bytes to read
        //lock_acquire(file->f_lock);        
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

            if(error_num) return -1;

            file->offset += l;

            update_dup_offsets(file, l);

            copyout(kerBuf, (userptr_t) buf, l);

            ret = l;

        } else {
            ret = 0;
            copyout(kerBuf, (userptr_t) buf, count);

        }
        //lock_release(file->f_lock);

    }

    lock_release(file->f_lock);


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



    node *file = getfile(fd);

    // stdin is read only
    lock_acquire(file->f_lock);
    if(file->type == 0) {
        lock_release(file->f_lock);
        error_num = EBADF;
        ret = -1;
    } else if (file != NULL) {
        lock_release(file->f_lock);

        // copy date from userland to kernel

        error_num = copyin((const_userptr_t)buf, kerBuf, count);

        if(error_num) return -1;

        // set up uio for kernel io
        lock_acquire(file->f_lock);
        uio_kinit(&iov, &unique_io, kerBuf, count, file->offset, UIO_WRITE);            
        error_num = VOP_WRITE(file->vn, &unique_io);
        lock_release(file->f_lock);

        if(error_num) return -1;

        // update offset
        lock_acquire(file->f_lock);  
        file->offset += count;
        lock_release(file->f_lock);


        lock_acquire(file->f_lock);  
        update_dup_offsets(file,count);
        lock_release(file->f_lock);  

        ret = count;
    } else {
        lock_release(file->f_lock);
        error_num = EBADF;
        ret = -1;
    }


    return ret;
}

off_t lseek(int fd, off_t offset, int whence)
{

    int ret = 0;
    int old_offset = 0;

    //kprintf("KKKKKKsdfghj %ld\n", offset); 

    node *file = getfile(fd); 

    // std's dont support lseek
    lock_acquire(file->f_lock);
    if(file->type == 0 || file->type == 1 || file->type == 2) {
        error_num = ESPIPE;
        ret = -1;
    } else if (file == NULL) {
        lock_release(file->f_lock);
        error_num = EBADF;
        ret = -1;
    } else {
        lock_release(file->f_lock);


        old_offset = file->offset;


        lock_acquire(file->f_lock);
        // need stats here 
        struct stat stats;
        VOP_STAT(file->vn, &stats);

        if(whence == SEEK_SET) {
            file->offset = offset;
        } else if (whence == SEEK_CUR) {
            file->offset += offset;
        } else if (whence == SEEK_END) {
        
            file->offset = (stats.st_size -1) + offset;

        } else {
            // whence was not a correct value
            error_num = EINVAL;
            ret = -1;
        }

        // now check the legally of the new offset
        error_num = VOP_ISSEEKABLE(file->vn);
        if(!error_num || file->offset > stats.st_size - 1) {
            error_num = ESPIPE;
            file->offset = old_offset;
            ret = -1;
        }

        lock_release(file->f_lock);


    }

    // if theres been no error
    if(ret == 0) {
        lock_acquire(file->f_lock);
        // check if offsets negative
        if(file->offset < 0) {
            file->offset = old_offset;
            error_num = EINVAL;
            ret = -1;
        } else {
            ret = file->offset;
        }
        lock_release(file->f_lock);  

    }


    return ret;
}

int close(int fd)
{
    int ret = 0;

    node *file = getfile(fd);


    lock_acquire(file->f_lock);
    if (file != NULL && file->isDup == 0) {
        lock_release(file->f_lock);

        // close all dups
        lock_acquire(file->f_lock);

        node *curr = file->dup;

        while(curr != NULL) {

            if(curr->isDup == 1) {
                vfs_close(curr->vn);
                //kfree(curr->vn);
                node *y = curr;
                curr = curr->dup;

                kfree(y);

            } else 
                break;

        }


        vfs_close(file->vn);
        //kfree(file->vn);
        lock_release(file->f_lock);

        // break the lock.. break it down duh duh
        lock_destroy(file->f_lock);
        kfree(file);
        filetable[fd] = NULL;

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

    node *oldfile = getfile(oldfd);
    node *newfile = getfile(newfd);

    lock_acquire(oldfile->f_lock);
    lock_acquire(newfile->f_lock);
    
    if(oldfile == NULL || newfile == NULL) {
        error_num = EBADF;
        ret = -1;
    } else {
        // close if already opened
        if(newfile != NULL) {

            error_num = close(newfd);
            

            if(error_num) return -1;
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
    lock_release(newfile->f_lock);
    lock_release(oldfile->f_lock);

    return ret;
}

