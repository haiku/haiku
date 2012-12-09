/*
  This file contains the code that will create a file system, mount
  a file system and unmount a file system.

  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include "myfs.h"


#ifndef min_c
#define min_c(a, b) (((a) < (b)) ? (a) : (b))
#endif /* min_c */



myfs_info *
myfs_create_fs(char *device, char *name, int block_size, char *opts)
{
    int        dev_block_size, bshift, warned = 0;
    char      *ptr;
    fs_off_t   num_dev_blocks;
    myfs_info *myfs;

    if ((block_size % sizeof(myfs_inode)) != 0) {
        printf("ERROR: inode size %d is not an even divisor of the block "
               "size %d\n", sizeof(myfs_inode), block_size);
        printf("       check myfs.h for more details and info.\n");
        return NULL;
    }

    if (name == NULL)
        name = "untitled";

    for(ptr=name; *ptr; ptr++) {
        if (*ptr == '/') {
            if (warned == 0) {
                fprintf(stderr, "Volume name: %s contains the '/' character. "
                        "They are being converted to '-' for safety.\n", name);
                warned = 1;
            }
            *ptr = '-';
        }
    }

    if (block_size < 512) {
        printf("minimum block size is 512 bytes\n");
        block_size = 512;
    }

    for(bshift=0; bshift < sizeof(int)*8; bshift++)
        if ((1 << bshift) == block_size)
            break;

    if (bshift >= sizeof(int)*8) {
        printf("block_size %d is not a power of two!\n", block_size);
        return NULL;
    }


    myfs = (myfs_info *)calloc(1, sizeof(myfs_info));
    if (myfs == NULL) {
        printf("can't allocate mem for myfs_info struct\n");
        return NULL;
    }

    myfs->fd = -1;

    myfs->nsid = (nspace_id)myfs;   /* we can only do this when creating */

    myfs->dsb.magic1 = SUPER_BLOCK_MAGIC1;
    myfs->dsb.magic2 = SUPER_BLOCK_MAGIC2;
    myfs->dsb.magic3 = SUPER_BLOCK_MAGIC3;
    myfs->dsb.fs_byte_order = MYFS_BIG_ENDIAN;  /* checked when mounting */

    myfs->sem = create_sem(MAX_READERS, "myfs_sem");
    if (myfs->sem < 0) {
        printf("can't create semaphore!\n");
        goto cleanup;
    }


    myfs->fd = open(device, O_RDWR);
    if (myfs->fd < 0) {
        printf("can't open device %s\n", device);
        goto cleanup;
    }

    dev_block_size = get_device_block_size(myfs->fd);
    num_dev_blocks = get_num_device_blocks(myfs->fd);
    if (block_size < dev_block_size) {
        printf("warning: fs block size too small, set to device block size %d\n",
               dev_block_size);
        block_size = dev_block_size;
    }

    if ((block_size % dev_block_size) != 0) {
        printf("error: block size %d is not an even multiple of ",
               block_size);
        printf("device block size %d\n", dev_block_size);

        goto cleanup;
    }

    myfs->dsb.block_size       = block_size;
    myfs->dsb.block_shift      = bshift;
    myfs->dev_block_conversion = block_size / dev_block_size;
    myfs->dev_block_size       = dev_block_size;
    myfs->dsb.num_blocks       = num_dev_blocks / myfs->dev_block_conversion;

    init_cache_for_device(myfs->fd, num_dev_blocks / myfs->dev_block_conversion);


    if (init_tmp_blocks(myfs) != 0) {
        printf("init_tmp_blocks failed\n");
        goto cleanup;
    }

    if (myfs_create_storage_map(myfs) != 0) {
        printf("create storage map failed\n");
        goto cleanup;
    }

    if (myfs_create_inodes(myfs) != 0) {
        printf("create inodes failed\n");
        goto cleanup;
    }

    if (myfs_create_journal(myfs) != 0) {
        printf("create journal failed\n");
        goto cleanup;
    }

    if (myfs_create_root_dir(myfs) != 0) {
        printf("create root dir failed\n");
        goto cleanup;
    }

    strncpy(myfs->dsb.name, name,
            min_c(sizeof(myfs->dsb.name) - 1, strlen(name)));

    /* now it's finally safe to write this */
    if (write_super_block(myfs) != 0) {
        printf("creating superblock failed\n");
        goto cleanup;
    }

    return myfs;


cleanup:
    if (myfs) {
        /* making the file system failed so make sure block zero is bogus */
        if (myfs->fd >= 0) {
            static char block[4096];

            memset(block, 0xff, sizeof(block));
            write_blocks(myfs, 0, block, 1);
        }

        myfs_shutdown_storage_map(myfs);
        myfs_shutdown_inodes(myfs);
        myfs_shutdown_journal(myfs);
        shutdown_tmp_blocks(myfs);

        close(myfs->fd);

        delete_sem(myfs->sem);

        free(myfs);
    }

    return NULL;

}


static int
super_block_is_sane(myfs_info *myfs)
{
    fs_off_t num_dev_blocks;
    int block_size;

    if (myfs->dsb.magic1 != SUPER_BLOCK_MAGIC1 ||
        myfs->dsb.magic2 != SUPER_BLOCK_MAGIC2 ||
        myfs->dsb.magic3 != SUPER_BLOCK_MAGIC3) {

        printf("warning: superblock magic numbers are wrong:\n");
        printf("0x%x (0x%x) 0x%x (0x%x) 0x%x (0x%x)\n",
               myfs->dsb.magic1, SUPER_BLOCK_MAGIC1,
               myfs->dsb.magic2, SUPER_BLOCK_MAGIC2,
               myfs->dsb.magic3, SUPER_BLOCK_MAGIC3);
        return 0;

    }

    if ((myfs->dsb.block_size % myfs->dev_block_size) != 0) {
        printf("warning: fs block size %d not a multiple of ",
                myfs->dsb.block_size);
        printf(" device block size %d\n", myfs->dev_block_size);

        return 0;
    }

    block_size = get_device_block_size(myfs->fd);
    if (block_size == 0) {
        printf("warning: could not fetch block size\n");
        return 0;
    }

    /* make sure that the partition is as big as the superblock
       says it is */
    num_dev_blocks = get_num_device_blocks(myfs->fd);
    if (myfs->dsb.num_blocks * myfs->dsb.block_size >
                num_dev_blocks * block_size) {
        printf("warning: fs blocks %lx larger than device blocks %lx\n",
                myfs->dsb.num_blocks * (myfs->dsb.block_size/block_size),
                num_dev_blocks);
        return 0;
    }

    if (myfs->dsb.block_size != (1 << myfs->dsb.block_shift)) {
        int i;

        printf("warning: block_shift %d does not match block size %d\n",
               myfs->dsb.block_shift, myfs->dsb.block_size);

        if (myfs->dsb.block_shift > 8 && myfs->dsb.block_shift < 16) {
            printf("setting block_size to %d\n", (1 << myfs->dsb.block_shift));
            myfs->dsb.block_size = (1 << myfs->dsb.block_shift);
        } else {
            for(i=0; i < sizeof(int) * 8; i++)
                if ((1 << i) == myfs->dsb.block_size)
                    break;

            if (i >= sizeof(int) * 8 || i > 16) {
                printf("neither block_size nor block_shift make sense!\n");
                return 0;
            }

            myfs->dsb.block_shift = i;
            printf("setting block_shift to %d\n", i);
        }
    }

    return 1;
}



int
myfs_mount(nspace_id nsid, const char *device, ulong flags,
                    void *parms, size_t len, void **data, vnode_id *vnid)
{
    int        ret = 0, oflags = O_RDWR;
    char       buff[128];
    myfs_info  *myfs;

    myfs = (myfs_info *)calloc(1, sizeof(myfs_info));
    if (myfs == NULL) {
        printf("no memory for myfs structure!\n");
        return ENOMEM;
    }

    myfs->nsid = nsid;
    *data = (void *)myfs;


    sprintf(buff, "myfs:%s", device);
    myfs->sem = create_sem(MAX_READERS, buff);
    if (myfs->sem < 0) {
        printf("could not create myfs sem!\n");
        ret = ENOMEM;
        goto error0;
    }

    myfs->fd = open(device, oflags);
    if (myfs->fd < 0) {
        printf("could not open %s to try and mount a myfs\n", device);
        ret = ENODEV;
        goto error1;
    }

    if (read_super_block(myfs) != 0) {
        printf("could not read superblock on device %s\n", device);
        ret = EBADF;
        goto error2;
    }

    if (super_block_is_sane(myfs) == 0) {
        printf("bad superblock\n");
        ret = EBADF;
        goto error2;
    }

    if ((myfs->dsb.block_size % sizeof(myfs_inode)) != 0) {
        printf("ERROR: inode size %d is not an even divisor of the block "
               "size %d\n", sizeof(myfs_inode), myfs->dsb.block_size);
        printf("       check myfs.h for more details and info.\n");
        ret = EINVAL;
        goto error2;
    }

    if (init_cache_for_device(myfs->fd, myfs->dsb.num_blocks) != 0) {
        printf("could not initialize cache access for fd %d\n", myfs->fd);
        ret = EBADF;
        goto error2;
    }

    if (init_tmp_blocks(myfs) != 0) {
        printf("could not init tmp blocks\n");
        ret = ENOMEM;
        goto error2;
    }

    if (myfs_init_journal(myfs) != 0) {
        printf("could not initialize the journal\n");
        ret = EBADF;
        goto error3;
    }

    if (myfs_init_inodes(myfs) != 0) {
        printf("could not initialize inodes\n");
        ret = ENOMEM;
        goto error5;
    }

    if (myfs_init_storage_map(myfs) != 0) {
        printf("could not initialize the storage map\n");
        ret = EBADF;
        goto error6;
    }

    *vnid = myfs->dsb.root_inum;
    if (myfs_read_vnode(myfs, *vnid, 0, (void **)&myfs->root_dir) != 0) {
        printf("could not read root dir inode\n");
        ret = EBADF;
        goto error7;
    }

    if (new_vnode(myfs->nsid, *vnid, (void *)myfs->root_dir) != 0) {
        printf("could not initialize a vnode for the root directory!\n");
        ret = ENOMEM;
        goto error7;
    }

    return 0;


 error7:
    myfs_shutdown_storage_map(myfs);
 error6:
    myfs_shutdown_inodes(myfs);
 error5:
    myfs_shutdown_journal(myfs);
 error3:
    shutdown_tmp_blocks(myfs);
 error2:
    remove_cached_device_blocks(myfs->fd, NO_WRITES);
    close(myfs->fd);
 error1:
    delete_sem(myfs->sem);
 error0:
    memset(myfs, 0xff, sizeof(*myfs));   /* yeah, I'm paranoid */
    free(myfs);

    return ret;
}


/*
   note that the order in which things are done here is *very*
   important. don't mess with it unless you know what you're doing
*/
int
myfs_unmount(void *ns)
{
    myfs_info *myfs = (myfs_info *)ns;

    if (myfs == NULL)
        return EINVAL;

    sync_journal(myfs);

    myfs_shutdown_storage_map(myfs);
    myfs_shutdown_inodes(myfs);

    /*
       have to do this after the above steps because the above steps
       might actually have to do transactions
    */
    sync_journal(myfs);

    remove_cached_device_blocks(myfs->fd, ALLOW_WRITES);
    myfs_shutdown_journal(myfs);

    write_super_block(myfs);

    shutdown_tmp_blocks(myfs);

    close(myfs->fd);

    if (myfs->sem > 0)
        delete_sem(myfs->sem);

    memset(myfs, 0xff, sizeof(*myfs));   /* trash it just to be sure */
    free(myfs);

    return 0;
}
