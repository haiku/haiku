#ifndef _CACHE_H_
#define _CACHE_H_

typedef struct hash_ent {
    int              dev;
    fs_off_t         bnum;
    fs_off_t         hash_val;
    void            *data;
    struct hash_ent *next;
} hash_ent;


typedef struct hash_table {
    hash_ent **table;
    int        max;
    int        mask;          /* == max - 1 */
    int        num_elements;
} hash_table;


#define HT_DEFAULT_MAX   128


typedef struct cache_ent {
    int               dev;
    fs_off_t          block_num;
    int               bsize;
    volatile int      flags;

    void             *data;
    void             *clone;         /* copy of data by set_block_info() */
    int               lock;

    void            (*func)(fs_off_t bnum, size_t num_blocks, void *arg);
    fs_off_t          logged_bnum;
    void             *arg;

    struct cache_ent *next,          /* points toward mru end of list */
                     *prev;          /* points toward lru end of list */

} cache_ent;

#define CE_NORMAL    0x0000     /* a nice clean pristine page */
#define CE_DIRTY     0x0002     /* needs to be written to disk */
#define CE_BUSY      0x0004     /* this block has i/o happening, don't touch it */


typedef struct cache_ent_list {
    cache_ent *lru;              /* tail of the list */
    cache_ent *mru;              /* head of the list */
} cache_ent_list;


typedef struct block_cache {
    struct lock     lock;
    int             flags;
    int             cur_blocks;
    int             max_blocks;
    hash_table      ht;

    cache_ent_list  normal,       /* list of "normal" blocks (clean & dirty) */
                    locked;       /* list of clean and locked blocks */
} block_cache;

#define ALLOW_WRITES  1
#define NO_WRITES     0

extern  int   init_block_cache(int max_blocks, int flags);
extern  void  shutdown_block_cache(void);

extern  void  force_cache_flush(int dev, int prefer_log_blocks);
extern  int   flush_blocks(int dev, fs_off_t bnum, int nblocks);
extern  int   flush_device(int dev, int warn_locked);

extern  int   init_cache_for_device(int fd, fs_off_t max_blocks);
extern  int   remove_cached_device_blocks(int dev, int allow_write);

extern  void *get_block(int dev, fs_off_t bnum, int bsize);
extern  void *get_empty_block(int dev, fs_off_t bnum, int bsize);
extern  int   release_block(int dev, fs_off_t bnum);
extern  int   mark_blocks_dirty(int dev, fs_off_t bnum, int nblocks);


extern  int  cached_read(int dev, fs_off_t bnum, void *data,
                         fs_off_t num_blocks, int bsize);
extern  int  cached_write(int dev, fs_off_t bnum, const void *data,
                          fs_off_t num_blocks, int bsize);
extern  int  cached_write_locked(int dev, fs_off_t bnum, const void *data,
                                 fs_off_t num_blocks, int bsize);
extern  int  set_blocks_info(int dev, fs_off_t *blocks, int nblocks,
                             void (*func)(fs_off_t bnum, size_t nblocks, void *arg),
                             void *arg);

extern  size_t read_phys_blocks (int fd, fs_off_t bnum, void *data,
                                 uint num_blocks, int bsize);
extern  size_t write_phys_blocks(int fd, fs_off_t bnum, void *data,
                                 uint num_blocks, int bsize);

#endif /* _CACHE_H_ */
