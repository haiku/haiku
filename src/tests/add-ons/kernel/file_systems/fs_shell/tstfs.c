/*
  This file contains a simple test program that can be used as a sort
  of stress test for the file system. It's not exhaustive but it provides
  a decent first level sanity check on whether a file system will work.

  Basically it just randomly creates and deletes files.  The defines
  just after the includes control how many files and how many iterations.
  Be careful if you just bump up the numbers really high -- it will take
  a long time to run and if you only have a 16 megabyte file system
  it will probably run out of space.
  
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#include "myfs.h"
#include "kprotos.h"

#define MAX_LOOPS  1024
#define MAX_FILES  512
#define MAX_NAME   24

char      buf[MAX_FILES][MAX_NAME];
fs_off_t  sizes[MAX_FILES];


static void
make_random_name(char *buf, int len)
{
    int i, max = (rand() % (len - 7)) + 6;

    for(i=0; i < max; i++) {
        buf[i] = 'a' + (rand() % 26);
    }

    buf[i] = '\0';
}

static void
SubTime(struct timeval *a, struct timeval *b, struct timeval *c)
{
  if ((long)(a->tv_usec - b->tv_usec) < 0)
   {
     a->tv_sec--;
     a->tv_usec += 1000000;
   }

  c->tv_sec  = a->tv_sec  - b->tv_sec;
  c->tv_usec = a->tv_usec - b->tv_usec;
}


static void
write_rand_data(int fd, int max_data)
{
    int    i, k, err;
    size_t j;
    static char buf[4096];
    ulong  sum = 0;

    for(i=0; max_data > 0; i++) {
        j = rand() % sizeof(buf);
        if ((int)(max_data - j) < 0)
            j = max_data;
        
        memset(buf, rand() >> 8, j);

        for(k=0; k < j; k++)
            sum += buf[k];
        
        /* printf("write: %d\n", j); */
        err = sys_write(1, fd, buf, j);
        if (err != j) {
            errno = err;
            perror("write_rand_data");
            printf("err %d j %d\n", err, j);
            if (errno != ENOSPC)
                while(1)
                    sleep(1);
            break;
        }

        max_data -= j;
    }

#if INSANELY_SLOW_CHECKSUM
    pos = 0;
    err = sys_lseek(1, fd, SEEK_SET, &pos);
    for(i=0; i < max; i++) {
        j = sizeof(buf);
        sys_read(1, fd, buf, j);
        for(k=0; k < j; k++)
            nsum += buf[k];
    }

    if (sum != nsum)
        printf("sum = 0x%x, nsum 0x%x\n", sum, nsum);
#endif
}


int
main(int argc, char **argv)
{
    int             i, j, fd, seed, err, size, sum, name_size = 0;
    struct my_stat  st;
    struct timeval  start, end, result;
    char           *disk_name = "big_file";
    myfs_info      *myfs;
        

    if (argv[1] != NULL && !isdigit(argv[1][0]))
        disk_name = argv[1];
    else if (argv[1] && isdigit(argv[1][0]))
        seed = strtoul(argv[1], NULL, 0);
    else
        seed = getpid() * time(NULL) | 1;
    printf("random seed == 0x%x\n", seed);

    srand(seed);

    myfs = init_fs(disk_name);


    for(i=0; i < MAX_FILES; i++)
        buf[i][0] = '\0';
    

    printf("creating & deleting files...\n"); fflush(stdout);
    gettimeofday(&start, NULL);
    for(i=0,sum=0; i < MAX_LOOPS; i++) {
        j = rand() % MAX_FILES;

        size = (rand() % 65536) + 1;
            
#if 1
        if ((i % 10) == 0) {
            printf("\r                                \r"); 
            printf("iteration: %7d", i);
            fflush(stdout);
        }
#endif

        if (buf[j][0] == '\0') {     /* then create a file */
            strcpy(&buf[j][0], "/myfs/");
            make_random_name(&buf[j][6], MAX_NAME-6);
            name_size += strlen(&buf[j][6]);
            
            sum += sizes[j] = size;
            
            /* printf("\rcreating: %s %d bytes", &buf[j][0], size); */

            fd = sys_open(1, -1, &buf[j][0], O_CREAT|O_RDWR,
                          MY_S_IFREG|MY_S_IRWXU, 0);
 
            if (fd < 0) {
                printf("error creating: %s\n", &buf[j][0]);
                break;
            }
            
            write_rand_data(fd, size);

            sys_close(1, fd);
        } else {                      /* then delete the file */
            /* printf("\runlinking %s", &buf[j][0]); */
            name_size -= strlen(&buf[j][6]);

            err = sys_unlink(1, -1, &buf[j][0]);
            if (err != 0) {
                printf("error removing: %s: %s\n", &buf[j][0], strerror(err));
                break;
            }

            buf[j][0] = '\0';
            sum -= sizes[j];
        }

#if 0   /* doing this is really anal */
        for(k=0; k < MAX_FILES; k++) {
            if (buf[k][0] == '\0')
                continue;
            
            fd = sys_open(1, -1, &buf[k][0], O_RDWR, 0, 0);
            if (fd < 0) {
                printf("file: %s is not present and should be!\n", &buf[k][0]);
                sys_unmount(1, -1, "/myfs");
                exit(0);
            }
            
            sys_close(1, fd);
        }
#endif
    }
    gettimeofday(&end, NULL);
    SubTime(&end, &start, &result);

    printf("\rcreated %d files in %2ld.%.6ld seconds (%d k data)\n", i,
           result.tv_sec, result.tv_usec, sum/1024);
    
    printf("now verifying files....\n");
    for(i=0; i < MAX_FILES; i++) {
        if (buf[i][0] == '\0')
            continue;

        printf("                                                       \r");
        printf("opening: %s\r", &buf[i][0]);
        fflush(stdout);
        
        fd = sys_open(1, -1, &buf[i][0], O_RDWR, 0, 0);
        if (fd != 0) { 
            printf("file: %s is not present and should be!\n", &buf[i][0]);
            sys_unmount(1, -1, "/myfs");
            exit(0);
        }
            
        err = sys_rstat(1, -1, &buf[i][0], &st, 1);
        if (err != 0) {
            printf("stat failed for: %s\n", &buf[i][0]);
            continue;
        }
        
        if (st.st_size != sizes[i]) {
            printf("size mismatch on %s: %ld != %ld\n", &buf[i][0],
                   st.st_size, sizes[i]);
        }

        sys_close(1, fd);
    }
    printf("done verifying files                                         \n");

    if (sys_unmount(1, -1, "/myfs") != 0) {
        printf("could not UNmount /myfs\n");
        return 5;
    }

    shutdown_block_cache();

    /* check_mem();  */
    
    return 0;   
}
