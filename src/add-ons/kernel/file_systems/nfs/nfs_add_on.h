#ifndef _NFS_ADD_ON_H

#define _NFS_ADD_ON_H

/* wrappers */
#ifdef __HAIKU__
#	include <fs_interface.h>
#	include <fs_info.h>
#	include <NodeMonitor.h>
typedef dev_t nspace_id;
#	define WSTAT_MODE FS_WRITE_STAT_MODE
#	define WSTAT_UID FS_WRITE_STAT_UID
#	define WSTAT_GID FS_WRITE_STAT_GID
#	define WSTAT_SIZE FS_WRITE_STAT_SIZE
#	define WSTAT_ATIME FS_WRITE_STAT_ATIME
#	define WSTAT_MTIME FS_WRITE_STAT_MTIME
#	define WSTAT_CRTIME FS_WRITE_STAT_CRTIME
	
#else
#	include "fsproto.h"
#	define publish_vnode new_vnode
#endif


#include "RPCPendingCalls.h"
#include "XDROutPacket.h"
#include "XDRInPacket.h"
#include "nfs.h"

#include <Errors.h>
#include <sys/stat.h>
#include <SupportDefs.h>

struct mount_nfs_params
{
	unsigned int serverIP;
	char *server;
	char *_export;	
	uid_t uid;
	gid_t gid;
	char *hostname;
};

struct fs_node
{
	vnode_id vnid;
	struct nfs_fhandle fhandle;
	struct fs_node *next;
};

struct fs_nspace
{
	nspace_id nsid;

	thread_id tid;	
	bool quit;
	int s;
	struct RPCPendingCalls pendingCalls;
	
	struct sockaddr_in mountAddr,nfsAddr;

	int32 xid;	

	vnode_id rootid;
	
	sem_id sem;
	struct fs_node *first;

	struct mount_nfs_params params;

	bigtime_t last_rfsstat;
	
};

void fs_nspaceInit (struct fs_nspace *nspace);
void fs_nspaceDestroy (struct fs_nspace *nspace);

struct fs_file_cookie
{
	int omode;
	off_t original_size;
	struct stat st;
};

typedef struct fs_nspace fs_nspace;
typedef struct fs_node fs_node;
typedef struct nfs_cookie nfs_cookie;
typedef struct fs_file_cookie fs_file_cookie;
typedef struct nfs_fhandle nfs_fhandle;

#if 0
int	fs_read_vnode(fs_nspace *ns, vnode_id vnid, char r, fs_node **node);
int	fs_write_vnode(fs_nspace *ns, fs_node *node, char r);

int	fs_walk(fs_nspace *ns, fs_node *base, const char *file, char **newpath,
					vnode_id *vnid);

int fs_opendir(fs_nspace *ns, fs_node *node, nfs_cookie **cookie);
int	fs_closedir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie);
int	fs_rewinddir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie);
int	fs_readdir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie, long *num,
					struct dirent *buf, size_t bufsize);

int fs_free_dircookie(fs_nspace *ns, fs_node *node, nfs_cookie *cookie);
int	fs_rstat(fs_nspace *ns, fs_node *node, struct stat *);
int	fs_mount(nspace_id nsid, const char *devname, ulong flags,
					struct mount_nfs_params *parms, size_t len, fs_nspace **data, vnode_id *vnid);
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
#endif

status_t create_socket (fs_nspace *ns);
status_t init_postoffice (fs_nspace *ns);
void shutdown_postoffice (fs_nspace *ns);
status_t postoffice_func (fs_nspace *ns);

extern uint8 *send_rpc_call(fs_nspace *ns, const struct sockaddr_in *addr, int32 prog, int32 vers, int32 proc, const struct XDROutPacket *packet);
extern bool is_successful_reply(struct XDRInPacket *reply);
extern status_t get_remote_address(fs_nspace *ns, int32 prog, int32 vers, int32 prot, struct sockaddr_in *addr);
extern status_t nfs_mount(fs_nspace *ns, const char *path, nfs_fhandle *fhandle);
extern status_t map_nfs_to_system_error(status_t nfsstatus);
extern void get_nfs_attr(struct XDRInPacket *reply, struct stat *st);
extern status_t nfs_lookup(fs_nspace *ns, const nfs_fhandle *dir, const char *filename, nfs_fhandle *fhandle, struct stat *st);
extern status_t nfs_truncate_file(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st);

nfs_fhandle handle_from_vnid (fs_nspace *ns, vnode_id vnid);

extern status_t nfs_getattr(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st);
extern void insert_node(fs_nspace *ns, fs_node *node);
extern void remove_node(fs_nspace *ns, vnode_id vnid);

extern int fs_access(void *ns, void *node, int mode);

enum
{
	C_ERROR_STALE=B_ERRORS_END+1
};

#define USE_SYSTEM_AUTHENTICATION 1

#endif
