// LDAPAdd.cpp : Defines the entry point for the console application.
//

#include "betalk.h"

#include "netdb.h"
#include "nfs_add_on.h"
#include "ksocket.h"

#define BT_RPC_THREAD_NAME			"BeServed Client RPC"
#define BT_MAX_TOTAL_RETRIES		3
#define BT_RETRIES_BEFORE_RESTART	1

typedef struct mimeCache
{
	vnode_id vnid;
	time_t time;
	char mimeType[256];
	struct mimeCache *prev, *next;
} bt_mimeCacheNode;


int btConnect(unsigned int serverIP, int port);
void btDisconnect(int session);
void btReconnect();
int32 btRPCReceive(void *data);
void btRPCInit(fs_nspace *ns);
void btRPCClose();

void btRPCRecordCall(bt_rpccall *call);
void btRPCRemoveCall(bt_rpccall *call);
void btRPCFreeCall(bt_rpccall *call);
void btCreateInPacket(bt_rpccall *call, char *buffer, unsigned int length);
void btDestroyInPacket(bt_inPacket *packet);

void btAddToMimeCache(vnode_id vnid, char *mimeType, char *pktBuffer);
bt_mimeCacheNode *btRemoveFromMimeCache(bt_mimeCacheNode *deadNode);
char *btGetMimeType(vnode_id vnid);

int btMount(int session, char *shareName, vnode_id *vnid);
int btLookup(int session, vnode_id dir_vnid, char *file, vnode_id *file_vnid, struct stat *st);
int btStat(int session, vnode_id vnid, struct stat *st);
int btGetFSInfo(int session, btFileHandle *root, bt_fsinfo *fsinfo);
int btReadDir(int session, btFileHandle *dhandle, vnode_id *vnid, char **filename, btCookie *cookie, struct stat *st);
size_t btRead(int session, btFileHandle *fhandle, off_t pos, size_t len, char *buf);
int btCreate(int session, btFileHandle *dhandle, char *name, vnode_id *file_vnid, int omode, int perms, struct stat *st);
int btTruncate(int session, btFileHandle *fhandle, int64 len);
int btUnlink(int session, btFileHandle *dhandle, char *name);
int btCreateDir(int session, btFileHandle *dhandle, char *name, int perms, vnode_id *file_vnid, struct stat *st);
int btRename(int session, btFileHandle *oldDHandle, char *oldName, btFileHandle *newDHandle, char *newName);
int btDeleteDir(int session, btFileHandle *dhandle, char *name);
int btReadLink(int session, btFileHandle *fhandle, char *buf, size_t *bufsize);
int btSymLink(int session, btFileHandle *dhandle, char *name, char *path);
int btReadAttrib(int session, btFileHandle *fhandle, char *name, int type, void *buffer, size_t len, off_t pos);
int btWriteAttrib(int session, btFileHandle *fhandle, char *name, int type, void *buffer, size_t len, off_t pos);
int btReadAttribDir(int session, btFileHandle *fhandle, char **attrName, btCookie *cookie);
int btRemoveAttrib(int session, btFileHandle *fhandle, char *name);
int btStatAttrib(int session, btFileHandle *fhandle, char *name, struct attr_info *info);
int btReadIndexDir(int session, char **indexName, int bufLength, btCookie *cookie);
int btCreateIndex(int session, char *name, int type, int flags);
int btRemoveIndex(int session, char *name);
int btStatIndex(int session, char *name, struct index_info *buf);
int btReadQuery(int session, char **fileName, int64 *inode, btQueryCookie *cookie);


bt_rpccall *rootCall = NULL;
bt_mimeCacheNode *mimeRoot = NULL;

thread_id rpcThread = 0;
sem_id callSem;
sem_id connectSem;
sem_id mimeSem;

fs_nspace *lastFS;

unsigned int lastServerIP;
int lastPort;

vint32 fsRefCount = 0;
vint32 nextXID = 1;
int32 quitXID = 0;


void btRPCInit(fs_nspace *ns)
{
	lastFS = ns;

	callSem = create_sem(1, "rpcCall");
	set_sem_owner(callSem, B_SYSTEM_TEAM);

	connectSem = create_sem(1, "rpcConnection");
	set_sem_owner(connectSem, B_SYSTEM_TEAM);

	// Find the RPC response recipient thread.  If none can be found, create one.
	// Otherwise, set the thread ID back to zero so that when we shut down, we
	// won't try to kill a thread we didn't create.
//	if (fsRefCount == 0)
//	{
		rpcThread = spawn_kernel_thread(btRPCReceive, BT_RPC_THREAD_NAME, B_NORMAL_PRIORITY, ns);
		resume_thread(rpcThread);
//	}
//	else
//		rpcThread = 0;
}

void btRPCClose()
{
//	if (fsRefCount == 0)
		if (rpcThread > 0)
		{
			status_t exitVal;
			wait_for_thread(rpcThread, &exitVal);
		}

	// Close the socket used for all file system RPC communications,
	// now that we know the RPC recipient thread is dead.
	kclosesocket(lastFS->s);

	delete_sem(connectSem);
	delete_sem(callSem);

//	if (fsRefCount == 0)
		lastFS = NULL;
}

int32 btRPCReceive(void *data)
{
	fs_nspace *ns = (fs_nspace *) data;
	bt_rpccall *call;
	char signature[20], *buffer;
	int32 xid, nsid, length, sigLen, bytesRead;
	int sock = ns->s;

	while (true)
	{
		sigLen = strlen(BT_RPC_SIGNATURE);
		memset(signature, 0, sigLen);
		if (krecv(sock, signature, sigLen, 0) == 0)
			continue;

		signature[sigLen] = 0;
		if (strcmp(signature, BT_RPC_SIGNATURE))
			continue;

//		krecv(sock, &nsid, sizeof(nsid), 0);
		krecv(sock, &xid, sizeof(int32), 0);
		krecv(sock, &length, sizeof(int32), 0);

		if (length == 0 || length >= BT_RPC_MAX_PACKET_SIZE)
			continue;

		buffer = (char *) malloc(length + 1);
		if (!buffer)
			continue;

		bytesRead = 0;
		do
		{
			bytesRead += krecv(sock, buffer + bytesRead, length - bytesRead, 0);
		} while (bytesRead < length);

		buffer[length] = 0;

		while (acquire_sem(callSem) == B_INTERRUPTED);

		call = rootCall;
		while (call)
		{
//			if (call->nsid == nsid)
				if (call->xid == xid)
				{
					btCreateInPacket(call, buffer, length);
					release_sem(call->sem);
					break;
				}

			call = call->next;
		}

		release_sem(callSem);

		// The originating RPC call was not found.  This is probably not a very
		// good sign.
		if (!call)
			free(buffer);

		// If a valid quit XID has been defined, and it's equal to the current
		// XID, quit.
		if (!fsRefCount && quitXID)
			if (quitXID == xid)
				break;
	}
}

void btCreateInPacket(bt_rpccall *call, char *buffer, unsigned int length)
{
	bt_inPacket *packet;

	packet = (bt_inPacket *) malloc(sizeof(bt_inPacket));
	if (!packet)
		return;

	packet->buffer = buffer;
	packet->length = length;
	packet->offset = 0;

	call->inPacket = packet;
}

void btDestroyInPacket(bt_inPacket *packet)
{
	if (packet)
	{
		if (packet->buffer)
			free(packet->buffer);

		free(packet);
	}
}

void btRPCRecordCall(bt_rpccall *call)
{
	bt_rpccall *curCall, *lastCall;

	while (acquire_sem(callSem) == B_INTERRUPTED);

	curCall = lastCall = rootCall;
	while (curCall)
	{
		lastCall = curCall;
		curCall = curCall->next;
	}

	call->next = NULL;
	call->prev = lastCall;

	if (lastCall == NULL)
		rootCall = call;
	else
		lastCall->next = call;

	release_sem(callSem);
}

// btRPCRemoveCall()
//
void btRPCRemoveCall(bt_rpccall *call)
{
	if (call)
	{
		while (acquire_sem(callSem) == B_INTERRUPTED);

		if (call->sem > 0)
			delete_sem(call->sem);

		// Make this entry's predecessor point to its successor.
		if (call->prev)
			call->prev->next = call->next;

		// Make this entry's successor point to its predecessor.
		if (call->next)
			call->next->prev = call->prev;

		// If an inbound packet was ever received for this RPC call,
		// deallocate it now.
		call->finished = true;

		release_sem(callSem);
	}
}

// btRPCFreeCall()
//
void btRPCFreeCall(bt_rpccall *call)
{
	if (call)
	{
//		btDestroyInPacket(call->inPacket);
//		free(call);
	}
}

// btConnect()
//
int btConnect(unsigned int serverIP, int port)
{
	struct sockaddr_in serverAddr;
	int session, addrLen;

	lastServerIP = serverIP;
	lastPort = port;

	// Store the length of the socket addressing structure for accept().
	addrLen = sizeof(struct sockaddr_in);

	// Initialize the server address structure.
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(port);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(serverIP); // inet_addr(server); // 16777343;

	// Create a new socket to receive incoming requests.
	session = ksocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (session == INVALID_SOCKET)
		return INVALID_SOCKET;

	// Bind that socket to the address constructed above.
	if (kconnect(session, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return INVALID_SOCKET;

	atomic_add(&fsRefCount, 1);
	return session;
}

void btDisconnect(int session)
{
	bt_outPacket *packet;
	bt_rpccall *call;

	// Note that we decrement the file system reference count before we
	// actually send the QUIT command, since the RPC response recipient
	// thread will need to exit when the packet is received and potentially
	// before we regain control here.  The worst case would be that
	// transmission of the QUIT command fails, and the recipient thread is
	// left running.  However, it will either be re-used on the next
	// mount or sit idle in a recv() call.
	atomic_add(&fsRefCount, -1);

	packet = btRPCPutHeader(BT_CMD_QUIT, 0, 0);
	call = btRPCInvoke(session, packet, true);
	if (call)
		btRPCRemoveCall(call);
}

bt_outPacket *btRPCPutHeader(unsigned char command, unsigned char argc, int32 length)
{
	bt_outPacket *packet;

	packet = (bt_outPacket *) malloc(sizeof(bt_outPacket));
	if (!packet)
		return NULL;

	packet->size = BT_RPC_MIN_PACKET_SIZE;
	packet->buffer = (char *) malloc(packet->size);
	packet->length = 0;

	if (!packet->buffer)
	{
		free(packet);
		return NULL;
	}

	strcpy(packet->buffer, BT_RPC_SIGNATURE);
	packet->length += strlen(BT_RPC_SIGNATURE);

//	btRPCPutChar(packet, BT_RPC_VERSION_HI);
//	btRPCPutChar(packet, BT_RPC_VERSION_LO);
	btRPCPutInt32(packet, 7 + (8 * argc) + length);
	btRPCPutChar(packet, command);
	btRPCPutChar(packet, argc);

	return packet;
}

void btRPCPutArg(bt_outPacket *packet, unsigned int type, void *data, int length)
{
	btRPCPutInt32(packet, type);
	btRPCPutInt32(packet, length);
	btRPCPutBinary(packet, data, length);
}

bt_rpccall *btRPCInvoke(int session, bt_outPacket *packet, bool lastPkt)
{
	status_t result;
	bt_rpccall *call;
	int retries;
	int32 bytesSent;

	call = (bt_rpccall *) malloc(sizeof(bt_rpccall));
	if (!call)
		return NULL;

	retries = 0;

	call->inPacket = NULL;
	call->finished = false;
	call->xid = atomic_add(&nextXID, 1);
	if ((call->sem = create_sem(0, "rpc call")) < B_OK)
	{
		free(call);
		return NULL;
	}

	set_sem_owner(call->sem, B_SYSTEM_TEAM);
	btRPCRecordCall(call);

//	btRPCPutInt32(packet, call->nsid);
	btRPCPutInt32(packet, call->xid);
	btRPCPutChar(packet, BT_CMD_TERMINATOR);

	// If this is the last RPC packet that will be transmitted, store
	// its XID so the RPC recipient thread will know when to quit.
	if (lastPkt)
		quitXID = call->xid;

doSend:
	bytesSent = 0;
	do
	{
		int bytes;
		bytes = ksend(session, packet->buffer + bytesSent, packet->length - bytesSent, 0);
		if (bytes > 0)
			bytesSent += bytes;
	} while (bytesSent < packet->length);

	do
	{
		result = acquire_sem_etc(call->sem, 1, B_RELATIVE_TIMEOUT, 5000000);
	}
	while (result == B_INTERRUPTED);

	if (result == B_TIMED_OUT)
	{
		retries++;
		if (retries > BT_MAX_TOTAL_RETRIES)
		{
			free(packet->buffer);
			free(packet);
			btRPCRemoveCall(call);
			return NULL;
		}
		else if (retries > BT_RETRIES_BEFORE_RESTART)
			btReconnect();

		goto doSend;
	}

	free(packet->buffer);
	free(packet);
	return call;
}

void btReconnect()
{
	static int counter = 0;
	int curCount = counter;

	while (acquire_sem(connectSem) == B_INTERRUPTED);

	if (curCount == counter)
	{
		kclosesocket(lastFS->s);
		atomic_add(&fsRefCount, -1);

		lastFS->s = btConnect(lastServerIP, lastPort);

		if (rpcThread > 0)
		{
			kill_thread(rpcThread);
			rpcThread = spawn_kernel_thread(btRPCReceive, BT_RPC_THREAD_NAME, B_NORMAL_PRIORITY, lastFS);
			resume_thread(rpcThread);
		}

		counter++;
	}

	release_sem(connectSem);
}

void btRPCGrowPacket(bt_outPacket *packet, int bytes)
{
	if (packet->length + bytes > packet->size)
	{
		int growth = ((bytes / BT_RPC_MIN_PACKET_SIZE) + 1) * BT_RPC_MIN_PACKET_SIZE;
		packet->buffer = (char *) realloc(packet->buffer, packet->size + growth);
		packet->size += growth;
	}
}

unsigned int btRPCGetInt32(bt_inPacket *packet)
{
	int32 value;

	if (packet->offset < packet->length)
		value = *((int32 *) &packet->buffer[packet->offset]);
	else
		value = 0;

	packet->offset += sizeof(value);
	return value;
}

int64 btRPCGetInt64(bt_inPacket *packet)
{
	int64 value;

	if (packet->offset < packet->length)
		value = *((int64 *) &packet->buffer[packet->offset]);
	else
		value = 0;

	packet->offset += sizeof(value);
	return value;
}

char *btRPCGetNewString(bt_inPacket *packet)
{
	char *str;
	unsigned int bytes;

	if (packet->offset >= packet->length)
		return NULL;

	bytes = *((int32 *) &packet->buffer[packet->offset]);
	packet->offset += sizeof(bytes);
	if (!bytes)
		return NULL;

	str = (char *) malloc(bytes + 1);
	if (!str)
		return NULL;

	memcpy(str, &packet->buffer[packet->offset], bytes);
	str[bytes] = 0;

	packet->offset += bytes;

	return str;
}

int btRPCGetString(bt_inPacket *packet, char *buffer, int length)
{
	unsigned int bytes;

	if (packet->offset >= packet->length)
		return 0;

	bytes = *((int32 *) &packet->buffer[packet->offset]);
	packet->offset += sizeof(bytes);
	if (!bytes)
		return 0L;

	if (length < bytes)
		return ERANGE;

	memcpy(buffer, &packet->buffer[packet->offset], bytes);
	packet->offset += bytes;
	return bytes;
}

int btRPCGetHandle(bt_inPacket *packet, btFileHandle *fhandle)
{
	memcpy(fhandle->opaque, &packet->buffer[packet->offset], BT_FILE_HANDLE_SIZE);
	packet->offset += BT_FILE_HANDLE_SIZE;
	return B_OK;
}

void btRPCPutChar(bt_outPacket *packet, char value)
{
	btRPCGrowPacket(packet, sizeof(value));
	packet->buffer[packet->length] = value;
	packet->length += sizeof(value);
}

void btRPCPutInt32(bt_outPacket *packet, int32 value)
{
	btRPCGrowPacket(packet, sizeof(value));
	*(int32 *)(&packet->buffer[packet->length]) = value;
	packet->length += sizeof(value);
}

void btRPCPutInt64(bt_outPacket *packet, int64 value)
{
	btRPCGrowPacket(packet, sizeof(value));
	*(int64 *)(&packet->buffer[packet->length]) = value;
	packet->length += sizeof(value);
}

void btRPCPutString(bt_outPacket *packet, char *buffer, int length)
{
	if (packet && buffer)
	{
		btRPCGrowPacket(packet, sizeof(length) + length);
		btRPCPutInt32(packet, length);
		memcpy(&packet->buffer[packet->length], buffer, length);
		packet->length += length;
	}
}

void btRPCPutBinary(bt_outPacket *packet, void *buffer, int length)
{
	if (packet && buffer)
	{
		btRPCGrowPacket(packet, length);
		memcpy(&packet->buffer[packet->length], buffer, length);
		packet->length += length;
	}
}

void btRPCPutHandle(bt_outPacket *packet, btFileHandle *fhandle)
{
	if (packet && fhandle)
	{
		btRPCGrowPacket(packet, BT_FILE_HANDLE_SIZE);
		memcpy(&packet->buffer[packet->length], fhandle->opaque, BT_FILE_HANDLE_SIZE);
		packet->length += BT_FILE_HANDLE_SIZE;
	}
}

int btRPCGetStat(bt_inPacket *packet, struct stat *st)
{
	st->st_dev = 0;
	st->st_nlink = btRPCGetInt32(packet);
	st->st_uid = btRPCGetInt32(packet);
	st->st_gid = btRPCGetInt32(packet);
	st->st_size = btRPCGetInt64(packet);
	st->st_blksize = btRPCGetInt32(packet);
	st->st_rdev = btRPCGetInt32(packet);
	st->st_ino = btRPCGetInt64(packet);
	st->st_mode = btRPCGetInt32(packet);
	st->st_atime = btRPCGetInt32(packet);
	st->st_mtime = btRPCGetInt32(packet);
	st->st_ctime = btRPCGetInt32(packet);
}

void btRPCPutStat(bt_outPacket *packet, struct stat *st)
{
	if (packet && st)
	{
		btRPCPutInt32(packet, (int) st->st_nlink);
		btRPCPutInt32(packet, (int) st->st_uid);
		btRPCPutInt32(packet, (int) st->st_gid);
		btRPCPutInt64(packet, (int64) st->st_size);
		btRPCPutInt32(packet, (int) st->st_blksize);
		btRPCPutInt32(packet, (int) st->st_rdev);
		btRPCPutInt64(packet, (int64) st->st_ino);
		btRPCPutInt32(packet, (int) st->st_mode);
		btRPCPutInt32(packet, (int) st->st_atime);
		btRPCPutInt32(packet, (int) st->st_mtime);
		btRPCPutInt32(packet, (int) st->st_ctime);
	}
}

void btEmptyLPBCache(btCookie *cookie)
{
	if (cookie->lpbCache)
	{
		cookie->lpbCache = false;
		if (cookie->inPacket.buffer)
		{
			free(cookie->inPacket.buffer);
			cookie->inPacket.buffer = NULL;
		}
	}
}

// btMount()
//
int btMount(int session, char *shareName, btFileHandle *rootNode)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_MOUNT, 1, strlen(shareName));
	btRPCPutArg(outPacket, B_STRING_TYPE, shareName, strlen(shareName));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				btRPCGetHandle(inPacket, rootNode);
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btLookup()
//
int btLookup(int session, btFileHandle *dhandle, char *file, btFileHandle *fhandle, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_LOOKUP, 2, BT_FILE_HANDLE_SIZE + strlen(file));
	btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, file, strlen(file));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				btRPCGetHandle(inPacket, fhandle);
				btRPCGetStat(inPacket, st);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btStat()
//
int btStat(int session, btFileHandle *fhandle, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_STAT, 1, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				btRPCGetStat(inPacket, st);
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btGetFSInfo()
//
int btGetFSInfo(int session, btFileHandle *root, bt_fsinfo *fsinfo)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_FSINFO, 1, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, root->opaque, BT_FILE_HANDLE_SIZE);
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				fsinfo->blockSize = btRPCGetInt32(inPacket);
				fsinfo->totalBlocks = btRPCGetInt32(inPacket);
				fsinfo->freeBlocks = btRPCGetInt32(inPacket);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btReadDir()
//
int btReadDir(int session, btFileHandle *dhandle, vnode_id *vnid, char **filename, btCookie *cookie, btFileHandle *fhandle, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	char *mimeType;
	int error, value;

	error = EHOSTUNREACH;
	call = NULL;
	inPacket = NULL;
	*filename = NULL;
	*vnid = 0;

	if (cookie->eof)
		return ENOENT;

	// If no entries are currently cached, construct a call to the server to
	// obtain the next series of directory entries.
	if (cookie->lpbCache)
	{
		// If we had an existing packet buffer stored, remove it now.
		inPacket = &cookie->inPacket;
		if (inPacket->offset >= inPacket->length)
			btEmptyLPBCache(cookie);
	}

	if (!cookie->lpbCache)
	{
		outPacket = btRPCPutHeader(BT_CMD_READDIR, 2, BT_FILE_HANDLE_SIZE + BT_COOKIE_SIZE);
		btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
		btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_COOKIE_SIZE);
		call = btRPCInvoke(session,"outPacket, false);

		if (call)
		{
			cookie->inPacket.buffer = (char *) malloc(call->inPacket->length);
			if (cookie->inPacket.buffer)
			{
				inPacket = &cookie->inPacket;
				memcpy(inPacket->buffer, call->inPacket->buffer, call->inPacket->length);
				inPacket->length = call->inPacket->length;
				inPacket->offset = 0;
				cookie->lpbCache = true;
			}
		}
	}

	if (inPacket)
	{
		error = btRPCGetInt32(inPacket);
		if (error == B_OK)
		{
			*vnid = btRPCGetInt64(inPacket);
			*filename = btRPCGetNewString(inPacket);
			if (!*filename)
				return ENOENT;

			value = btRPCGetInt32(inPacket);
			memcpy(cookie->opaque, &value, sizeof(value));
			btRPCGetHandle(inPacket, fhandle);
			btRPCGetStat(inPacket, st);
		}
		else
		{
			btEmptyLPBCache(cookie);
			cookie->eof = true;
		}
	}

	if (call)
		btRPCRemoveCall(call);

	return error;
}

// btRead()
//
size_t btRead(int session, btFileHandle *fhandle, off_t pos, size_t len, char *buf)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	struct stat st;
	size_t bytesRead, filePos;
	int32 byteCount;
	int error;

	error = B_OK;
	bytesRead = 0;
	filePos = pos;

	// Get the file size from the server and verify we're not going beyond it.
	// If we're already positioned at the end of the file, there's no point in
	// trying to read any further.
	btStat(session, fhandle, &st);
	if (filePos >= st.st_size)
		return 0;

	while (filePos < st.st_size && bytesRead < len && error == B_OK)
	{
		byteCount = min(BT_MAX_IO_BUFFER, len - bytesRead);

		outPacket = btRPCPutHeader(BT_CMD_READ, 3, BT_FILE_HANDLE_SIZE + 8);
		btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
		btRPCPutArg(outPacket, B_INT32_TYPE, &filePos, sizeof(filePos));
		btRPCPutArg(outPacket, B_INT32_TYPE, &byteCount, sizeof(byteCount));
		call = btRPCInvoke(session, outPacket, false);

		if (call)
		{
			inPacket = call->inPacket;
			if (inPacket)
			{
				error = btRPCGetInt32(inPacket);
				if (error == B_OK)
				{
					int32 bytesRecv;
					bytesRecv = btRPCGetString(inPacket, buf + bytesRead, byteCount);
					bytesRead += bytesRecv;
					filePos += bytesRecv;
				}
			}

			btRPCRemoveCall(call);
		}
	}

	return bytesRead;
}

// btWrite()
//
size_t btWrite(int session, btFileHandle *fhandle, off_t pos, size_t len, char *buf)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	struct stat st;
	size_t bytesWritten;
	int32 byteCount;
	int error;
	off_t curPos;

	error = B_OK;
	bytesWritten = 0;
	curPos = pos;

	// Get the file size from the server and verify we're not going beyond it.
	// If we're already positioned at the end of the file, there's no point in
	// trying to write any further.
	btStat(session, fhandle, &st);
	if (pos > st.st_size + 1)
		return 0;

	while (bytesWritten < len && error == B_OK)
	{
		byteCount = min(BT_MAX_IO_BUFFER, len - bytesWritten);

		outPacket = btRPCPutHeader(BT_CMD_WRITE, 4, BT_FILE_HANDLE_SIZE + 12 + byteCount);
		btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
		btRPCPutArg(outPacket, B_INT64_TYPE, &curPos, sizeof(curPos));
		btRPCPutArg(outPacket, B_INT32_TYPE, &byteCount, sizeof(byteCount));
		btRPCPutArg(outPacket, B_STRING_TYPE, buf + bytesWritten, byteCount);
		call = btRPCInvoke(session, outPacket, false);

		if (call)
		{
			inPacket = call->inPacket;
			if (inPacket)
			{
				error = btRPCGetInt32(inPacket);
				if (error == B_OK)
				{
					int bytes = btRPCGetInt32(inPacket);
					bytesWritten += bytes;
					curPos += bytes;
				}
			}

			btRPCRemoveCall(call);
		}
		else
			error = EHOSTUNREACH;
	}

	return bytesWritten;
}

int btCreate(int session, btFileHandle *dhandle, char *name, btFileHandle *fhandle, int omode, int perms, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_CREATE, 4, BT_FILE_HANDLE_SIZE + strlen(name) + 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &omode, sizeof(omode));
	btRPCPutArg(outPacket, B_INT32_TYPE, &perms, sizeof(perms));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				btRPCGetHandle(inPacket, fhandle);
				btRPCGetStat(inPacket, st);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btTruncate(int session, btFileHandle *fhandle, int64 len)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_TRUNCATE, 2, BT_FILE_HANDLE_SIZE + 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_INT64_TYPE, &len, sizeof(len));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btUnlink(int session, btFileHandle *dhandle, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_UNLINK, 2, BT_FILE_HANDLE_SIZE + strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btCreateDir(int session, btFileHandle *dhandle, char *name, int perms, btFileHandle *fhandle, struct stat *st)

{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_MKDIR, 3, BT_FILE_HANDLE_SIZE + strlen(name) + 4);
	btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &perms, sizeof(perms));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				btRPCGetHandle(inPacket, fhandle);
				btRPCGetStat(inPacket, st);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btRename(int session, btFileHandle *oldDHandle, char *oldName, btFileHandle *newDHandle, char *newName)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_RENAME, 4, 2 * BT_FILE_HANDLE_SIZE + strlen(oldName) + strlen(newName));
	btRPCPutArg(outPacket, B_STRING_TYPE, oldDHandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, oldName, strlen(oldName));
	btRPCPutArg(outPacket, B_STRING_TYPE, newDHandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, newName, strlen(newName));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btDeleteDir(int session, btFileHandle *dhandle, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_RMDIR, 2, BT_FILE_HANDLE_SIZE + strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btReadLink(int session, btFileHandle *fhandle, char *buf, size_t *bufsize)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_READLINK, 1, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*bufsize = (size_t) btRPCGetInt32(inPacket);
				if (*bufsize > 0)
					btRPCGetString(inPacket, buf, (int) *bufsize);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btSymLink(int session, btFileHandle *dhandle, char *name, char *path)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_SYMLINK, 3, BT_FILE_HANDLE_SIZE + strlen(name) + strlen(path));
	btRPCPutArg(outPacket, B_STRING_TYPE, dhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, path, strlen(path));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btWStat(int session, btFileHandle *fhandle, struct stat *st, long mask)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_WSTAT, 8, BT_FILE_HANDLE_SIZE + 28);
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_INT32_TYPE, &mask, sizeof(mask));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_mode, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_uid, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_gid, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_size, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_atime, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_mtime, sizeof(int32));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btReadAttrib(int session, btFileHandle *fhandle, char *name, int type, void *buffer, size_t len, off_t pos)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;
	int32 bytesRead;
	int32 newlen = (int32) len;
	int32 newpos = (int32) pos;

	bytesRead = -1;

	outPacket = btRPCPutHeader(BT_CMD_READATTRIB, 5, BT_FILE_HANDLE_SIZE + strlen(name) + 12);
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &type, sizeof(type));
	btRPCPutArg(outPacket, B_INT32_TYPE, &newpos, sizeof(newpos));
	btRPCPutArg(outPacket, B_INT32_TYPE, &newlen, sizeof(newlen));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				bytesRead = btRPCGetInt32(inPacket);
				if (bytesRead > 0)
					btRPCGetString(inPacket, (char *) buffer, len);
			}
			else
			{
				bytesRead = -1;
				errno = error;
			}
		}

		btRPCRemoveCall(call);
	}

	return bytesRead;
}

int btWriteAttrib(int session, btFileHandle *fhandle, char *name, int type, void *buffer, size_t len, off_t pos)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;
	int32 bytesWritten;
	int32 newlen = (int32) len;
	int32 newpos = (int32) pos;

	bytesWritten = -1;

	outPacket = btRPCPutHeader(BT_CMD_WRITEATTRIB, 6, BT_FILE_HANDLE_SIZE + strlen(name) + len + 12);
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &type, sizeof(type));
	btRPCPutArg(outPacket, B_STRING_TYPE, buffer, len);
	btRPCPutArg(outPacket, B_INT32_TYPE, &newpos, sizeof(newpos));
	btRPCPutArg(outPacket, B_INT32_TYPE, &newlen, sizeof(newlen));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				bytesWritten = btRPCGetInt32(inPacket);
			else
			{
				bytesWritten = -1;
				errno = error;
			}
		}

		btRPCRemoveCall(call);
	}

	return bytesWritten;
}

int btReadAttribDir(int session, btFileHandle *fhandle, char **attrName, btCookie *cookie)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error, value;

	error = EHOSTUNREACH;
	call = NULL;
	inPacket = NULL;
	*attrName = NULL;

	if (cookie->eof)
		return ENOENT;

	// If no entries are currently cached, construct a call to the server to
	// obtain the next series of directory entries.
	if (cookie->lpbCache)
	{
		// If we had an existing packet buffer stored, remove it now.
		inPacket = &cookie->inPacket;
		if (inPacket->offset >= inPacket->length)
			btEmptyLPBCache(cookie);
	}

	if (!cookie->lpbCache)
	{
		outPacket = btRPCPutHeader(BT_CMD_READATTRIBDIR, 2, BT_FILE_HANDLE_SIZE + BT_COOKIE_SIZE);
		btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
		btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_COOKIE_SIZE);
		call = btRPCInvoke(session, outPacket, false);

		if (call)
		{
			cookie->inPacket.buffer = (char *) malloc(call->inPacket->length);
			if (cookie->inPacket.buffer)
			{
				inPacket = &cookie->inPacket;
				memcpy(inPacket->buffer, call->inPacket->buffer, call->inPacket->length);
				inPacket->length = call->inPacket->length;
				inPacket->offset = 0;
				cookie->lpbCache = true;
			}
		}
	}

	if (inPacket)
	{
		error = btRPCGetInt32(inPacket);
		if (error == B_OK)
		{
			*attrName = btRPCGetNewString(inPacket);
			if (!*attrName)
				return ENOENT;

			value = btRPCGetInt32(inPacket);
			memcpy(cookie->opaque, &value, sizeof(value));
		}
		else
		{
			btEmptyLPBCache(cookie);
			cookie->eof = true;
		}
	}

	if (call)
		btRPCRemoveCall(call);

	return error;
}

int btRemoveAttrib(int session, btFileHandle *fhandle, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_REMOVEATTRIB, 2, BT_FILE_HANDLE_SIZE + strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btStatAttrib(int session, btFileHandle *fhandle, char *name, struct attr_info *info)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_STATATTRIB, 2, BT_FILE_HANDLE_SIZE + strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, fhandle->opaque, BT_FILE_HANDLE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				info->type = btRPCGetInt32(inPacket);
				info->size = btRPCGetInt64(inPacket);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btReadIndexDir(int session, char **indexName, int bufLength, btCookie *cookie)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error, value;

	error = EHOSTUNREACH;
	*indexName = NULL;

	if (cookie->eof)
		return ENOENT;

	outPacket = btRPCPutHeader(BT_CMD_READINDEXDIR, 1, BT_COOKIE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_COOKIE_SIZE);
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*indexName = btRPCGetNewString(inPacket);
				value = btRPCGetInt32(inPacket);
				memcpy(cookie->opaque, &value, sizeof(value));
			}
			else
				cookie->eof = true;
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btCreateIndex(int session, char *name, int type, int flags)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_CREATEINDEX, 3, strlen(name) + 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &type, sizeof(type));
	btRPCPutArg(outPacket, B_INT32_TYPE, &flags, sizeof(flags));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btRemoveIndex(int session, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_REMOVEINDEX, 1, strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btStatIndex(int session, char *name, struct index_info *buf)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_STATINDEX, 1, strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				buf->type = btRPCGetInt32(inPacket);
				buf->size = btRPCGetInt64(inPacket);
				buf->modification_time = btRPCGetInt32(inPacket);
				buf->creation_time = btRPCGetInt32(inPacket);
				buf->uid = btRPCGetInt32(inPacket);
				buf->gid = btRPCGetInt32(inPacket);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btReadQuery(int session, char **fileName, int64 *inode, btQueryCookie *cookie)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error, value;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_READQUERY, 2, BT_QUERY_COOKIE_SIZE + strlen(cookie->query));
	btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_QUERY_COOKIE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, cookie->query, strlen(cookie->query));
	call = btRPCInvoke(session, outPacket, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*inode = btRPCGetInt64(inPacket);
				*fileName = btRPCGetNewString(inPacket);
				if (!*fileName)
					error = ENOENT;

				value = btRPCGetInt32(inPacket);
				memcpy(cookie->opaque, &value, sizeof(value));
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}
