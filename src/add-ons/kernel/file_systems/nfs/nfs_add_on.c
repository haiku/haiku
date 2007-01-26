#include <posix/stdlib.h>

#include "nfs_add_on.h"
#include "ksocket.h"

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

/* Haiku requires a string for mount params... */
#define PARAMS_AS_STRING 1

#ifndef UDP_SIZE_MAX
#define UDP_SIZE_MAX 65515
#endif
#define B_UDP_MAX_SIZE UDP_SIZE_MAX

/* declare gSocket and others */
KSOCKET_MODULE_DECL;

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

/* *** DEBUG stuff *** */
//#define my_notify_listener(p_a, p_b, p_c, p_d, p_e, p_f) ({dprintf("nfs: notify_listener(%08lx)\n", p_a); notify_listener(p_a, p_b, p_c, p_d, p_e, p_f); })
#define my_notify_listener notify_listener

static vint32 refcount = 0; /* we only want to read the config once ? */

static status_t read_config(void)
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

extern status_t 
create_socket(fs_nspace *ns)
{
	struct sockaddr_in addr;
	uint16 port=conf_high_port;
	
	ns->s=ksocket(AF_INET,SOCK_DGRAM,0);
	
	if (ns->s<0)
		return errno;

	do
	{
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=htonl(INADDR_ANY);
		//addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
		addr.sin_port=htons(port);
		memset (addr.sin_zero,0,sizeof(addr.sin_zero));
		
		if (kbind(ns->s,(const struct sockaddr *)&addr,sizeof(addr))<0)
		{
			if (errno!=EADDRINUSE)
			{
				int result=errno;
				kclosesocket(ns->s);
				return result;
			}

			port--;
			if (port==conf_low_port)
			{
				kclosesocket(ns->s);
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

extern status_t 
connect_socket(fs_nspace *ns)
{
	uint16 port=conf_high_port;
	
	struct sockaddr_in addr;
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	//addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	addr.sin_port=htons(port);
	memset (addr.sin_zero,0,sizeof(addr.sin_zero));
	
	if (kconnect(ns->s,(const struct sockaddr *)&ns->nfsAddr,sizeof(ns->nfsAddr))<0)
	{
		return -1;
	}
	return B_OK;
}

extern status_t 
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

extern void
shutdown_postoffice(fs_nspace *ns)
{
	status_t result;
	
	ns->quit=true;
	kclosesocket(ns->s);
	
	wait_for_thread (ns->tid,&result);
}

extern status_t 
postoffice_func(fs_nspace *ns)
{
	uint8 *buffer=(uint8 *)malloc(B_UDP_MAX_SIZE);
	
	while (!ns->quit)
	{		
		struct sockaddr_in from;
		socklen_t fromLen=sizeof(from);

		ssize_t bytes=krecvfrom (ns->s,buffer,B_UDP_MAX_SIZE,0,(struct sockaddr *)&from,&fromLen);
		
		if (bytes>=4)
		{
			struct PendingCall *call;
			int32 xid=B_BENDIAN_TO_HOST_INT32(*((int32 *)buffer));
			
			call=RPCPendingCallsFindAndRemovePendingCall(&ns->pendingCalls,xid,&from);

			if (call)
			{
				call->buffer=(uint8 *)malloc(bytes);
				memcpy (call->buffer,buffer,bytes);
				
				while (release_sem (call->sem)==B_INTERRUPTED);
			} else {
				dprintf("nfs: postoffice: can't find pending call to remove for xid %ld\n", xid);
			}
		}
	}

	free (buffer);
		
	return B_OK;
}

uint8 *send_rpc_call (fs_nspace *ns, const struct sockaddr_in *addr,
							int32 prog, int32 vers, int32 proc, 
							const struct XDROutPacket *packet)
{
	int32 xid;
	size_t authSize;
	struct PendingCall *pending;
	int32 retries=conf_call_tries;
	status_t result;
	struct PendingCall *call;
	
	struct XDROutPacket rpc_call;
	XDROutPacketInit (&rpc_call);
	
	xid=atomic_add(&ns->xid,1);
#ifdef DEBUG_XID
	//dbgprintxid(logfd1, xid);
#endif
	
	XDROutPacketAddInt32(&rpc_call,xid);
	XDROutPacketAddInt32(&rpc_call,RPC_CALL);
	XDROutPacketAddInt32(&rpc_call,RPC_VERSION);
	XDROutPacketAddInt32(&rpc_call,prog);
	XDROutPacketAddInt32(&rpc_call,vers);
	XDROutPacketAddInt32(&rpc_call,proc);
	
#if !defined(USE_SYSTEM_AUTHENTICATION)
	XDROutPacketAddInt32(&rpc_call,RPC_AUTH_NONE);
	XDROutPacketAddDynamic (&rpc_call,NULL,0);
#else
	XDROutPacketAddInt32(&rpc_call,RPC_AUTH_SYS);
	authSize=4+4+((strlen(ns->params.server)+3)&~3)+4+4+4;
	XDROutPacketAddInt32(&rpc_call,authSize);
	XDROutPacketAddInt32(&rpc_call,0);
	XDROutPacketAddString(&rpc_call,ns->params.server);
	XDROutPacketAddInt32(&rpc_call,ns->params.uid);
	XDROutPacketAddInt32(&rpc_call,ns->params.gid);
	XDROutPacketAddInt32(&rpc_call,0);
#endif
		
	XDROutPacketAddInt32(&rpc_call,RPC_AUTH_NONE);
	XDROutPacketAddDynamic (&rpc_call,NULL,0);

	XDROutPacketAppend (&rpc_call,packet);

	pending=RPCPendingCallsAddPendingCall(&ns->pendingCalls,xid,addr);
#ifdef DEBUG_XID
	checksemstate(xid, pending->sem, 0);
#endif

	do
	{		
		ssize_t bytes;
		do
		{
			bytes=ksendto (ns->s,(const void *)XDROutPacketBuffer(&rpc_call),
								XDROutPacketLength(&rpc_call),0,
							(const struct sockaddr *)addr,sizeof(*addr));
		}
		while ((bytes<0)&&(errno==EINTR));
	
		do
		{
			result=acquire_sem_etc (pending->sem,1,B_TIMEOUT,(retries)?(conf_call_timeout):(2*conf_call_timeout));
		}
		while (result==B_INTERRUPTED);
		
		retries--;
	}
	while ((result==B_TIMED_OUT)&&(retries>=0));

	if (result>=B_OK)
	{
		uint8 *buffer=pending->buffer;
		pending->buffer=NULL;
		SemaphorePoolPut(&ns->pendingCalls.fPool,pending->sem);
		
		PendingCallDestroy(pending);
		free(pending);
		
		XDROutPacketDestroy (&rpc_call);
		return buffer;
	}

	// we timed out

	call=RPCPendingCallsFindAndRemovePendingCall(&ns->pendingCalls,xid,addr);	

	kmessage ("nfs: xid %ld timed out, removing from queue",xid);

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
	
	free(pending->buffer);
	
	/* mmu_man */
	if (call) /* if the call has been found and removed (atomic op), the sem hasn't been released */
		SemaphorePoolPut(&ns->pendingCalls.fPool,pending->sem);
	else
		delete_sem(pending->sem); /* else it's in an unknown state, forget it */

	PendingCallDestroy (pending);
	free(pending);
	
	XDROutPacketDestroy (&rpc_call);
	return NULL;
}

bool is_successful_reply (struct XDRInPacket *reply)
{
	bool success=false;
	
	int32 xid=XDRInPacketGetInt32(reply);
	rpc_msg_type mtype=(rpc_msg_type)XDRInPacketGetInt32(reply);
	rpc_reply_stat replyStat=(rpc_reply_stat)XDRInPacketGetInt32(reply);
	(void)xid;
	(void)mtype;
	
	if (replyStat==RPC_MSG_DENIED)
	{
		rpc_reject_stat rejectStat=(rpc_reject_stat)XDRInPacketGetInt32(reply);
		
		if (rejectStat==RPC_RPC_MISMATCH)
		{
			int32 low=XDRInPacketGetInt32(reply);
			int32 high=XDRInPacketGetInt32(reply);
			
			kmessage ("RPC_MISMATCH (%ld,%ld)",low,high);
		}
		else
		{
			rpc_auth_stat authStat=(rpc_auth_stat)XDRInPacketGetInt32(reply);
			
			kmessage ("RPC_AUTH_ERROR (%d)",authStat);
		}
	}
	else
	{
		rpc_auth_flavor flavor=(rpc_auth_flavor)XDRInPacketGetInt32(reply);
		char body[400];
		size_t bodyLength=XDRInPacketGetDynamic(reply,body);
		
		rpc_accept_stat acceptStat=(rpc_accept_stat)XDRInPacketGetInt32(reply);
		(void)flavor;
		(void)bodyLength;
		
		if (acceptStat==RPC_PROG_MISMATCH)
		{
			int32 low=XDRInPacketGetInt32(reply);
			int32 high=XDRInPacketGetInt32(reply);
			
			kmessage ("RPC_PROG_MISMATCH (%ld,%ld)",low,high);				
		}
		else if (acceptStat!=RPC_SUCCESS)
			kmessage ("Accepted but failed (%d)",acceptStat);
		else
			success=true;
	}

	return success;	
}

status_t get_remote_address (fs_nspace *ns, int32 prog, int32 vers, int32 prot, struct sockaddr_in *addr)
{
	struct XDROutPacket call;
	uint8 *replyBuf;
	
	XDROutPacketInit (&call);
		
	addr->sin_port=htons(PMAP_PORT);

	XDROutPacketAddInt32 (&call,prog);		
	XDROutPacketAddInt32 (&call,vers);
	XDROutPacketAddInt32 (&call,prot);
	XDROutPacketAddInt32 (&call,0);
	
	replyBuf=send_rpc_call (ns,addr,PMAP_PROGRAM,PMAP_VERSION,PMAPPROC_GETPORT,&call);

	if (replyBuf)
	{
		struct XDRInPacket reply;
		XDRInPacketInit (&reply);
		
		XDRInPacketSetTo (&reply,replyBuf,0);

		if (is_successful_reply(&reply))
		{
			addr->sin_port=htons(XDRInPacketGetInt32(&reply));
			memset (addr->sin_zero,0,sizeof(addr->sin_zero));
			
			XDRInPacketDestroy(&reply);
			XDROutPacketDestroy (&call);
			return B_OK;
		}

		XDRInPacketDestroy(&reply);
	}
	
	XDROutPacketDestroy (&call);
	return EHOSTUNREACH;
}

status_t nfs_mount (fs_nspace *ns, const char *path, nfs_fhandle *fhandle)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	uint8 *replyBuf;
	int32 fhstatus;

	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
	
	XDROutPacketAddString (&call,path);
	
	replyBuf=send_rpc_call (ns,&ns->mountAddr,MOUNT_PROGRAM,MOUNT_VERSION,MOUNTPROC_MNT,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
		
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	fhstatus=XDRInPacketGetInt32(&reply);
	
	if (fhstatus!=0)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(fhstatus);
	}
			
	XDRInPacketGetFixed (&reply,fhandle->opaque,NFS_FHSIZE);		
					
	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return B_OK;
}					

status_t nfs_lookup (fs_nspace *ns, const nfs_fhandle *dir, const char *filename, nfs_fhandle *fhandle,
						struct stat *st)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	int32 status;
	uint8 *replyBuf;
	
	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
	
	XDROutPacketAddFixed (&call,dir->opaque,NFS_FHSIZE);
	XDROutPacketAddString (&call,filename);
	
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_LOOKUP,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo(&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);
	}
		
	XDRInPacketGetFixed (&reply,fhandle->opaque,NFS_FHSIZE);

	if (st)
		get_nfs_attr (&reply,st);

	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return B_OK;
}

status_t nfs_getattr (fs_nspace *ns, const nfs_fhandle *fhandle,
						struct stat *st)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	uint8 *replyBuf;
	int32 status;
		
	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
	
	XDROutPacketAddFixed (&call,fhandle->opaque,NFS_FHSIZE);
	
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_GETATTR,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);
	}
		
	get_nfs_attr (&reply,st);

	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return B_OK;
}

extern status_t 
nfs_truncate_file(fs_nspace *ns, const nfs_fhandle *fhandle, struct stat *st)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	uint8 *replyBuf;
	int32 status;
	
	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
		
	XDROutPacketAddFixed(&call,fhandle->opaque,NFS_FHSIZE);

	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,0);
	XDROutPacketAddInt32 (&call,time(NULL));	
	XDROutPacketAddInt32 (&call,0);
	XDROutPacketAddInt32 (&call,time(NULL));	
	XDROutPacketAddInt32 (&call,0);

	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_SETATTR,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo(&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);
	}
	
	if (st)	
		get_nfs_attr (&reply,st);

	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return B_OK;
}

void get_nfs_attr (struct XDRInPacket *reply, struct stat *st)
{
	nfs_ftype ftype=(nfs_ftype)XDRInPacketGetInt32(reply);
	(void) ftype;
	st->st_mode=XDRInPacketGetInt32(reply);
	
	st->st_dev=0;	// just to be sure
	st->st_nlink=XDRInPacketGetInt32(reply);
	st->st_uid=XDRInPacketGetInt32(reply);
	st->st_gid=XDRInPacketGetInt32(reply);
	st->st_size=XDRInPacketGetInt32(reply);
#if 0
	XDRInPacketGetInt32(reply);	// blksize
	st->st_blksize=NFS_MAXDATA;
#else
	st->st_blksize=XDRInPacketGetInt32(reply);
#endif
	st->st_rdev=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// blocks
	XDRInPacketGetInt32(reply);	// fsid
	st->st_ino=XDRInPacketGetInt32(reply);
	st->st_atime=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// usecs
	st->st_mtime=st->st_crtime=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// usecs
	st->st_ctime=XDRInPacketGetInt32(reply);
	XDRInPacketGetInt32(reply);	// usecs
}

status_t map_nfs_to_system_error (status_t nfsstatus)
{
	switch (nfsstatus)
	{
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

extern nfs_fhandle 
handle_from_vnid(fs_nspace *ns, vnode_id vnid)
{
	fs_node *current;
	
	while (acquire_sem (ns->sem)==B_INTERRUPTED);
	
	current=ns->first;
	
	while ((current)&&(current->vnid!=vnid))
		current=current->next;
	
	while (release_sem (ns->sem)==B_INTERRUPTED);
		
	return current->fhandle;
}

void insert_node (fs_nspace *ns, fs_node *node)
{
	fs_node *current;
	
	while (acquire_sem (ns->sem)==B_INTERRUPTED);
	
	current=ns->first;

	while ((current)&&(current->vnid!=node->vnid))
		current=current->next;

	if (current)
	{
		free(node);
		while (release_sem (ns->sem)==B_INTERRUPTED);
		return;
	}
	
	node->next=ns->first;
	ns->first=node;
	
	while (release_sem (ns->sem)==B_INTERRUPTED);
}

void remove_node (fs_nspace *ns, vnode_id vnid)
{
	fs_node *current;
	fs_node *previous;
	
	while(acquire_sem (ns->sem)==B_INTERRUPTED);	
	
	current=ns->first;
	previous=NULL;
	
	while ((current)&&(current->vnid!=vnid))
	{
		previous=current;
		current=current->next;
	}
	
	if (current)
	{
		if (previous)
			previous->next=current->next;
		else
			ns->first=current->next;
		
		free(current);
	}
		
	while (release_sem (ns->sem)==B_INTERRUPTED);
}

extern int 
#ifdef __HAIKU__
fs_read_vnode(fs_nspace *ns, vnode_id vnid, fs_node **node, char r)
#else
fs_read_vnode(fs_nspace *ns, vnode_id vnid, char r, fs_node **node)
#endif
{
	fs_node *current;
	
	if (!r)
	{
		while (acquire_sem (ns->sem)==B_INTERRUPTED);
	}
	
	current=ns->first;

	while ((current)&&(current->vnid!=vnid))
		current=current->next;

	if (!current)
		return EINVAL;
		
	*node=current;
	
	if (!r)	
	{
		while (release_sem (ns->sem)==B_INTERRUPTED);
	}
	
	return B_OK;	
}

extern int 
#ifdef __HAIKU__
fs_release_vnode(fs_nspace *ns, fs_node *node, char r)
#else
fs_write_vnode(fs_nspace *ns, fs_node *node, char r)
#endif
{
	(void) ns;
	(void) node;
	(void) r;
	return B_OK;
}

#ifdef __HAIKU__
extern int
fs_get_vnode_name(fs_nspace *ns, fs_node *node, char *buffer, size_t len)
{
	return ENOSYS;
}
#endif

extern int 
#ifdef __HAIKU__
fs_walk(fs_nspace *ns, fs_node *base, const char *file, vnode_id *vnid, int *type)
#else
fs_walk(fs_nspace *ns, fs_node *base, const char *file, char **newpath, vnode_id *vnid)
#endif
{
	bool isLink;
	fs_node *dummy;
	status_t result;
	//dprintf("nfs: walk(%s)\n", file);//XXX:mmu_man:debug
	
	if (!strcmp(".",file))
	{
		*vnid=base->vnid;
#ifdef __HAIKU__
		*type = S_IFDIR;
#endif
		isLink=false;
	}
	else
	{
		fs_node *newNode=(fs_node *)malloc(sizeof(fs_node));
		struct stat st;
		
		if ((result=nfs_lookup(ns,&base->fhandle,file,&newNode->fhandle,&st))<B_OK)
		{
			free(newNode);
			return result;
		}
		
		newNode->vnid=st.st_ino;
		*vnid=newNode->vnid;
#ifdef __HAIKU__
		*type = st.st_mode & S_IFMT;
#endif
		
		insert_node (ns,newNode);
		
		isLink=S_ISLNK(st.st_mode);
	}
	
	if ((result=get_vnode (ns->nsid,*vnid,(void **)&dummy))<B_OK)
		return result;

#ifndef __HAIKU__
	if ((isLink)&&(newpath))
	{
		char path[NFS_MAXPATHLEN+1];
		size_t pathLen=NFS_MAXPATHLEN;
		
		if ((result=fs_readlink(ns,dummy,path,&pathLen))<B_OK)
		{
			put_vnode (ns->nsid,*vnid);
			return result;
		}
			
		path[pathLen]=0;
		
		result=new_path(path,newpath);
		
		if (result<B_OK)
		{
			put_vnode (ns->nsid,*vnid);
			return result;
		}
			
		return put_vnode (ns->nsid,*vnid);
	}
#endif
	
	return B_OK;
}

extern int 
fs_opendir(fs_nspace *ns, fs_node *node, nfs_cookie **cookie)
{
	struct stat st;
	
	status_t result;
	if ((result=nfs_getattr(ns,&node->fhandle,&st))<B_OK)
		return result;
	
	if (!S_ISDIR(st.st_mode))
		return ENOTDIR;

	*cookie=(nfs_cookie *)malloc(sizeof(nfs_cookie));
	memset ((*cookie)->opaque,0,NFS_COOKIESIZE);
			
	return B_OK;
}

extern int 
fs_closedir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie)
{
	(void) ns;
	(void) node;
	(void) cookie;
	return B_OK;
}

extern int 
fs_rewinddir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie)
{
	(void) ns;
	(void) node;
	memset (cookie->opaque,0,NFS_COOKIESIZE);
	
	return B_OK;
}

extern int
#ifdef __HAIKU__
fs_readdir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie, struct dirent *buf, size_t bufsize, uint32 *num)
#else
fs_readdir(fs_nspace *ns, fs_node *node, nfs_cookie *cookie, long *num, struct dirent *buf, size_t bufsize)
#endif
{
	int32 max=*num;
	int32 eof;
	
	size_t count=min_c(6000,max*300);
	
	*num=0;

	do
	{
		vnode_id vnid;
		char *filename;
		uint8 *replyBuf;
		struct XDROutPacket call;
		struct XDRInPacket reply;
		int32 status;
		
		XDROutPacketInit (&call);
		XDRInPacketInit (&reply);
		
		XDROutPacketAddFixed (&call,node->fhandle.opaque,NFS_FHSIZE);
		XDROutPacketAddFixed (&call,cookie->opaque,NFS_COOKIESIZE);
		XDROutPacketAddInt32 (&call,count);
			
		replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_READDIR,&call);
	
		if (!replyBuf)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
			
		XDRInPacketSetTo(&reply,replyBuf,0);
	
		if (!is_successful_reply(&reply))
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
					
		status=XDRInPacketGetInt32(&reply);
		
		if (status!=NFS_OK)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return map_nfs_to_system_error(status);
		}
				
		while (XDRInPacketGetInt32(&reply)==1)
		{			
			nfs_cookie newCookie;

			vnid=XDRInPacketGetInt32(&reply);			
			filename=XDRInPacketGetString(&reply);
			
			XDRInPacketGetFixed(&reply,newCookie.opaque,NFS_COOKIESIZE);
			
			//if (strcmp(".",filename)&&strcmp("..",filename))
			//if ((ns->rootid != node->vnid) || (strcmp(".",filename)&&strcmp("..",filename)))
			if (conf_ls_root_parent || ((ns->rootid != node->vnid) || strcmp("..",filename)))
			{		
				status_t result;
				struct stat st;

				fs_node *newNode=(fs_node *)malloc(sizeof(fs_node));
				newNode->vnid=vnid;
				
				if ((result=nfs_lookup(ns,&node->fhandle,filename,&newNode->fhandle,&st))<B_OK)
				{
					free (filename);
					free(newNode);
					XDRInPacketDestroy (&reply);
					XDROutPacketDestroy (&call);
					return result;
				}
				
				insert_node (ns,newNode);	

				if (bufsize<2*(sizeof(dev_t)+sizeof(ino_t))+sizeof(unsigned short)+strlen(filename)+1)
				{
					XDRInPacketDestroy (&reply);
					XDROutPacketDestroy (&call);
					return B_OK;
				}
				
				buf->d_dev=ns->nsid;
				buf->d_pdev=ns->nsid;
				buf->d_ino=vnid;
				buf->d_pino=node->vnid;
				buf->d_reclen=2*(sizeof(dev_t)+sizeof(ino_t))+sizeof(unsigned short)+strlen(filename)+1;
				strcpy (buf->d_name,filename);
//				if ((ns->rootid == node->vnid))//XXX:mmu_man:test
//					dprintf("nfs: dirent %d {d:%ld pd:%ld i:%lld pi:%lld '%s'}\n", *num, buf->d_dev, buf->d_pdev, buf->d_ino, buf->d_pino, buf->d_name);

				bufsize-=buf->d_reclen;
				buf=(struct dirent *)((char *)buf+buf->d_reclen);

				memcpy (cookie->opaque,newCookie.opaque,NFS_COOKIESIZE);

				(*num)++;
			}	
			else {
				memcpy (cookie->opaque,newCookie.opaque,NFS_COOKIESIZE);
			}
				
			free (filename);
			
			if ((*num)==max)
			{
				XDRInPacketDestroy (&reply);
				XDROutPacketDestroy (&call);
				return B_OK;
			}
		}
		
		eof=XDRInPacketGetInt32(&reply);

		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
	}
	while (eof==0);
	
	return B_OK;
}

extern int 
fs_free_dircookie(fs_nspace *ns, fs_node *node, nfs_cookie *cookie)
{
	(void) ns;
	(void) node;
	free(cookie);
	return B_OK;
}

extern int 
fs_rstat(fs_nspace *ns, fs_node *node, struct stat *st)
{
	status_t result;
	//dprintf("nfs: rstat()\n");//XXX:mmu_man:debug
	if ((result=nfs_getattr(ns,&node->fhandle,st))<B_OK)
		return result;
		
	st->st_dev=ns->nsid;
//st->st_nlink = 1; //XXX:mmu_man:test
	return B_OK;
}

extern void 
fs_nspaceInit(struct fs_nspace *nspace)
{
	RPCPendingCallsInit (&nspace->pendingCalls);
}

extern void 
fs_nspaceDestroy(struct fs_nspace *nspace)
{
	RPCPendingCallsDestroy (&nspace->pendingCalls);
}

#ifdef PARAMS_AS_STRING
int parse_nfs_params(const char *str, struct mount_nfs_params *params)
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
	params->server[e-p] = '\0';
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
	
	params->_export = malloc(i+1);
	params->_export[i] = '\0';
	strncpy(params->_export, p, i);
	
	p = strstr(str, "hostname=");
	if (!p)
		return EINVAL;
dprintf("nfs:hn!\n");
	p += 9;
	e = strchr(p, ',');
	i = (e) ? (e - p) : (strlen(p));
	
	params->hostname = malloc(i+1);
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
#endif

extern int 
#ifdef __HAIKU__
fs_mount(nspace_id nsid, const char *devname, uint32 flags, const char *_parms, fs_nspace **data, vnode_id *vnid)
#else
fs_mount(nspace_id nsid, const char *devname, ulong flags, const char *_parms, size_t len, fs_nspace **data, vnode_id *vnid)
#endif
{
#ifndef PARAMS_AS_STRING
	struct mount_nfs_params *parms = (struct mount_nfs_params *)_parms; // XXX: FIXME
#endif
	status_t result;
	fs_nspace *ns;
	fs_node *rootNode;
	struct stat st;
#ifndef __HAIKU__
	(void) len;
#endif

	if (_parms==NULL)
		return EINVAL;

dprintf("nfs: mount(%ld, %s, %08lx)\n", nsid, devname, flags);
#ifndef PARAMS_AS_STRING
dprintf("nfs: nfs_params(ip:%lu, server:%s, export:%s, uid:%d, gid:%d, hostname:%s)\n", 
        parms->serverIP, 
        parms->server, 
        parms->_export, 
        parms->uid, 
        parms->gid, 
        parms->hostname);
#else
dprintf("nfs: nfs_params: %s\n", _parms);
#endif

	// HAIKU: this should go to std_ops
	if (!refcount)
		read_config();
	
	result = ksocket_init();
	if (result < B_OK)
		return result;

	result = ENOMEM;
	ns=(fs_nspace *)malloc(sizeof(fs_nspace));
	if (!ns)
		goto err_nspace;
	fs_nspaceInit (ns);

	ns->nsid=nsid;
	
	ns->params.server=NULL;
	ns->params._export=NULL;
	ns->params.hostname=NULL;
#ifdef PARAMS_AS_STRING
	if ((result = parse_nfs_params(_parms, &ns->params)) < 0)
		goto err_params;
#else
	ns->params.serverIP=parms->serverIP;
	ns->params.server=strdup(parms->server);
	ns->params._export=strdup(parms->_export);
	ns->params.uid=parms->uid;
	ns->params.gid=parms->gid;
	ns->params.hostname=strdup(parms->hostname);
#endif
	ns->xid=0;
	ns->mountAddr.sin_family=AF_INET;
	ns->mountAddr.sin_addr.s_addr=htonl(ns->params.serverIP);
	memset (ns->mountAddr.sin_zero,0,sizeof(ns->mountAddr.sin_zero));

	if ((result=create_socket(ns))<B_OK)
		goto err_socket;

	if ((result=init_postoffice(ns))<B_OK)
		goto err_postoffice;

	if ((result=get_remote_address(ns,MOUNT_PROGRAM,MOUNT_VERSION,PMAP_IPPROTO_UDP,&ns->mountAddr))<B_OK)
		goto err_sem;

	memcpy (&ns->nfsAddr,&ns->mountAddr,sizeof(ns->mountAddr));
dprintf("nfs: mountd at %08lx:%d\n", ns->mountAddr.sin_addr.s_addr, ntohs(ns->mountAddr.sin_port));
	
	if ((result=get_remote_address(ns,NFS_PROGRAM,NFS_VERSION,PMAP_IPPROTO_UDP,&ns->nfsAddr))<B_OK)
		goto err_sem;
dprintf("nfs: nfsd at %08lx:%d\n", ns->nfsAddr.sin_addr.s_addr, ntohs(ns->nfsAddr.sin_port));
//	result = connect_socket(ns);
//dprintf("nfs: connect: %s\n", strerror(result));

	if ((result=create_sem(1,"nfs_sem"))<B_OK)
		goto err_sem;

	ns->sem=result;
	
	set_sem_owner (ns->sem,B_SYSTEM_TEAM);
		
	result = ENOMEM;
	rootNode=(fs_node *)malloc(sizeof(fs_node));
	if (!rootNode)
		goto err_rootvn;
	rootNode->next=NULL;
	
	if ((result=nfs_mount(ns,ns->params._export,&rootNode->fhandle))<B_OK)
		goto err_mount;

	if ((result=nfs_getattr(ns,&rootNode->fhandle,&st))<B_OK)
		goto err_publish;

	ns->rootid=st.st_ino;
	rootNode->vnid=ns->rootid;
			
	*vnid=ns->rootid;

	if ((result=publish_vnode(nsid,*vnid,rootNode))<B_OK)
		goto err_publish;
	
	*data=ns;

	ns->first=rootNode;

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
	kclosesocket(ns->s);
err_socket:
err_params:
	free (ns->params.hostname);
	free (ns->params._export);
	free (ns->params.server);

	fs_nspaceDestroy (ns);
	free(ns);
err_nspace:

	ksocket_cleanup();
	if (result >= 0) {
		dprintf("nfs:bad error from mount!\n");
		result = EINVAL;
	}
	dprintf("nfs: error in nfs_mount: %s\n", strerror(result));
	return result;
}

extern int 
fs_unmount(fs_nspace *ns)
{
	free (ns->params.hostname);
	free (ns->params._export);
	free (ns->params.server);
	
	while (ns->first)
	{
		fs_node *next=ns->first->next;
		free(ns->first);
		ns->first=next;
	}

	// Unlike in BeOS, we need to put the reference to our root node ourselves
#ifdef __HAIKU__
	put_vnode(ns->nsid, ns->rootid);
#endif

	delete_sem (ns->sem);
	shutdown_postoffice(ns);
	fs_nspaceDestroy (ns);
	free(ns);
	ksocket_cleanup();
	return B_OK;
}

extern int 
fs_rfsstat(fs_nspace *ns, struct fs_info *info)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	nfs_fhandle rootHandle=handle_from_vnid (ns,ns->rootid);
	uint8 *replyBuf;
	int32 status;
	//dprintf("nfs: rfsstat()\n");//XXX:mmu_man:debug
		
	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
		
	XDROutPacketAddFixed (&call,rootHandle.opaque,NFS_FHSIZE);
	
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_STATFS,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo(&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		//dprintf("nfs: rfsstat() error 0x%08lx\n", map_nfs_to_system_error(status));
		return map_nfs_to_system_error(status);
	}
	
	info->dev=ns->nsid;
	info->root=ns->rootid;
	info->flags=NFS_FS_FLAGS;

	XDRInPacketGetInt32(&reply);	// tsize

	info->block_size=XDRInPacketGetInt32(&reply);
	info->io_size=8192;
	info->total_blocks=XDRInPacketGetInt32(&reply);
	info->free_blocks=XDRInPacketGetInt32(&reply);
	info->total_nodes=100;
	info->free_nodes=100;
	//strcpy (info->device_name,"nfs_device");
	strcpy (info->device_name,"");
	strcpy (info->volume_name,"nfs://");
	strcat (info->volume_name,ns->params.server);
	strcat (info->volume_name,ns->params._export);
	//strcpy (info->fsh_name,"nfs_fsh");
	strcpy (info->fsh_name,"nfs");

	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return B_OK;
}

extern int 
fs_open(fs_nspace *ns, fs_node *node, int omode, fs_file_cookie **cookie)
{
	struct stat st;
	
	status_t result;
	if ((result=nfs_getattr(ns,&node->fhandle,&st))<B_OK)
		return result;
	
	if (S_ISDIR(st.st_mode)) {
		/* permit opening of directories */
		if (conf_allow_dir_open) {
			*cookie = NULL;
			return B_OK;
		} else
			return EISDIR;
	}

	*cookie=(fs_file_cookie *)malloc(sizeof(fs_file_cookie));
	(*cookie)->omode=omode;
	(*cookie)->original_size=st.st_size;
	(*cookie)->st=st;
	
	return B_OK;
}

extern int 
fs_close(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie)
{
	(void) ns;
	(void) node;
	(void) cookie;
/*	//XXX:mmu_man:why that ?? makes Tracker go nuts updating the stats
	if ((cookie->omode & O_RDWR)||(cookie->omode & O_WRONLY))
		return my_notify_listener (B_STAT_CHANGED,ns->nsid,0,0,node->vnid,NULL);
*/		
	return B_OK;
}

extern int 
fs_free_cookie(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie)
{
	(void) ns;
	(void) node;
	if (cookie)
		free(cookie);
	return B_OK;
}

extern int 
fs_read(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, void *buf, size_t *len)
{
	size_t max=*len;
	*len=0;
	
	if (!cookie) return EISDIR; /* do not permit reading of directories */
	while ((*len)<max)
	{
		size_t count=min_c(NFS_MAXDATA,max-(*len));
		
		struct XDROutPacket call;
		struct XDRInPacket reply;

		int32 status;
		uint8 *replyBuf;
		
		struct stat st;
		size_t readbytes;
		
		XDROutPacketInit (&call);
		XDRInPacketInit (&reply);
		
		XDROutPacketAddFixed (&call,&node->fhandle.opaque,NFS_FHSIZE);
		XDROutPacketAddInt32 (&call,pos);
		XDROutPacketAddInt32 (&call,count);
		XDROutPacketAddInt32 (&call,0);
		
		replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_READ,&call);
	
		if (!replyBuf)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
		
		XDRInPacketSetTo(&reply,replyBuf,0);
	
		if (!is_successful_reply(&reply))
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
					
		status=XDRInPacketGetInt32(&reply);
		
		if (status!=NFS_OK)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return map_nfs_to_system_error(status);
		}
					
		get_nfs_attr (&reply,&st);

		cookie->st=st;
				
		readbytes=XDRInPacketGetDynamic(&reply,buf);
		
		buf=(char *)buf+readbytes;
		(*len)+=readbytes;
		pos+=readbytes;
		
		if (pos>=st.st_size)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			break;
		}
		
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
	}
	
	return B_OK;
}

extern int 
fs_write(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, const void *buf, size_t *len)
{
	size_t bytesWritten=0;
	
	if (!cookie) return EISDIR; /* do not permit reading of directories */
	if (cookie->omode & O_APPEND)
		pos+=cookie->original_size;
			
	while (bytesWritten<*len)
	{
		size_t count=min_c(NFS_MAXDATA,(*len)-bytesWritten);

		struct XDROutPacket call;
		struct XDRInPacket reply;
		int32 status;
		uint8 *replyBuf;
		struct stat st;
		
		XDROutPacketInit (&call);
		XDRInPacketInit (&reply);
				
		XDROutPacketAddFixed (&call,&node->fhandle.opaque,NFS_FHSIZE);
		XDROutPacketAddInt32 (&call,0);
		XDROutPacketAddInt32 (&call,pos+bytesWritten);
		XDROutPacketAddInt32 (&call,0);
		XDROutPacketAddDynamic (&call,(const char *)buf+bytesWritten,count);
		
		replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_WRITE,&call);
	
		if (!replyBuf)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
			
		XDRInPacketSetTo (&reply,replyBuf,0);
	
		if (!is_successful_reply(&reply))
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
			
		status=XDRInPacketGetInt32(&reply);
		
		if (status!=NFS_OK)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return map_nfs_to_system_error(status);
		}
					
		get_nfs_attr (&reply,&st);

		cookie->st=st;

		bytesWritten+=count;		

		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
	}

	return B_OK;
}

extern int 
fs_wstat(fs_nspace *ns, fs_node *node, struct stat *st, long mask)
{
	struct XDROutPacket call;
	struct XDRInPacket reply;
	
	uint8 *replyBuf;
	int32 status;
	
	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
	
	XDROutPacketAddFixed (&call,node->fhandle.opaque,NFS_FHSIZE);

	XDROutPacketAddInt32 (&call,(mask & WSTAT_MODE) ? st->st_mode : -1);
	XDROutPacketAddInt32 (&call,(mask & WSTAT_UID) ? st->st_uid : -1);
	XDROutPacketAddInt32 (&call,(mask & WSTAT_GID) ? st->st_gid : -1);
	XDROutPacketAddInt32 (&call,(mask & WSTAT_SIZE) ? st->st_size : -1);
	XDROutPacketAddInt32 (&call,(mask & WSTAT_ATIME) ? st->st_atime : -1);	
	XDROutPacketAddInt32 (&call,(mask & WSTAT_ATIME) ? 0 : -1);
	XDROutPacketAddInt32 (&call,(mask & WSTAT_MTIME) ? st->st_mtime : -1);	
	XDROutPacketAddInt32 (&call,(mask & WSTAT_MTIME) ? 0 : -1);

	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_SETATTR,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}

	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
		return map_nfs_to_system_error(status);
	
	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return my_notify_listener (B_STAT_CHANGED,ns->nsid,0,0,node->vnid,NULL);
}

extern int 
fs_wfsstat(fs_nspace *ns, struct fs_info *info, long mask)
{
	(void) ns;
	(void) info;
	(void) mask;
	return B_OK;
}

extern int 
#ifdef __HAIKU__
fs_create(fs_nspace *ns, fs_node *dir, const char *name, int omode, int perms, fs_file_cookie **cookie, vnode_id *vnid)
#else
fs_create(fs_nspace *ns, fs_node *dir, const char *name, int omode, int perms, vnode_id *vnid, fs_file_cookie **cookie)
#endif
{
	nfs_fhandle fhandle;
	struct stat st;
	
	status_t result;
	result=nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st);
	
	if (result==B_OK)
	{
		void *dummy;
		fs_node *newNode=(fs_node *)malloc(sizeof(fs_node));
		newNode->fhandle=fhandle;
		newNode->vnid=st.st_ino;
		insert_node (ns,newNode);

		*vnid=st.st_ino;

		if ((result=get_vnode(ns->nsid,*vnid,&dummy))<B_OK)
			return result;

		if (S_ISDIR(st.st_mode))
			return EISDIR;
										
		if (omode & O_EXCL)
			return EEXIST;

		if (omode & O_TRUNC)
		{
			if ((result=nfs_truncate_file(ns,&fhandle,NULL))<B_OK)
				return result;
		}
		
		*cookie=(fs_file_cookie *)malloc(sizeof(fs_file_cookie));
		(*cookie)->omode=omode;
		(*cookie)->original_size=st.st_size;
		(*cookie)->st=st;
					
		return B_OK;
	}
	else if (result!=ENOENT)
		return result;
	else
	{
		struct XDROutPacket call;
		struct XDRInPacket reply;

		uint8 *replyBuf;
		int32 status;

		fs_node *newNode;
				
		if (!(omode & O_CREAT))
			return ENOENT;

		XDROutPacketInit (&call);
		XDRInPacketInit (&reply);
				
		XDROutPacketAddFixed (&call,dir->fhandle.opaque,NFS_FHSIZE);
		XDROutPacketAddString(&call,name);
		XDROutPacketAddInt32 (&call,perms|S_IFREG);
		XDROutPacketAddInt32 (&call,-1);
		XDROutPacketAddInt32 (&call,-1);
		XDROutPacketAddInt32 (&call,0);
		XDROutPacketAddInt32 (&call,time(NULL));
		XDROutPacketAddInt32 (&call,0);
		XDROutPacketAddInt32 (&call,time(NULL));
		XDROutPacketAddInt32 (&call,0);
				
		replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_CREATE,&call);
	
		if (!replyBuf)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
				
		XDRInPacketSetTo (&reply,replyBuf,0);
	
		if (!is_successful_reply(&reply))
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return B_ERROR;
		}
					
		status=XDRInPacketGetInt32(&reply);
		
		if (status!=NFS_OK)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return map_nfs_to_system_error(status);
		}
				
		XDRInPacketGetFixed (&reply,fhandle.opaque,NFS_FHSIZE);
		
		get_nfs_attr (&reply,&st);

		newNode=(fs_node *)malloc(sizeof(fs_node));
		newNode->fhandle=fhandle;
		newNode->vnid=st.st_ino;

		insert_node (ns,newNode);

		*vnid=st.st_ino;
		*cookie=(fs_file_cookie *)malloc(sizeof(fs_file_cookie));
		(*cookie)->omode=omode;
		(*cookie)->original_size=st.st_size;
		(*cookie)->st=st;
					
		result=new_vnode (ns->nsid,*vnid,newNode);
		
		if (result<B_OK)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return result;
		}
				
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return my_notify_listener (B_ENTRY_CREATED,ns->nsid,dir->vnid,0,*vnid,name);
	}	
}

extern int 
fs_unlink(fs_nspace *ns, fs_node *dir, const char *name)
{
	status_t result;
	fs_node *newNode;
	fs_node *dummy;

	struct XDROutPacket call;	
	struct XDRInPacket reply;

	struct stat st;
	nfs_fhandle fhandle;
	uint8 *replyBuf;

	int32 status;
	
	XDROutPacketInit (&call);	
	XDRInPacketInit (&reply);
	
	if ((result=nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st))<B_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return result;
	}
	
	newNode=(fs_node *)malloc(sizeof(fs_node));
	newNode->fhandle=fhandle;
	newNode->vnid=st.st_ino;

	insert_node (ns,newNode);

	if ((result=get_vnode(ns->nsid,st.st_ino,(void **)&dummy))<B_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return result;
	}
	
	if (!S_ISREG(st.st_mode)&&!S_ISLNK(st.st_mode))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return EISDIR;
	}
	
	if ((result=remove_vnode(ns->nsid,st.st_ino))<B_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return result;
	}
	
	if ((result=put_vnode(ns->nsid,st.st_ino))<B_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return result;
	}
											
	XDROutPacketAddFixed (&call,dir->fhandle.opaque,NFS_FHSIZE);
	XDROutPacketAddString(&call,name);
			
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_REMOVE,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo(&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);	
		return map_nfs_to_system_error(status);
	}
	
	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);	
	
	return my_notify_listener (B_ENTRY_REMOVED,ns->nsid,dir->vnid,0,st.st_ino,name);
}

extern int 
fs_remove_vnode(fs_nspace *ns, fs_node *node, char r)
{
	(void) r;
	remove_node (ns,node->vnid);

	return B_OK;
}

#ifndef __HAIKU__
extern int 
fs_secure_vnode(fs_nspace *ns, fs_node *node)
{
	(void) ns;
	(void) node;
	return B_OK;
}
#endif

extern int 
fs_mkdir(fs_nspace *ns, fs_node *dir, const char *name, int perms)
{
	nfs_fhandle fhandle;
	struct stat st;
	fs_node *newNode;
	
	status_t result;
	uint8 *replyBuf;
	int32 status;
	
	struct XDROutPacket call;
	struct XDRInPacket reply;

	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
	
	result=nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st);

	if (result==B_OK)
	{
		//void *dummy;

		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		// XXX: either OK or not get_vnode !!! ??
		//if ((result=get_vnode(ns->nsid,st.st_ino,&dummy))<B_OK)
		//	return result;
		return EEXIST;
	}
	else if (result!=ENOENT)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return result;
	}
		
	XDROutPacketAddFixed (&call,dir->fhandle.opaque,NFS_FHSIZE);
	XDROutPacketAddString(&call,name);
	XDROutPacketAddInt32 (&call,perms|S_IFDIR);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,time(NULL));
	XDROutPacketAddInt32 (&call,0);
	XDROutPacketAddInt32 (&call,time(NULL));
	XDROutPacketAddInt32 (&call,0);
			
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_MKDIR,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
		
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);
	}
		
	XDRInPacketGetFixed(&reply,fhandle.opaque,NFS_FHSIZE);
	
	get_nfs_attr (&reply,&st);

	newNode=(fs_node *)malloc(sizeof(fs_node));
	newNode->fhandle=fhandle;
	newNode->vnid=st.st_ino;

	insert_node (ns,newNode);

	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);

	return my_notify_listener (B_ENTRY_CREATED,ns->nsid,dir->vnid,0,st.st_ino,name);		
}

extern int 
fs_rename(fs_nspace *ns, fs_node *olddir, const char *oldname, fs_node *newdir, const char *newname)
{
	struct stat st;
	nfs_fhandle fhandle;
	status_t result;
	struct XDROutPacket call;
	struct XDRInPacket reply;
	int32 status;
	uint8 *replyBuf;
		
	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
	
	if ((result=nfs_lookup(ns,&newdir->fhandle,newname,&fhandle,&st))==B_OK)
	{
		if (S_ISREG(st.st_mode))
			result=fs_unlink (ns,newdir,newname);
		else
			result=fs_rmdir (ns,newdir,newname);
			
		if (result<B_OK)
		{
			XDRInPacketDestroy (&reply);
			XDROutPacketDestroy (&call);
			return result;
		}
	}
	
	if ((result=nfs_lookup(ns,&olddir->fhandle,oldname,&fhandle,&st))<B_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return result;
	}
			
	XDROutPacketAddFixed (&call,olddir->fhandle.opaque,NFS_FHSIZE);
	XDROutPacketAddString(&call,oldname);
	XDROutPacketAddFixed (&call,newdir->fhandle.opaque,NFS_FHSIZE);
	XDROutPacketAddString(&call,newname);
			
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_RENAME,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
		
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);			
	}
	
	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);

	return my_notify_listener (B_ENTRY_MOVED,ns->nsid,olddir->vnid,newdir->vnid,st.st_ino,newname);			
}

extern int 
fs_rmdir(fs_nspace *ns, fs_node *dir, const char *name)
{
	status_t result;
	fs_node *newNode;
	fs_node *dummy;
	struct XDROutPacket call;
	struct XDRInPacket reply;
	int32 status;
	uint8 *replyBuf;
		
	struct stat st;
	nfs_fhandle fhandle;

	XDROutPacketInit(&call);
	XDRInPacketInit(&reply);
	
	if ((result=nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st))<B_OK)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}
			
	newNode=(fs_node *)malloc(sizeof(fs_node));
	newNode->fhandle=fhandle;
	newNode->vnid=st.st_ino;

	insert_node (ns,newNode);

	if ((result=get_vnode(ns->nsid,st.st_ino,(void **)&dummy))<B_OK)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}
	
	if (!S_ISDIR(st.st_mode))
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return ENOTDIR;
	}
	
	if ((result=remove_vnode(ns->nsid,st.st_ino))<B_OK)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}
	
	if ((result=put_vnode(ns->nsid,st.st_ino))<B_OK)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return result;
	}
	
	XDROutPacketAddFixed (&call,dir->fhandle.opaque,NFS_FHSIZE);
	XDROutPacketAddString(&call,name);
			
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_RMDIR,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy(&call);
		return map_nfs_to_system_error(status);
	}
	
	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy(&call);
	return my_notify_listener (B_ENTRY_REMOVED,ns->nsid,dir->vnid,0,st.st_ino,name);
}

extern int 
fs_readlink(fs_nspace *ns, fs_node *node, char *buf, size_t *bufsize)
{
	struct XDROutPacket call;
	uint8 *replyBuf;
	int32 status;
	size_t length;
	char data[NFS_MAXPATHLEN];
	struct XDRInPacket reply;
	
	XDROutPacketInit (&call);
	XDRInPacketInit(&reply);
	
	XDROutPacketAddFixed (&call,node->fhandle.opaque,NFS_FHSIZE);
			
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_READLINK,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy (&call);
		return EHOSTUNREACH;
	}
			
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
	if (status!=NFS_OK)
	{
		XDRInPacketDestroy(&reply);
		XDROutPacketDestroy (&call);
		return map_nfs_to_system_error(status);
	}
	
	length=XDRInPacketGetDynamic(&reply,data);
	
	length=min_c(length,*bufsize);
	memcpy (buf,data,length);
	*bufsize=length;

	XDRInPacketDestroy(&reply);
	XDROutPacketDestroy (&call);
	return B_OK;
}

extern int 
#ifdef __HAIKU__
fs_symlink(fs_nspace *ns, fs_node *dir, const char *name, const char *path, int mode)
#else
fs_symlink(fs_nspace *ns, fs_node *dir, const char *name, const char *path)
#endif
{
	nfs_fhandle fhandle;
	struct stat st;
	struct XDROutPacket call;	
	struct XDRInPacket reply;
	status_t result;
	uint8 *replyBuf;
	int32 status;
	fs_node *newNode;

	XDROutPacketInit (&call);
	XDRInPacketInit (&reply);
			
	result=nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st);

	if (result==B_OK)
	{
		void *dummy;
		if ((result=get_vnode(ns->nsid,st.st_ino,&dummy))<B_OK)
			return result;

		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return EEXIST;
	}
	else if (result!=ENOENT)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return result;
	}
	
	XDROutPacketAddFixed (&call,dir->fhandle.opaque,NFS_FHSIZE);
	XDROutPacketAddString(&call,name);
	XDROutPacketAddString(&call,path);
	XDROutPacketAddInt32 (&call,S_IFLNK);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,-1);
	XDROutPacketAddInt32 (&call,time(NULL));
	XDROutPacketAddInt32 (&call,0);
	XDROutPacketAddInt32 (&call,time(NULL));
	XDROutPacketAddInt32 (&call,0);
			
	replyBuf=send_rpc_call (ns,&ns->nfsAddr,NFS_PROGRAM,NFS_VERSION,NFSPROC_SYMLINK,&call);

	if (!replyBuf)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
		
	XDRInPacketSetTo (&reply,replyBuf,0);

	if (!is_successful_reply(&reply))
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return B_ERROR;
	}
			
	status=XDRInPacketGetInt32(&reply);
	
/*	if (status!=NFS_OK)
		return map_nfs_to_system_error(status);

	ignore status here, weird thing, nfsservers that is	
*/

	result=nfs_lookup(ns,&dir->fhandle,name,&fhandle,&st);

	if (result<B_OK)
	{
		XDRInPacketDestroy (&reply);
		XDROutPacketDestroy (&call);
		return result;
	}
							
	newNode=(fs_node *)malloc(sizeof(fs_node));
	newNode->fhandle=fhandle;
	newNode->vnid=st.st_ino;

	insert_node (ns,newNode);

	result=my_notify_listener (B_ENTRY_CREATED,ns->nsid,dir->vnid,0,st.st_ino,name);
	
	XDRInPacketDestroy (&reply);
	XDROutPacketDestroy (&call);
	return result;
}

int	fs_access(void *ns, void *node, int mode)
{
	(void) ns;
	(void) node;
	(void) mode;
	/* XXX */
	return B_OK;
}

#ifdef __HAIKU__

static status_t
nfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			dprintf("nfs:std_ops(INIT)\n");
			return B_OK;
		case B_MODULE_UNINIT:
			dprintf("nfs:std_ops(UNINIT)\n");
			return B_OK;
		default:
			return B_ERROR;
	}
}


static file_system_module_info sNFSModule = {
	{
		"file_systems/nfs" B_CURRENT_FS_API_VERSION,
		0,
		nfs_std_ops,
	},

	"Network File System v2",

	// scanning
	NULL,	// fs_identify_partition,
	NULL,	// fs_scan_partition,
	NULL,	// fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,
	&fs_unmount,
	&fs_rfsstat,
	&fs_wfsstat,
	NULL,	// &fs_sync,

	/* vnode operations */
	&fs_walk,
	&fs_get_vnode_name,
	&fs_read_vnode,
	&fs_release_vnode,
	&fs_remove_vnode,

	/* VM file access */
	NULL, 	// &fs_can_page
	NULL,	// &fs_read_pages
	NULL, 	// &fs_write_pages

	NULL,	// &fs_get_file_map,

	NULL, 	// &fs_ioctl
	NULL,	// &fs_setflags,
	NULL,	// &fs_select
	NULL,	// &fs_deselect
	NULL, 	// &fs_fsync

	&fs_readlink,
	NULL,	// &fs_write link,
	&fs_symlink,

	NULL,	// &fs_link,
	&fs_unlink,
	&fs_rename,

	&fs_access,
	&fs_rstat,
	&fs_wstat,

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

module_info *modules[] = {
	(module_info *)&sNFSModule,
	NULL,
};


#else

_EXPORT vnode_ops fs_entry =
{
	(op_read_vnode *)&fs_read_vnode,
	(op_write_vnode *)&fs_write_vnode,
	(op_remove_vnode *)&fs_remove_vnode,
	(op_secure_vnode *)&fs_secure_vnode,
	(op_walk *)&fs_walk,
	(op_access *)&fs_access,
	(op_create *)&fs_create,
	(op_mkdir *)&fs_mkdir,
	(op_symlink *)&fs_symlink,
	NULL, //	&fs_link,
	(op_rename *)&fs_rename,
	(op_unlink *)&fs_unlink,
	(op_rmdir *)&fs_rmdir,
	(op_readlink *)&fs_readlink,
	(op_opendir *)&fs_opendir,
	(op_closedir *)&fs_closedir,
	(op_free_cookie *)&fs_free_dircookie,
	(op_rewinddir *)&fs_rewinddir,
	(op_readdir *)&fs_readdir,
	(op_open *)&fs_open,
	(op_close *)&fs_close,
	(op_free_cookie *)&fs_free_cookie,
	(op_read *)&fs_read,
	(op_write *)&fs_write,
	NULL, //	&fs_readv,
	NULL, //	&fs_writev,
	NULL, //	&fs_ioctl,
	NULL, //	&fs_setflags,
	(op_rstat *)&fs_rstat,
	(op_wstat *)&fs_wstat,
	NULL, //	&fs_fsync,
	NULL, //	&fs_initialize,
	(op_mount *)&fs_mount,
	(op_unmount *)&fs_unmount,
	NULL, //	&fs_sync,
	(op_rfsstat *)&fs_rfsstat,
	(op_wfsstat *)&fs_wfsstat,
	NULL, //	&fs_select,
	NULL, //	&fs_deselect,
	NULL, //	&fs_open_indexdir,
	NULL, //	&fs_close_indexdir,
	NULL, //	&fs_free_indexdircookie,
	NULL, //	&fs_rewind_indexdir,
	NULL, //	&fs_read_indexdir,
	NULL, //	&fs_create_index,
	NULL, //	&fs_remove_index,
	NULL, //	&fs_rename_index,
	NULL, //	&fs_stat_index,
	NULL, //	&fs_open_attrdir,
	NULL, //	&fs_close_attrdir,
	NULL, //	&fs_free_attrdircookie,
	NULL, //	&fs_rewind_attrdir,
	NULL, //	&fs_read_attrdir,
	NULL, //	&fs_write_attr,
	NULL, //	&fs_read_attr,
	NULL, //	&fs_remove_attr,
	NULL, //	&fs_rename_attr,
	NULL, //	&fs_stat_attr,
	NULL, //	&fs_open_query,
	NULL, //	&fs_close_query,
	NULL, //	&fs_free_querycookie,
	NULL, //	&fs_read_query	
};

_EXPORT int32 api_version=B_CUR_FS_API_VERSION;

#endif
