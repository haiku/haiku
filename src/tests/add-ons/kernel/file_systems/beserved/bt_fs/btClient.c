// LDAPAdd.cpp : Defines the entry point for the console application.
//

#include "betalk.h"

#include "netdb.h"
#include "nfs_add_on.h"
#include "sysdepdefs.h"

#ifndef BONE_VERSION
#include "ksocket.h"
#else
#include <sys/socket_module.h>
extern struct bone_socket_info *bone_module;
#endif

#define BT_RPC_THREAD_NAME			"BeServed RPC Marshaller"
#define BT_MAX_TOTAL_ATTEMPTS		4
#define BT_ATTEMPTS_BEFORE_RESTART	2


int btConnect(fs_nspace *ns, unsigned int serverIP, int port);
void btDisconnect(fs_nspace *ns);
bool btGetAuthentication(fs_nspace *ns);
bool btReconnect(fs_nspace *ns);
int32 btRPCReceive(void *data);
void btRPCInit(fs_nspace *ns);
void btRPCClose(fs_nspace *ns);

int btRecv(int sock, void *data, int dataLen, int flags);

void btRPCRecordCall(bt_rpccall *call);
void btRPCRemoveCall(bt_rpccall *call);
void btRPCFreeCall(bt_rpccall *call);
void btCreateInPacket(bt_rpccall *call, char *buffer, unsigned int length);
void btDestroyInPacket(bt_inPacket *packet);
bt_rpccall *btRPCInvoke(fs_nspace *ns, bt_outPacket *packet, bool lastPkt, bool burst);

int btMount(fs_nspace *ns, char *shareName, char *user, char *password, vnode_id *vnid);
int btLookup(fs_nspace *ns, vnode_id dir_vnid, char *file, vnode_id *file_vnid, struct stat *st);
int btStat(fs_nspace *ns, vnode_id vnid, struct stat *st);
int btGetFSInfo(fs_nspace *ns, vnode_id vnid, bt_fsinfo *fsinfo);
int btReadDir(fs_nspace *ns, vnode_id dir_vnid, vnode_id *file_vnid, char **filename, btCookie *cookie);
size_t btRead(fs_nspace *ns, vnode_id vnid, off_t pos, size_t len, char *buf);
int btCreate(fs_nspace *ns, vnode_id dir_vnid, char *name, vnode_id *file_vnid, int omode, int perms, struct stat *st);
int btTruncate(fs_nspace *ns, vnode_id vnid, int64 len);
int btUnlink(fs_nspace *ns, vnode_id vnid, char *name);
int btCreateDir(fs_nspace *ns, vnode_id dir_vnid, char *name, int perms, vnode_id *file_vnid, struct stat *st);
int btRename(fs_nspace *ns, vnode_id old_vnid, char *oldName, vnode_id new_vnid, char *newName);
int btDeleteDir(fs_nspace *ns, vnode_id vnid, char *name);
int btReadLink(fs_nspace *ns, vnode_id vnid, char *buf, size_t *bufsize);
int btSymLink(fs_nspace *ns, vnode_id vnid, char *name, char *path);
int btReadAttrib(fs_nspace *ns, vnode_id vnid, char *name, int type, void *buffer, size_t len, off_t pos);
int btWriteAttrib(fs_nspace *ns, vnode_id vnid, char *name, int type, void *buffer, size_t len, off_t pos);
int btReadAttribDir(fs_nspace *ns, vnode_id vnid, char **attrName, btCookie *cookie);
int btRemoveAttrib(fs_nspace *ns, vnode_id vnid, char *name);
int btStatAttrib(fs_nspace *ns, vnode_id vnid, char *name, struct attr_info *info);
int btReadIndexDir(fs_nspace *ns, char **indexName, int bufLength, btCookie *cookie);
int btCreateIndex(fs_nspace *ns, char *name, int type, int flags);
int btRemoveIndex(fs_nspace *ns, char *name);
int btStatIndex(fs_nspace *ns, char *name, struct index_info *buf);
int btReadQuery(fs_nspace *ns, char **fileName, vnode_id *vnid, vnode_id *parent, btQueryCookie *cookie);
int btCommit(fs_nspace *ns, vnode_id vnid);

// DNLC Routines
void AddDNLCEntry(fs_nspace *ns, vnode_id parent, char *name, vnode_id vnid, struct stat *st);
void DeleteDNLCEntry(fs_nspace *ns, vnode_id vnid);
void RemoveDNLCEntry(fs_nspace *ns, dnlc_entry *dnlc);
bool CheckDNLCByName(fs_nspace *ns, vnode_id parent, const char *name, vnode_id *vnid, struct stat *st);
bool CheckDNLCByVnid(fs_nspace *ns, vnode_id vnid, struct stat *st);
void PurgeDNLCEntries(fs_nspace *ns, vnode_id vnid, bool entireDir);

bt_rpccall *rootCall = NULL;

sem_id callSem;
sem_id connectSem;

vint32 nextXID = 1;


void btRPCInit(fs_nspace *ns)
{
	ns->rpcThread = 0;
	ns->quitXID = 0;

	callSem = create_sem(1, "rpcCall");
	set_sem_owner(callSem, B_SYSTEM_TEAM);

	connectSem = create_sem(1, "rpcConnection");
	set_sem_owner(connectSem, B_SYSTEM_TEAM);

	ns->rpcThread = spawn_kernel_thread(btRPCReceive, BT_RPC_THREAD_NAME, B_NORMAL_PRIORITY, ns);
	resume_thread(ns->rpcThread);
}

void btRPCClose(fs_nspace *ns)
{
	if (ns->rpcThread > 0)
	{
		status_t exitVal;
		wait_for_thread(ns->rpcThread, &exitVal);
	}

	// Close the socket used for all file system RPC communications,
	// now that we know the RPC recipient thread is dead.
	kclosesocket(ns->s);

	delete_sem(connectSem);
	delete_sem(callSem);
}

int btRecvMsg(int sock, void *data, int dataLen, int flags)
{
	int bytesRead = 0;
	do
	{
		int bytes = btRecv(sock, (char *) data + bytesRead, dataLen - bytesRead, flags);
		if (bytes == -1)
			return -1;

		bytesRead += bytes;
	} while (bytesRead < dataLen);

	return bytesRead;
}

int btRecv(int sock, void *data, int dataLen, int flags)
{
	int bytes;

	for (;;)
	{
		bytes = krecv(sock, data, dataLen, flags);
		if (bytes == 0)
			return -1;
		else if (bytes == -1)
			if (errno == EINTR)
				continue;
			else
				return -1;
		else
			break;
	}

	return bytes;
}

int btSendMsg(int sock, void *data, int dataLen, int flags)
{
	int bytesSent = 0;
	do
	{
		int bytes = btSend(sock, (char *) data + bytesSent, dataLen - bytesSent, flags);
		if (bytes == -1)
			return -1;

		bytesSent += bytes;
	} while (bytesSent < dataLen);

	return bytesSent;
}

int btSend(int sock, void *data, int dataLen, int flags)
{
	int bytes;

	for (;;)
	{
		bytes = ksend(sock, data, dataLen, flags);
		if (bytes == -1)
			if (errno == EINTR)
				continue;
			else
				return -1;
		else
			break;
	}

	return bytes;
}

int32 btRPCReceive(void *data)
{
	fs_nspace *ns = (fs_nspace *) data;
	bt_rpccall *call;
	char signature[20], *buffer;
	int32 xid, nsid, length, sigLen;
	int sock = ns->s;
	int bytes;
	bool failure = false;

#ifdef BONE_VERSION
	fd_set sockSet;
	struct timeval timeout;

	FD_ZERO(&sockSet);
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
#endif

	while (!failure)
	{
#ifdef BONE_VERSION
		FD_SET(sock, &sockSet);
		select(sock + 1, &sockSet, NULL, NULL, &timeout);

		if (FD_ISSET(sock, &sockSet))
		{
#endif

		// Receive the signature.  If a socket error occurs, break out of the loop and
		// effectively exit this thread because the socket is closed.
		sigLen = strlen(BT_RPC_SIGNATURE);
		memset(signature, 0, sigLen);
		if (btRecvMsg(sock, signature, sigLen, 0) == -1)
			break;

		// Make sure the signature is correct.  Otherwise, ignore it and start over.
		signature[sigLen] = 0;
		if (strcmp(signature, BT_RPC_SIGNATURE))
			continue;

		// Now read the transaction id (XID) and the length of the packet body.
		bytes = btRecvMsg(sock, &xid, sizeof(int32), 0);
		if (bytes > 0)
			bytes = btRecvMsg(sock, &length, sizeof(int32), 0);

		xid = B_LENDIAN_TO_HOST_INT32(xid);
		length = B_LENDIAN_TO_HOST_INT32(length);
		if (length <= 0 || length >= BT_RPC_MAX_PACKET_SIZE)
			continue;

		buffer = (char *) malloc(length + 1);
		if (!buffer)
			continue;

		// Read the remainder of the packet.
		if (btRecvMsg(sock, buffer, length, 0) == -1)
			failure = true;

		buffer[length] = 0;

		while (acquire_sem(callSem) == B_INTERRUPTED);

		call = rootCall;
		while (call)
		{
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
		if (ns->quitXID)
			if (ns->quitXID == xid)
				break;

#ifdef BONE_VERSION
	}
#endif

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
	while (acquire_sem(callSem) == B_INTERRUPTED);

	call->next = rootCall;
	call->prev = NULL;
	if (rootCall)
		rootCall->prev = call;

	rootCall = call;

	release_sem(callSem);
}

// btRPCRemoveCall()
//
void btRPCRemoveCall(bt_rpccall *call)
{
	if (call)
	{
		if (call->sem > 0)
			delete_sem(call->sem);

		while (acquire_sem(callSem) == B_INTERRUPTED);

		// Make this entry's predecessor point to its successor.
		if (call->prev)
			call->prev->next = call->next;

		// Make this entry's successor point to its predecessor.
		if (call->next)
			call->next->prev = call->prev;

		// If we just deleted the root node of the list, then the next node
		// has become the root.
		if (call == rootCall)
			rootCall = call->next;

		release_sem(callSem);

		// Now we can release the memory allocated for this packet.
		btDestroyInPacket(call->inPacket);
		free(call);
	}
}

// btConnect()
//
int btConnect(fs_nspace *ns, unsigned int serverIP, int port)
{
	struct sockaddr_in serverAddr;
	int session, addrLen, flags;

	ns->serverIP = serverIP;
	ns->serverPort = port;

	// Store the length of the socket addressing structure for accept().
	addrLen = sizeof(struct sockaddr_in);

	// Initialize the server address structure.
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(port);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(serverIP);

	// Create a new socket to receive incoming requests.
	session = ksocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (session == INVALID_SOCKET)
		return INVALID_SOCKET;

	// Bind that socket to the address constructed above.
	if (kconnect(session, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return INVALID_SOCKET;

	// Enabled periodic keep-alive messages, so we stay connected during times of
	// no activity.  This isn't supported by ksocketd, only in BONE.  :-(
#ifdef BONE_VERSION
	flags = 1;
	ksetsockopt(session, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
#endif

	return session;
}

void btDisconnect(fs_nspace *ns)
{
	bt_outPacket *packet;
	bt_rpccall *call;

	packet = btRPCPutHeader(BT_CMD_QUIT, 0, 0);
	call = btRPCInvoke(ns, packet, true, false);
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

bt_rpccall *btRPCInvoke(fs_nspace *ns, bt_outPacket *packet, bool lastPkt, bool burst)
{
	status_t result;
	bt_rpccall *call;
	int attempts;
	bool failure;

	call = (bt_rpccall *) malloc(sizeof(bt_rpccall));
	if (!call)
		return NULL;

	attempts = 0;
	call->inPacket = NULL;
	call->xid = atomic_add(&nextXID, 1);

	if (!burst)
	{
		if ((call->sem = create_sem(0, "rpc call")) < B_OK)
		{
			free(call);
			return NULL;
		}

		set_sem_owner(call->sem, B_SYSTEM_TEAM);
		btRPCRecordCall(call);
	}

	btRPCPutInt32(packet, call->xid);
	btRPCPutChar(packet, BT_CMD_TERMINATOR);

	// If this is the last RPC packet that will be transmitted, store
	// its XID so the RPC recipient thread will know when to quit.
	if (lastPkt)
		ns->quitXID = call->xid;

doSend:
	failure = false;
	if (btSendMsg(ns->s, packet->buffer, packet->length, 0) == -1)
		failure = true;

	if (!burst)
	{
		if (!failure)
			do
			{
				result = acquire_sem_etc(call->sem, 1, B_RELATIVE_TIMEOUT, 2500000);
			}
			while (result == B_INTERRUPTED);

		if (failure || result == B_TIMED_OUT)
		{
			dprintf("RPC call timed out\n");
			attempts++;
			if (attempts >= BT_MAX_TOTAL_ATTEMPTS)
			{
				free(packet->buffer);
				free(packet);
				btRPCRemoveCall(call);
				return NULL;
			}
			else if (attempts == BT_ATTEMPTS_BEFORE_RESTART)
				btReconnect(ns);

			goto doSend;
		}
	}

	free(packet->buffer);
	free(packet);
	return call;
}

bool btReconnect(fs_nspace *ns)
{
	static int counter = 0;
	int curCount = counter;
	bool connected = true;

	while (acquire_sem(connectSem) == B_INTERRUPTED);

	if (curCount == counter)
	{
		connected = false;

		kclosesocket(ns->s);
		if (ns->rpcThread > 0)
			kill_thread(ns->rpcThread);

		ns->s = btConnect(ns, ns->serverIP, ns->serverPort);
		if (ns->s != INVALID_SOCKET)
		{
			ns->rpcThread = spawn_kernel_thread(btRPCReceive, BT_RPC_THREAD_NAME, B_NORMAL_PRIORITY, ns);
			resume_thread(ns->rpcThread);
			connected = true;
		}

		counter++;
	}

	release_sem(connectSem);
	return connected;
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
		value = B_LENDIAN_TO_HOST_INT32(*((int32 *) &packet->buffer[packet->offset]));
	else
		value = 0;

	packet->offset += sizeof(value);
	return value;
}

int64 btRPCGetInt64(bt_inPacket *packet)
{
	int64 value;

	if (packet->offset < packet->length)
		value = B_LENDIAN_TO_HOST_INT64(*((int64 *) &packet->buffer[packet->offset]));
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

	bytes = B_LENDIAN_TO_HOST_INT32(*((int32 *) &packet->buffer[packet->offset]));
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

	bytes = B_LENDIAN_TO_HOST_INT64(*((int32 *) &packet->buffer[packet->offset]));
	packet->offset += sizeof(bytes);
	if (!bytes)
		return 0L;

	if (length < bytes)
		return ERANGE;

	memcpy(buffer, &packet->buffer[packet->offset], bytes);
	packet->offset += bytes;
	return bytes;
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
	*(int32 *)(&packet->buffer[packet->length]) = B_HOST_TO_LENDIAN_INT32(value);
	packet->length += sizeof(value);
}

void btRPCPutInt64(bt_outPacket *packet, int64 value)
{
	btRPCGrowPacket(packet, sizeof(value));
	*(int64 *)(&packet->buffer[packet->length]) = B_HOST_TO_LENDIAN_INT64(value);
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
int btMount(fs_nspace *ns, char *shareName, char *user, char *password, vnode_id *vnid)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_MOUNT, 3, strlen(shareName) + strlen(user) + BT_AUTH_TOKEN_LENGTH);
	btRPCPutArg(outPacket, B_STRING_TYPE, shareName, strlen(shareName));
	btRPCPutArg(outPacket, B_STRING_TYPE, user, strlen(user));
	btRPCPutArg(outPacket, B_STRING_TYPE, password, BT_AUTH_TOKEN_LENGTH);
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				*vnid = btRPCGetInt64(inPacket);
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btLookup()
//
int btLookup(fs_nspace *ns, vnode_id dir_vnid, char *file, vnode_id *file_vnid, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	if (CheckDNLCByName(ns, dir_vnid, file, file_vnid, st))
		return B_OK;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_LOOKUP, 2, 8 + strlen(file));
	btRPCPutArg(outPacket, B_INT64_TYPE, &dir_vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, file, strlen(file));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*file_vnid = btRPCGetInt64(inPacket);
				btRPCGetStat(inPacket, st);
				AddDNLCEntry(ns, dir_vnid, file, *file_vnid, st);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

// btStat()
//
int btStat(fs_nspace *ns, vnode_id vnid, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	if (CheckDNLCByVnid(ns, vnid, st))
		return B_OK;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_STAT, 1, 8);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	call = btRPCInvoke(ns, outPacket, false, false);

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
int btGetFSInfo(fs_nspace *ns, vnode_id vnid, bt_fsinfo *fsinfo)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_FSINFO, 1, 8);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	call = btRPCInvoke(ns, outPacket, false, false);

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
int btReadDir(fs_nspace *ns, vnode_id dir_vnid, vnode_id *file_vnid, char **filename, btCookie *cookie)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	struct stat st;
	char *mimeType;
	int error, value;

	error = EHOSTUNREACH;
	call = NULL;
	inPacket = NULL;
	*filename = NULL;
	*file_vnid = 0;

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
		outPacket = btRPCPutHeader(BT_CMD_READDIR, 2, 8 + BT_COOKIE_SIZE);
		btRPCPutArg(outPacket, B_INT64_TYPE, &dir_vnid, 8);
		btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_COOKIE_SIZE);
		call = btRPCInvoke(ns, outPacket, false, false);

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
			*file_vnid = btRPCGetInt64(inPacket);
			*filename = btRPCGetNewString(inPacket);
			if (!*filename)
				return ENOENT;

			value = btRPCGetInt32(inPacket);
			memcpy(cookie->opaque, &value, sizeof(value));
			btRPCGetStat(inPacket, &st);

			// Now add a DNLC entry for this file.
			AddDNLCEntry(ns, dir_vnid, *filename, *file_vnid, &st);
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
size_t btRead(fs_nspace *ns, vnode_id vnid, off_t pos, size_t len, char *buf)
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
	btStat(ns, vnid, &st);
	if (filePos >= st.st_size)
		return 0;

	while (filePos < st.st_size && bytesRead < len && error == B_OK)
	{
		byteCount = min(BT_MAX_IO_BUFFER, len - bytesRead);

		outPacket = btRPCPutHeader(BT_CMD_READ, 3, 16);
		btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
		btRPCPutArg(outPacket, B_INT32_TYPE, &filePos, sizeof(filePos));
		btRPCPutArg(outPacket, B_INT32_TYPE, &byteCount, sizeof(byteCount));
		call = btRPCInvoke(ns, outPacket, false, false);

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
size_t btWrite(fs_nspace *ns, vnode_id vnid, off_t pos, size_t len, char *buf)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	struct stat st;
	size_t bytesWritten;
	int32 byteCount, totalLen;
	int error;
	off_t curPos;

	bytesWritten = 0;
	curPos = pos;
	totalLen = (int32) len;

	while (bytesWritten < len)
	{
		byteCount = min(BT_MAX_IO_BUFFER, len - bytesWritten);

		outPacket = btRPCPutHeader(BT_CMD_WRITE, 5, 24 + byteCount);
		btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
		btRPCPutArg(outPacket, B_INT64_TYPE, &curPos, sizeof(curPos));
		btRPCPutArg(outPacket, B_INT32_TYPE, &byteCount, sizeof(byteCount));
		btRPCPutArg(outPacket, B_INT32_TYPE, &totalLen, sizeof(totalLen));
		btRPCPutArg(outPacket, B_STRING_TYPE, buf + bytesWritten, byteCount);
		call = btRPCInvoke(ns, outPacket, false, true);

		bytesWritten += byteCount;
		curPos += byteCount;

		// All subsequent calls must specify a total length of zero.
		totalLen = 0;
	}

	if (btCommit(ns, vnid) != B_OK)
		bytesWritten = 0;

	DeleteDNLCEntry(ns, vnid);
	return bytesWritten;
}

int btCreate(fs_nspace *ns, vnode_id dir_vnid, char *name, vnode_id *file_vnid, int omode, int perms, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_CREATE, 4, 8 + strlen(name) + 8);
	btRPCPutArg(outPacket, B_INT64_TYPE, &dir_vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &omode, sizeof(omode));
	btRPCPutArg(outPacket, B_INT32_TYPE, &perms, sizeof(perms));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*file_vnid = btRPCGetInt64(inPacket);
				btRPCGetStat(inPacket, st);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btTruncate(fs_nspace *ns, vnode_id vnid, int64 len)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_TRUNCATE, 2, 16);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_INT64_TYPE, &len, sizeof(len));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btUnlink(fs_nspace *ns, vnode_id vnid, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_UNLINK, 2, 8 + strlen(name));
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	PurgeDNLCEntries(ns, vnid, false);
	return error;
}

int btCreateDir(fs_nspace *ns, vnode_id dir_vnid, char *name, int perms, vnode_id *file_vnid, struct stat *st)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_MKDIR, 3, 8 + strlen(name) + 4);
	btRPCPutArg(outPacket, B_INT64_TYPE, &dir_vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &perms, sizeof(perms));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*file_vnid = btRPCGetInt64(inPacket);
				btRPCGetStat(inPacket, st);
			}
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btRename(fs_nspace *ns, vnode_id old_vnid, char *oldName, vnode_id new_vnid, char *newName)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_RENAME, 4, 16 + strlen(oldName) + strlen(newName));
	btRPCPutArg(outPacket, B_INT64_TYPE, &old_vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, oldName, strlen(oldName));
	btRPCPutArg(outPacket, B_INT64_TYPE, &new_vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, newName, strlen(newName));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	PurgeDNLCEntries(ns, old_vnid, false);
	return error;
}

int btDeleteDir(fs_nspace *ns, vnode_id vnid, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_RMDIR, 2, 8 + strlen(name));
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	PurgeDNLCEntries(ns, vnid, true);
	return error;
}

int btReadLink(fs_nspace *ns, vnode_id vnid, char *buf, size_t *bufsize)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_READLINK, 1, 8);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				*bufsize = btRPCGetString(inPacket, buf, (int) *bufsize);
		}

		btRPCRemoveCall(call);
	}

	return error;
}

int btSymLink(fs_nspace *ns, vnode_id vnid, char *name, char *path)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_SYMLINK, 3, 8 + strlen(name) + strlen(path));
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, path, strlen(path));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btWStat(fs_nspace *ns, vnode_id vnid, struct stat *st, long mask)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_WSTAT, 8, 36);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_INT32_TYPE, &mask, sizeof(mask));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_mode, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_uid, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_gid, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_size, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_atime, sizeof(int32));
	btRPCPutArg(outPacket, B_INT32_TYPE, &st->st_mtime, sizeof(int32));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btReadAttrib(fs_nspace *ns, vnode_id vnid, char *name, int type, void *buffer, size_t len, off_t pos)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;
	int32 bytesRead;
	int32 newlen = (int32) len;
	int32 newpos = (int32) pos;

	bytesRead = -1;

	outPacket = btRPCPutHeader(BT_CMD_READATTRIB, 5, 8 + strlen(name) + 12);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &type, sizeof(type));
	btRPCPutArg(outPacket, B_INT32_TYPE, &newpos, sizeof(newpos));
	btRPCPutArg(outPacket, B_INT32_TYPE, &newlen, sizeof(newlen));
	call = btRPCInvoke(ns, outPacket, false, false);

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

int btWriteAttrib(fs_nspace *ns, vnode_id vnid, char *name, int type, void *buffer, size_t len, off_t pos)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;
	int32 bytesWritten;
	int32 newlen = (int32) len;
	int32 newpos = (int32) pos;

	bytesWritten = -1;

	outPacket = btRPCPutHeader(BT_CMD_WRITEATTRIB, 6, 8 + strlen(name) + len + 12);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	btRPCPutArg(outPacket, B_INT32_TYPE, &type, sizeof(type));
	btRPCPutArg(outPacket, B_STRING_TYPE, buffer, len);
	btRPCPutArg(outPacket, B_INT32_TYPE, &newpos, sizeof(newpos));
	btRPCPutArg(outPacket, B_INT32_TYPE, &newlen, sizeof(newlen));
	call = btRPCInvoke(ns, outPacket, false, false);

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

int btReadAttribDir(fs_nspace *ns, vnode_id vnid, char **attrName, btCookie *cookie)
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
		outPacket = btRPCPutHeader(BT_CMD_READATTRIBDIR, 2, 8 + BT_COOKIE_SIZE);
		btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
		btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_COOKIE_SIZE);
		call = btRPCInvoke(ns, outPacket, false, false);

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

int btRemoveAttrib(fs_nspace *ns, vnode_id vnid, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_REMOVEATTRIB, 2, 8 + strlen(name));
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btStatAttrib(fs_nspace *ns, vnode_id vnid, char *name, struct attr_info *info)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_STATATTRIB, 2, 8 + strlen(name));
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(ns, outPacket, false, false);

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

int btReadIndexDir(fs_nspace *ns, char **indexName, int bufLength, btCookie *cookie)
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
	call = btRPCInvoke(ns, outPacket, false, false);

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

int btCreateIndex(fs_nspace *ns, char *name, int type, int flags)
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
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btRemoveIndex(fs_nspace *ns, char *name)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_REMOVEINDEX, 1, strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

int btStatIndex(fs_nspace *ns, char *name, struct index_info *buf)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_STATINDEX, 1, strlen(name));
	btRPCPutArg(outPacket, B_STRING_TYPE, name, strlen(name));
	call = btRPCInvoke(ns, outPacket, false, false);

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

int btReadQuery(fs_nspace *ns, char **fileName, vnode_id *vnid, vnode_id *parent, btQueryCookie *cookie)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error, value;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_READQUERY, 2, BT_COOKIE_SIZE + strlen(cookie->query));
	btRPCPutArg(outPacket, B_STRING_TYPE, cookie->opaque, BT_COOKIE_SIZE);
	btRPCPutArg(outPacket, B_STRING_TYPE, cookie->query, strlen(cookie->query));
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
			{
				*vnid = btRPCGetInt64(inPacket);
				*parent = btRPCGetInt64(inPacket);
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

// btCommit()
//
int btCommit(fs_nspace *ns, vnode_id vnid)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	bt_rpccall *call;
	int error;
	size_t bytes;

	error = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_COMMIT, 1, 8);
	btRPCPutArg(outPacket, B_INT64_TYPE, &vnid, 8);
	call = btRPCInvoke(ns, outPacket, false, false);

	if (call)
	{
		inPacket = call->inPacket;
		if (inPacket)
			error = btRPCGetInt32(inPacket);

		btRPCRemoveCall(call);
	}

	return error;
}

/////////////////////////

// AddDNLCEntry()
//
void AddDNLCEntry(fs_nspace *ns, vnode_id parent, char *name, vnode_id vnid, struct stat *st)
{
	time_t curTime = time(NULL);
	dnlc_entry *dnlc;

	beginWriting(&ns->dnlcData);

	// Clean expired entries from the DNLC first.
	dnlc = ns->dnlcRoot;
	while (dnlc)
	{
		int secs = S_ISDIR(dnlc->st.st_mode) ? 30 : 15;
		if (curTime - dnlc->entryTime > secs)
		{
			dnlc_entry *ent = dnlc->next;
			RemoveDNLCEntry(ns, dnlc);
			dnlc = ent;
		}

		if (dnlc)
			dnlc = dnlc->next;
	}

	// Now add the new entry.
	dnlc = (dnlc_entry *) malloc(sizeof(dnlc_entry));
	if (dnlc)
	{
		dnlc->prev = NULL;
		dnlc->next = ns->dnlcRoot;
		if (ns->dnlcRoot)
			ns->dnlcRoot->prev = dnlc;

		ns->dnlcRoot = dnlc;
		dnlc->vnid = vnid;
		dnlc->parent = parent;
		strcpy(dnlc->name, name);
		memcpy(&dnlc->st, st, sizeof(struct stat));
		dnlc->entryTime = time(NULL);
	}

	endWriting(&ns->dnlcData);
}

// DeleteDNLCEntry()
//
void DeleteDNLCEntry(fs_nspace *ns, vnode_id vnid)
{
	dnlc_entry *dnlc;

	beginWriting(&ns->dnlcData);

	dnlc = ns->dnlcRoot;
	while (dnlc && dnlc->vnid != vnid)
		dnlc = dnlc->next;

	if (dnlc)
		RemoveDNLCEntry(ns, dnlc);

	endWriting(&ns->dnlcData);
}

// RemoveDNLCEntry()
//
void RemoveDNLCEntry(fs_nspace *ns, dnlc_entry *dnlc)
{
	if (dnlc)
	{
		if (dnlc == ns->dnlcRoot)
			ns->dnlcRoot = dnlc->next;

		if (dnlc->next)
			dnlc->next->prev = dnlc->prev;

		if (dnlc->prev)
			dnlc->prev->next = dnlc->next;

		free(dnlc);
	}
}

// CheckDNLCByName()
//
bool CheckDNLCByName(fs_nspace *ns, vnode_id parent, const char *name, vnode_id *vnid, struct stat *st)
{
	dnlc_entry *dnlc;

	beginReading(&ns->dnlcData);

	dnlc = ns->dnlcRoot;
	while (dnlc)
	{
		if (dnlc && dnlc->parent == parent)
			if (strcmp(dnlc->name, name) == 0)
			{
				*vnid = dnlc->vnid;
				memcpy(st, &dnlc->st, sizeof(struct stat));
				endReading(&ns->dnlcData);
				return true;
			}

		if (dnlc)
			dnlc = dnlc->next;
	}

	endReading(&ns->dnlcData);
	return false;
}

// CheckDNLCByVnid()
//
bool CheckDNLCByVnid(fs_nspace *ns, vnode_id vnid, struct stat *st)
{
	dnlc_entry *dnlc;

	beginReading(&ns->dnlcData);

	dnlc = ns->dnlcRoot;
	while (dnlc)
	{
		if (dnlc && dnlc->vnid == vnid)
		{
			memcpy(st, &dnlc->st, sizeof(struct stat));
			endReading(&ns->dnlcData);
			return true;
		}

		if (dnlc)
			dnlc = dnlc->next;
	}

	endReading(&ns->dnlcData);
	return false;
}

// PurgeDNLCEntries()
//
void PurgeDNLCEntries(fs_nspace *ns, vnode_id vnid, bool entireDir)
{
	dnlc_entry *dnlc;

	beginWriting(&ns->dnlcData);

	dnlc = ns->dnlcRoot;
	while (dnlc)
	{
		if (vnid == dnlc->vnid || (entireDir && vnid == dnlc->parent))
		{
			dnlc_entry *ent = dnlc->next;
			RemoveDNLCEntry(ns, dnlc);

			// If we're not purging an entire folder, then we're obviously
			// here because this is the single vnode id we're deleting.
			// Now that we've done it, just exit and save time.
			if (!entireDir)
				break;

			dnlc = ent;
		}

		if (dnlc)
			dnlc = dnlc->next;
	}

	endWriting(&ns->dnlcData);
}
