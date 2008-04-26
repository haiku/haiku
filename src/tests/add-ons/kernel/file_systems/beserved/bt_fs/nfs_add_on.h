#ifndef _NFS_ADD_ON_H
#define _NFS_ADD_ON_H

#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif

#include "fsproto.h"
#include "betalk.h"
#include "readerWriter.h"

#include <Errors.h>
#include <sys/stat.h>
#include <SupportDefs.h>

struct mount_bt_params
{
	unsigned int serverIP;
	char *server;
	char *export;	
	uid_t uid;
	gid_t gid;
	char *hostname;
	char *folder;
	char user[MAX_USERNAME_LENGTH + 1];
	char password[BT_AUTH_TOKEN_LENGTH * 2 + 1];
};

struct fs_node
{
	vnode_id parent;
	vnode_id vnid;
	char name[B_FILE_NAME_LENGTH + 1];
	struct fs_node *next;
};

typedef struct dnlcEntry
{
	time_t entryTime;
	struct stat st;
	vnode_id vnid;
	vnode_id parent;
	char name[B_FILE_NAME_LENGTH];
	struct dnlcEntry *next, *prev;
} dnlc_entry;

struct fs_nspace
{
	nspace_id nsid;

	thread_id tid;	
	bool quit;

	struct sockaddr_in mountAddr, nfsAddr;
	uint32 serverIP;
	int32 serverPort;
	int s;

	thread_id rpcThread;
	int32 xid;	
	int32 quitXID;

	vnode_id rootid;
	sem_id sem;
	struct fs_node *first;

	dnlc_entry *dnlcRoot;
	bt_managed_data dnlcData;

	struct mount_bt_params params;
};

struct fs_file_cookie
{
	int omode;
	off_t original_size;
	struct stat st;
};

typedef struct fs_nspace fs_nspace;
typedef struct fs_node fs_node;
typedef struct fs_file_cookie fs_file_cookie;

int	fs_read_vnode(fs_nspace *ns, vnode_id vnid, char r, fs_node **node);
int	fs_write_vnode(fs_nspace *ns, fs_node *node, char r);

int	fs_walk(fs_nspace *ns, fs_node *base, const char *file, char **newpath,
					vnode_id *vnid);

int fs_opendir(fs_nspace *ns, fs_node *node, btCookie **cookie);
int	fs_closedir(fs_nspace *ns, fs_node *node, btCookie *cookie);
int	fs_rewinddir(fs_nspace *ns, fs_node *node, btCookie *cookie);
int	fs_readdir(fs_nspace *ns, fs_node *node, btCookie *cookie, long *num,
					struct dirent *buf, size_t bufsize);

int fs_free_dircookie(fs_nspace *ns, fs_node *node, btCookie *cookie);
int	fs_rstat(fs_nspace *ns, fs_node *node, struct stat *);
int	fs_mount(nspace_id nsid, const char *devname, ulong flags,
					struct mount_bt_params *parms, size_t len, fs_nspace **data, vnode_id *vnid);
int	fs_unmount(fs_nspace *ns);
int fs_rfsstat(fs_nspace *ns, struct fs_info *);

int	fs_open(fs_nspace *ns, fs_node *node, int omode, fs_file_cookie **cookie);
int	fs_close(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie);
int fs_free_cookie(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie);
int fs_read(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, void *buf,
					size_t *len);

int fs_write(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos,
					const void *buf, size_t *len);

int fs_wstat(fs_nspace *ns, fs_node *node, struct stat *, long mask);
int fs_wfsstat(fs_nspace *ns, struct fs_info *, long mask);
int	fs_create(fs_nspace *ns, fs_node *dir, const char *name,
					int omode, int perms, vnode_id *vnid, fs_file_cookie **cookie);

int	fs_unlink(fs_nspace *ns, fs_node *dir, const char *name);
int	fs_remove_vnode(fs_nspace *ns, fs_node *node, char r);
int	fs_secure_vnode(fs_nspace *ns, fs_node *node);

int	fs_mkdir(fs_nspace *ns, fs_node *dir, const char *name,	int perms);

int	fs_rename(fs_nspace *ns, fs_node *olddir, const char *oldname,
					fs_node *newdir, const char *newname);

int	fs_rmdir(fs_nspace *ns, fs_node *dir, const char *name);
int	fs_readlink(fs_nspace *ns, fs_node *node, char *buf, size_t *bufsize);

int	fs_symlink(fs_nspace *ns, fs_node *dir, const char *name,
					const char *path);

fs_node *create_node(fs_nspace *ns, vnode_id dir_vnid, const char *name, vnode_id file_vnid, bool newVnid);
fs_node *getDuplicateVnid(fs_nspace *ns, vnode_id vnid);
fs_node *getDuplicateName(fs_nspace *ns, vnode_id parent, const char *name);
void remove_obsolete_node(fs_nspace *ns, fs_node *node);
void rename_node(fs_nspace *ns, vnode_id old_dir_vnid, const char *oldname, vnode_id new_dir_vnid, const char *newname, vnode_id file_vnid);
void remove_node(fs_nspace *ns, vnode_id vnid);
void print_vnode_list(fs_nspace *ns);

extern int fs_access(void *ns, void *node, int mode);

extern int fs_read_attrib(fs_nspace *ns, fs_node *node, const char *name, int type, void *buf, size_t *len, off_t pos);
extern int fs_write_attrib(fs_nspace *ns, fs_node *node, const char *name, int type, const void *buf, size_t *len, off_t pos);
extern int fs_open_attribdir(fs_nspace *ns, fs_node *node, btCookie **cookie);
extern int fs_close_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie);
extern int fs_rewind_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie);
extern int fs_read_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie, long *num, struct dirent *buf, size_t bufsize);
extern int fs_free_attribdircookie(fs_nspace *ns, btCookie *cookie);
extern int fs_remove_attrib(fs_nspace *ns, fs_node *node, const char *name);
extern int fs_stat_attrib(fs_nspace *ns, fs_node *node, const char *name, struct attr_info *buf);

extern int fs_open_indexdir(fs_nspace *ns, btCookie **cookie);
extern int fs_close_indexdir(fs_nspace *ns, btCookie *cookie);
extern int fs_rewind_indexdir(fs_nspace *ns, btCookie *cookie);
extern int fs_read_indexdir(fs_nspace *ns, btCookie *cookie, long *num, struct dirent *buf, size_t bufsize);
extern int fs_free_indexdircookie(fs_nspace *ns, btCookie *cookie);
extern int fsCreateIndex(fs_nspace *ns, const char *name, int type, int flags);
extern int fsRemoveIndex(fs_nspace *ns, const char *name);
extern int fsStatIndex(fs_nspace *ns, const char *name, struct index_info *buf);

extern int fsOpenQuery(fs_nspace *ns, const char *query, ulong flags, port_id port, long token, btQueryCookie **cookie);
extern int fsCloseQuery(fs_nspace *ns, btQueryCookie *cookie);
extern int fsReadQuery(fs_nspace *ns, btQueryCookie *cookie, long *num, struct dirent *buf, size_t bufsize);
extern int fsFreeQueryCookie(fs_nspace *ns, btQueryCookie *cookie);

enum
{
	C_ERROR_STALE=B_ERRORS_END+1
};

#define USE_SYSTEM_AUTHENTICATION 1

#endif
