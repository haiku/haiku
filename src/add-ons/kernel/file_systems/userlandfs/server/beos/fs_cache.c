/*
   This file contains the global device cache for the BeOS.  All
   file system I/O comes through here.  The cache can handle blocks
   of different sizes for multiple different underlying physical
   devices.

   The cache is organized as a hash table (for lookups by device
   and block number) and two doubly-linked lists.  The normal
   list is for "normal" blocks which are either clean or dirty.
   The locked list is for blocks that are locked in the cache by
   BFS.  The lists are LRU ordered.

   Most of the work happens in the function cache_block_io() which
   is quite lengthy.  The other functions of interest are get_ents()
   which picks victims to be kicked out of the cache; flush_ents()
   which does the work of flushing them; and set_blocks_info() which
   handles cloning blocks and setting callbacks so that the BFS
   journal will work properly.  If you want to modify this code it
   will take some study but it's not too bad.  Do not think about
   separating the list of clean and dirty blocks into two lists as
   I did that already and it's slower.

   Originally this cache code was written while listening to the album
   "Ride the Lightning" by Metallica.  The current version was written
   while listening to "Welcome to SkyValley" by Kyuss as well as an
   ambient collection on the German label FAX.  Music helps a lot when
   writing code.

   THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED
   OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
   NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

   FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

   Dominic Giampaolo
   dbg@be.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


#include <OS.h>
#include <KernelExport.h>

#include "fs_cache.h"
#include "fs_cache_priv.h"
#include "lock.h"



#ifndef USER
#define printf dprintf
#endif
#ifdef USER
#define kprintf printf
#endif


typedef off_t fs_off_t;

/* forward prototypes */
static int   flush_ents(cache_ent **ents, int n_ents);

//static int   do_dump(int argc, char **argv);
//static int   do_find_block(int argc, char **argv);
//static int   do_find_data(int argc, char **argv);
//static void  cache_flusher(void *arg, int phase);


int chatty_io = 0;

#define CHUNK (512 * 1024)   /* a hack to work around scsi driver bugs */

static void
beos_panic(const char *format, ...)
{
    va_list     ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    while (TRUE)
        ;
}

size_t
beos_read_phys_blocks(int fd, fs_off_t bnum, void *data, uint num_blocks, int bsize)
{
    size_t ret = 0;
    size_t sum;

    if (chatty_io)
        printf("R: %8" B_PRIdOFF " : %3d\n", bnum, num_blocks);

    if (num_blocks * bsize < CHUNK)
        ret = read_pos(fd, bnum * bsize, data, num_blocks * bsize);
    else {
        for(sum=0; (sum + CHUNK) <= (num_blocks * bsize); sum += CHUNK) {
            ret = read_pos(fd, (bnum * bsize) + sum, data, CHUNK);
            if (ret != CHUNK)
                break;

            data = (void *)((char *)data + CHUNK);
        }

        if (ret == CHUNK && ((num_blocks * bsize) - sum) > 0) {
            ret = read_pos(fd, (bnum * bsize) + sum, data,
                           (num_blocks * bsize) - sum);

            if (ret == (num_blocks * bsize) - sum)
                ret = num_blocks * bsize;
        } else if (ret == CHUNK) {
            ret = num_blocks * bsize;
        }
    }

    if (ret == num_blocks * bsize)
        return 0;
    else
        return EBADF;
}

size_t
beos_write_phys_blocks(int fd, fs_off_t bnum, void *data, uint num_blocks, int bsize)
{
    size_t ret = 0;
    size_t sum;

    if (chatty_io)
        printf("W: %8" B_PRIdOFF " : %3d\n", bnum, num_blocks);

    if (num_blocks * bsize < CHUNK)
        ret = write_pos(fd, bnum * bsize, data, num_blocks * bsize);
    else {
        for(sum=0; (sum + CHUNK) <= (num_blocks * bsize); sum += CHUNK) {
            ret = write_pos(fd, (bnum * bsize) + sum, data, CHUNK);
            if (ret != CHUNK)
                break;

            data = (void *)((char *)data + CHUNK);
        }

        if (ret == CHUNK && ((num_blocks * bsize) - sum) > 0) {
            ret = write_pos(fd, (bnum * bsize) + sum, data,
                            (num_blocks * bsize) - sum);

            if (ret == (num_blocks * bsize) - sum)
                ret = num_blocks * bsize;
        } else if (ret == CHUNK) {
            ret = num_blocks * bsize;
        }
    }


    if (ret == num_blocks * bsize)
        return 0;
    else
        return EBADF;
}

//	#pragma mark -

static int
init_hash_table(hash_table *ht)
{
    ht->max   = HT_DEFAULT_MAX;
    ht->mask  = ht->max - 1;
    ht->num_elements = 0;

    ht->table = (hash_ent **)calloc(ht->max, sizeof(hash_ent *));
    if (ht->table == NULL)
        return ENOMEM;

    return 0;
}


static void
shutdown_hash_table(hash_table *ht)
{
    int       i, hash_len;
    hash_ent *he, *next;

    for(i=0; i < ht->max; i++) {
        he = ht->table[i];

        for(hash_len=0; he; hash_len++, he=next) {
            next = he->next;
            free(he);
        }
    }

    if (ht->table)
        free(ht->table);
    ht->table = NULL;
}

#if 0
static void
print_hash_stats(hash_table *ht)
{
    int       i, hash_len, max = -1, sum = 0;
    hash_ent *he, *next;

    for(i=0; i < ht->max; i++) {
        he = ht->table[i];

        for(hash_len=0; he; hash_len++, he=next) {
            next = he->next;
        }
        if (hash_len)
            printf("bucket %3d : %3d\n", i, hash_len);

        sum += hash_len;
        if (hash_len > max)
            max = hash_len;
    }

    printf("max # of chains: %d,  average chain length %d\n", max,sum/ht->max);
}
#endif

#define HASH(d, b)   ((((fs_off_t)d) << (sizeof(fs_off_t)*8 - 6)) | (b))

static hash_ent *
new_hash_ent(int dev, fs_off_t bnum, void *data)
{
    hash_ent *he;

    he = (hash_ent *)malloc(sizeof(*he));
    if (he == NULL)
        return NULL;

    he->hash_val = HASH(dev, bnum);
    he->dev      = dev;
    he->bnum     = bnum;
    he->data     = data;
    he->next     = NULL;

    return he;
}


static int
grow_hash_table(hash_table *ht)
{
    int        i, omax, newsize, newmask;
    fs_off_t      hash;
    hash_ent **new_table, *he, *next;

    if (ht->max & ht->mask) {
        printf("*** hashtable size %d or mask %d looks weird!\n", ht->max,
               ht->mask);
    }

    omax    = ht->max;
    newsize = omax * 2;        /* have to grow in powers of two */
    newmask = newsize - 1;

    new_table = (hash_ent **)calloc(newsize, sizeof(hash_ent *));
    if (new_table == NULL)
        return ENOMEM;

    for(i=0; i < omax; i++) {
        for(he=ht->table[i]; he; he=next) {
            hash = he->hash_val & newmask;
            next = he->next;

            he->next        = new_table[hash];
            new_table[hash] = he;
        }
    }

    free(ht->table);
    ht->table = new_table;
    ht->max   = newsize;
    ht->mask  = newmask;

    return 0;
}




static int
hash_insert(hash_table *ht, int dev, fs_off_t bnum, void *data)
{
    fs_off_t    hash;
    hash_ent *he, *curr;

    hash = HASH(dev, bnum) & ht->mask;

    curr = ht->table[hash];
    for(; curr != NULL; curr=curr->next)
        if (curr->dev == dev && curr->bnum == bnum)
            break;

    if (curr && curr->dev == dev && curr->bnum == bnum) {
        printf("entry %d:%" B_PRIdOFF " already in the hash table!\n",
            dev, bnum);
        return EEXIST;
    }

    he = new_hash_ent(dev, bnum, data);
    if (he == NULL)
        return ENOMEM;

    he->next        = ht->table[hash];
    ht->table[hash] = he;

    ht->num_elements++;
    if (ht->num_elements >= ((ht->max * 3) / 4)) {
        if (grow_hash_table(ht) != 0)
            return ENOMEM;
    }

    return 0;
}

static void *
hash_lookup(hash_table *ht, int dev, fs_off_t bnum)
{
    hash_ent *he;

    he = ht->table[HASH(dev, bnum) & ht->mask];

    for(; he != NULL; he=he->next) {
        if (he->dev == dev && he->bnum == bnum)
            break;
    }

    if (he)
        return he->data;
    else
        return NULL;
}


static void *
hash_delete(hash_table *ht, int dev, fs_off_t bnum)
{
    void     *data;
    fs_off_t     hash;
    hash_ent *he, *prev = NULL;

    hash = HASH(dev, bnum) & ht->mask;
    he = ht->table[hash];

    for(; he != NULL; prev=he,he=he->next) {
        if (he->dev == dev && he->bnum == bnum)
            break;
    }

    if (he == NULL) {
        printf("*** hash_delete: tried to delete non-existent block %d:%"
            B_PRIdOFF "\n", dev, bnum);
        return NULL;
    }

    data = he->data;

    if (ht->table[hash] == he)
        ht->table[hash] = he->next;
    else if (prev)
        prev->next = he->next;
    else
        beos_panic("hash table is inconsistent\n");

    free(he);
    ht->num_elements--;

    return data;
}

//	#pragma mark -

/*
  These are the global variables for the cache.
*/
static block_cache  bc;

#define       MAX_IOVECS  64           /* # of iovecs for use by cache code */
static beos_lock   iovec_lock;
static struct iovec *iovec_pool[MAX_IOVECS];  /* each ptr is to an array of iovecs */
static int    iovec_used[MAX_IOVECS];  /* non-zero == iovec is in use */

#define NUM_FLUSH_BLOCKS 64    /* size of the iovec array pointed by each ptr */


#define DEFAULT_READ_AHEAD_SIZE  (32 * 1024)
static int read_ahead_size = DEFAULT_READ_AHEAD_SIZE;

/* this array stores the size of each device so we can error check requests */
#define MAX_DEVICES  256
fs_off_t max_device_blocks[MAX_DEVICES];


/* has the time of the last cache access so cache flushing doesn't interfere */
static bigtime_t last_cache_access = 0;


int
beos_init_block_cache(int max_blocks, int flags)
{
    memset(&bc, 0, sizeof(bc));
    memset(iovec_pool, 0, sizeof(iovec_pool));
    memset(iovec_used, 0, sizeof(iovec_used));
    memset(&max_device_blocks, 0, sizeof(max_device_blocks));

    if (init_hash_table(&bc.ht) != 0)
        return ENOMEM;

    bc.lock.s = iovec_lock.s = -1;

    bc.max_blocks = max_blocks;
    bc.flags      = flags;
    if (beos_new_lock(&bc.lock, "bollockcache") != 0)
        goto err;

    if (beos_new_lock(&iovec_lock, "iovec_lock") != 0)
        goto err;

    /* allocate two of these up front so vm won't accidently re-enter itself */
    iovec_pool[0] = (struct iovec *)malloc(sizeof(struct iovec)*NUM_FLUSH_BLOCKS);
    iovec_pool[1] = (struct iovec *)malloc(sizeof(struct iovec)*NUM_FLUSH_BLOCKS);

#ifndef USER
#ifdef DEBUG
    add_debugger_command("bcache", do_dump, "dump the block cache list");
    add_debugger_command("fblock", do_find_block, "find a block in the cache");
    add_debugger_command("fdata",  do_find_data, "find a data block ptr in the cache");
#endif
    register_kernel_daemon(cache_flusher, NULL, 3);
#endif

    return 0;

 err:
    if (bc.lock.s >= 0)
        beos_free_lock(&bc.lock);

    if (iovec_lock.s >= 0)
        beos_free_lock(&iovec_lock);

    shutdown_hash_table(&bc.ht);
    memset((void *)&bc, 0, sizeof(bc));
    return ENOMEM;
}


static struct iovec *
get_iovec_array(void)
{
    int i;
    struct iovec *iov;

    LOCK(iovec_lock);

    for(i = 0; i < MAX_IOVECS; i++) {
        if (iovec_used[i] == 0)
            break;
    }

    if (i >= MAX_IOVECS)       /* uh-oh */
        beos_panic("cache: ran out of iovecs (pool 0x%x, used 0x%x)!\n",
              &iovec_pool[0], &iovec_used[0]);

    if (iovec_pool[i] == NULL) {
        iovec_pool[i] = (struct iovec *)malloc(sizeof(struct iovec)*NUM_FLUSH_BLOCKS);
        if (iovec_pool[i] == NULL)
            beos_panic("can't allocate an iovec!\n");
    }

    iov = iovec_pool[i];
    iovec_used[i] = 1;

    UNLOCK(iovec_lock);

    return iov;
}


static void
release_iovec_array(struct iovec *iov)
{
    int i;

    LOCK(iovec_lock);

    for(i=0; i < MAX_IOVECS; i++) {
        if (iov == iovec_pool[i])
            break;
    }

    if (i < MAX_IOVECS)
        iovec_used[i] = 0;
    else                     /* uh-oh */
        printf("cache: released an iovec I don't own (iov %p)\n", iov);


    UNLOCK(iovec_lock);
}




static void
real_dump_cache_list(cache_ent_list *cel)
{
    cache_ent *ce;

    kprintf("starting from LRU end:\n");

    for (ce = cel->lru; ce; ce = ce->next) {
        kprintf("ce %p dev %2d bnum %6" B_PRIdOFF " lock %d flag %d arg %p "
               "clone %p\n", ce, ce->dev, ce->block_num, ce->lock,
               ce->flags, ce->arg, ce->clone);
    }
    kprintf("MRU end\n");
}

static void
dump_cache_list(void)
{
    kprintf("NORMAL BLOCKS\n");
    real_dump_cache_list(&bc.normal);

    kprintf("LOCKED BLOCKS\n");
    real_dump_cache_list(&bc.locked);

    kprintf("cur blocks %d, max blocks %d ht @ %p\n", bc.cur_blocks,
           bc.max_blocks, &bc.ht);
}

#if 0
static void
check_bcache(char *str)
{
    int count = 0;
    cache_ent *ce, *prev = NULL;

    LOCK(bc.lock);

    for(ce=bc.normal.lru; ce; prev=ce, ce=ce->next) {
        count++;
    }

    for(ce=bc.locked.lru; ce; prev=ce, ce=ce->next) {
        count++;
    }

    if (count != bc.cur_blocks) {
        if (count < bc.cur_blocks - 16)
            beos_panic("%s: count == %d, cur_blocks %d, prev 0x%x\n",
                    str, count, bc.cur_blocks, prev);
        else
            printf("%s: count == %d, cur_blocks %d, prev 0x%x\n",
                    str, count, bc.cur_blocks, prev);
    }

    UNLOCK(bc.lock);
}


static void
dump_lists(void)
{
    cache_ent *nce;

    printf("LOCKED 0x%x  (tail 0x%x, head 0x%x)\n", &bc.locked,
           bc.locked.lru, bc.locked.mru);
    for(nce=bc.locked.lru; nce; nce=nce->next)
        printf("nce @ 0x%x dev %d bnum %ld flags %d lock %d clone 0x%x func 0x%x\n",
               nce, nce->dev, nce->block_num, nce->flags, nce->lock, nce->clone,
               nce->func);

    printf("NORMAL 0x%x  (tail 0x%x, head 0x%x)\n", &bc.normal,
           bc.normal.lru, bc.normal.mru);
    for(nce=bc.normal.lru; nce; nce=nce->next)
        printf("nce @ 0x%x dev %d bnum %ld flags %d lock %d clone 0x%x func 0x%x\n",
               nce, nce->dev, nce->block_num, nce->flags, nce->lock, nce->clone,
               nce->func);
}


static void
check_lists(void)
{
    cache_ent *ce, *prev, *oce;
    cache_ent_list *cel;

    cel = &bc.normal;
    for(ce=cel->lru,prev=NULL; ce; prev=ce, ce=ce->next) {
        for(oce=bc.locked.lru; oce; oce=oce->next) {
            if (oce == ce) {
                dump_lists();
                beos_panic("1:ce @ 0x%x is in two lists(cel 0x%x &LOCKED)\n",ce,cel);
            }
        }
    }
    if (prev && prev != cel->mru) {
        dump_lists();
        beos_panic("*** last element in list != cel mru (ce 0x%x, cel 0x%x)\n",
              prev, cel);
    }

    cel = &bc.locked;
    for(ce=cel->lru,prev=NULL; ce; prev=ce, ce=ce->next) {
        for(oce=bc.normal.lru; oce; oce=oce->next) {
            if (oce == ce) {
                dump_lists();
                beos_panic("3:ce @ 0x%x is in two lists(cel 0x%x & DIRTY)\n",ce,cel);
            }
        }
    }
    if (prev && prev != cel->mru) {
        dump_lists();
        beos_panic("*** last element in list != cel mru (ce 0x%x, cel 0x%x)\n",
              prev, cel);
    }
}
#endif


#ifdef DEBUG
#if 0
static int
do_dump(int argc, char **argv)
{
    dump_cache_list();
    return 1;
}


static int
do_find_block(int argc, char **argv)
{
    int        i;
    fs_off_t  bnum;
    cache_ent *ce;

    if (argc < 2) {
        kprintf("%s: needs a block # argument\n", argv[0]);
        return 1;
    }

    for(i=1; i < argc; i++) {
        bnum = strtoul(argv[i], NULL, 0);

        for(ce=bc.normal.lru; ce; ce=ce->next) {
            if (ce->block_num == bnum) {
                kprintf("found clean bnum %ld @ 0x%lx (data @ 0x%lx)\n",
                        bnum, ce, ce->data);
            }
        }

        for(ce=bc.locked.lru; ce; ce=ce->next) {
            if (ce->block_num == bnum) {
                kprintf("found locked bnum %ld @ 0x%lx (data @ 0x%lx)\n",
                        bnum, ce, ce->data);
            }
        }
    }

    return 0;
}

static int
do_find_data(int argc, char **argv)
{
    int        i;
    void      *data;
    cache_ent *ce;

    if (argc < 2) {
        kprintf("%s: needs a block # argument\n", argv[0]);
        return 1;
    }

    for(i=1; i < argc; i++) {
        data = (void *)strtoul(argv[i], NULL, 0);

        for(ce=bc.normal.lru; ce; ce=ce->next) {
            if (ce->data == data) {
                kprintf("found normal data ptr for bnum %ld @ ce 0x%lx\n",
                        ce->block_num, ce);
            }
        }

        for(ce=bc.locked.lru; ce; ce=ce->next) {
            if (ce->data == data) {
                kprintf("found locked data ptr for bnum %ld @ ce 0x%lx\n",
                        ce->block_num, ce);
            }
        }
    }

    return 0;
}
#endif
#endif /* DEBUG */



/*
  this function detaches the cache_ent from the list.
*/
static void
delete_from_list(cache_ent_list *cel, cache_ent *ce)
{
    if (ce->next)
        ce->next->prev = ce->prev;
    if (ce->prev)
        ce->prev->next = ce->next;

    if (cel->lru == ce)
        cel->lru = ce->next;
    if (cel->mru == ce)
        cel->mru = ce->prev;

    ce->next = NULL;
    ce->prev = NULL;
}



/*
  this function adds the cache_ent ce to the head of the
  list (i.e. the MRU end).  the cache_ent should *not*
  be in any lists.
*/
static void
add_to_head(cache_ent_list *cel, cache_ent *ce)
{
if (ce->next != NULL || ce->prev != NULL) {
    beos_panic("*** ath: ce has non-null next/prev ptr (ce 0x%x nxt 0x%x, prv 0x%x)\n",
           ce, ce->next, ce->prev);
}

    ce->next = NULL;
    ce->prev = cel->mru;

    if (cel->mru)
        cel->mru->next = ce;
    cel->mru = ce;

    if (cel->lru == NULL)
        cel->lru = ce;
}


/*
  this function adds the cache_ent ce to the tail of the
  list (i.e. the MRU end).  the cache_ent should *not*
  be in any lists.
*/
static void
add_to_tail(cache_ent_list *cel, cache_ent *ce)
{
if (ce->next != NULL || ce->prev != NULL) {
    beos_panic("*** att: ce has non-null next/prev ptr (ce 0x%x nxt 0x%x, prv 0x%x)\n",
           ce, ce->next, ce->prev);
}

    ce->next = cel->lru;
    ce->prev = NULL;

    if (cel->lru)
        cel->lru->prev = ce;
    cel->lru = ce;

    if (cel->mru == NULL)
        cel->mru = ce;
}


static int
cache_ent_cmp(const void *a, const void *b)
{
    fs_off_t  diff;
    cache_ent *p1 = *(cache_ent **)a, *p2 = *(cache_ent **)b;

    if (p1 == NULL || p2 == NULL)
        beos_panic("cache_ent pointers are null?!? (a 0x%lx, b 0x%lx\n)\n", a, b);

    if (p1->dev == p2->dev) {
        diff = p1->block_num - p2->block_num;
        return (int)diff;
    } else {
        return p1->dev - p2->dev;
    }
}

#if 0
// ToDo: add this to the fsh (as a background thread)?
static void
cache_flusher(void *arg, int phase)
{
    int    i, num_ents, err;
    bigtime_t now = system_time();
    static cache_ent *ce = NULL;
    static cache_ent *ents[NUM_FLUSH_BLOCKS];

    /*
       if someone else was in the cache recently then just bail out so
       we don't lock them out unnecessarily
    */
    if ((now - last_cache_access) < 1000000)
        return;

    LOCK(bc.lock);

    ce = bc.normal.lru;

    for(num_ents=0; ce && num_ents < NUM_FLUSH_BLOCKS; ce=ce->next) {
        if (ce->flags & CE_BUSY)
            continue;

        if ((ce->flags & CE_DIRTY) == 0 && ce->clone == NULL)
            continue;

        ents[num_ents] = ce;
        ents[num_ents]->flags |= CE_BUSY;
        num_ents++;
    }

    /* if we've got some room left over, look for cloned locked blocks */
    if (num_ents < NUM_FLUSH_BLOCKS) {
        ce = bc.locked.lru;

        for(; num_ents < NUM_FLUSH_BLOCKS;) {
            for(;
                ce && ((ce->flags & CE_BUSY) || ce->clone == NULL);
                ce=ce->next)
                /* skip ents that meet the above criteria */;

            if (ce == NULL)
                break;

            ents[num_ents] = ce;
            ents[num_ents]->flags |= CE_BUSY;
            ce = ce->next;
            num_ents++;
        }
    }

    UNLOCK(bc.lock);

    if (num_ents == 0)
        return;

    qsort(ents, num_ents, sizeof(cache_ent **), cache_ent_cmp);

    if ((err = flush_ents(ents, num_ents)) != 0) {
        printf("flush ents failed (ents @ 0x%lx, num_ents %d!\n",
               (ulong)ents, num_ents);
    }

    for(i=0; i < num_ents; i++) {       /* clear the busy bit on each of ent */
        ents[i]->flags &= ~CE_BUSY;
    }
}
#endif


static int
flush_cache_ent(cache_ent *ce)
{
    int   ret = 0;
    void *data;

    /* if true, then there's nothing to flush */
    if ((ce->flags & CE_DIRTY) == 0 && ce->clone == NULL)
        return 0;

    /* same thing here */
    if (ce->clone == NULL && ce->lock != 0)
        return 0;

 restart:
	if (ce->clone)
		data = ce->clone;
	else
		data = ce->data;

	if (chatty_io > 2) printf("flush: %7" B_PRIdOFF "\n", ce->block_num);
	ret = beos_write_phys_blocks(ce->dev, ce->block_num, data, 1, ce->bsize);

    if (ce->func) {
        ce->func(ce->logged_bnum, 1, ce->arg);
        ce->func = NULL;
    }

    if (ce->clone) {
        free(ce->clone);
        ce->clone = NULL;

        if (ce->lock == 0 && (ce->flags & CE_DIRTY))
            goto restart;     /* also write the real data ptr */
    } else {
        ce->flags &= ~CE_DIRTY;
    }

    return ret;
}


static int
flush_ents(cache_ent **ents, int n_ents)
{
    int    i, j, k, ret = 0, bsize, iocnt, do_again = 0;
    fs_off_t  start_bnum;
    struct iovec *iov;

    iov = get_iovec_array();
    if (iov == NULL)
        return ENOMEM;

restart:
    for(i=0; i < n_ents; i++) {
        /* if true, then there's nothing to flush */
        if ((ents[i]->flags & CE_DIRTY) == 0 && ents[i]->clone == NULL)
            continue;

        /* if true we can't touch the dirty data yet because it's locked */
        if (ents[i]->clone == NULL && ents[i]->lock != 0)
            continue;


        bsize      = ents[i]->bsize;
        start_bnum = ents[i]->block_num;

        for(j=i+1; j < n_ents && (j - i) < NUM_FLUSH_BLOCKS; j++) {
            if (ents[j]->dev != ents[i]->dev ||
                ents[j]->block_num != start_bnum + (j - i))
                break;

            if (ents[j]->clone == NULL && ents[j]->lock != 0)
                break;
        }

        if (j == i+1) {           /* only one block, just flush it directly */
            if ((ret = flush_cache_ent(ents[i])) != 0)
                break;
            continue;
        }


        for(k=i,iocnt=0; k < j; k++,iocnt++) {
            if (ents[k]->clone)
                iov[iocnt].iov_base = ents[k]->clone;
            else
                iov[iocnt].iov_base = ents[k]->data;

            iov[iocnt].iov_len = bsize;
        }

		if (chatty_io)
	        printf("writev @ %" B_PRIdOFF " for %d blocks\n",
                start_bnum, iocnt);

        ret = writev_pos(ents[i]->dev, start_bnum * (fs_off_t)bsize,
                         &iov[0], iocnt);
        if (ret != iocnt*bsize) {
            int idx;

            printf("flush_ents: writev failed: "
                "iocnt %d start bnum %" B_PRIdOFF " bsize %d, ret %d\n",
                iocnt, start_bnum, bsize, ret);

            for(idx=0; idx < iocnt; idx++)
                printf("iov[%2d] = %p :: %ld\n", idx, iov[idx].iov_base,
                       iov[idx].iov_len);

            printf("error %s writing blocks %" B_PRIdOFF ":%d (%d != %d)\n",
                   strerror(errno), start_bnum, iocnt, ret, iocnt*bsize);
            ret = EINVAL;
            break;
        }
        ret = 0;


        for(k=i; k < j; k++) {
            if (ents[k]->func) {
                ents[k]->func(ents[k]->logged_bnum, 1, ents[k]->arg);
                ents[k]->func = NULL;
            }

            if (ents[k]->clone) {
                free(ents[k]->clone);
                ents[k]->clone = NULL;
            } else {
                ents[k]->flags &= ~CE_DIRTY;
            }
        }


        i = j - 1;  /* i gets incremented by the outer for loop */
    }

    /*
       here we have to go back through and flush any blocks that are
       still dirty.  with an arched brow you astutely ask, "but how
       could this happen given the above loop?"  Ahhh young grasshopper
       I say, the path through the cache is long and twisty and fraught
       with peril.  The reason it can happen is that a block can be both
       cloned and dirty.  The above loop would only flush the cloned half
       of the data, not the main dirty block.  So we have to go back
       through and see if there are any blocks that are still dirty.  If
       there are we go back to the top of the function and do the whole
       thing over.  Kind of grody but it is necessary to insure the
       correctness of the log for the Be file system.
    */
    if (do_again == 0) {
        for(i=0; i < n_ents; i++) {
            if ((ents[i]->flags & CE_DIRTY) == 0 || ents[i]->lock)
                continue;

            do_again = 1;
            break;
        }

        if (do_again)
            goto restart;
    }

    release_iovec_array(iov);


    return ret;
}

static void
delete_cache_list(cache_ent_list *cel)
{
    void      *junk;
    cache_ent *ce, *next;

    for (ce = cel->lru; ce; ce = next) {
        next = ce->next;
        if (ce->lock != 0) {
            if (ce->func)
                printf("*** shutdown_block_cache: "
                    "block %" B_PRIdOFF ", lock == %d (arg %p)!\n",
                    ce->block_num, ce->lock, ce->arg);
            else
                printf("*** shutdown_block_cache: "
                    "block %" B_PRIdOFF ", lock == %d!\n",
                    ce->block_num, ce->lock);
        }

        if (ce->flags & CE_BUSY) {
            printf("* shutdown block cache: "
                "bnum %" B_PRIdOFF " is busy? ce %p\n", ce->block_num, ce);
        }

        if ((ce->flags & CE_DIRTY) || ce->clone) {
            flush_cache_ent(ce);
        }

        if (ce->clone)
            free(ce->clone);
        ce->clone = NULL;

        if (ce->data)
            free(ce->data);
        ce->data = NULL;

        if ((junk = hash_delete(&bc.ht, ce->dev, ce->block_num)) != ce) {
            printf("*** free_device_cache: "
                "bad hash table entry %" B_PRIdOFF " %p != %p\n",
                ce->block_num, junk, ce);
        }

        explicit_bzero(ce, sizeof(*ce));
        free(ce);

        bc.cur_blocks--;
    }
}


void
beos_shutdown_block_cache(void)
{
    /* print_hash_stats(&bc.ht); */

    if (bc.lock.s > 0)
        LOCK(bc.lock);

#ifndef USER
    unregister_kernel_daemon(cache_flusher, NULL);
#endif

    delete_cache_list(&bc.normal);
    delete_cache_list(&bc.locked);

    bc.normal.lru = bc.normal.mru = NULL;
    bc.locked.lru = bc.locked.mru = NULL;

    shutdown_hash_table(&bc.ht);

    if (bc.lock.s > 0)
        beos_free_lock(&bc.lock);
    bc.lock.s = -1;

    if (iovec_lock.s >= 0)
        beos_free_lock(&iovec_lock);
}



int
beos_init_cache_for_device(int fd, fs_off_t max_blocks)
{
    int ret = 0;

    if (fd >= MAX_DEVICES)
        return -1;

    LOCK(bc.lock);

    if (max_device_blocks[fd] != 0) {
        printf("device %d is already initialized!\n", fd);
        ret = -1;
    } else {
        max_device_blocks[fd] = max_blocks;
    }

    UNLOCK(bc.lock);

    return ret;
}



/*
   this routine assumes that bc.lock has been acquired
*/
static cache_ent *
block_lookup(int dev, fs_off_t bnum)
{
    int        count = 0;
    cache_ent *ce;

    while (1) {
        ce = hash_lookup(&bc.ht, dev, bnum);
        if (ce == NULL)
            return NULL;

        if ((ce->flags & CE_BUSY) == 0) /* it's ok, break out and return it */
            break;

        /* else, it's busy and we need to retry our lookup */
        UNLOCK(bc.lock);

        snooze(5000);
        if (count++ == 5000) {  /* then a lot of time has elapsed */
            printf("block %" B_PRIdOFF " isn't coming un-busy (ce @ %p)\n",
                   ce->block_num, ce);
        }

        LOCK(bc.lock);
    }

    if (ce->flags & CE_BUSY)
        beos_panic("block lookup: returning a busy block @ 0x%lx?!?\n",(ulong)ce);

    return ce;
}



int
beos_set_blocks_info(int dev, fs_off_t *blocks, int nblocks,
               void (*func)(fs_off_t bnum, size_t nblocks, void *arg), void *arg)
{
    int        i, j, cur;
    cache_ent *ce;
    cache_ent *ents[NUM_FLUSH_BLOCKS];

    LOCK(bc.lock);


    for(i=0, cur=0; i < nblocks; i++) {

        /* printf("sbi:   %ld (arg 0x%x)\n", blocks[i], arg); */
        ce = block_lookup(dev, blocks[i]);
        if (ce == NULL) {
            beos_panic("*** set_block_info can't find bnum %ld!\n", blocks[i]);
            UNLOCK(bc.lock);
            return ENOENT;   /* hopefully this doesn't happen... */
        }


        if (blocks[i] != ce->block_num || dev != ce->dev) {
            UNLOCK(bc.lock);
            beos_panic("** error1: looked up dev %d block %ld but found dev %d "
                    "bnum %ld\n", dev, blocks[i], ce->dev, ce->block_num);
            return EBADF;
        }

        if (ce->lock == 0) {
            beos_panic("* set_block_info on bnum %ld (%d) but it's not locked!\n",
                    blocks[i], nblocks);
        }


        if ((ce->flags & CE_DIRTY) == 0) {
            beos_panic("*** set_block_info on non-dirty block bnum %ld (%d)!\n",
                    blocks[i], nblocks);
        }

        ce->flags |= CE_BUSY;     /* mark all blocks as busy till we're done */

        /* if there is cloned data, it needs to be flushed now */
        if (ce->clone && ce->func) {
            ents[cur++] = ce;

            if (cur >= NUM_FLUSH_BLOCKS) {
                UNLOCK(bc.lock);

                qsort(ents, cur, sizeof(cache_ent **), cache_ent_cmp);

                flush_ents(ents, cur);

                LOCK(bc.lock);
                for(j=0; j < cur; j++)
                    ents[j]->flags &= ~CE_BUSY;
                cur = 0;
            }
        }
    }


    if (cur != 0) {
        UNLOCK(bc.lock);

        qsort(ents, cur, sizeof(cache_ent **), cache_ent_cmp);

        flush_ents(ents, cur);

        LOCK(bc.lock);
        for(j=0; j < cur; j++)
            ents[j]->flags &= ~CE_BUSY;
        cur = 0;
    }


    /* now go through and set the info that we were asked to */
    for (i = 0; i < nblocks; i++) {
        /* we can call hash_lookup() here because we know it's around */
        ce = hash_lookup(&bc.ht, dev, blocks[i]);
        if (ce == NULL) {
            beos_panic("*** set_block_info can't find bnum %Ld!\n", blocks[i]);
            UNLOCK(bc.lock);
            return ENOENT;   /* hopefully this doesn't happen... */
        }

        ce->flags &= ~(CE_DIRTY | CE_BUSY);

        if (ce->func != NULL) {
            beos_panic("*** set_block_info non-null callback on bnum %Ld\n",
                    ce->block_num);
        }

        if (ce->clone != NULL) {
            beos_panic("*** ce->clone == %p, not NULL in set_block_info\n",
                    ce->clone);
        }

        ce->clone = (void *)malloc(ce->bsize);
        if (ce->clone == NULL)
            beos_panic("*** can't clone bnum %Ld (bsize %d)\n",
                    ce->block_num, ce->bsize);


        memcpy(ce->clone, ce->data, ce->bsize);

        ce->func   = func;
        ce->arg    = arg;

        ce->logged_bnum = blocks[i];

        ce->lock--;
        if (ce->lock < 0) {
            printf("sbi: "
                "whoa nellie! ce @ %p (%" B_PRIdOFF ") has lock == %d\n",
                   ce, ce->block_num, ce->lock);
        }

        if (ce->lock == 0) {
            delete_from_list(&bc.locked, ce);
            add_to_head(&bc.normal, ce);
        }
    }

    UNLOCK(bc.lock);

    return 0;
}


/* this function is only for use by flush_device() */
static void
do_flush(cache_ent **ents, int max)
{
    int i;

    for(i=0; i < max; i++) {
        ents[i]->flags |= CE_BUSY;
    }

    UNLOCK(bc.lock);

    qsort(ents, max, sizeof(cache_ent **), cache_ent_cmp);
    flush_ents(ents, max);

    LOCK(bc.lock);
    for(i=0; i < max; i++) {
        ents[i]->flags &= ~CE_BUSY;
    }
}

int
beos_flush_device(int dev, int warn_locked)
{
    int cur;
    cache_ent *ce;
    cache_ent *ents[NUM_FLUSH_BLOCKS];

    LOCK(bc.lock);

    cur = 0;
    ce = bc.normal.lru;
    while (ce) {
        if (ce->dev != dev || (ce->flags & CE_BUSY)) {
            ce = ce->next;
            continue;
        }

        if ((ce->flags & CE_DIRTY) || ce->clone) {
            ents[cur++] = ce;
            if (cur >= NUM_FLUSH_BLOCKS) {
                do_flush(ents, cur);

                ce = bc.normal.lru;
                cur = 0;
                continue;
            }
        }

        ce = ce->next;
    }

    if (cur != 0)
        do_flush(ents, cur);

    cur = 0;
    ce = bc.locked.lru;
    while (ce) {
        if (ce->dev != dev || (ce->flags & CE_BUSY)) {
            ce = ce->next;
            continue;
        }

        if (ce->clone) {
            ents[cur++] = ce;
            if (cur >= NUM_FLUSH_BLOCKS) {
                do_flush(ents, cur);

                ce = bc.locked.lru;
                cur = 0;
                continue;
            }
        }

        ce = ce->next;
    }

    if (cur != 0)
        do_flush(ents, cur);

    UNLOCK(bc.lock);

    return 0;
}


static void
real_remove_cached_blocks(int dev, int allow_writes, cache_ent_list *cel)
{
    void      *junk;
    cache_ent *ce, *next = NULL;

    for(ce=cel->lru; ce; ce=next) {
        next = ce->next;

        if (ce->dev != dev) {
            continue;
        }

        if (ce->lock != 0 || (ce->flags & CE_BUSY)) {
            printf("*** remove_cached_dev: "
                "block %" B_PRIdOFF " has lock = %d, flags 0x%x! ce @ 0x%lx\n",
                ce->block_num, ce->lock, ce->flags, (ulong)ce);
        }

        if (allow_writes == ALLOW_WRITES &&
            ((ce->flags & CE_DIRTY) || ce->clone)) {
            ce->flags |= CE_BUSY;
            flush_cache_ent(ce);
            ce->flags &= ~CE_BUSY;
        }

        /* unlink this guy */
        if (cel->lru == ce)
            cel->lru = ce->next;

        if (cel->mru == ce)
            cel->mru = ce->prev;

        if (ce->prev)
            ce->prev->next = ce->next;
        if (ce->next)
            ce->next->prev = ce->prev;

        if (ce->clone)
            free(ce->clone);
        ce->clone = NULL;

        if (ce->data)
            free(ce->data);
        ce->data = NULL;

        if ((junk = hash_delete(&bc.ht, ce->dev, ce->block_num)) != ce) {
            beos_panic("*** remove_cached_device: bad hash table entry %ld "
                   "0x%lx != 0x%lx\n", ce->block_num, (ulong)junk, (ulong)ce);
        }

        free(ce);

        bc.cur_blocks--;
    }

}

int
beos_remove_cached_device_blocks(int dev, int allow_writes)
{
    LOCK(bc.lock);

    real_remove_cached_blocks(dev, allow_writes, &bc.normal);
    real_remove_cached_blocks(dev, allow_writes, &bc.locked);

    max_device_blocks[dev] = 0;

    UNLOCK(bc.lock);

    return 0;
}



int
beos_flush_blocks(int dev, fs_off_t bnum, int nblocks)
{
    int        cur, i;
    cache_ent *ce;
    cache_ent *ents[NUM_FLUSH_BLOCKS];

    if (nblocks == 0)   /* might as well check for this */
        return 0;

    LOCK(bc.lock);

    cur = 0;
    for(; nblocks > 0; nblocks--, bnum++) {
        ce = block_lookup(dev, bnum);
        if (ce == NULL)
            continue;

        if (bnum != ce->block_num || dev != ce->dev) {
            UNLOCK(bc.lock);
            beos_panic("error2: looked up dev %d block %ld but found %d %ld\n",
                  dev, bnum, ce->dev, ce->block_num);
            return EBADF;
        }

        if ((ce->flags & CE_DIRTY) == 0 && ce->clone == NULL)
            continue;

        ce->flags |= CE_BUSY;
        ents[cur++] = ce;

        if (cur >= NUM_FLUSH_BLOCKS) {
            UNLOCK(bc.lock);

            qsort(ents, cur, sizeof(cache_ent **), cache_ent_cmp);
            flush_ents(ents, cur);

            LOCK(bc.lock);
            for(i=0; i < cur; i++) {
                ents[i]->flags &= ~CE_BUSY;
            }
            cur = 0;
        }
    }

    UNLOCK(bc.lock);

    if (cur == 0)     /* nothing more to do */
        return 0;

    /* flush out the last few buggers */
    qsort(ents, cur, sizeof(cache_ent **), cache_ent_cmp);
    flush_ents(ents, cur);

    for(i=0; i < cur; i++) {
        ents[i]->flags &= ~CE_BUSY;
    }

    return 0;
}


int
beos_mark_blocks_dirty(int dev, fs_off_t bnum, int nblocks)
{
    int        ret = 0;
    cache_ent *ce;

    LOCK(bc.lock);

    while(nblocks > 0) {
        ce = block_lookup(dev, bnum);
        if (ce) {
            ce->flags |= CE_DIRTY;
            bnum      += 1;
            nblocks   -= 1;
        } else {     /* hmmm, that's odd, didn't find it */
            printf("** mark_blocks_diry couldn't find block %" B_PRIdOFF " "
                "(len %d)\n", bnum, nblocks);
            ret = ENOENT;
            break;
        }
    }

    UNLOCK(bc.lock);

    return ret;
}



int
beos_release_block(int dev, fs_off_t bnum)
{
    cache_ent *ce;

    /* printf("rlsb: %ld\n", bnum); */
    LOCK(bc.lock);

    ce = block_lookup(dev, bnum);
    if (ce) {
        if (bnum != ce->block_num || dev != ce->dev) {
            beos_panic("*** error3: looked up dev %d block %ld but found %d %ld\n",
                    dev, bnum, ce->dev, ce->block_num);
            UNLOCK(bc.lock);
            return EBADF;
        }

        ce->lock--;

        if (ce->lock < 0) {
            printf("rlsb: whoa nellie! ce %" B_PRIdOFF " has lock == %d\n",
                   ce->block_num, ce->lock);
        }

        if (ce->lock == 0) {
            delete_from_list(&bc.locked, ce);
            add_to_head(&bc.normal, ce);
        }

    } else {     /* hmmm, that's odd, didn't find it */
        beos_panic("** release_block asked to find %ld but it's not here\n",
               bnum);
    }

    UNLOCK(bc.lock);

    return 0;
}


static cache_ent *
new_cache_ent(int bsize)
{
    cache_ent *ce;

    ce = (cache_ent *)calloc(1, sizeof(cache_ent));
    if (ce == NULL) {
        beos_panic("*** error: cache can't allocate memory!\n");
        return NULL;
    }

    ce->data = malloc(bsize);
    if (ce->data == NULL) {
        free(ce);
        beos_panic("** error cache can't allocate data memory\n");
        UNLOCK(bc.lock);
        return NULL;
    }

    ce->dev       = -1;
    ce->block_num = -1;

    return ce;
}


static void
get_ents(cache_ent **ents, int num_needed, int max, int *num_gotten, int bsize)
{
    int        cur, retry_counter = 0, max_retry = num_needed * 256;
    cache_ent *ce;

    if (num_needed > max)
        beos_panic("get_ents: num_needed %d but max %d (doh!)\n", num_needed, max);

    /* if the cache isn't full yet, just allocate the blocks */
    for(cur=0; bc.cur_blocks < bc.max_blocks && cur < num_needed; cur++) {
        ents[cur] = new_cache_ent(bsize);
        if (ents[cur] == NULL)
            break;
        bc.cur_blocks++;
    }

    /* pluck off blocks from the LRU end of the normal list, keep trying too */
    while(cur < num_needed && retry_counter < max_retry) {
        for(ce=bc.normal.lru; ce && cur < num_needed; ce=ce->next) {
            if (ce->lock)
                beos_panic("get_ents: normal list has locked blocks (ce 0x%x)\n",ce);

            if (ce->flags & CE_BUSY)   /* don't touch busy blocks */
                continue;

            ce->flags   |= CE_BUSY;
            ents[cur++]  = ce;
        }

        if (cur < num_needed) {
            UNLOCK(bc.lock);
            snooze(10000);
            LOCK(bc.lock);
            retry_counter++;
        }
    }

    if (cur < num_needed && retry_counter >= max_retry) {  /* oh shit! */
        dump_cache_list();
        UNLOCK(bc.lock);
        beos_panic("get_ents: waited too long; can't get enough ce's (c %d n %d)\n",
              cur, num_needed);
    }

    /*
      If the last block is a dirty one, try to get more of 'em so
      that we can flush a bunch of blocks at once.
    */
    if (cur && cur < max &&
        ((ents[cur-1]->flags & CE_DIRTY) || ents[cur-1]->clone)) {

        for(ce=ents[cur-1]->next; ce && cur < max; ce=ce->next) {
            if (ce->flags & CE_BUSY)   /* don't touch busy blocks */
                continue;

            if (ce->lock)
                beos_panic("get_ents:2 dirty list has locked blocks (ce 0x%x)\n",ce);

            ce->flags   |= CE_BUSY;
            ents[cur++]  = ce;
        }
    }

    *num_gotten = cur;
}


static int
read_into_ents(int dev, fs_off_t bnum, cache_ent **ents, int num, int bsize)
{
    int    i, ret;
    struct iovec *iov;

    iov = get_iovec_array();

    for (i = 0; i < num; i++) {
        iov[i].iov_base = ents[i]->data;
        iov[i].iov_len  = bsize;
    }

	if (chatty_io > 2)
		printf("readv @ %" B_PRIdOFF " for %d blocks "
			"(at %" B_PRIdOFF ", block_size = %d)\n",
			bnum, num, bnum*bsize, bsize);
    ret = readv_pos(dev, bnum*bsize, iov, num);

    release_iovec_array(iov);

    if (ret != num*bsize) {
        printf("read_into_ents: asked to read %d bytes but got %d\n",
               num*bsize, ret);
        printf("*** iov @ %p (num %d)\n", iov, num);
        return EINVAL;
    } else
        return 0;
}



#define CACHE_READ          0x0001
#define CACHE_WRITE         0x0002
#define CACHE_NOOP          0x0004     /* for getting empty blocks */
#define CACHE_LOCKED        0x0008
#define CACHE_READ_AHEAD_OK 0x0010     /* it's ok to do read-ahead */


static char *
op_to_str(int op)
{
    static char buff[128];

    if (op & CACHE_READ)
        strcpy(buff, "READ");
    else if (op & CACHE_WRITE)
        strcpy(buff, "WRITE");
    else if (op & CACHE_NOOP)
        strcpy(buff, "NOP");

    if (op & CACHE_LOCKED)
        strcat(buff, " LOCKED");

    if (op & CACHE_READ_AHEAD_OK)
        strcat(buff, " (AHEAD)");

    return buff;
}

static int
cache_block_io(int dev, fs_off_t bnum, void *data, fs_off_t num_blocks, int bsize,
               int op, void **dataptr)
{
    size_t          err = 0;
    cache_ent      *ce;
    cache_ent_list *cel;

    if (chatty_io > 1)
        printf("cbio: bnum = %" B_PRIdOFF ", num_blocks = %" B_PRIdOFF ", "
            "bsize = %d, op = %s\n", bnum, num_blocks, bsize, op_to_str(op));

    /* some sanity checks first */
    if (bsize == 0)
        beos_panic("cache_io: block size == 0 for bnum %Ld?!?\n", bnum);

    if (num_blocks == 0)
        beos_panic("cache_io: bnum %Ld has num_blocks == 0!\n", bnum);

    if (data == NULL && dataptr == NULL) {
        printf("major butthead move: "
            "null data and dataptr! bnum %" B_PRIdOFF ":%" B_PRIdOFF "\n",
            bnum, num_blocks);
        return ENOMEM;
    }

    if (data == NULL) {
        if (num_blocks != 1)    /* get_block() should never do that */
            beos_panic("cache_io: num_blocks %Ld but should be 1\n",
                  num_blocks);

        if (op & CACHE_WRITE)
            beos_panic("cache_io: get_block() asked to write?!?\n");
    }

    if (bnum + num_blocks > max_device_blocks[dev]) {
        printf("dev %d: "
            "access to blocks %" B_PRIdOFF ":%" B_PRIdOFF " "
            "but max_dev_blocks is %" B_PRIdOFF "\n",
            dev, bnum, num_blocks, max_device_blocks[dev]);

		// let the app crash here
		*(int *)0x3100 = 0xc0debabe;
        return EINVAL;
    }

    last_cache_access = system_time();

    /* if the i/o is greater than 64k, do it directly */
    if (num_blocks * bsize >= 64 * 1024) {
        char  *ptr;
        fs_off_t  tmp;

        if (data == NULL || (op & CACHE_LOCKED)) {
            beos_panic("*** asked to do a large locked io that's too hard!\n");
        }


        if (op & CACHE_READ) {
            if (beos_read_phys_blocks(dev, bnum, data, num_blocks, bsize) != 0) {
                printf("cache read:read_phys_blocks failed "
                    "(%s on blocks %" B_PRIdOFF ":%" B_PRIdOFF ")!\n",
                     strerror(errno), bnum, num_blocks);
                return EINVAL;
            }

            LOCK(bc.lock);

            /* if any of the blocks are in the cache, grab them instead */
            ptr = data;
            for(tmp=bnum; tmp < bnum+num_blocks; tmp++, ptr+=bsize) {
                ce = block_lookup(dev, tmp);
                /*
                    if we find a block in the cache we have to copy its
                    data just in case it is more recent than what we just
                    read from disk (which could happen if someone wrote
                    these blocks after we did the read but before we locked
                    the cache and entered this loop).
                */
                if (ce) {
                    if (tmp != ce->block_num || dev != ce->dev) {
                        UNLOCK(bc.lock);
                        beos_panic("*** error4: looked up dev %d block %Ld but "
                                "found %d %Ld\n", dev, tmp, ce->dev,
                                ce->block_num);
                    }

                    memcpy(ptr, ce->data, bsize);
                }
            }

            UNLOCK(bc.lock);
        } else if (op & CACHE_WRITE) {
            LOCK(bc.lock);

            /* if any of the blocks are in the cache, update them too */
            ptr = data;
            for(tmp=bnum; tmp < bnum+num_blocks; tmp++, ptr+=bsize) {
                ce = block_lookup(dev, tmp);
                if (ce) {
                    if (tmp != ce->block_num || dev != ce->dev) {
                        UNLOCK(bc.lock);
                        beos_panic("*** error5: looked up dev %d block %Ld but "
                                "found %d %Ld\n", dev, tmp, ce->dev,
                                ce->block_num);
                        return EBADF;
                    }

                    /* XXXdbg -- this isn't strictly necessary */
                    if (ce->clone) {
                        printf("over-writing cloned data "
                            "(ce %p bnum %" B_PRIdOFF ")...\n", ce, tmp);
                        flush_cache_ent(ce);
                    }

                    /* copy the data into the cache */
                    memcpy(ce->data, ptr, bsize);
                }
            }

            UNLOCK(bc.lock);

            if (beos_write_phys_blocks(dev, bnum, data, num_blocks, bsize) != 0) {
                printf("cache write: write_phys_blocks failed (%s on blocks "
                    "%" B_PRIdOFF ":%" B_PRIdOFF ")!\n",
                    strerror(errno), bnum, num_blocks);
                return EINVAL;
            }
        } else {
            printf("bad cache op %d "
                "(bnum %" B_PRIdOFF " nblocks %" B_PRIdOFF ")\n",
                op, bnum, num_blocks);
            return EINVAL;
        }

        return 0;
    }


    LOCK(bc.lock);
    while(num_blocks) {

        ce = block_lookup(dev, bnum);
        if (ce) {
            if (bnum != ce->block_num || dev != ce->dev) {
                UNLOCK(bc.lock);
                beos_panic("*** error6: looked up dev %d block %ld but found "
                        "%d %ld\n", dev, bnum, ce->dev, ce->block_num);
                return EBADF;
            }

            if (bsize != ce->bsize) {
                beos_panic("*** requested bsize %d but ce->bsize %d ce @ 0x%x\n",
                        bsize, ce->bsize, ce);
            }

            /* delete this ent from the list it is in because it may change */
            if (ce->lock)
                cel = &bc.locked;
            else
                cel = &bc.normal;

            delete_from_list(cel, ce);

            if (op & CACHE_READ) {
                if (data && data != ce->data) {
                    memcpy(data, ce->data, bsize);
                } else if (dataptr) {
                    *dataptr = ce->data;
                } else {
                    printf("cbio:data %p dptr %p ce @ %p ce->data %p\n",
                           data, dataptr, ce, ce->data);
                }
            } else if (op & CACHE_WRITE) {
                if (data && data != ce->data)
                    memcpy(ce->data, data, bsize);

                ce->flags |= CE_DIRTY;
            } else if (op & CACHE_NOOP) {
                memset(ce->data, 0, bsize);
                if (data)
                    memset(data, 0, bsize);

                if (dataptr)
                    *dataptr = ce->data;

                ce->flags |= CE_DIRTY;
            } else {
                beos_panic("cached_block_io: bogus op %d\n", op);
            }

            if (op & CACHE_LOCKED)
                ce->lock++;

            if (ce->lock)
                cel = &bc.locked;
            else
                cel = &bc.normal;

            /* now put this ent at the head of the appropriate list */
            add_to_head(cel, ce);

            if (data != NULL)
                data = (void *)((char *)data + bsize);

            bnum       += 1;
            num_blocks -= 1;

            continue;
        } else {                                  /* it's not in the cache */
            int        cur, cur_nblocks, num_dirty, real_nblocks, num_needed;
            cache_ent *ents[NUM_FLUSH_BLOCKS];

            /*
               here we find out how many additional blocks in this request
               are not in the cache.  the idea is that then we can do one
               big i/o on that many blocks at once.
            */
            for(cur_nblocks=1;
                cur_nblocks < num_blocks && cur_nblocks < NUM_FLUSH_BLOCKS;
                cur_nblocks++) {

                /* we can call hash_lookup() directly instead of
                   block_lookup() because we don't care about the
                   state of the busy bit of the block at this point
                */
                if (hash_lookup(&bc.ht, dev, bnum + cur_nblocks))
                    break;
            }

            /*
              here we try to figure out how many extra blocks we should read
              for read-ahead.  we want to read as many as possible that are
              not already in the cache and that don't cause us to try and
              read beyond the end of the disk.
            */
            if ((op & CACHE_READ) && (op & CACHE_READ_AHEAD_OK) &&
                (cur_nblocks * bsize) < read_ahead_size) {

                for(num_needed=cur_nblocks;
                    num_needed < (read_ahead_size / bsize);
                    num_needed++) {

                    if ((bnum + num_needed) >= max_device_blocks[dev])
                        break;

                    if (hash_lookup(&bc.ht, dev, bnum + num_needed))
                        break;
                }
            } else {
                num_needed = cur_nblocks;
            }

            /* this will get us pointers to a bunch of cache_ents we can use */
            get_ents(ents, num_needed, NUM_FLUSH_BLOCKS, &real_nblocks, bsize);

            if (real_nblocks < num_needed) {
                beos_panic("don't have enough cache ents (need %d got %d %ld::%d)\n",
                      num_needed, real_nblocks, bnum, num_blocks);
            }

            /*
              There are now three variables used as limits within the ents
              array.  This is how they are related:

                 cur_nblocks <= num_needed <= real_nblocks

              Ents from 0 to cur_nblocks-1 are going to be used to fulfill
              this IO request.  Ents from cur_nblocks to num_needed-1 are
              for read-ahead.  Ents from num_needed to real_nblocks are
              extra blocks that get_ents() asked us to flush.  Often (and
              always on writes) cur_nblocks == num_needed.

              Below, we sort the list of ents so that when we flush them
              they go out in order.
            */

            qsort(ents, real_nblocks, sizeof(cache_ent **), cache_ent_cmp);

            /*
              delete each ent from its list because it will change.  also
              count up how many dirty blocks there are and insert into the
              hash table any new blocks so that no one else will try to
              read them in when we release the cache semaphore to do our I/O.
            */
            for(cur=0,num_dirty=0; cur < real_nblocks; cur++) {
                ce = ents[cur];
                ce->flags |= CE_BUSY;

                /*
                   insert the new block into the hash table with its new block
                   number. note that the block is still in the hash table for
                   its old block number -- and it has to be until we are done
                   flushing it from the cache (to prevent someone else from
                   sneaking in in front of us and trying to read the same
                   block that we're flushing).
                */
                if (cur < num_needed) {
                    if (hash_insert(&bc.ht, dev, bnum + cur, ce) != 0)
                        beos_panic("could not insert cache ent for %d %ld (0x%lx)\n",
                              dev, bnum + cur, (ulong)ents[cur]);
                }

                if (ce->dev == -1)
                    continue;

                if ((ce->flags & CE_DIRTY) || ce->clone)
                    num_dirty++;

                if (ce->lock) {
                    beos_panic("cbio: can't use locked blocks here ce @ 0x%x\n",ce);
                } else {
                    cel = &bc.normal;
	                delete_from_list(cel, ce);
				}
            }
            ce = NULL;


            /*
               we release the block cache semaphore here so that we can
               go do all the i/o we need to do (flushing dirty blocks
               that we're kicking out as well as reading any new data).

               because all the blocks we're touching are marked busy
               no one else should mess with them while we're doing this.
            */
            if (num_dirty || (op & CACHE_READ)) {
                UNLOCK(bc.lock);

                /* this flushes any blocks we're kicking out that are dirty */
                if (num_dirty && (err = flush_ents(ents, real_nblocks)) != 0) {
                    printf("flush ents failed (ents @ 0x%lx, nblocks %d!\n",
                           (ulong)ents, cur_nblocks);
                    goto handle_err;
                }

            }

            /*
               now that everything is flushed to disk, go through and
               make sure that the data blocks we're going to use are
               the right block size for this current request (it's
               possible we're kicking out some smaller blocks and need
               to reallocate the data block pointer). We do this in two
               steps, first free'ing everything and then going through
               and doing the malloc's to try and be nice to the memory
               system (i.e. allow it to coalesce stuff, etc).
            */
            err = 0;
            for(cur=0; cur < num_needed; cur++) {
                if (ents[cur]->bsize != bsize) {
                    free(ents[cur]->data);
                    ents[cur]->data = NULL;

                    if (ents[cur]->clone) {
                        free(ents[cur]->clone);
                        ents[cur]->clone = NULL;
                    }
                }
            }

            for(cur=0; cur < num_needed; cur++) {
                if (ents[cur]->data == NULL) {
                    ents[cur]->data  = (void *)malloc(bsize);
                    ents[cur]->bsize = bsize;
                }

                if (ents[cur]->data == NULL) {
                    printf("cache: no memory for block (bsize %d)!\n",
                           bsize);
                    err = ENOMEM;
                    break;
                }
            }

            /*
               if this condition is true it's a pretty serious error.
               we'll try and back out gracefully but we're in pretty
               deep at this point and it ain't going to be easy.
            */
  handle_err:
            if (err) {
                for(cur=0; cur < num_needed; cur++) {
                    cache_ent *tmp_ce;

                    tmp_ce = (cache_ent *)hash_delete(&bc.ht,dev,bnum+cur);
                    if (tmp_ce != ents[cur]) {
                        beos_panic("hash_del0: %d %ld got 0x%lx, not 0x%lx\n",
                                dev, bnum+cur, (ulong)tmp_ce,
                                (ulong)ents[cur]);
                    }

                    tmp_ce = (cache_ent *)hash_delete(&bc.ht,ents[cur]->dev,
                                                        ents[cur]->block_num);
                    if (tmp_ce != ents[cur]) {
                        beos_panic("hash_del1: %d %ld got 0x%lx, not 0x%lx\n",
                                ents[cur]->dev, ents[cur]->block_num, (ulong)tmp_ce,
                                (ulong)ents[cur]);
                    }

                    ents[cur]->flags &= ~CE_BUSY;
                    if (ents[cur]->data)
                        free(ents[cur]->data);
                    free(ents[cur]);
                    ents[cur] = NULL;

                    bc.cur_blocks--;
                }

                if (cur < real_nblocks) {
                    LOCK(bc.lock);
                    for(; cur < real_nblocks; cur++) {
                        ents[cur]->flags &= ~CE_BUSY;

                        /* we have to put them back here */
                        add_to_tail(&bc.normal, ents[cur]);
                    }
                    UNLOCK(bc.lock);
                }

                return ENOMEM;
            }


            /*
               If we go into this if statement, the block cache lock
               has *already been released* up above when we flushed the
               dirty entries.  As always, since the blocks we're mucking
               with are marked busy, they shouldn't get messed with.
            */
            err = 0;
            if (num_dirty || (op & CACHE_READ)) {
                /* this section performs the i/o that we need to do */
                if (op & CACHE_READ) {
                    err = read_into_ents(dev, bnum, ents, num_needed, bsize);
                } else {
                    err = 0;
                }

                if (err != 0) {
                    printf("err %s on dev %d block %" B_PRIdOFF ":%d (%d) "
                           "data %p, ents[0] %p\n",
                           strerror(errno), dev, bnum, cur_nblocks,
                           bsize, data, ents[0]);
                }

                /*
                   acquire the semaphore here so that we can go on mucking
                   with the cache data structures.  We need to delete old
                   block numbers from the hash table and set the new block
                   number's for the blocks we just read in.  We also put the
                   read-ahead blocks at the head of mru list.
                */

                LOCK(bc.lock);
            }

            for(cur=0; cur < num_needed; cur++) {
                cache_ent *tmp_ce;

                ce = ents[cur];
                if (ce->dev != -1) {
                    tmp_ce = hash_delete(&bc.ht, ce->dev, ce->block_num);
                    if (tmp_ce == NULL || tmp_ce != ce) {
                        beos_panic("*** hash_delete failure (ce 0x%x tce 0x%x)\n",
                              ce, tmp_ce);
                    }
                }

                if (err == 0 && cur >= cur_nblocks) {
                    ce->dev       = dev;
                    ce->block_num = bnum + cur;
                    ce->flags    &= ~CE_BUSY;
                    add_to_head(&bc.normal, ce);
                }
            }
            ce = NULL;

            /*
              clear the busy bit on the blocks we force-flushed and
              put them on the normal list since they're now clean.
            */
            for(; cur < real_nblocks; cur++) {
                ents[cur]->flags &= ~CE_BUSY;

                if (ents[cur]->lock)
                    beos_panic("should not have locked blocks here (ce 0x%x)\n",
                          ents[cur]);

                add_to_tail(&bc.normal, ents[cur]);
            }

            if (err) {   /* then we have some cleanup to do */
                for(cur=0; cur < num_needed; cur++) {
                    cache_ent *tmp_ce;

                    /* we delete all blocks from the cache so we don't
                       leave partially written blocks in the cache */

                    tmp_ce = (cache_ent *)hash_delete(&bc.ht,dev,bnum+cur);
                    if (tmp_ce != ents[cur]) {
                        beos_panic("hash_del: %d %ld got 0x%lx, not 0x%lx\n",
                                dev, bnum+cur, (ulong)tmp_ce,
                                (ulong)ents[cur]);
                    }

                    ce = ents[cur];
                    ce->flags &= ~CE_BUSY;

                    free(ce->data);
                    ce->data = NULL;

                    free(ce);
                    ents[cur] = NULL;

                    bc.cur_blocks--;
                }
                ce = NULL;

                UNLOCK(bc.lock);
                return err;
            }


            /*
               last step: go through and make sure all the cache_ent
               structures have the right data in them, delete old guys, etc.
            */
            for(cur=0; cur < cur_nblocks; cur++) {
                ce = ents[cur];

                if (ce->dev != -1) {   /* then clean this guy up */
                    if (ce->next || ce->prev)
                        beos_panic("ce @ 0x%x should not be in a list yet!\n", ce);

                    if (ce->clone)
                        free(ce->clone);

                    if (ce->data == NULL)
                        beos_panic("ce @ 0x%lx has a null data ptr\n", (ulong)ce);
                }

                ce->dev        = dev;
                ce->block_num  = bnum + cur;
                ce->bsize      = bsize;
                ce->flags      = CE_NORMAL;
                ce->lock       = 0;
                ce->clone      = NULL;
                ce->func = ce->arg  = NULL;
                ce->next = ce->prev = NULL;

                if (op & CACHE_READ) {
                    if (data)
                        memcpy(data, ce->data, bsize);
                } else if (op & CACHE_WRITE) {
                    ce->flags  |= CE_DIRTY;
                    memcpy(ce->data, data, bsize);
                } else if (op & CACHE_NOOP) {
                    memset(ce->data, 0, bsize);
                    if (data)
                        memset(data, 0, bsize);

                    ce->flags |= CE_DIRTY;
                }

                if (op & CACHE_LOCKED) {
                    ce->lock++;
                    cel = &bc.locked;
                } else {
                    cel = &bc.normal;
                }

                /* now stick this puppy at the head of the mru list */
                add_to_head(cel, ce);


                if (dataptr) {
                    *dataptr = ce->data;
                }

                if (data != NULL)
                    data = (void *)((char *)data + bsize);
                else if (cur_nblocks != 1)
                    beos_panic("cache can't handle setting data_ptr twice!\n");
            }  /* end of for(cur=0; cur < cur_nblocks; cur++) */

            bnum       += cur_nblocks;
            num_blocks -= cur_nblocks;

        }   /* end of else it's not in the cache */

    }   /* end of while(num_blocks) */

    UNLOCK(bc.lock);

    return 0;
}


void *
beos_get_block(int dev, fs_off_t bnum, int bsize)
{
    void *data;

    if (cache_block_io(dev, bnum, NULL, 1, bsize, CACHE_READ|CACHE_LOCKED|CACHE_READ_AHEAD_OK,
                       &data) != 0)
        return NULL;

    return data;
}

void *
beos_get_empty_block(int dev, fs_off_t bnum, int bsize)
{
    void *data;

    if (cache_block_io(dev, bnum, NULL, 1, bsize, CACHE_NOOP|CACHE_LOCKED,
                       &data) != 0)
        return NULL;

    return data;
}

int
beos_cached_read(int dev, fs_off_t bnum, void *data, fs_off_t num_blocks, int bsize)
{
    return cache_block_io(dev, bnum, data, num_blocks, bsize,
                          CACHE_READ | CACHE_READ_AHEAD_OK, NULL);
}


int
beos_cached_write(int dev, fs_off_t bnum, const void *data, fs_off_t num_blocks,int bsize)
{
    return cache_block_io(dev, bnum, (void *)data, num_blocks, bsize,
                          CACHE_WRITE, NULL);
}

int
beos_cached_write_locked(int dev, fs_off_t bnum, const void *data,
                    fs_off_t num_blocks, int bsize)
{
    return cache_block_io(dev, bnum, (void *)data, num_blocks, bsize,
                          CACHE_WRITE | CACHE_LOCKED, NULL);
}


void
beos_force_cache_flush(int dev, int prefer_log_blocks)
{
    int        i, count = 0;
    cache_ent *ce;
    cache_ent *ents[NUM_FLUSH_BLOCKS];


    LOCK(bc.lock);

    for(ce=bc.normal.lru; ce; ce=ce->next) {
        if ((ce->dev == dev) &&
            (ce->flags & CE_BUSY) == 0 &&
            ((ce->flags & CE_DIRTY) || ce->clone) &&
            ((prefer_log_blocks && ce->func) || (prefer_log_blocks == 0))) {

            ce->flags |= CE_BUSY;
            ents[count++] = ce;

            if (count >= NUM_FLUSH_BLOCKS) {
                break;
            }
        }
    }

    /* if we've got some room left, try and grab any cloned blocks */
    if (count < NUM_FLUSH_BLOCKS) {
        for(ce=bc.locked.lru; ce; ce=ce->next) {
            if ((ce->dev == dev) &&
                (ce->flags & CE_BUSY) == 0 &&
                (ce->clone)) {

                ce->flags |= CE_BUSY;
                ents[count++] = ce;

                if (count >= NUM_FLUSH_BLOCKS) {
                    break;
                }
            }
        }
    }

    UNLOCK(bc.lock);

    if (count != 0) {
        qsort(ents, count, sizeof(cache_ent **), cache_ent_cmp);
        flush_ents(ents, count);

        for(i=0; i < count; i++)
            ents[i]->flags &= ~CE_BUSY;
    }
}
