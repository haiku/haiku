/*
  This file contains some stub routines to cover up the differences
  between the BeOS and the rest of the world.
  
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include <stdio.h>
#include <stdlib.h>

#include "myfs.h"

#ifndef __BEOS__ 

void
unload_kernel_addon(aid)
{
}

sem_id
create_sem(long count, const char *name)
{
    int *ptr;
    ptr = (int *)malloc(sizeof(int) + strlen(name) + 1);  /* a hack */
    *ptr = count;
    memcpy(ptr+1, name, strlen(name));
    
    return (sem_id)ptr;
}


long
delete_sem(sem_id semid)
{
    int *ptr = (int *)semid;

    free(ptr);
    return 0;
}

long
acquire_sem(sem_id sem)
{
    int *ptr = (int *)sem;

    if (*ptr <= 0) {
        myfs_die("You lose sucka! acquire of sem with count == %d\n", *ptr);
    }

    *ptr -= 1;

    return 0;
}


long
acquire_sem_etc(sem_id sem, int count, int j1, bigtime_t j2)
{
    int *ptr = (int *)sem;

    if (*ptr <= 0) {
        myfs_die("You lose sucka! acquire_sem_etc of sem with count == %d\n",
                 *ptr);
    }

    *ptr -= count;

    return 0;
}

long
release_sem(sem_id sem)
{
    int *ptr = (int *)sem;

    *ptr += 1;

    return 0;
}

long
release_sem_etc(sem_id sem, long count, long j1)
{
    int *ptr = (int *)sem;

    *ptr += count;

    return 0;
}


long
atomic_add(long *ptr, long val)
{
    int old = *ptr;

    *ptr += val;

    return old;
}

int
snooze(bigtime_t f)
{
    sleep(1);
    return 1;
}

#endif  /* __BEOS__ */
