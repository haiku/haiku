#include "betalk.h"
#include "authentication.h"
#include "sysdepdefs.h"
#include "netdb.h"

#include "ctype.h"
#include "signal.h"
#include "stdlib.h"

bt_inPacket *btRPCSimpleCall(unsigned int serverIP, int port, bt_outPacket *outPacket);
int btRPCConnect(unsigned int serverIP, int port);
bool btRPCSend(int session, bt_outPacket *outPacket);
bool btRPCCheckSignature(int session);
bt_outPacket *btRPCPutHeader(unsigned char command, unsigned char argc, int32 length);
void btRPCPutArg(bt_outPacket *packet, unsigned int type, void *data, int length);


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

			// Now allocate a buffer of the appropriate length.  If one cannot be
			// allocated, we won't be able to store incoming information and the call
			// must be aborted.
			xid = B_LENDIAN_TO_HOST_INT32(xid);
			length = B_LENDIAN_TO_HOST_INT32(length);
			if (length > 0 && length < BT_RPC_MAX_PACKET_SIZE)
			{
				buffer = (char *) malloc(length + 1);
				if (buffer)
				{
					// Read the remaining packet contents.  The btRecvMsg() function takes
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

bool authenticateUser(char *user, char *password)
{
	extern unsigned int authServerIP;
	bt_outPacket *outPacket;
	bt_inPacket *inPacket;
	bool authenticated = false;
	int error;

	outPacket = btRPCPutHeader(BT_CMD_AUTH, 2, strlen(user) + BT_AUTH_TOKEN_LENGTH);
	if (outPacket)
	{
		btRPCPutArg(outPacket, B_STRING_TYPE, user, strlen(user));
		btRPCPutArg(outPacket, B_STRING_TYPE, password, BT_AUTH_TOKEN_LENGTH);
		inPacket = btRPCSimpleCall(authServerIP, BT_BESURE_PORT, outPacket);
		if (inPacket)
		{
			error = btRPCGetInt32(inPacket);
			if (error == B_OK)
				authenticated = true;
	
			free(inPacket->buffer);
			free(inPacket);
		}

		free(outPacket->buffer);
		free(outPacket);
	}

	return authenticated;
}

void getUserGroups(char *user, char **groups)
{
	extern unsigned int authServerIP;
	bt_outPacket *outPacket;
	bt_inPacket *inPacket;
	int i, error;

	outPacket = btRPCPutHeader(BT_CMD_WHICHGROUPS, 1, strlen(user));
	if (outPacket)
	{
		btRPCPutArg(outPacket, B_STRING_TYPE, user, strlen(user));
		inPacket = btRPCSimpleCall(authServerIP, BT_BESURE_PORT, outPacket);
		if (inPacket)
		{
			i = 0;
			error = btRPCGetInt32(inPacket);
			while (error == B_OK)
			{
				groups[i++] = btRPCGetNewString(inPacket);
				error = btRPCGetInt32(inPacket);
			}
	
			free(inPacket->buffer);
			free(inPacket);
		}

		free(outPacket->buffer);
		free(outPacket);
	}
}
