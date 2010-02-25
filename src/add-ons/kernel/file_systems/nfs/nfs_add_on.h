#ifndef _NFS_ADD_ON_H
#define _NFS_ADD_ON_H

/* wrappers */
#ifdef __HAIKU__
#	include <fs_interface.h>
#	include <fs_info.h>
#	include <NodeMonitor.h>
typedef dev_t nspace_id;
#	define WSTAT_MODE B_STAT_MODE
#	define WSTAT_UID B_STAT_UID
#	define WSTAT_GID B_STAT_GID
#	define WSTAT_SIZE B_STAT_SIZE
#	define WSTAT_ATIME B_STAT_ACCESS_TIME
#	define WSTAT_MTIME B_STAT_MODIFICATION_TIME
#	define WSTAT_CRTIME B_STAT_CREATION_TIME

#else
#	include "fsproto.h"
#	define publish_vnode new_vnode
typedef int socklen_t;
#endif

#include "RPCPendingCalls.h"
#include "XDROutPacket.h"
#include "XDRInPacket.h"
#include "nfs.h"

#include <Errors.h>
#include <sys/stat.h>
#include <SupportDefs.h>


struct mount_nfs_params {
	unsigned int serverIP;
	char *server;
	char *_export;
	uid_t uid;
	gid_t gid;
	char *hostname;
};

struct fs_node {
	int mode;
	ino_t vnid;
	struct nfs_fhandle fhandle;
	struct fs_node *next;
};

struct fs_nspace {
	nspace_id nsid;

	thread_id tid;
	bool quit;
	int s;
	struct RPCPendingCalls pendingCalls;

	struct sockaddr_in mountAddr,nfsAddr;

	int32 xid;

	ino_t rootid;

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

status_t create_socket(fs_nspace *ns);
status_t init_postoffice(fs_nspace *ns);
void shutdown_postoffice(fs_nspace *ns);
status_t postoffice_func(fs_nspace *ns);

extern uint8 *send_rpc_call(fs_nspace *ns, const struct sockaddr_in *addr, int32 prog, int32 vers, int32 proc, const struct XDROutPacket *packet);
extern bool is_successful_reply(struct XDRInPacket *reply);
extern status_t get_remote_address(fs_nspace *ns, int32 prog, int32 vers, int32 prot, struct sockaddr_in *addr);
extern status_t nfs_mount(fs_nspace *ns, const char *path, nfs_fhandle *fhandle);
extern status_t map_nfs_to_system_error(status_t nfsstatus);
extern void get_nfs_attr(struct XDRInPacket *reply, struct stat *st);
extern status_t nfs_lookup(fs_nspace *ns, const nfs_fhandle *dir, const char *filename, nfs_fhandle *fhandle, struct stat *st);
extern status_t nfs_truncate_file(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st);

nfs_fhandle handle_from_vnid (fs_nspace *ns, ino_t vnid);

extern status_t nfs_getattr(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st);
extern void insert_node(fs_nspace *ns, fs_node *node);
extern void remove_node(fs_nspace *ns, ino_t vnid);

enum {
	C_ERROR_STALE = B_ERRORS_END + 1
};


extern fs_volume_ops sNFSVolumeOps;
extern fs_vnode_ops sNFSVnodeOps;

#define USE_SYSTEM_AUTHENTICATION 1

#endif	/* _NFS_ADD_ON_H */
