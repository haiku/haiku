#include "betalk.h"
#include "sysdepdefs.h"
#include "rpc.h"

#include "signal.h"


int btRPCConnect(unsigned int serverIP, int port)
{
	struct sockaddr_in serverAddr;
	int session;

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

	return session;
}

bool btRPCSend(int session, bt_outPacket *outPacket)
{
	// The XID will be 0.
	btRPCPutInt32(outPacket, 0);
	btRPCPutChar(outPacket, BT_CMD_TERMINATOR);

	if (btSendMsg(session, outPacket->buffer, outPacket->length, 0) == -1)
		return false;

	return true;
}

bool btRPCCheckSignature(int session)
{
	char signature[20];
	unsigned int sigLen;

	sigLen = strlen(BT_RPC_SIGNATURE);
	memset(signature, 0, sigLen);
	if (btRecvMsg(session, signature, sigLen, 0) == -1)
		return false;

	// Check the signature's validity.
	signature[sigLen] = 0;
	return (strcmp(signature, BT_RPC_SIGNATURE) == 0);
}

// btRPCSimpleCall()
//
bt_inPacket *btRPCSimpleCall(unsigned int serverIP, int port, bt_outPacket *outPacket)
{
	struct timeval timeout;
	bt_inPacket *inPacket;
	fd_set sockSet;
	char *buffer;
	int session;
	int32 xid, length;

	// Establish a connection with the requested server, on the requested port.
	// If we can't connect, abort and return a NULL packet.
	inPacket = NULL;
	session = btRPCConnect(serverIP, port);
	if (session == INVALID_SOCKET)
		return NULL;

	// If we connected, send the requested RPC packet.  If the packet cannot be
	// sent, the connection has dropped and we'll abort the call.
	if (!btRPCSend(session, outPacket))
	{
		closesocket(session);
		return NULL;
	}

	// Set a reasonable timeout period.  Select() is used in leiu of alarm() because
	// select() also aborts on error, alarm() effects all threads in a process.
	FD_ZERO(&sockSet);
	timeout.tv_sec = 8;
	timeout.tv_usec = 0;

	// Block in select() waiting for activity.  This will block until data is available
	// or until a socket error is pending.
	FD_SET(session, &sockSet);
	select(session + 1, &sockSet, NULL, NULL, &timeout);

	// If our socket has data pending, then read the incoming RPC response packet.
	// This should consist of a valid RPC signature, a tranaction ID (xid), the length
	// of the variable data, and the data itself.
	if (FD_ISSET(session, &sockSet))
		if (btRPCCheckSignature(session))
		{
			if (btRecvMsg(session, &xid, sizeof(int32), 0) == -1 ||
				btRecvMsg(session, &length, sizeof(int32), 0) == -1)
				goto abortCall;

			xid = B_LENDIAN_TO_HOST_INT32(xid);
			length = B_LENDIAN_TO_HOST_INT32(length);

			// Now allocate a buffer of the appropriate length.  If one cannot be
			// allocated, we won't be able to store incoming information and the call
			// must be aborted.
			if (length > 0 && length < BT_RPC_MAX_PACKET_SIZE)
			{
				buffer = (char *) malloc(length + 1);
				if (buffer)
				{
					// Read the remaining packet contents.  The btRecv() function takes
					// care of restarting the recv() when signal interrupts occur.  It
					// will always return -1 on error, even upon orderly shutdown of the peer.
					if (btRecvMsg(session, buffer, length, 0) == -1)
					{
						free(buffer);
						goto abortCall;
					}

					// Terminate the buffer.
					buffer[length] = 0;

					// Allocate a new incoming packet and set its buffer and length.
					inPacket = (bt_inPacket *) malloc(sizeof(bt_inPacket));
					if (inPacket)
					{
						inPacket->buffer = buffer;
						inPacket->length = length;
						inPacket->offset = 0;
					}
					else
						free(buffer);
				}
			}
		}

	// Execution can naturally lead here or we can jump here from a failed attempt to
	// send or receive an RPC packet.  The socket is closed and the current incoming
	// packet returned, which will be NULL upon failure.
abortCall:
	shutdown(session, 2);
	close(session);
	return inPacket;
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

// btRecv()
//
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

// btSend()
//
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

void btDestroyInPacket(bt_inPacket *packet)
{
	if (packet)
	{
		if (packet->buffer)
			free(packet->buffer);

		free(packet);
	}
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

void btRPCCreateAck(bt_outPacket *packet, unsigned int xid, int error, int length)
{
	packet->size = BT_RPC_MIN_PACKET_SIZE;
	packet->buffer = (char *) malloc(packet->size);
	packet->length = 0;

	if (!packet->buffer)
		return;

	strcpy(packet->buffer, BT_RPC_SIGNATURE);
	packet->length += strlen(BT_RPC_SIGNATURE);
	btRPCPutInt32(packet, xid);
	btRPCPutInt32(packet, 0);
	btRPCPutInt32(packet, error);
}

void btRPCSendAck(int client, bt_outPacket *packet)
{
	if (packet)
		if (packet->buffer)
		{
			*(int32 *)(&packet->buffer[9]) = B_HOST_TO_LENDIAN_INT32(packet->length - 13);
			btSendMsg(client, packet->buffer, packet->length, 0);
			free(packet->buffer);
		}
}

unsigned char btRPCGetChar(bt_inPacket *packet)
{
	unsigned char value;

	if (packet->offset < packet->length)
		value = (unsigned char) packet->buffer[packet->offset];
	else
		value = 0;

	packet->offset += sizeof(value);
	return value;
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
	if (bytes < 0 || bytes > BT_MAX_IO_BUFFER)
		return NULL;

	str = (char *) malloc(bytes + 1);
	if (!str)
		return NULL;

	if (bytes > 0)
		memcpy(str, &packet->buffer[packet->offset], bytes);

	str[bytes] = 0;
	packet->offset += bytes;

	return str;
}

int btRPCGetString(bt_inPacket *packet, char *buffer, int length)
{
	unsigned int bytes;

	if (packet->offset >= packet->length)
		return ERANGE;

	bytes = B_LENDIAN_TO_HOST_INT32(*((int32 *) &packet->buffer[packet->offset]));
	packet->offset += sizeof(bytes);
	if (bytes < 0 || bytes > BT_MAX_IO_BUFFER)
		return ERANGE;

	if (length < bytes)
		return ERANGE;

	if (bytes > 0)
		memcpy(buffer, &packet->buffer[packet->offset], bytes);

	packet->offset += bytes;
	return bytes;
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
