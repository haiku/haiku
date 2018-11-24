#include <posix/stdlib.h>

#include "nfs_add_on.h"
#include <sys/socket.h>

#include "rpc.h"
#include "pmap.h"
#include "nfs.h"
#include "mount.h"
#include <errno.h>
#include <string.h>
#include <KernelExport.h>
#include <driver_settings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <SupportDefs.h>
#include <ByteOrder.h>
#include <netinet/udp.h>

#ifndef UDP_SIZE_MAX
#define UDP_SIZE_MAX 65515
#endif
#define B_UDP_MAX_SIZE UDP_SIZE_MAX

static status_t fs_rmdir(fs_volume *_volume, fs_vnode *_dir, const char *name);

/* *** configuration *** */

//#define NFS_FS_FLAGS B_FS_IS_SHARED
#define NFS_FS_FLAGS B_FS_IS_SHARED|B_FS_IS_PERSISTENT

/* port numbers: most NFS servers insist on the client port to be < 1024 (secure option) */
/* ports to bind() to; we start at conf_high_port, then go down */
static int16 conf_high_port = 1023;
static int16 conf_low_port = 900;

/* Allow open() to open directories too */
static bool conf_allow_dir_open = true;

/* Do we list ".." in readdir(rootid) ? (the VFS corrects the dirents anyway) */
/* this seems to be mandatory for Dano... BEntry::GetPath() needs that */
static bool conf_ls_root_parent = true;

/* timeout when waiting for an answer to a call */
static bigtime_t conf_call_timeout = 2000000;

/* number of retries when waiting for an anwser to a call */
static unsigned long conf_call_tries = 3;

/* don't check who the answers come from for requests */
bool conf_no_check_ip_xid = false;

static vint32 refcount = 0; /* we only want to read the config once ? */

static status_t
read_config(void)
{
	void *handle;
	const char *str, *endptr;

	handle = load_driver_settings("nfs");
	if (handle == NULL)
		return ENOENT;

	str = get_driver_parameter(handle, "high_port", NULL, NULL);
	if (str) {
		endptr = str + strlen(str);
		conf_high_port = (int16)strtoul(str, (char **)&endptr, 10);
	}
	str = get_driver_parameter(handle, "low_port", NULL, NULL);
	if (str) {
		endptr = str + strlen(str);
		conf_low_port = (int16)strtoul(str, (char **)&endptr, 10);
	}

	conf_allow_dir_open = get_driver_boolean_parameter(handle, "allow_dir_open", conf_allow_dir_open, true);
	conf_ls_root_parent = get_driver_boolean_parameter(handle, "ls_root_parent", conf_ls_root_parent, true);
	conf_no_check_ip_xid = get_driver_boolean_parameter(handle, "no_check_ip_xid", conf_no_check_ip_xid, true);

	str = get_driver_parameter(handle, "call_timeout", NULL, NULL);
	if (str) {
		endptr = str + strlen(str);
		conf_call_timeout = (bigtime_t)1000 * strtoul(str, (char **)&endptr, 10);
		if (conf_call_timeout < 1000)
			conf_call_timeout = 1000;
	}

	str = get_driver_parameter(handle, "call_tries", NULL, NULL);
	if (str) {
		endptr = str + strlen(str);
		conf_call_tries = strtoul(str, (char **)&endptr, 10);
	}

	unload_driver_settings(handle);
	return B_OK;
}


status_t
create_socket(fs_nspace *ns)
{
	struct sockaddr_in addr;
	uint16 port=conf_high_port;

	ns->s=socket(AF_INET,SOCK_DGRAM,0);

	if (ns->s<0)
		return errno;

	do
	{
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=htonl(INADDR_ANY);
		//addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
		addr.sin_port=htons(port);
		memset (addr.sin_zero,0,sizeof(addr.sin_zero));

		if (bind(ns->s,(const struct sockaddr *)&addr,sizeof(addr))<0)
		{
			if (errno!=EADDRINUSE)
			{
				int result=errno;
				close(ns->s);
				return result;
			}

			port--;
			if (port==conf_low_port)
			{
				close(ns->s);
				return B_ERROR;
			}
		}
		else
			break;//return B_OK;
	}
	while (true);

	// doesn't seem to help with autoincrementing port on source address...
	addr.sin_addr = ns->mountAddr.sin_addr;
	addr.sin_port = htons(111);
	//kconnect(ns->s,(const struct sockaddr *)&addr,sizeof(addr));

	return B_OK;
}


#if 0
static status_t
connect_socket(fs_nspace *ns)
{
	uint16 port = conf_high_port;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	memset(addr.sin_zero,0,sizeof(addr.sin_zero));

	if (kconnect(ns->s,(const struct sockaddr *)&ns->nfsAddr,sizeof(ns->nfsAddr))<0)
	{
		return -1;
	}
	return B_OK;
}
#endif


status_t
init_postoffice(fs_nspace *ns)
{
	status_t result;

	ns->tid=spawn_kernel_thread ((thread_func)postoffice_func,"NFSv2 Postoffice",B_NORMAL_PRIORITY,ns);

	if (ns->tid<B_OK)
		return ns->tid;

	ns->quit=false;

	result=resume_thread (ns->tid);

	if (result<B_OK)
	{
		kill_thread (ns->tid);

		return result;
	}

	return B_OK;
}


void
shutdown_postoffice(fs_nspace *ns)
{
	status_t result;

	ns->quit=true;
	close(ns->s);

	wait_for_thread (ns->tid,&result);
}


status_t
postoffice_func(fs_nspace *ns)
{
	uint8 *buffer=(uint8 *)malloc(B_UDP_MAX_SIZE);

	while (!ns->quit) {
		struct sockaddr_in from;
		socklen_t fromLen=sizeof(from);

		ssize_t bytes = recvfrom(ns->s, buffer, B_UDP_MAX_SIZE, 0,
			(struct sockaddr *)&from, &fromLen);

		if (bytes >= 4) {
			struct PendingCall *call;
			int32 xid=B_BENDIAN_TO_HOST_INT32(*((int32 *)buffer));

			call=RPCPendingCallsFindAndRemovePendingCall(&ns->pendingCalls, xid,
				&from);

			if (call) {
				call->buffer=(uint8 *)malloc(bytes);
				memcpy(call->buffer, buffer, bytes);

				while (release_sem (call->sem) == B_INTERRUPTED);
			} else {
				dprintf("nfs: postoffice: can't find pending call to remove "
					"for xid %" B_PRId32 "\n", xid);
			}
		}
	}

	free (buffer);

	return B_OK;
}


uint8 *
send_rpc_call(fs_nspace *ns, const struct sockaddr_in *addr, int32 prog,
	int32 vers, int32 proc, const struct XDROutPacket *packet)
{
	int32 xid;
	size_t authSize;
	struct PendingCall *pending;
	int32 retries=conf_call_tries;
	status_t result;
	struct PendingCall *call;

	struct XDROutPacket rpc_call;
	XDROutPacketInit(&rpc_call);

	xid=atomic_add(&ns->xid, 1);
#ifdef DEBUG_XID
	//dbgprintxid(logfd1, xid);
#endif

	XDROutPacketAddInt32(&rpc_call, xid);
	XDROutPacketAddInt32(&rpc_call, RPC_CALL);
	XDROutPacketAddInt32(&rpc_call, RPC_VERSION);
	XDROutPacketAddInt32(&rpc_call, prog);
	XDROutPacketAddInt32(&rpc_call, vers);
	XDROutPacketAddInt32(&rpc_call, proc);

#if !defined(USE_SYSTEM_AUTHENTICATION)
	XDROutPacketAddInt32(&rpc_call, RPC_AUTH_NONE);
	XDROutPacketAddDynamic (&rpc_call, NULL, 0);
#else
	XDROutPacketAddInt32(&rpc_call, RPC_AUTH_SYS);
	authSize = 4 + 4 + ((strlen(ns->params.server) + 3) &~3) + 4 + 4 + 4;
	XDROutPacketAddInt32(&rpc_call, authSize);
	XDROutPacketAddInt32(&rpc_call, 0);
	XDROutPacketAddString(&rpc_call, ns->params.server);
	XDROutPacketAddInt32(&rpc_call, ns->params.uid);
	XDROutPacketAddInt32(&rpc_call, ns->params.gid);
	XDROutPacketAddInt32(&rpc_call, 0);
#endif

	XDROutPacketAddInt32(&rpc_call, RPC_AUTH_NONE);
	XDROutPacketAddDynamic (&rpc_call, NULL, 0);

	XDROutPacketAppend (&rpc_call, packet);

	pending = RPCPendingCallsAddPendingCall(&ns->pendingCalls, xid, addr);
#ifdef DEBUG_XID
	checksemstate(xid, pending->sem, 0);
#endif

	do {
		ssize_t bytes;
		do {
			bytes = sendto(ns->s,(const void *)XDROutPacketBuffer(&rpc_call),
				XDROutPacketLength(&rpc_call), 0,
				(const struct sockaddr *)addr, sizeof(*addr));
		}
		while (bytes < 0 && errno == EINTR);

		do {
			result = acquire_sem_etc (pending->sem, 1, B_TIMEOUT,
				(retries) ? (conf_call_timeout) : (2*conf_call_timeout));
		}
		while (result == B_INTERRUPTED);

		retries--;
	} while (result == B_TIMED_OUT && retries >= 0);

	if (result >= B_OK) {
		uint8 *buffer = pending->buffer;
		pending->buffer = NULL;
		SemaphorePoolPut(&ns->pendingCalls.fPool, pending->sem);

		PendingCallDestroy(pending);
		free(pending);

		XDROutPacketDestroy(&rpc_call);
		return buffer;
	}

	// we timed out

	call = RPCPendingCallsFindAndRemovePendingCall(&ns->pendingCalls, xid, addr);

	dprintf("nfs: xid %" B_PRId32 " timed out, removing from queue", xid);

#if 0
	if (call==NULL)
	{
#if 1
		//XXX:mmu_man:???
		while (acquire_sem(pending->sem)==B_INTERRUPTED);
#else
		status_t err;
		/* NOTE(mmu_man): there can be a race condition here where the sem is returned
		 * to the pool without the correct value, compromising the next call using it.
		 * however it seems waiting forever can lead to lockups...
		 */
		while ((err = acquire_sem_etc(pending->sem,1,B_TIMEOUT,5000000))==B_INTERRUPTED);
		dprintf("nfs: acquire(pending->sem) = 0x%08lx\n", err);
		if (err == B_TIMED_OUT)
			dprintf("nfs: timed out waiting on sem\n");
#endif
	}
#endif

	/* mmu_man */
	if (call) /* if the call has been found and removed (atomic op), the sem hasn't been released */
		SemaphorePoolPut(&ns->pendingCalls.fPool, pending->sem);
	else
		delete_sem(pending->sem); /* else it's in an unknown state, forget it */

	PendingCallDestroy(pending);
	free(pending);

	XDROutPacketDestroy (&rpc_call);
	return NULL;
}


bool
is_successful_reply(struct XDRInPacket *reply)
{
	bool success = false;

	int32 xid = XDRInPacketGetInt32(reply);
	rpc_msg_type mtype=(rpc_msg_type)XDRInPacketGetInt32(reply);
	rpc_reply_stat replyStat=(rpc_reply_stat)XDRInPacketGetInt32(reply);
	(void)xid;
	(void)mtype;

	if (replyStat == RPC_MSG_DENIED) {
		rpc_reject_stat rejectStat = (rpc_reject_stat)XDRInPacketGetInt32(reply);

		if (rejectStat == RPC_RPC_MISMATCH) {
			int32 low = XDRInPacketGetInt32(reply);
			int32 high = XDRInPacketGetInt32(reply);

			dprintf("nfs: RPC_MISMATCH (%" B_PRId32 ",%" B_PRId32 ")", low,
				high);
		} else {
			rpc_auth_stat authStat = (rpc_auth_stat)XDRInPacketGetInt32(reply);

			dprintf("nfs: RPC_AUTH_ERROR (%d)", authStat);
		}
	} else {
		rpc_auth_flavor flavor = (rpc_auth_flavor)XDRInPacketGetInt32(reply);
		char body[400];
		size_t bodyLength = XDRInPacketGetDynamic(reply, body);

		rpc_accept_stat acceptStat = (rpc_accept_stat)XDRInPacketGetInt32(reply);
		(void)flavor;
		(void)bodyLength;

		if (acceptStat == RPC_PROG_MISMATCH) {
			int32 low = XDRInPacketGetInt32(reply);
			int32 high = XDRInPacketGetInt32(reply);

			dprintf("nfs: RPC_PROG_MISMATCH (%" B_PRId32 ",%" B_PRId32 ")",
				low, high);
		} else if (acceptStat != RPC_SUCCESS)
			dprintf("nfs: Accepted but failed (%d)", acceptStat);
		else
			success = true;
	}

	return success;
}


status_t
get_remote_address(fs_nspace *ns, int32 prog, int32 vers, int32 prot,
	struct sockaddr_in *addr)
{
	struct XDROutPacket call;
	uint8 *replyBuf;

	XDROutPacketInit(&call);

	addr->sin_port = htons(PMAP_PORT);

	XDROutPacketAddInt32(&call, prog);
	XDROutPacketAddInt32(&call, vers);
	XDROutPacketAddInt32(&call, prot);
	XDROutPacketAddInt32(&call, 0);

	replyBuf = send_rpc_call(ns, addr, PMAP_PROGRAM, PMAP_VERSION,
		PMAPPROC_GETPORT, &call);

	if (replyBuf) {
		struct XDRInPacket reply;
		XDRInPacketInit(&reply);

		XDRInPacketSetTo(&reply,replyBuf,0);

		if (is_successful_reply(&reply)) {
			addr->sin_port = htons(XDRInPacketGetInt32(&reply));
			memset(addr->sin_zero, 0, sizeof(addr->sin_zero));

			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_OK;
		}

		XDRInPacketDestroy(&reply);
	}

	XDROutPacketDestroy (&call);
	return EHOSTUNREACH;
}

status_t
nfs_mount(fs_nspace *ns, const char *path, nfs_fhandle *fhandle)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	uint8 *replyBuf;
	int32 fhstatus;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddString(&call,path);

	replyBuf = send_rpc_call(ns, &ns->mountAddr, MOUNT_PROGRAM, MOUNT_VERSION,
		MOUNTPROC_MNT, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	fhstatus = XDRInPacketGetInt32(&reply);

	if (fhstatus != 0) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(fhstatus);
	}

	XDRInPacketGetFixed(&reply, fhandle->opaque, NFS_FHSIZE);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return B_OK;
}


status_t
nfs_lookup (fs_nspace *ns, const nfs_fhandle *dir, const char *filename,
	nfs_fhandle *fhandle, struct stat *st)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	int32 status;
	uint8 *replyBuf;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddFixed(&call, dir->opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, filename);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_LOOKUP, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	XDRInPacketGetFixed(&reply, fhandle->opaque, NFS_FHSIZE);

	if (st)
		get_nfs_attr(&reply, st);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return B_OK;
}


status_t
nfs_getattr(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	uint8 *replyBuf;
	int32 status;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddFixed(&call, fhandle->opaque, NFS_FHSIZE);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_GETATTR, &call);
	if (replyBuf == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);
	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	get_nfs_attr(&reply, st);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return B_OK;
}


status_t
nfs_truncate_file(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	uint8 *replyBuf;
	int32 status;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddFixed(&call, fhandle->opaque, NFS_FHSIZE);

	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, 0);
	XDROutPacketAddInt32(&call, time(NULL));
	XDROutPacketAddInt32(&call, 0);
	XDROutPacketAddInt32(&call, time(NULL));
	XDROutPacketAddInt32(&call, 0);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_SETATTR, &call);
	if (replyBuf == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);
	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	if (st)
		get_nfs_attr(&reply,st);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return B_OK;
}


void
get_nfs_attr(struct XDRInPacket *reply, struct stat *st)
{
	nfs_ftype ftype=(nfs_ftype)XDRInPacketGetInt32(reply);
	(void) ftype;
	st->st_mode=XDRInPacketGetInt32(reply);

	st->st_dev=0;	// just to be sure
	st->st_nlink=XDRInPacketGetInt32(reply);
	st->st_uid=XDRInPacketGetInt32(reply);
	st->st_gid=XDRInPacketGetInt32(reply);
	st->st_size=XDRInPacketGetInt32(reply);
	st->st_blksize=XDRInPacketGetInt32(reply);
	st->st_rdev=XDRInPacketGetInt32(reply);
	st->st_blocks=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// fsid
	st->st_ino=XDRInPacketGetInt32(reply);
	st->st_atime=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// usecs
	st->st_mtime=st->st_crtime=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// usecs
	st->st_ctime=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// usecs
}


status_t
map_nfs_to_system_error(status_t nfsstatus)
{
	switch (nfsstatus) {
		case NFS_OK:
			return B_OK;

		case NFSERR_PERM:
			return EPERM;

		case NFSERR_NOENT:
			return ENOENT;

		case NFSERR_IO:
			return EIO;

		case NFSERR_NXIO:
			return ENXIO;

		case NFSERR_ACCES:
			return EACCES;

		case NFSERR_EXIST:
			return EEXIST;

		case NFSERR_NODEV:
			return ENODEV;

		case NFSERR_NOTDIR:
			return ENOTDIR;

		case NFSERR_ISDIR:
			return EISDIR;

		case NFSERR_FBIG:
			return EFBIG;

		case NFSERR_NOSPC:
			return ENOSPC;

		case NFSERR_ROFS:
			return EROFS;

		case NFSERR_NAMETOOLONG:
			return ENAMETOOLONG;

		case NFSERR_NOTEMPTY:
			return ENOTEMPTY;

		case NFSERR_STALE:
			return C_ERROR_STALE;

		default:
			return B_ERROR;
	}
}


nfs_fhandle
handle_from_vnid(fs_nspace *ns, ino_t vnid)
{
	fs_node *current;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	current = ns->first;

	while (current != NULL && current->vnid != vnid)
		current = current->next;

	while (release_sem(ns->sem) == B_INTERRUPTED);

	return current->fhandle;
}


void
insert_node(fs_nspace *ns, fs_node *node)
{
	fs_node *current;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	current = ns->first;

	while (current != NULL && current->vnid != node->vnid)
		current = current->next;

	if (current) {
		free(node);
		while (release_sem(ns->sem) == B_INTERRUPTED);
		return;
	}

	node->next = ns->first;
	ns->first = node;

	while (release_sem (ns->sem) == B_INTERRUPTED);
}


void
remove_node(fs_nspace *ns, ino_t vnid)
{
	fs_node *current;
	fs_node *previous;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	current = ns->first;
	previous = NULL;

	while (current != NULL && current->vnid != vnid) {
		previous = current;
		current = current->next;
	}

	if (current) {
		if (previous)
			previous->next = current->next;
		else
			ns->first = current->next;

		free(current);
	}

	while (release_sem(ns->sem) == B_INTERRUPTED);
}


//	#pragma mark -


static status_t
fs_read_vnode(fs_volume *_volume, ino_t vnid, fs_vnode *_node, int *_type,
	uint32 *_flags, bool r)
{
	fs_nspace *ns;
	fs_node *current;

	ns = _volume->private_volume;

	if (!r) {
		while (acquire_sem(ns->sem) == B_INTERRUPTED);
	}

	current = ns->first;

	while (current != NULL && current->vnid != vnid)
		current = current->next;

	if (!current)
		return EINVAL;

	current->vnid = vnid;
	_node->private_node = current;
	_node->ops = &sNFSVnodeOps;
	*_type = current->mode;
	*_flags = 0;

	if (!r) {
		while (release_sem(ns->sem) == B_INTERRUPTED);
	}

	return B_OK;
}


static status_t
fs_release_vnode(fs_volume *_volume, fs_vnode *node, bool r)
{
	(void) _volume;
	(void) node;
	(void) r;
	return B_OK;
}


static status_t
fs_walk(fs_volume *_volume, fs_vnode *_base, const char *file, ino_t *vnid)
{
	fs_node *dummy;
	status_t result;
	fs_nspace *ns;
	fs_node *base;
	//dprintf("nfs: walk(%s)\n", file);//XXX:mmu_man:debug

	ns = _volume->private_volume;
	base = _base->private_node;

	if (!strcmp(".", file))
		*vnid = base->vnid;
	else {
		fs_node *newNode = (fs_node *)malloc(sizeof(fs_node));
		struct stat st;

		if ((result = nfs_lookup(ns, &base->fhandle, file, &newNode->fhandle,
			&st)) < B_OK) {
			free(newNode);
			return result;
		}

		newNode->vnid = st.st_ino;
		newNode->mode = st.st_mode;
		*vnid = newNode->vnid;

		insert_node(ns, newNode);
	}

	if ((result = get_vnode (_volume, *vnid, (void **)&dummy)) < B_OK)
		return result;

	return B_OK;
}


static status_t
fs_opendir(fs_volume *_volume, fs_vnode *_node, void **_cookie)
{
	fs_nspace *ns;
	fs_node *node;
	nfs_cookie **cookie;

	struct stat st;
	status_t result;

	ns = _volume->private_volume;
	node = _node->private_node;
	cookie = (nfs_cookie **)_cookie;

	if ((result = nfs_getattr(ns, &node->fhandle, &st)) < B_OK)
		return result;

	if (!S_ISDIR(st.st_mode))
		return ENOTDIR;

	*cookie = (nfs_cookie *)malloc(sizeof(nfs_cookie));
	memset((*cookie)->opaque,0,NFS_COOKIESIZE);

	return B_OK;
}


static status_t
fs_closedir(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	(void) _volume;
	(void) _node;
	(void) cookie;
	return B_OK;
}


static status_t
fs_rewinddir(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	nfs_cookie *cookie = (nfs_cookie *)_cookie;
	(void) _volume;
	(void) _node;
	memset (cookie->opaque, 0, NFS_COOKIESIZE);

	return B_OK;
}


static status_t
fs_readdir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
		struct dirent *buf, size_t bufsize, uint32 *num)
{
	nfs_cookie *cookie = (nfs_cookie *)_cookie;
	uint32 max = *num;
	int32 eof;

	fs_nspace *ns;
	fs_node *node;

	size_t count = min_c(6000, max * 300);

	*num = 0;

	ns = _volume->private_volume;
	node = _node->private_node;

	do {
		ino_t vnid;
		char *filename;
		uint8 *replyBuf;
		struct XDROutPacket call;
		struct XDRInPacket reply;
		int32 status;

		XDROutPacketInit(&call);
		XDRInPacketInit(&reply);

		XDROutPacketAddFixed(&call, node->fhandle.opaque, NFS_FHSIZE);
		XDROutPacketAddFixed(&call, cookie->opaque, NFS_COOKIESIZE);
		XDROutPacketAddInt32(&call, count);

		replyBuf = send_rpc_call (ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
			NFSPROC_READDIR, &call);

		if (!replyBuf) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		XDRInPacketSetTo(&reply, replyBuf, 0);

		if (!is_successful_reply(&reply)) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		status = XDRInPacketGetInt32(&reply);

		if (status != NFS_OK) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return map_nfs_to_system_error(status);
		}

		while (XDRInPacketGetInt32(&reply) == 1) {
			nfs_cookie newCookie;

			vnid=XDRInPacketGetInt32(&reply);
			filename=XDRInPacketGetString(&reply);

			XDRInPacketGetFixed(&reply, newCookie.opaque, NFS_COOKIESIZE);

			//if (strcmp(".",filename)&&strcmp("..",filename))
			//if ((ns->rootid != node->vnid) || (strcmp(".",filename)&&strcmp("..",filename)))
			if (conf_ls_root_parent
				|| ((ns->rootid != node->vnid) || strcmp("..", filename))) {
				status_t result;
				struct stat st;

				fs_node *newNode = (fs_node *)malloc(sizeof(fs_node));
				newNode->vnid = vnid;

				if ((result = nfs_lookup(ns, &node->fhandle, filename,
					&newNode->fhandle, &st)) < B_OK) {
					free (filename);
					free(newNode);
					XDRInPacketDestroy (&reply);
					XDROutPacketDestroy (&call);
					return result;
				}

				newNode->mode = st.st_mode;
				insert_node(ns,newNode);

				if (bufsize < 2 * (sizeof(dev_t) + sizeof(ino_t))
					+ sizeof(unsigned short) + strlen(filename) + 1) {
					XDRInPacketDestroy(&reply);
					XDROutPacketDestroy(&call);
					return B_OK;
				}

				buf->d_dev = ns->nsid;
				buf->d_pdev = ns->nsid;
				buf->d_ino = vnid;
				buf->d_pino = node->vnid;
				buf->d_reclen = 2 * (sizeof(dev_t) + sizeof(ino_t))
					+ sizeof(unsigned short) + strlen(filename) + 1;
				strcpy (buf->d_name,filename);
//				if ((ns->rootid == node->vnid))//XXX:mmu_man:test
//					dprintf("nfs: dirent %d {d:%ld pd:%ld i:%lld pi:%lld '%s'}\n", *num, buf->d_dev, buf->d_pdev, buf->d_ino, buf->d_pino, buf->d_name);

				bufsize -= buf->d_reclen;
				buf = (struct dirent *)((char *)buf + buf->d_reclen);

				memcpy(cookie->opaque, newCookie.opaque, NFS_COOKIESIZE);

				(*num)++;
			} else {
				memcpy(cookie->opaque, newCookie.opaque, NFS_COOKIESIZE);
			}

			free (filename);

			if ((*num) == max) {
				XDRInPacketDestroy(&reply);
				XDROutPacketDestroy(&call);
				return B_OK;
			}
		}

		eof=XDRInPacketGetInt32(&reply);

		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
	}
	while (eof == 0);

	return B_OK;
}


static status_t
fs_free_dircookie(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	(void) _volume;
	(void) _node;
	free(cookie);
	return B_OK;
}


static status_t
fs_rstat(fs_volume *_volume, fs_vnode *_node, struct stat *st)
{
	fs_nspace *ns;
	fs_node *node;
	status_t result;

	ns = _volume->private_volume;
	node = _node->private_node;

	//dprintf("nfs: rstat()\n");//XXX:mmu_man:debug
	if ((result = nfs_getattr(ns, &node->fhandle, st)) < B_OK)
		return result;

	st->st_dev = ns->nsid;
//st->st_nlink = 1; //XXX:mmu_man:test
	return B_OK;
}


void
fs_nspaceInit(struct fs_nspace *nspace)
{
	RPCPendingCallsInit(&nspace->pendingCalls);
}


void
fs_nspaceDestroy(struct fs_nspace *nspace)
{
	RPCPendingCallsDestroy(&nspace->pendingCalls);
}


static status_t
parse_nfs_params(const char *str, struct mount_nfs_params *params)
{
	const char *p, *e;
	long v;
	int i;
	// sprintf(buf, "nfs:%s:%s,uid=%u,gid=%u,hostname=%s",
	if (!str || !params)
		return EINVAL;
	if (strncmp(str, "nfs:", 4))
		return EINVAL;
dprintf("nfs:ip!\n");
	p = str + 4;
	e = strchr(p, ':');
	if (!e)
		return EINVAL;
	params->server = malloc(e - p + 1);
	params->server[e - p] = '\0';
	strncpy(params->server, p, e - p);
	// hack
	params->serverIP = 0;
	v = strtol(p, (char **)&p, 10);
dprintf("IP:%ld.", v);
	if (!p)
		return EINVAL;
	params->serverIP |= (v << 24);
	p++;
	v = strtol(p, (char **)&p, 10);
dprintf("%ld.", v);
	if (!p)
		return EINVAL;
	params->serverIP |= (v << 16);
	p++;
	v = strtol(p, (char **)&p, 10);
dprintf("%ld.", v);
	if (!p)
		return EINVAL;
	params->serverIP |= (v << 8);
	p++;
	v = strtol(p, (char **)&p, 10);
dprintf("%ld\n", v);
	if (!p)
		return EINVAL;
	params->serverIP |= (v);
	if (*p++ != ':')
		return EINVAL;

	e = strchr(p, ',');
	i = (e) ? (e - p) : (strlen(p));

	params->_export = malloc(i + 1);
	params->_export[i] = '\0';
	strncpy(params->_export, p, i);

	p = strstr(str, "hostname=");
	if (!p)
		return EINVAL;
dprintf("nfs:hn!\n");
	p += 9;
	e = strchr(p, ',');
	i = (e) ? (e - p) : (strlen(p));

	params->hostname = malloc(i + 1);
	params->hostname[i] = '\0';
	strncpy(params->hostname, p, i);

	p = strstr(str, "uid=");
dprintf("nfs:uid!\n");
	if (p) {
		p += 4;
		v = strtol(p, (char **)&p, 10);
		params->uid = v;
	}
dprintf("nfs:gid!\n");
	p = strstr(str, "gid=");
	if (p) {
		p += 4;
		v = strtol(p, (char **)&p, 10);
		params->gid = v;
	}
	dprintf("nfs: ip:%08x server:'%s' export:'%s' hostname:'%s' uid=%d gid=%d \n",
		params->serverIP, params->server, params->_export,
		params->hostname, params->uid, params->gid);
	return B_OK;
}


static status_t
fs_mount(fs_volume *_vol, const char *devname, uint32 flags, const char *_parms, ino_t *vnid)
{
	status_t result;
	fs_nspace *ns;
	fs_node *rootNode;
	struct stat st;

	if (_parms == NULL)
		return EINVAL;

	dprintf("nfs: mount(%" B_PRId32 ", %s, %08" B_PRIx32 ")\n", _vol->id,
		devname, flags);
	dprintf("nfs: nfs_params: %s\n", _parms);

	// HAIKU: this should go to std_ops
	if (!refcount)
		read_config();


	result = ENOMEM;
	ns = (fs_nspace *)malloc(sizeof(fs_nspace));
	if (!ns)
		goto err_nspace;
	fs_nspaceInit(ns);

	ns->nsid = _vol->id;

	ns->params.server = NULL;
	ns->params._export = NULL;
	ns->params.hostname = NULL;
	if ((result = parse_nfs_params(_parms, &ns->params)) < 0)
		goto err_params;
	ns->xid = 0;
	ns->mountAddr.sin_family = AF_INET;
	ns->mountAddr.sin_addr.s_addr = htonl(ns->params.serverIP);
	memset(ns->mountAddr.sin_zero, 0, sizeof(ns->mountAddr.sin_zero));

	if ((result = create_socket(ns)) < B_OK) {
		dprintf("nfs: could not create socket (%" B_PRId32 ")\n", result);
		goto err_socket;
	}

	if ((result = init_postoffice(ns)) < B_OK) {
		dprintf("nfs: could not init_postoffice() (%" B_PRId32 ")\n", result);
		goto err_postoffice;
	}

	if ((result = get_remote_address(ns, MOUNT_PROGRAM, MOUNT_VERSION,
			PMAP_IPPROTO_UDP, &ns->mountAddr)) < B_OK) {
		dprintf("could not get_remote_address() (%" B_PRId32 ")\n", result);
		goto err_sem;
	}

	memcpy(&ns->nfsAddr, &ns->mountAddr, sizeof(ns->mountAddr));
dprintf("nfs: mountd at %08x:%d\n", ns->mountAddr.sin_addr.s_addr, ntohs(ns->mountAddr.sin_port));

	if ((result = get_remote_address(ns, NFS_PROGRAM, NFS_VERSION,
			PMAP_IPPROTO_UDP, &ns->nfsAddr)) < B_OK)
		goto err_sem;
dprintf("nfs: nfsd at %08x:%d\n", ns->nfsAddr.sin_addr.s_addr, ntohs(ns->nfsAddr.sin_port));
//	result = connect_socket(ns);
//dprintf("nfs: connect: %s\n", strerror(result));

	if ((result = create_sem(1, "nfs_sem")) < B_OK)
		goto err_sem;

	ns->sem = result;

	set_sem_owner(ns->sem, B_SYSTEM_TEAM);

	result = ENOMEM;
	rootNode = (fs_node *)malloc(sizeof(fs_node));
	if (!rootNode)
		goto err_rootvn;
	rootNode->next = NULL;

	if ((result = nfs_mount(ns, ns->params._export, &rootNode->fhandle)) < B_OK)
		goto err_mount;

	if ((result = nfs_getattr(ns, &rootNode->fhandle, &st)) < B_OK)
		goto err_publish;

	ns->rootid = st.st_ino;
	rootNode->vnid = ns->rootid;

	*vnid = ns->rootid;

	_vol->private_volume = ns;
	_vol->ops = &sNFSVolumeOps;

	// TODO: set right mode
	if ((result = publish_vnode(_vol, *vnid, rootNode, &sNFSVnodeOps,
					S_IFDIR, 0)) < B_OK)
		goto err_publish;

	ns->first = rootNode;

	return B_OK;

err_publish:
	// XXX: unmount ??
err_mount:
	free(rootNode);
err_rootvn:
	delete_sem (ns->sem);
err_sem:
	shutdown_postoffice(ns);
	goto err_socket;
err_postoffice:
	close(ns->s);
err_socket:
err_params:
	free(ns->params.hostname);
	free(ns->params._export);
	free(ns->params.server);

	fs_nspaceDestroy(ns);
	free(ns);
err_nspace:

	if (result >= 0) {
		dprintf("nfs:bad error from mount!\n");
		result = EINVAL;
	}
	dprintf("nfs: error in nfs_mount: %s\n", strerror(result));
	return result;
}


static status_t
fs_unmount(fs_volume *_volume)
{
	fs_nspace *ns = (fs_nspace *)_volume->private_volume;
	free(ns->params.hostname);
	free(ns->params._export);
	free(ns->params.server);

	while (ns->first) {
		fs_node *next = ns->first->next;
		free(ns->first);
		ns->first = next;
	}

	// We need to put the reference to our root node ourselves
	put_vnode(_volume, ns->rootid);

	delete_sem(ns->sem);
	shutdown_postoffice(ns);
	fs_nspaceDestroy(ns);
	free(ns);
	return B_OK;
}


static status_t
fs_rfsstat(fs_volume *_volume, struct fs_info *info)
{
	fs_nspace *ns;
	struct XDROutPacket call;
	struct XDRInPacket reply;
	nfs_fhandle rootHandle;
	uint8 *replyBuf;
	int32 status;

	ns = (fs_nspace *)_volume->private_volume;

	rootHandle = handle_from_vnid (ns,ns->rootid);
	//dprintf("nfs: rfsstat()\n");//XXX:mmu_man:debug

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddFixed(&call, rootHandle.opaque, NFS_FHSIZE);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_STATFS, &call);
	if (replyBuf == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);
	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		//dprintf("nfs: rfsstat() error 0x%08lx\n", map_nfs_to_system_error(status));
		return map_nfs_to_system_error(status);
	}

	info->dev = ns->nsid;
	info->root = ns->rootid;
	info->flags = NFS_FS_FLAGS;

	XDRInPacketGetInt32(&reply);	// tsize

	info->block_size = XDRInPacketGetInt32(&reply);
	info->io_size = 8192;
	info->total_blocks = XDRInPacketGetInt32(&reply);
	info->free_blocks = XDRInPacketGetInt32(&reply);
	info->total_nodes = 100;
	info->free_nodes = 100;
	strcpy(info->volume_name, "nfs://");
	strcat(info->volume_name, ns->params.server);
	strcat(info->volume_name, ns->params._export);
	strcpy(info->fsh_name, "nfs");

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return B_OK;
}


static status_t
fs_open(fs_volume *_volume, fs_vnode *_node, int omode, void **_cookie)
{
	fs_nspace *ns;
	fs_node *node;
	struct stat st;
	status_t result;
	fs_file_cookie **cookie;

	ns = _volume->private_volume;
	node = _node->private_node;
	cookie = (fs_file_cookie **)_cookie;

	if ((result = nfs_getattr(ns, &node->fhandle, &st)) < B_OK)
		return result;

	if (S_ISDIR(st.st_mode)) {
		/* permit opening of directories */
		if (conf_allow_dir_open) {
			*cookie = NULL;
			return B_OK;
		} else
			return EISDIR;
	}

	*cookie = (fs_file_cookie *)malloc(sizeof(fs_file_cookie));
	(*cookie)->omode = omode;
	(*cookie)->original_size = st.st_size;
	(*cookie)->st = st;

	return B_OK;
}


static status_t
fs_close(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	(void) _volume;
	(void) _node;
	(void) cookie;
/*	//XXX:mmu_man:why that ?? makes Tracker go nuts updating the stats
	if ((cookie->omode & O_RDWR)||(cookie->omode & O_WRONLY))
		return my_notify_listener (B_STAT_CHANGED,ns->nsid,0,0,node->vnid,NULL);
*/
	return B_OK;
}


static status_t
fs_free_cookie(fs_volume *_volume, fs_vnode *_node, void *cookie)
{
	(void) _volume;
	(void) _node;
	free(cookie);
	return B_OK;
}


static status_t
fs_read(fs_volume *_volume, fs_vnode *_node, void *_cookie, off_t pos,
	void *buf, size_t *len)
{
	fs_nspace *ns;
	fs_node *node;
	fs_file_cookie *cookie;
	size_t max = *len;
	*len = 0;

	ns = _volume->private_volume;
	node = _node->private_node;
	cookie = (fs_file_cookie *)_cookie;

	if (!cookie)
		return EISDIR; /* do not permit reading of directories */

	while ((*len) < max) {
		size_t count = min_c(NFS_MAXDATA, max - (*len));
		struct XDROutPacket call;
		struct XDRInPacket reply;
		int32 status;
		uint8 *replyBuf;
		struct stat st;
		size_t readbytes;

		XDROutPacketInit(&call);
		XDRInPacketInit(&reply);

		XDROutPacketAddFixed(&call, &node->fhandle.opaque, NFS_FHSIZE);
		XDROutPacketAddInt32(&call, pos);
		XDROutPacketAddInt32(&call, count);
		XDROutPacketAddInt32(&call, 0);

		replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
			NFSPROC_READ, &call);
		if (replyBuf == NULL) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		XDRInPacketSetTo(&reply, replyBuf, 0);

		if (!is_successful_reply(&reply)) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		status = XDRInPacketGetInt32(&reply);
		if (status != NFS_OK) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return map_nfs_to_system_error(status);
		}

		get_nfs_attr(&reply, &st);
		cookie->st = st;

		readbytes = XDRInPacketGetDynamic(&reply, buf);

		buf = (char *)buf + readbytes;
		(*len) += readbytes;
		pos += readbytes;

		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);

		if (pos >= st.st_size)
			break;
	}

	return B_OK;
}


static status_t
fs_write(fs_volume *_volume, fs_vnode *_node, void *_cookie, off_t pos,
	const void *buf, size_t *len)
{
	fs_nspace *ns;
	fs_node *node;
	fs_file_cookie *cookie;
	size_t bytesWritten = 0;

	ns = _volume->private_volume;
	node = _node->private_node;
	cookie = (fs_file_cookie *)_cookie;

	if (!cookie)
		return EISDIR; /* do not permit reading of directories */
	if (cookie->omode & O_APPEND)
		pos += cookie->original_size;

	while (bytesWritten < *len) {
		size_t count = min_c(NFS_MAXDATA,(*len) - bytesWritten);

		struct XDROutPacket call;
		struct XDRInPacket reply;
		int32 status;
		uint8 *replyBuf;
		struct stat st;

		XDROutPacketInit(&call);
		XDRInPacketInit(&reply);

		XDROutPacketAddFixed(&call, &node->fhandle.opaque, NFS_FHSIZE);
		XDROutPacketAddInt32(&call, 0);
		XDROutPacketAddInt32(&call, pos + bytesWritten);
		XDROutPacketAddInt32(&call, 0);
		XDROutPacketAddDynamic(&call, (const char *)buf + bytesWritten, count);

		replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
			NFSPROC_WRITE, &call);

		if (!replyBuf) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		XDRInPacketSetTo(&reply, replyBuf, 0);

		if (!is_successful_reply(&reply)) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		status = XDRInPacketGetInt32(&reply);

		if (status != NFS_OK) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return map_nfs_to_system_error(status);
		}

		get_nfs_attr(&reply, &st);

		cookie->st = st;

		bytesWritten += count;

		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
	}

	return B_OK;
}


static status_t
fs_wstat(fs_volume *_volume, fs_vnode *_node, const struct stat *st, uint32 mask)
{
	fs_nspace *ns;
	fs_node *node;
	struct XDROutPacket call;
	struct XDRInPacket reply;

	uint8 *replyBuf;
	int32 status;

	ns = _volume->private_volume;
	node = _node->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddFixed(&call,node->fhandle.opaque,NFS_FHSIZE);

	XDROutPacketAddInt32(&call, (mask & WSTAT_MODE) ? st->st_mode : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_UID) ? st->st_uid : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_GID) ? st->st_gid : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_SIZE) ? st->st_size : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_ATIME) ? st->st_atime : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_ATIME) ? 0 : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_MTIME) ? st->st_mtime : -1);
	XDROutPacketAddInt32(&call, (mask & WSTAT_MTIME) ? 0 : -1);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_SETATTR, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK)
		return map_nfs_to_system_error(status);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);

	return notify_stat_changed(_volume->id, -1, node->vnid, mask);
}

static status_t
fs_wfsstat(fs_volume *_volume, const struct fs_info *info, uint32 mask)
{
	(void) _volume;
	(void) info;
	(void) mask;
	return B_OK;
}

static status_t
fs_create(fs_volume *_volume, fs_vnode *_dir, const char *name, int omode,
	int perms, void **_cookie, ino_t *vnid)
{
	nfs_fhandle fhandle;
	struct stat st;
	fs_file_cookie **cookie;

	fs_nspace *ns;
	fs_node *dir;

	status_t result;

	ns = _volume->private_volume;
	dir = _dir->private_node;
	cookie = (fs_file_cookie **)_cookie;

	result = nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st);

	if (result == B_OK) {
		void *dummy;
		fs_node *newNode = (fs_node *)malloc(sizeof(fs_node));
		if (newNode == NULL)
			return B_NO_MEMORY;

		newNode->fhandle = fhandle;
		newNode->vnid = st.st_ino;
		newNode->mode = st.st_mode;
		insert_node(ns, newNode);

		*vnid = st.st_ino;

		if ((result = get_vnode(_volume,*vnid,&dummy)) < B_OK)
			return result;

		if (S_ISDIR(st.st_mode))
			return EISDIR;

		if (omode & O_EXCL)
			return EEXIST;

		if (omode & O_TRUNC)
		{
			if ((result = nfs_truncate_file(ns, &fhandle, NULL)) < B_OK)
				return result;
		}

		*cookie=(fs_file_cookie *)malloc(sizeof(fs_file_cookie));
		if (*cookie == NULL)
			return B_NO_MEMORY;

		(*cookie)->omode=omode;
		(*cookie)->original_size=st.st_size;
		(*cookie)->st=st;

		return B_OK;
	} else if (result != ENOENT) {
		return result;
	} else {
		struct XDROutPacket call;
		struct XDRInPacket reply;

		uint8 *replyBuf;
		int32 status;

		fs_node *newNode;

		if (!(omode & O_CREAT))
			return ENOENT;

		XDROutPacketInit(&call);
		XDRInPacketInit(&reply);

		XDROutPacketAddFixed(&call, dir->fhandle.opaque, NFS_FHSIZE);
		XDROutPacketAddString(&call, name);
		XDROutPacketAddInt32(&call, perms | S_IFREG);
		XDROutPacketAddInt32(&call, -1);
		XDROutPacketAddInt32(&call, -1);
		XDROutPacketAddInt32(&call, 0);
		XDROutPacketAddInt32(&call, time(NULL));
		XDROutPacketAddInt32(&call, 0);
		XDROutPacketAddInt32(&call, time(NULL));
		XDROutPacketAddInt32(&call, 0);

		replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
			NFSPROC_CREATE, &call);

		if (!replyBuf) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		XDRInPacketSetTo(&reply, replyBuf, 0);

		if (!is_successful_reply(&reply)) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_ERROR;
		}

		status = XDRInPacketGetInt32(&reply);

		if (status != NFS_OK) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return map_nfs_to_system_error(status);
		}

		XDRInPacketGetFixed(&reply, fhandle.opaque, NFS_FHSIZE);

		get_nfs_attr(&reply,&st);

		newNode = (fs_node *)malloc(sizeof(fs_node));
		if (newNode == NULL) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_NO_MEMORY;
		}
		newNode->fhandle = fhandle;
		newNode->vnid = st.st_ino;
		newNode->mode = st.st_mode;

		insert_node (ns, newNode);

		*vnid = st.st_ino;
		*cookie = (fs_file_cookie *)malloc(sizeof(fs_file_cookie));
		if (*cookie == NULL) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return B_NO_MEMORY;
		}
		(*cookie)->omode = omode;
		(*cookie)->original_size = st.st_size;
		(*cookie)->st = st;

		result = new_vnode(_volume, *vnid, newNode, &sNFSVnodeOps);

		if (result < B_OK) {
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy(&call);
			return result;
		}

		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return notify_entry_created(_volume->id, dir->vnid, name, *vnid);
	}
}


static status_t
fs_unlink(fs_volume *_volume, fs_vnode *_dir, const char *name)
{
	status_t result;
	fs_nspace *ns;
	fs_node *dir;
	fs_node *newNode;
	fs_node *dummy;

	struct XDROutPacket call;
	struct XDRInPacket reply;

	struct stat st;
	nfs_fhandle fhandle;
	uint8 *replyBuf;

	int32 status;

	ns = _volume->private_volume;
	dir = _dir->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	if ((result = nfs_lookup(ns, &dir->fhandle, name, &fhandle, &st)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	newNode = (fs_node *)malloc(sizeof(fs_node));
	if (newNode == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_NO_MEMORY;
	}
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	newNode->mode = st.st_mode;

	insert_node(ns, newNode);

	if ((result = get_vnode(_volume, st.st_ino, (void **)&dummy)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EISDIR;
	}

	if ((result=remove_vnode(_volume,st.st_ino)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	if ((result=put_vnode(_volume, st.st_ino)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	XDROutPacketAddFixed(&call, dir->fhandle.opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, name);

	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_REMOVE,&call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);

	return notify_entry_removed(_volume->id, dir->vnid, name, st.st_ino);
}


static status_t
fs_remove_vnode(fs_volume *_volume, fs_vnode *_node, bool r)
{
	fs_nspace *ns = _volume->private_volume;
	fs_node *node = _node->private_node;

	(void) r;

	remove_node (ns, node->vnid);

	return B_OK;
}


static status_t
fs_mkdir(fs_volume *_volume, fs_vnode *_dir, const char *name, int perms)
{
	fs_nspace *ns;
	fs_node *dir;

	nfs_fhandle fhandle;
	struct stat st;
	fs_node *newNode;

	status_t result;
	uint8 *replyBuf;
	int32 status;

	struct XDROutPacket call;
	struct XDRInPacket reply;

	ns = _volume->private_volume;
	dir = _dir->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	result = nfs_lookup(ns, &dir->fhandle, name, &fhandle, &st);

	if (result == B_OK) {
		//void *dummy;

		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		// XXX: either OK or not get_vnode !!! ??
		//if ((result=get_vnode(_volume,st.st_ino,&dummy))<B_OK)
		//	return result;
		return EEXIST;
	} else if (result != ENOENT) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	XDROutPacketAddFixed(&call, dir->fhandle.opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, name);
	XDROutPacketAddInt32(&call, perms | S_IFDIR);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, time(NULL));
	XDROutPacketAddInt32(&call, 0);
	XDROutPacketAddInt32(&call, time(NULL));
	XDROutPacketAddInt32(&call, 0);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_MKDIR, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK) {
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	XDRInPacketGetFixed(&reply, fhandle.opaque, NFS_FHSIZE);

	get_nfs_attr(&reply, &st);

	newNode=(fs_node *)malloc(sizeof(fs_node));
	if (newNode == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_NO_MEMORY;
	}
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	newNode->mode = st.st_mode;

	insert_node(ns, newNode);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);

	return notify_entry_created(_volume->id, dir->vnid, name, st.st_ino);
}

static status_t
fs_rename(fs_volume *_volume, fs_vnode *_olddir, const char *oldname,
		fs_vnode *_newdir, const char *newname)
{
	struct stat st;
	nfs_fhandle fhandle;
	status_t result;
	struct XDROutPacket call;
	struct XDRInPacket reply;
	int32 status;
	uint8 *replyBuf;
	fs_nspace *ns;
	fs_node *olddir;
	fs_node *newdir;

	ns = _volume->private_volume;
	olddir = _olddir->private_node;
	newdir = _newdir->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	if ((result = nfs_lookup(ns, &newdir->fhandle, newname, &fhandle, &st))
		== B_OK) {
		if (S_ISREG(st.st_mode))
			result = fs_unlink (_volume,_newdir,newname);
		else
			result = fs_rmdir (_volume,_newdir,newname);

		if (result < B_OK) {
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return result;
		}
	}

	if ((result = nfs_lookup(ns, &olddir->fhandle, oldname, &fhandle, &st))
		< B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	XDROutPacketAddFixed(&call, olddir->fhandle.opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, oldname);
	XDROutPacketAddFixed(&call, newdir->fhandle.opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, newname);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_RENAME, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);

	return notify_entry_moved(_volume->id, olddir->vnid, oldname, newdir->vnid,
		newname, st.st_ino);
}


static status_t
fs_rmdir(fs_volume *_volume, fs_vnode *_dir, const char *name)
{
	fs_nspace *ns;
	fs_node *dir;

	status_t result;
	fs_node *newNode;
	fs_node *dummy;
	struct XDROutPacket call;
	struct XDRInPacket reply;
	int32 status;
	uint8 *replyBuf;

	struct stat st;
	nfs_fhandle fhandle;

	ns = _volume->private_volume;
	dir = _dir->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	if ((result = nfs_lookup(ns, &dir->fhandle, name, &fhandle, &st)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	newNode = (fs_node *)malloc(sizeof(fs_node));
	if (newNode == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_NO_MEMORY;
	}
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	newNode->mode = st.st_mode;

	insert_node(ns, newNode);

	if ((result = get_vnode(_volume, st.st_ino, (void **)&dummy)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	if (!S_ISDIR(st.st_mode)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return ENOTDIR;
	}

	if ((result = remove_vnode(_volume, st.st_ino)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	if ((result = put_vnode(_volume, st.st_ino)) < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	XDROutPacketAddFixed (&call, dir->fhandle.opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, name);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_RMDIR, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return notify_entry_removed(_volume->id, dir->vnid, name, st.st_ino);
}


static status_t
fs_readlink(fs_volume *_volume, fs_vnode *_node, char *buf, size_t *bufsize)
{
	struct XDROutPacket call;
	uint8 *replyBuf;
	int32 status;
	size_t length;
	char data[NFS_MAXPATHLEN];
	struct XDRInPacket reply;
	fs_nspace *ns;
	fs_node *node;

	ns = _volume->private_volume;
	node = _node->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	XDROutPacketAddFixed(&call, node->fhandle.opaque, NFS_FHSIZE);

	replyBuf = send_rpc_call(ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_READLINK, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo (&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);

	if (status != NFS_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);
	}

	length = XDRInPacketGetDynamic(&reply, data);

	length = min_c(length, *bufsize);
	memcpy(buf, data, length);
	*bufsize = length;

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return B_OK;
}

static status_t
fs_symlink(fs_volume *_volume, fs_vnode *_dir, const char *name,
	const char *path, int mode)
{
	fs_nspace *ns;
	fs_node *dir;
	nfs_fhandle fhandle;
	struct stat st;
	struct XDROutPacket call;
	struct XDRInPacket reply;
	status_t result;
	uint8 *replyBuf;
	int32 status;
	fs_node *newNode;

	ns = _volume->private_volume;
	dir = _dir->private_node;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);

	result = nfs_lookup(ns, &dir->fhandle, name, &fhandle, &st);

	if (result == B_OK) {
		void *dummy;
		if ((result = get_vnode(_volume, st.st_ino, &dummy)) < B_OK)
			return result;

		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EEXIST;
	} else if (result != ENOENT) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	XDROutPacketAddFixed(&call, dir->fhandle.opaque, NFS_FHSIZE);
	XDROutPacketAddString(&call, name);
	XDROutPacketAddString(&call, path);
	XDROutPacketAddInt32(&call, S_IFLNK);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, -1);
	XDROutPacketAddInt32(&call, time(NULL));
	XDROutPacketAddInt32(&call, 0);
	XDROutPacketAddInt32(&call, time(NULL));
	XDROutPacketAddInt32(&call, 0);

	replyBuf = send_rpc_call (ns, &ns->nfsAddr, NFS_PROGRAM, NFS_VERSION,
		NFSPROC_SYMLINK, &call);

	if (!replyBuf) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	XDRInPacketSetTo(&reply, replyBuf, 0);

	if (!is_successful_reply(&reply)) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}

	status = XDRInPacketGetInt32(&reply);
/*	if (status!=NFS_OK)
		return map_nfs_to_system_error(status);

	ignore status here, weird thing, nfsservers that is
*/
	(void)status;

	result = nfs_lookup(ns, &dir->fhandle, name, &fhandle, &st);

	if (result < B_OK) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}

	newNode = (fs_node *)malloc(sizeof(fs_node));
	if (newNode == NULL) {
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_NO_MEMORY;
	}
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;

	insert_node(ns, newNode);

	result = notify_entry_created (_volume->id, dir->vnid, name, st.st_ino);

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return result;
}


static status_t
fs_access(fs_volume *_volume, fs_vnode *node, int mode)
{
	(void) _volume;
	(void) node;
	(void) mode;
	/* XXX */
	return B_OK;
}


static status_t
nfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


fs_volume_ops sNFSVolumeOps = {
	&fs_unmount,
	&fs_rfsstat,
	&fs_wfsstat,
	NULL,			// no sync!
	&fs_read_vnode,

	/* index directory & index operations */
	NULL,	// &fs_open_index_dir
	NULL,	// &fs_close_index_dir
	NULL,	// &fs_free_index_dir_cookie
	NULL,	// &fs_read_index_dir
	NULL,	// &fs_rewind_index_dir

	NULL,	// &fs_create_index
	NULL,	// &fs_remove_index
	NULL,	// &fs_stat_index

	/* query operations */
	NULL,	// &fs_open_query,
	NULL,	// &fs_close_query,
	NULL,	// &fs_free_query_cookie,
	NULL,	// &fs_read_query,
	NULL,	// &fs_rewind_query,
};


fs_vnode_ops sNFSVnodeOps = {
	/* vnode operations */
	&fs_walk,
	NULL, // fs_get_vnode_name
	&fs_release_vnode,
	&fs_remove_vnode,

	/* VM file access */
	NULL, 	// &fs_can_page
	NULL,	// &fs_read_pages
	NULL, 	// &fs_write_pages

	NULL,	// io()
	NULL,	// cancel_io()

	NULL,	// &fs_get_file_map,

	NULL, 	// &fs_ioctl
	NULL,	// &fs_setflags,
	NULL,	// &fs_select
	NULL,	// &fs_deselect
	NULL, 	// &fs_fsync

	&fs_readlink,
	&fs_symlink,

	NULL,	// &fs_link,
	&fs_unlink,
	&fs_rename,

	&fs_access,
	&fs_rstat,
	&fs_wstat,
	NULL,	// fs_preallocate()

	/* file operations */
	&fs_create,
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	&fs_write,

	/* directory operations */
	&fs_mkdir,
	&fs_rmdir,
	&fs_opendir,
	&fs_closedir,
	&fs_free_dircookie,
	&fs_readdir,
	&fs_rewinddir,

	/* attribute directory operations */
	NULL,	// &fs_open_attrdir,
	NULL,	// &fs_close_attrdir,
	NULL,	// &fs_free_attrdircookie,
	NULL,	// &fs_read_attrdir,
	NULL,	// &fs_rewind_attrdir,

	/* attribute operations */
	NULL,	// &fs_create_attr
	NULL,	// &fs_open_attr_h,
	NULL,	// &fs_close_attr_h,
	NULL,	// &fs_free_attr_cookie_h,
	NULL,	// &fs_read_attr_h,
	NULL,	// &fs_write_attr_h,

	NULL,	// &fs_read_attr_stat_h,
	NULL,	// &fs_write_attr_stat
	NULL,	// &fs_rename_attr
	NULL,	// &fs_remove_attr
};

file_system_module_info sNFSFileSystem = {
	{
		"file_systems/nfs" B_CURRENT_FS_API_VERSION,
		0,
		nfs_std_ops,
	},
	"nfs",				// short name
	"Network File System v2",	// pretty name
	B_DISK_SYSTEM_SUPPORTS_WRITING, // DDM flags

	// scanning
	NULL,	// fs_identify_partition,
	NULL,	// fs_scan_partition,
	NULL,	// fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,
};

module_info *modules[] = {
	(module_info *)&sNFSFileSystem,
	NULL,
};
