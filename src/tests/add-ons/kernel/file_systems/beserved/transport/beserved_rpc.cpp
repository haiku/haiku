#include "betalk.h"
#include "sysdepdefs.h"
#include "beserved_rpc.h"

#include "netdb.h"

#define BT_RPC_THREAD_NAME			"BeServed RPC Marshaller"
#define BT_MAX_TOTAL_ATTEMPTS		4
#define BT_ATTEMPTS_BEFORE_RESTART	2

bool btReconnect(bt_rpcinfo *info);
int32 btRPCReceive(void *data);

int btRecv(int sock, void *data, int dataLen, int flags);
int btSend(int sock, void *data, int dataLen, int flags);

void btRPCRecordCall(bt_rpccall *call);
void btRPCFreeCall(bt_rpccall *call);
void btCreateInPacket(bt_rpccall *call, char *buffer, unsigned int length);
void btDestroyInPacket(bt_inPacket *packet);

bt_rpccall *rootCall;
sem_id callSem;
sem_id connectSem;
int32 nextXID = 1;


void btRPCInit(bt_rpcinfo *info)
{
	info->s = INVALID_SOCKET;
	info->rpcThread = 0;
	info->quitXID = 0;

	callSem = create_sem(1, "rpcCall");
//	set_sem_owner(callSem, B_SYSTEM_TEAM);

	connectSem = create_sem(1, "rpcConnection");
//	set_sem_owner(connectSem, B_SYSTEM_TEAM);

	info->rpcThread = spawn_thread(btRPCReceive, BT_RPC_THREAD_NAME, B_NORMAL_PRIORITY, info);
	resume_thread(info->rpcThread);
}

void btRPCClose(bt_rpcinfo *info)
{
	if (info->rpcThread > 0)
	{
		status_t exitVal;
		wait_for_thread(info->rpcThread, &exitVal);
	}

	// Close the socket used for all file system RPC communications,
	// now that we know the RPC recipient thread is dead.
	closesocket(info->s);

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
		bytes = recv(sock, data, dataLen, flags);
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
		bytes = send(sock, data, dataLen, flags);
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
	bt_rpcinfo *info = (bt_rpcinfo *) data;
	bt_rpccall *call;
	char signature[20], *buffer;
	int32 xid, length, sigLen;
	int bytes;
	bool failure = false;

	while (info->s == INVALID_SOCKET)
		snooze(100);

	int sock = info->s;

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
		if (info->quitXID)
			if (info->quitXID == xid)
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
int btConnect(bt_rpcinfo *info, unsigned int serverIP, int port)
{
	struct sockaddr_in serverAddr;
	int session, addrLen;
#ifdef BONE_VERSION
	int flags;
#endif

	info->serverIP = serverIP;
	info->serverPort = port;

	// Store the length of the socket addressing structure for accept().
	addrLen = sizeof(struct sockaddr_in);

	// Initialize the server address structure.
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(port);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(serverIP);

	// Create a new socket to receive incoming requests.
	session = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (session == INVALID_SOCKET)
		return INVALID_SOCKET;

	// Bind that socket to the address constructed above.
	if (connect(session, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return INVALID_SOCKET;

	// Enabled periodic keep-alive messages, so we stay connected during times of
	// no activity.  This isn't supported by ksocketd, only in BONE.  :-(
#ifdef BONE_VERSION
	flags = 1;
	setsockopt(session, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
#endif

	return session;
}

void btDisconnect(bt_rpcinfo *info)
{
	bt_outPacket *packet;
	bt_rpccall *call;

	packet = btRPCPutHeader(BT_CMD_QUIT, 0, 0);
	call = btRPCInvoke(info, packet, true);
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

bt_rpccall *btRPCInvoke(bt_rpcinfo *info, bt_outPacket *packet, bool lastPkt)
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
	if ((call->sem = create_sem(0, "rpc call")) < B_OK)
	{
		free(call);
		return NULL;
	}

	btRPCRecordCall(call);

	btRPCPutInt32(packet, call->xid);
	btRPCPutChar(packet, BT_CMD_TERMINATOR);

	// If this is the last RPC packet that will be transmitted, store
	// its XID so the RPC recipient thread will know when to quit.
	if (lastPkt)
		info->quitXID = call->xid;

doSend:
	failure = false;
	if (btSendMsg(info->s, packet->buffer, packet->length, 0) == -1)
		failure = true;

	if (!failure)
		do
		{
			result = acquire_sem_etc(call->sem, 1, B_RELATIVE_TIMEOUT, 2500000);
		}
		while (result == B_INTERRUPTED);

	if (failure || result == B_TIMED_OUT)
	{
		attempts++;
		if (attempts >= BT_MAX_TOTAL_ATTEMPTS)
		{
			free(packet->buffer);
			free(packet);
			btRPCRemoveCall(call);
			return NULL;
		}
		else if (attempts == BT_ATTEMPTS_BEFORE_RESTART)
			btReconnect(info);

		goto doSend;
	}

	free(packet->buffer);
	free(packet);
	return call;
}

bool btReconnect(bt_rpcinfo *info)
{
	static int counter = 0;
	int curCount = counter;
	bool connected = true;

	while (acquire_sem(connectSem) == B_INTERRUPTED);

	if (curCount == counter)
	{
		connected = false;

		closesocket(info->s);
		if (info->rpcThread > 0)
			kill_thread(info->rpcThread);

		info->s = btConnect(info, info->serverIP, info->serverPort);
		if (info->s != INVALID_SOCKET)
		{
			info->rpcThread = spawn_thread(btRPCReceive, BT_RPC_THREAD_NAME, B_NORMAL_PRIORITY, info);
			resume_thread(info->rpcThread);
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
