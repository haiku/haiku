/*
  This file contains a simple memory based file system that is used
  as the top-level name space by the vnode layer.  It is a complete
  file system in and of itself but it only supports creating directories
  and symlinks.  It is also entirely memory based so it is re-created
  each time the vnode layer is initialized.  It is only used to mount
  other file systems (i.e. to create a directory that can be used as
  a mount point for another file system).
  
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include "compat.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "skiplist.h"
#include "lock.h"

#include "fsproto.h"

typedef struct vnode vnode;
typedef struct nspace nspace;
typedef struct dirpos dirpos;


struct nspace {
    nspace_id   nsid;
    long        vnnum;
    vnode *     root;
    vnode_id    nxvnid;
    lock        lock;
    SkipList    skiplist;
};

struct vnode {
    char *      name;
    char        removed;
    nspace      *ns;
    vnode_id    vnid;
    vnode       *parent;
    time_t      crtime;
    time_t      mtime;
    uid_t       uid;
    gid_t       gid;
    mode_t      mode;
    vnode *     next;       
    vnode *     prev;
    vnode *     head;           /* for directories */
    char *      symlink;        /* for symbolic links */
};

struct dirpos {
    lock        lock;
    int         pos;
    char        name[FILE_NAME_LENGTH];
};


static int      rootfs_read_vnode(void *ns, vnode_id vnid, char r,
                    void **node);
static int      rootfs_write_vnode(void *ns, void *node, char r);
static int      rootfs_remove_vnode(void *ns, void *node, char r);
static int      rootfs_walk(void *ns, void *base, const char *file,
                        char **newpath, vnode_id *vnid);
static int      rootfs_access(void *ns, void *node, int mode);
static int      rootfs_symlink(void *ns, void *dir, const char *name,
                        const char *path);
static int      rootfs_mkdir(void *ns, void *dir, const char *name,
                        int perms);
static int      rootfs_rename(void *ns, void *olddir, const char *oldname,
                        void *newdir, const char *newname);
static int      rootfs_unlink(void *ns, void *dir, const char *name);
static int      rootfs_rmdir(void *ns, void *dir, const char *name);
static int      rootfs_readlink(void *ns, void *node, char *buf,
                        size_t *bufsize);
static int      rootfs_opendir(void *ns, void *node, void **cookie);
static int      rootfs_closedir(void *ns, void *node, void *cookie);
static int      rootfs_free_dircookie(void *ns, void *node, void *cookie);
static int      rootfs_rewinddir(void *ns, void *node, void *cookie);
static int      rootfs_readdir(void *ns, void *node, void *cookie,
                    long *num, struct my_dirent *buf, size_t bufsize);
static int      rootfs_rstat(void *ns, void *node, struct my_stat *st);
static int      rootfs_wstat(void *ns, void *node, struct my_stat *st, long mask);
static int      rootfs_mount(nspace_id nsid, const char *device, ulong flags,
                        void *parms, size_t len, void **data, vnode_id *vnid);
static int      rootfs_unmount(void *ns);

static int      compare_vnode(vnode *vna, vnode *vnb);
static int      do_create(nspace *ns, vnode *dir, const char *name,
                    mode_t mode, vnode **vnp);
static int      do_unlink(nspace *ns, vnode *dir, const char *name, bool isdir);

vnode_ops rootfs =  {
    &rootfs_read_vnode,
    &rootfs_write_vnode,
    &rootfs_remove_vnode,
    NULL,
    &rootfs_walk,
    &rootfs_access,
    NULL,
    &rootfs_mkdir,
    &rootfs_symlink,
    NULL,
    &rootfs_rename,
    &rootfs_unlink,
    &rootfs_rmdir,
    &rootfs_readlink,
    &rootfs_opendir,
    &rootfs_closedir,
    &rootfs_free_dircookie,
    &rootfs_rewinddir,
    &rootfs_readdir,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &rootfs_rstat,
    &rootfs_wstat,
    NULL,
    NULL,
    &rootfs_mount,
    &rootfs_unmount,
    NULL
};

    
#define     LCK_MULTIPLE    1
#define     LCK_EXCLUSIVE   1000

#define     OMODE_MASK      (O_RDONLY | O_WRONLY | O_RDWR)


/* ----------------------------------------------------------------- */


static int
rootfs_walk(void *_ns, void *_base, const char *file, char **newpath,
            vnode_id *vnid)
{
    nspace      *ns;
    vnode       *base;
    int         err;
    vnode       *vn;
    char        *np;

    ns = (nspace *) _ns;
    base = (vnode *) _base;

    LOCK(ns->lock);

    /*
    make sure base is a directory and that it has not been removed.
    */

    if (!MY_S_ISDIR(base->mode)) {
        err = FS_ENOTDIR;
        goto exit;
    }
    if (base->removed) {
        err = FS_ENOENT;
        goto exit;
    }

    /*
    lookup the special directory '.'
    */

    if (!strcmp(file, ".")) {
        err = get_vnode(ns->nsid, base->vnid, (void *)&vn);
        if (!err)
            *vnid = base->vnid;
        goto exit;
    }

    /*
    lookup the special directory '..'
    */

    if (!strcmp(file, "..")) {
        err = get_vnode(ns->nsid, base->parent->vnid, (void *)&vn);
        if (!err)
            *vnid = vn->vnid;
        goto exit;
    }

    /*
    lookup the name in the directory        
    */

    vn = base->head;
    while (vn) {
        if (!strcmp(vn->name, file))
            break;
        vn = vn->next;
    }

    /*
    the name has not been found. advance the path pointer, update the vnid
    and report an error.
    */

    if (!vn) {
        err = FS_ENOENT;
        goto exit;
    }

    /*
    we have found the item.
    */

    /*
    it is a symbolic link that the kernel wants us to eat.
    */

    if (MY_S_ISLNK(vn->mode) && newpath) {
        err = new_path(vn->symlink, &np);
        if (err)
            goto exit;
            
        *newpath = np;

    } else {

    /*
    it is a directory or it is a symbolic link that the kernel does
    not want to 'eat'.
    */

        err = get_vnode(ns->nsid, vn->vnid, (void *)&vn);
        if (!err)
            *vnid = vn->vnid;
    }
    
exit:
    UNLOCK(ns->lock);
    return err;
}


static int
rootfs_mkdir(void *_ns, void *_dir, const char *name, int perms)
{
    nspace      *ns;
    vnode       *dir;
    int         err;
    vnode       *vn;

    ns = (nspace *) _ns;
    dir = (vnode *) _dir;

    LOCK(ns->lock);
    err = do_create(ns, dir, name, (perms & ~MY_S_IFMT) | MY_S_IFDIR, &vn);
    UNLOCK(ns->lock);
    return err;
}

static int
rootfs_symlink(void *_ns, void *_dir, const char *name, const char *path)
{
    nspace      *ns;
    vnode       *dir;
    int         err;
    char        *buf;
    vnode       *vn;

    ns = (nspace *) _ns;
    dir = (vnode *) _dir;

    buf = (char *) malloc(strlen(path)+1);
    if (!buf) {
        err = FS_ENOMEM;
        goto error1;
    }
    strcpy(buf, path);
    LOCK(ns->lock);
    err = do_create(ns, dir, name, MY_S_IFLNK, &vn);
    if (err)
        goto error2;
    vn->symlink = buf;
    UNLOCK(ns->lock);
    return 0;

error2:
    UNLOCK(ns->lock);
error1:
    return err;
}

    
static int
rootfs_rename(void *_ns, void *_olddir, const char *oldname, void *_newdir,
                    const char *newname)
{
    nspace      *ns;
    vnode       *olddir, *newdir;
    int         err;
    vnode       *vn, *nvn, *pvn, *avn;
    char        *p;

    ns = (nspace *) _ns;
    olddir = (vnode *) _olddir;
    newdir = (vnode *) _newdir;

    LOCK(ns->lock);
    if (!MY_S_ISDIR(olddir->mode) || !MY_S_ISDIR(newdir->mode)) {
        err = FS_ENOTDIR;
        goto error1;
    }
    
    /*
    find (olddir, oldname)
    */

    if (!strcmp(oldname, ".") || !strcmp(oldname, "..")) {
        err = FS_EPERM;
        goto error1;
    }

    vn = olddir->head;
    while (vn) {
        if (!strcmp(vn->name, oldname))
            break;
        vn = vn->next;
    }
    if (!vn) {
        err = FS_ENOENT;
        goto error1;
    }

    /*
    look for (newdir, newname)
    */

    if (!strcmp(newname, ".") || !strcmp(newname, "..")) {
        err = FS_EPERM;
        goto error1;
    }

    nvn = newdir->head;
    while (nvn) {
        if (!strcmp(nvn->name, newname))
            break;
        nvn = nvn->next;
    }

    /*
    don't do anything if old and new are the same
    */

    if (vn == nvn)
        goto exit;

    /*
    make sure new is not a subdirectory of old
    */

    avn = newdir;
    while (avn != ns->root) {
        avn = avn->parent;
        if (avn == olddir) {
            err = FS_EINVAL;
            goto error1;
        }
    }

    if (strlen(newname) > strlen(vn->name)) {
        p = (char *) realloc(vn->name, strlen(newname)+1);
        if (!p) {
            err = FS_ENOMEM;
            goto error1;
        }
    } else
        p = vn->name;

    /*
    if (newdir, newname) exists, remove it from the name space
    */

    if (nvn) {

    /*
    make sure it is not the root and it is not empty
    */

        if (nvn == nvn->ns->root) {
            err = FS_EBUSY;
            goto error1;
        }
        if (MY_S_ISDIR(nvn->mode) && nvn->head) {
            err = FS_ENOTEMPTY;
            goto error1;
        }

        err = get_vnode(ns->nsid, nvn->vnid, (void *)&nvn);
        if (err)
            goto error1;

        err = remove_vnode(ns->nsid, nvn->vnid);
        if (err)
            goto error1;

        if (nvn->prev)
            nvn->prev->next = nvn->next;
        else
            nvn->parent->head = nvn->next;
        if (nvn->next)
            nvn->next->prev = nvn->prev;
        nvn->prev = nvn->next = NULL;

        put_vnode(ns->nsid, nvn->vnid);
    }

    if (vn->prev)
        vn->prev->next = vn->next;
    else
        vn->parent->head = vn->next;
    if (vn->next)
        vn->next->prev = vn->prev;

    pvn = NULL;
    nvn = newdir->head;
    while (nvn && (strcmp(newname, nvn->name) > 0)) {
        pvn = nvn;
        nvn = nvn->next;
    }
    vn->next = nvn;
    if (nvn)
        nvn->prev = vn;
    vn->prev = pvn;
    if (pvn)
        pvn->next = vn;
    else
        newdir->head = vn;

    vn->parent = newdir;
    newdir->mtime = olddir->mtime = time(NULL);
    strcpy(p, newname);
    vn->name = p;

exit:
    UNLOCK(ns->lock);
    return 0;

error1:
    UNLOCK(ns->lock);
    return err;
}

static int
rootfs_unlink(void *_ns, void *_dir, const char *name)
{
    nspace      *ns;
    vnode       *dir;

    ns = (nspace *) _ns;
    dir = (vnode *) _dir;

    return do_unlink(ns, dir, name, FALSE);
}

static int
rootfs_rmdir(void *_ns, void *_dir, const char *name)
{
    nspace      *ns;
    vnode       *dir;

    ns = (nspace *) _ns;
    dir = (vnode *) _dir;

    return do_unlink(ns, dir, name, TRUE);
}



static int
rootfs_read_vnode(void *_ns, vnode_id vnid, char r, void **node)
{
    nspace      *ns;
    vnode       *vn;
    vnode       fakevn;

    ns = (nspace *) _ns;

    if (!r)
        LOCK(ns->lock);
    fakevn.vnid = vnid;
    fakevn.ns = ns;
    vn = SearchSL(ns->skiplist, &fakevn);
    if (vn)
        *node = vn;
    if (!r)
        UNLOCK(ns->lock);
    return (vn ? 0 : ENOENT);
}

static int
rootfs_write_vnode(void *_ns, void *_node, char r)
{
    return 0;
}

static int
rootfs_remove_vnode(void *_ns, void *_node, char r)
{
    nspace      *ns;
    vnode       *node;

    ns = (nspace *) _ns;
    node = (vnode *) _node;

    if (!r)
        LOCK(ns->lock);
    DeleteSL(ns->skiplist, node);
    if (!r)
        UNLOCK(ns->lock);

    atomic_add(&ns->vnnum, -1);
    if (node->symlink)
        free(node->symlink);
    free(node->name);
    free(node);
    return 0;
}


static int
rootfs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize)
{
    nspace      *ns;
    vnode       *node;
    int         err;
    size_t      l;

    ns = (nspace *) _ns;
    node = (vnode *) _node;

    if (!MY_S_ISLNK(node->mode)) {
        err = FS_EINVAL;
        goto error1;
    }
    l = strlen(node->symlink);
    if (l > *bufsize)
        memcpy(buf, node->symlink, *bufsize);
    else
        memcpy(buf, node->symlink, l);
    *bufsize = l;
    return 0;

error1:
    return err;
}

static int
rootfs_opendir(void *_ns, void *_node, void **cookie)
{
    nspace      *ns;
    vnode       *node;
    int         err;
    dirpos      *pos;

    ns = (nspace *) _ns;
    node = (vnode *) _node;

    if (!MY_S_ISDIR(node->mode)) {
        err = FS_ENOTDIR;
        goto error1;
    }
    pos = (dirpos *) malloc(sizeof(dirpos));
    if (!pos) {
        err = FS_ENOMEM;
        goto error1;
    }
    if (new_lock(&pos->lock, "rootdirlock") < 0) {
        err = FS_EINVAL;
        goto error2;
    }
    pos->pos = 0;
    pos->name[0] = '\0';
    *cookie = pos;
    return 0;

error2:
    free(pos);
error1:
    return err;
}

static int
rootfs_closedir(void *_ns, void *_node, void *_cookie)
{
    return 0;
}

static int
rootfs_free_dircookie(void *_ns, void *_node, void *_cookie)
{
    nspace      *ns;
    vnode       *node;
    dirpos      *cookie;

    ns = (nspace *) _ns;
    node = (vnode *) _node;
    cookie = (dirpos *) _cookie;

    free_lock(&cookie->lock);
    free(cookie);
    return 0;
}

static int
rootfs_rewinddir(void *_ns, void *_node, void *_cookie)
{
    nspace      *ns;
    vnode       *node;
    dirpos      *cookie;

    ns = (nspace *) _ns;
    node = (vnode *) _node;
    cookie = (dirpos *) _cookie;

    LOCK(cookie->lock);
    cookie->pos = 0;
    cookie->name[0] = '\0';
    UNLOCK(cookie->lock);
    return 0;
}

static int
rootfs_readdir(void *_ns, void *_node, void *_cookie, long *num,
                    struct my_dirent *buf, size_t bufsize)
{
    nspace              *ns;
    vnode               *node;
    dirpos              *cookie;
    char                *e;
    struct my_dirent    *p;
    long                 i;
    vnode               *vn;
    vnode_id             vnid;
    char                *name, *last = "";
    int                  sl, rl;

    ns = (nspace *) _ns;
    node = (vnode *) _node;
    cookie = (dirpos *) _cookie;

    LOCK(ns->lock);
    LOCK(cookie->lock);
    vn = node->head;
    p = (struct my_dirent *) buf;
    e = (char *) buf + bufsize;
    if (cookie->pos > 2)
        while (vn && (strcmp(cookie->name, vn->name) >= 0))
            vn = vn->next;
    for(i=0; (i < *num) && ((cookie->pos < 2) || vn); i++, cookie->pos++) {
        switch(cookie->pos) {
        case 0:
            name = ".";
            vnid = node->vnid;
            break;
        case 1:
            name = "..";
            vnid = node->parent->vnid;
            break;
        default:
            name = vn->name;
            vnid = vn->vnid;
            vn = vn->next;
            break;
        }
        sl = strlen(name) + 1;
        rl = sizeof(struct my_dirent) + sl - 1;
        if ((char *)p + rl > e)
            break;
        last = name;
        p->d_reclen = (rl + 7) & ~7;
        p->d_ino = vnid;
        memcpy(p->d_name, name, sl);
        p = (struct my_dirent *)((char *)p + p->d_reclen);
    }
    if ((cookie->pos > 2) && (i > 0))
        strcpy(cookie->name, last);

    *num = i;

    UNLOCK(cookie->lock);
    UNLOCK(ns->lock);
    return 0;
}

static int
rootfs_rstat(void *_ns, void *_node, struct my_stat *st)
{
    nspace      *ns;
    vnode       *node;

    ns = (nspace *) _ns;
    node = (vnode *) _node;

    LOCK(ns->lock);
    st->dev = ns->nsid;
    st->ino = node->vnid;
    st->mode = node->mode;
    st->nlink = 1;
    st->uid = node->uid;
    st->gid = node->gid;
    st->size = 0;
    st->blksize = 0;
    st->atime = st->ctime = st->mtime = node->mtime;
    UNLOCK(ns->lock);
    return 0;
}

static int
rootfs_wstat(void *_ns, void *_node, struct my_stat *st, long mask)
{
    nspace      *ns;
    vnode       *node;

    ns = (nspace *) _ns;
    node = (vnode *) _node;

    if (mask & WSTAT_SIZE)
        return FS_EINVAL;

    LOCK(ns->lock);

    if (mask & WSTAT_MODE)
        node->mode = (node->mode & MY_S_IFMT) | (st->mode & ~MY_S_IFMT);
    if (mask & WSTAT_UID)
        node->uid = st->uid;
    if (mask & WSTAT_GID)
        node->gid = st->gid;
    if (mask & WSTAT_MTIME)
        node->mtime = st->mtime;
    if (mask & WSTAT_ATIME)
        node->mtime = st->atime;

    UNLOCK(ns->lock);
    return 0;
}

static int
rootfs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
        size_t len, void **data, vnode_id *vnid)
{
    int             err;
    nspace          *ns;
    vnode           *root;
    vnode_id        rvnid;

    if (device || parms || (len != 0)) {
        err = FS_EINVAL;
        goto error1;
    }

    ns = (nspace *) malloc(sizeof(nspace));
    if (!ns) {
        err = FS_ENOMEM;
        goto error1;
    }

    root = (vnode *) malloc(sizeof(vnode));
    if (!root) {
        err = FS_ENOMEM;
        goto error2;
    }

    rvnid = 1;

    ns->nsid = nsid;
    ns->vnnum = 0;
    ns->nxvnid = rvnid;
    ns->root = root;
    if (new_lock(&ns->lock, "rootfs") < 0) {
        err = -1;
        goto error3;
    }
    ns->skiplist = NewSL(&compare_vnode, NULL, NO_DUPLICATES);
    if (!ns->skiplist) {
        err = -1;
        goto error4;
    }

    root->vnid = rvnid;
    root->parent = root;
    root->ns = ns;
    root->removed = FALSE;
    root->name = NULL;
    root->next = root->prev = NULL;
    root->head = NULL;
    root->symlink = NULL;

    /* ### do it for real */
    root->uid = 0;
    root->gid = 0;
    root->mode = MY_S_IFDIR | 0777;
    root->mtime = time(NULL);

    err = new_vnode(nsid, rvnid, root);
    if (err)
        goto error5;

    *data = ns;
    *vnid = rvnid;

    return 0;

error5:
    FreeSL(ns->skiplist);
error4:
    free_lock(&ns->lock);
error3:
    free(root);
error2:
    free(ns);
error1:
    return err;
}

static int
rootfs_unmount(void *_ns)
{
    nspace      *ns;
    vnode       *vn, *avn;

    ns = (nspace *) _ns;

    vn = ns->root;
    while (TRUE) {
        while(vn->head)
            vn = vn->head;
        vn = vn->parent;
        if (vn == ns->root)
            break;
        avn = vn->head;
        if (avn->prev)
            avn->prev->next = avn->next;
        else
            vn->head = avn->next;
        if (avn->next)
            avn->next->prev = avn->prev;
        rootfs_remove_vnode(ns, avn, TRUE);
    }
    free(ns->root);
    free_lock(&ns->lock);
    FreeSL(ns->skiplist);
    free(ns);
    return 0;
}

/* ### should do real permission check */
static int
rootfs_access(void *_ns, void *_node, int mode)
{
    return 0;
}

static int
compare_vnode(vnode *vna, vnode *vnb)
{
    if (vna->vnid > vnb->vnid)
        return 1;
    else
        if (vna->vnid < vnb->vnid)
            return -1;
        else
            return 0;
}

static int
do_create(nspace *ns, vnode *dir, const char *name, mode_t mode, vnode **vnp)
{
    int         err;
    int         c;
    vnode       *vn, *pvn, *nvn;
    char        *buf;
    vnode_id    vnid;

    if (!MY_S_ISDIR(dir->mode)) {
        err = FS_ENOTDIR;
        goto error1;
    }
    
    /*
    make sure we are not trying to create something in a directory
    that has been removed.
    */

    if (dir->removed) {
        err = FS_ENOENT;
        goto error1;
    }

    /*
    filter the name:
    can't be '.', or '..'
    */

    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        err = FS_EEXIST;
        goto error1;
    }
        
    /*
    lookup the name in the directory        
    */

    vn = dir->head;
    while (vn) {
        c = strcmp(name, vn->name);
        if (c < 0)
            vn = NULL;
        if (c <= 0)
            break;
        vn = vn->next;
    }
    

    /*
    if it was found, report an error.
    */

    if (vn) {
        err = FS_EEXIST;
        goto error1;
    }

    /*
    allocate a vnode and fill it
    */

    vn = NULL;
    buf = NULL;
    vn = (vnode *) malloc(sizeof(vnode));
    buf = (char *) malloc(strlen(name)+1);
    if (!vn || !buf) {
        err = FS_ENOMEM;
        goto error2;
    }
    strcpy(buf, name);

    vnid = ++ns->nxvnid;

    vn->vnid = vnid;
    vn->parent = dir;
    vn->ns = ns;
    vn->removed = FALSE;
    vn->name = buf;

    vn->mode = mode;
    vn->uid = 0;
    vn->gid = 0;
    dir->mtime = vn->mtime = time(NULL);

    pvn = NULL;
    nvn = dir->head;
    while (nvn && (strcmp(name, nvn->name) > 0)) {
        pvn = nvn;
        nvn = nvn->next;
    }
    vn->next = nvn;
    if (nvn)
        nvn->prev = vn;
    vn->prev = pvn;
    if (pvn)
        pvn->next = vn;
    else
        dir->head = vn;

    vn->head = NULL;
    vn->symlink = NULL;

    atomic_add(&ns->vnnum, 1);

    InsertSL(ns->skiplist, vn);

    *vnp = vn;

    return 0;

error2:
    if (vn)
        free(vn);
    if (buf)
        free(buf);
error1:
    return err;
}

static int
do_unlink(nspace *ns, vnode *dir, const char *name, bool isdir)
{
    int         err;
    vnode       *vn;

    LOCK(ns->lock);
    if (!MY_S_ISDIR(dir->mode)) {
        err = FS_ENOTDIR;
        goto error1;
    }
    
    /*
    can't delete '..' and '.'
    */

    if (!strcmp(name, "..") || !strcmp(name, ".")) {
        err = FS_EINVAL;
        goto error1;
    }
        
    /*
    lookup the name in the directory        
    */

    vn = dir->head;
    while (vn) {
        if (!strcmp(vn->name, name))
            break;
        vn = vn->next;
    }
    
    /*
    if it was not found, report an error.
    */

    if (!vn) {
        err = FS_ENOENT;
        goto error1;
    }

    /*
    ensure it is of the appropriate type.
    */

    if (isdir && !(vn->mode & MY_S_IFDIR)) {
        err = FS_ENOTDIR;
        goto error1;
    }

    if (!isdir && (vn->mode & MY_S_IFDIR)) {
        err = FS_EISDIR;
        goto error1;
    }
        
    /*
    make sure it is not the root
    */

    if (vn == vn->ns->root) {
        err = FS_EBUSY;
        goto error1;
    }

    /*
    if it is a directory, make sure it is empty
    */

    if (MY_S_ISDIR(vn->mode) && vn->head) {
        err = FS_ENOTEMPTY;
        goto error1;
    }

    err = get_vnode(ns->nsid, vn->vnid, (void *)&vn);
    if (err)
        goto error1;

    err = remove_vnode(ns->nsid, vn->vnid);
    if (err)
        goto error1;

    if (vn->prev)
        vn->prev->next = vn->next;
    else
        vn->parent->head = vn->next;
    if (vn->next)
        vn->next->prev = vn->prev;
    vn->prev = vn->next = NULL;
    vn->removed = TRUE;
    dir->mtime = time(NULL);

    put_vnode(ns->nsid, vn->vnid);

    UNLOCK(ns->lock);

    return 0;

error1:
    UNLOCK(ns->lock);
    return err;
}
