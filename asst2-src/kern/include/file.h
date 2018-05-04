/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <vnode.h>



/*
 * Put your function declarations and data types here ...
 */

typedef struct node *list;

typedef struct head {
    list rest;
} head;

typedef struct node {
    int fd;
    int isDup;
    int offset;

    // content
    struct vnode *vn;

    // pointers
    
    list dup;
} node;

#define FILETABLE_SIZE 32
struct node * filetable[FILETABLE_SIZE];

int error_num;

void update_dup_offsets(node * curr, int l);
node *getfile(int fd);

node *init_node(int fd, int isDup, node *dup,int offset);

int open(char *filename, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int close(int fd);
int dup2(int oldfd, int newfd);




#endif /* _FILE_H_ */
