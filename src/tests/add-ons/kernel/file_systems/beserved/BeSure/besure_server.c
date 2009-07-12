// btAdd.cpp : Defines the entry point for the console application.
//

#include "FindDirectory.h"

#include "betalk.h"
#include "sysdepdefs.h"
#include "besure_auth.h"
#include "BlowFish.h"

#include "netdb.h"
#include "utime.h"
#include "ctype.h"
#include "time.h"
#include "signal.h"
#include "stdlib.h"
#include "syslog.h"

#define BT_MAX_THREADS		128
#define BT_MAX_RETRIES		3
#define BT_THREAD_NAME		"BeSure Handler"

typedef struct
{
	unsigned int type;
	unsigned int length;
	char *data;
} bt_arg_t;

typedef struct session
{
	int socket;
	unsigned int client_s_addr;
	thread_id handlerID;
	struct session *next;
} bt_session_t;

typedef void (*bt_net_func)(bt_session_t *, unsigned int, int, bt_arg_t *);

typedef struct dirCommand
{
	unsigned char command;
	bt_net_func handler;
	bool supported;
	uint8 args;
	uint32 argTypes[MAX_COMMAND_ARGS];
} bt_command_t;


int main(int argc, char *argv[]);
void daemonInit();
void startService();
void endService();
void restartService();
int32 requestThread(void *data);
int receiveRequest(bt_session_t *session);
void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[]);
void launchThread(int client, struct sockaddr_in *addr);
void sendErrorToClient(int client, unsigned int xid, int error);
void getArguments(bt_session_t *session, bt_inPacket *packet, unsigned char command);

int btRecv(int sock, void *data, int dataLen, int flags);
int btSend(int sock, void *data, int dataLen, int flags);

void btLock(sem_id semaphore, int32 *atomic);
void btUnlock(sem_id semaphore, int32 *atomic);

int generateTicket(const char *client, const char *server, unsigned char *key, char *ticket);
void recordLogin(char *server, char *share, char *client, bool authenticated);
//void strlwr(char *str);

int cmdAuthenticate(unsigned int addr, char *client, char *token, char *response);
int cmdReadUsers(DIR **dir, char *user, char *fullName);
int cmdReadGroups(DIR **dir, char *group);
int cmdWhichGroups(DIR **dir, char *user, char *group);

void netcmdAuthenticate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netcmdReadUsers(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netcmdReadGroups(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netcmdWhichGroups(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netcmdQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);

bt_session_t *rootSession = NULL;
bool running = true;
int server;
sem_id sessionSem;
int32 sessionVar;

bt_command_t dirCommands[] =
{
	{ BT_CMD_AUTH,				netcmdAuthenticate,		true,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_READUSERS,			netcmdReadUsers,		true,	1,	{ B_INT32_TYPE } },
	{ BT_CMD_READGROUPS,		netcmdReadGroups,		true,	1,	{ B_INT32_TYPE } },
	{ BT_CMD_WHICHGROUPS,		netcmdWhichGroups,		true,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_QUIT,				netcmdQuit,				true,	0,	{ 0 } },
	{ 0,						NULL,					false,	0,	{ 0 } }
};


int main(int argc, char *argv[])
{
	daemonInit();

	signal(SIGINT, endService);
	signal(SIGTERM, endService);
	signal(SIGHUP, restartService);
	signal(SIGPIPE, SIG_IGN);

	if ((sessionSem = create_sem(0, "Session Semaphore")) > 0)
	{
		// Run the daemon.  We will not return until the service is being stopped.
		startService();
		delete_sem(sessionSem);
	}

	return 0;
}

void daemonInit()
{
	int i;

	// Cause the parent task to terminate, freeing the terminal.
	if (fork() != 0)
		exit(0);

	// In the child process, become the session leader.
	setsid();

	// Now fork again, causing the first child to exit, since the session
	// leader can be assigned a controlling terminal under SVR4.
	signal(SIGHUP, SIG_IGN);
	if (fork() != 0)
		exit(0);

	// Change to the root directory, since if we hold on to a working
	// folder that was in a mounted file system, that file system cannot
	// be unmounted.
	chdir("/");

	// Reset the file creation mask to zero to eliminate the inherited value.
	umask(0);

	// Close open file descriptors.  Since we can't know how many of a
	// potentially unlimited value can be open, just close the first 64
	// and assume that will be enough.
	for (i = 0; i < 64; i++)
		close(i);

	// Open the syslog.
	openlog("besure_server", LOG_PID, LOG_DAEMON);
}

void restartService()
{
}

void startService()
{
	struct sockaddr_in serverAddr, clientAddr;
	int client, addrLen;
	int flags;

	// Store the length of the socket addressing structure for accept().
	addrLen = sizeof(struct sockaddr_in);

	// Initialize the server address structure.
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(BT_BESURE_PORT);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Create a new socket to receive incoming requests.
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == INVALID_SOCKET)
		return;

	// Set the socket option to reuse the current address in case it was
	// in use by a prior version of the service that has not yet relinquished
	// the socket.
	flags = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));

	// Bind that socket to the address constructed above.
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return;

	// Listen for incoming connections.
	if (listen(server, 5))
		return;

	// Continually accept incoming connections.  When one is found,
	// fire off a handler thread to accept commands.
	while (running)
	{
		client = accept(server, (struct sockaddr *) &clientAddr, &addrLen);
		if (client != INVALID_SOCKET)
			launchThread(client, &clientAddr);
	}

	// Close the socket.  Technically, I believe we should call shutdown()
	// first, but the BeOS header file socket.h indicates that this
	// function is not currently working.  It is present but may not have
	// any effect.
	shutdown(server, 2);
	closesocket(server);
	server = INVALID_SOCKET;
}

void endService()
{
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	exit(0);
}

// launchThread()
//
void launchThread(int client, struct sockaddr_in *addr)
{
	bt_session_t *s, *cur, *last = NULL;
	int count = 0;

	// First verify that the server's not too busy by scanning the list of active
	// sessions.  This is also useful because we need to eliminate unused sessions
	// from the list, i.e., sessions that have closed.
	btLock(sessionSem, &sessionVar);

	s = rootSession;
	while (s)
	{
		if (s->socket == INVALID_SOCKET)
		{
			if (last)
				last->next = s->next;
			else
				rootSession = s->next;

			cur = s->next;			
			free(s);
			s = cur;
			continue;
		}

		last = s;
		s = s->next;
		count++;
	}

	// If the total number of valid sessions was less than our allowed maximum, then
	// we can create a new session.
	if (count < BT_MAX_THREADS)
	{
		// We need to create an available session for this connection.
		bt_session_t *session = (bt_session_t *) malloc(sizeof(bt_session_t));
		if (session)
		{
			session->socket = client;
			session->client_s_addr = addr->sin_addr.s_addr;

			session->handlerID =
				spawn_thread(requestThread, BT_THREAD_NAME, B_NORMAL_PRIORITY, session);
			resume_thread(session->handlerID);
	
			// Add this to the session list.
			session->next = rootSession;
			rootSession = session;
			btUnlock(sessionSem, &sessionVar);
			return;
		}
	}

	btUnlock(sessionSem, &sessionVar);

	// We must have too many threads active, so let the client know we're busy.
	sendErrorToClient(client, 0, EBUSY);
	shutdown(client, 2);
	closesocket(client);
}

int32 requestThread(void *data)
{
	bt_session_t *session = (bt_session_t *) data;
//	int flags;

	if (!session)
		return 0;

//	flags = 1;
//	setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));

	while (receiveRequest(session));

	shutdown(session->socket, 2);
	closesocket(session->socket);
	session->socket = INVALID_SOCKET;
	return 0;
}

int receiveRequest(bt_session_t *session)
{
	bt_inPacket packet;
	char signature[20], *buffer;
	unsigned char command;
	int client, sigLen;
	int32 length;

	client = session->socket;

	// Read the BeTalk RPC header.
	sigLen = strlen(BT_RPC_SIGNATURE);
	if (btRecvMsg(client, signature, sigLen, 0) == -1)
		return 0;

//	recv(client, &verHi, sizeof(verHi), 0);
//	recv(client, &verLo, sizeof(verLo), 0);

	signature[sigLen] = 0;
	if (strcmp(signature, BT_RPC_SIGNATURE))
		return 0;

	// Read in the rest of the packet.
	if (btRecvMsg(client, &length, sizeof(int32), 0) == -1)
		return 0;

	length = B_LENDIAN_TO_HOST_INT32(length);
	if (length == 0 || length > BT_RPC_MAX_PACKET_SIZE)
		return 0;

	buffer = (char *) malloc(length + 1);
	if (!buffer)
		return 0;

	if (btRecvMsg(client, buffer, length, 0) == -1)
	{
		free(buffer);
		return 0;
	}

	buffer[length] = 0;
	packet.buffer = buffer;
	packet.length = length;
	packet.offset = 0;

	// Read the transmission ID and command.
	command = btRPCGetChar(&packet);
	getArguments(session, &packet, command);
	free(buffer);
	return (command != BT_CMD_AUTH && command != BT_CMD_QUIT);
}

void getArguments(bt_session_t *session, bt_inPacket *packet, unsigned char command)
{
	bt_arg_t args[MAX_COMMAND_ARGS];
	int i, client;
	bool error;
	unsigned char argc, terminator;
	int32 xid;

	error = false;
	client = session->socket;
	argc = btRPCGetChar(packet);
	if (argc > MAX_COMMAND_ARGS)
		return;

	for (i = 0; i < argc && !error; i++)
	{
		args[i].type = btRPCGetInt32(packet);
		args[i].data = btRPCGetNewString(packet);
		if (args[i].data == NULL)
			error = true;
	}

	if (!error)
	{
		xid = btRPCGetInt32(packet);
		terminator = btRPCGetChar(packet);
		if (terminator == BT_CMD_TERMINATOR)
			handleRequest(session, xid, command, argc, args);
	}
	else
		sendErrorToClient(session->socket, 0, EINVAL);

	while (--i >= 0)
		free(args[i].data);
}

void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[])
{
	bool validated = true;
	int i, j;

	for (i = 0; dirCommands[i].handler; i++)
		if (command == dirCommands[i].command)
		{
			// We may have received a valid command, but one that is not supported by this
			// server.  In this case, we'll want to return an operation not supported error,
			// as opposed to an invalid command error.
			if (!dirCommands[i].supported)
			{
				sendErrorToClient(session->socket, xid, EOPNOTSUPP);
				return;
			}

			// Now verify that the argument count is correct, and if so, the type of all
			// arguments is correct.  If not, an invalid command error is returned.
			// Otherise, the command is executed, and the handler returns any necessary
			// acknowledgement.
			if (argc == dirCommands[i].args)
			{
				for (j = 0; j < argc; j++)
					if (dirCommands[i].argTypes[j] != argv[j].type)
					{
						validated = false;
						break;
					}

				if (validated)
				{
					(*dirCommands[i].handler)(session, xid, argc, argv);
					return;
				}
			}
		}

	sendErrorToClient(session->socket, xid, EINVAL);
}

void sendErrorToClient(int client, unsigned int xid, int error)
{
	bt_outPacket packet;
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
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

void btRPCCreateAck(bt_outPacket *packet, unsigned int xid, int error)
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

/////////////////////////////////////////////////////////////////////

void btLock(sem_id semaphore, int32 *atomic)
{
	int32 previous = atomic_add(atomic, 1);
	if (previous >= 1)
		while (acquire_sem(semaphore) == B_INTERRUPTED);
}

void btUnlock(sem_id semaphore, int32 *atomic)
{
	int32 previous = atomic_add(atomic, -1);
	if (previous > 1)
		release_sem(semaphore);
}

////////////////////////////////////////////////////////////////////

int generateTicket(const char *client, const char *server, unsigned char *key, char *ticket)
{
	blf_ctx ctx;
	char buffer[128], session[128];
	int length;

	// Generate a session key.
	sprintf(session, "%s.%s.%lx", client, server, time(NULL));

	// Generate the connection ticket.  Because the ticket will be encrypted via the
	// BlowFish algorithm, it's length must be divisible by four.  We can pad the
	// buffer with NULL characters so it won't affect the string.
	sprintf(buffer, "%s,%s", client, session);
	length = strlen(buffer);
	while (length % 4)
		buffer[length++] = ' ';

	buffer[length] = 0;
	blf_key(&ctx, key, strlen(key));
	blf_enc(&ctx, (unsigned long *) buffer, length / 4);

	// Now we have the ticket.  It gets wrapped into a response buffer here.
	sprintf(ticket, "%s,%s,%s,%s", client, server, session, buffer);
	return strlen(ticket);
}

// strlwr()
/*
void strlwr(char *str)
{
	char *p;
	for (p = str; *p; p++)
		*p = tolower(*p);
}*/

////////////////////////////////////////////////////////////////////

int cmdAuthenticate(unsigned int addr, char *client, char *token, char *response)
{
	blf_ctx ctx;
	char server[B_FILE_NAME_LENGTH + 1], share[B_FILE_NAME_LENGTH + 1], user[MAX_USERNAME_LENGTH + 1];
	char buffer[BT_AUTH_TOKEN_LENGTH * 2 + 1];
	unsigned char clientKey[MAX_KEY_LENGTH + 1];
	int i;

	// Obtain the host name of our peer from the socket information.
	struct hostent *ent = gethostbyaddr((const char *) &addr, sizeof(addr), AF_INET);
	if (ent)
		strcpy(server, ent->h_name);
	else
	{
		uint8 *p = (uint8 *) &addr;
		sprintf(server, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	}

//	printf("\tClient = %s\n", client);
//	printf("\tToken = %s\n", token);

	// Obtain the specified client's key.
	if (!getUserPassword(client, clientKey, sizeof(clientKey)))
		return EACCES;

//	printf("\tClient key = %s\n", clientKey);

	// Get the encrypted authentication information.
	memcpy(buffer, token, BT_AUTH_TOKEN_LENGTH);
	buffer[BT_AUTH_TOKEN_LENGTH] = 0;

	// Now decrypt the request using the client's key.
	blf_key(&ctx, clientKey, strlen(clientKey));
	blf_dec(&ctx, (unsigned long *) buffer, BT_AUTH_TOKEN_LENGTH / 4);

//	printf("\tDecrypted buffer = [%s]\n", buffer);

	//  The encrypted buffer may have been padded with spaces to make its length
	// evenly divisible by four.
	i = strlen(buffer);
	while (buffer[--i] == ' ' && i >= 0)
		buffer[i] = 0;

	// Once decrypted, we can extract the server being contacted.
	memcpy(share, buffer, B_FILE_NAME_LENGTH);
	for (i = B_FILE_NAME_LENGTH - 1; i && share[i] == ' '; i--);
	share[i + 1] = 0;
//	printf("\tShare = %s\n", share);

	// The remainder of the buffer is the MD5-encoded client name.
	memcpy(user, &buffer[B_FILE_NAME_LENGTH], MAX_USERNAME_LENGTH);
	for (i = MAX_USERNAME_LENGTH - 1; i && user[i] == ' '; i--);
	user[i + 1] = 0;

//	printf("\tComparing: Client [%s] and tok [%s]\n", client, user);
	if (strcmp(client, user) != 0)
	{
		recordLogin(server, share, client, false);
		return EACCES;
	}

	recordLogin(server, share, client, true);
	generateTicket(client, share, clientKey, response);

	return B_OK;
}

void recordLogin(char *server, char *share, char *client, bool authenticated)
{
	FILE *fp;
	struct tm *localTime;
	time_t curTime;
	char path[B_PATH_NAME_LENGTH], timeStamp[50];

	if (!isServerRecordingLogins(server))
		return;

	// Obtain the filename for the log file.
	find_directory(B_COMMON_LOG_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/BeServed-Logins.log");

	// Build the time stamp.
	curTime = time(NULL);
	localTime = localtime(&curTime);
	strftime(timeStamp, sizeof(timeStamp), "%m/%d/%Y %H:%M:%S", localTime);

	fp = fopen(path, "a");
	if (fp)
	{
		fprintf(fp, "%s: %s [%s] %s %s\n", timeStamp, server,
			authenticated ? share : "--",
			client,
			authenticated ? "authenticated" : "rejected");
		fclose(fp);
	}
}

int cmdReadUsers(DIR **dir, char *user, char *fullName)
{
	struct dirent *dirInfo;

	if (!user || !fullName)
		return EINVAL;

	if (!*dir)
		*dir = OpenUsers();

	if (*dir)
		if ((dirInfo = ReadUser(*dir)) != NULL)
		{
			strcpy(user, dirInfo->d_name);
			getUserFullName(user, fullName, MAX_DESC_LENGTH);
			return B_OK;
		}
		else
		{
			CloseUsers(*dir);
			return ENOENT;
		}

	return EINVAL;
}

int cmdReadGroups(DIR **dir, char *group)
{
	struct dirent *dirInfo;

	if (!group)
		return EINVAL;

	if (!*dir)
		*dir = OpenGroups();

	if (*dir)
		if ((dirInfo = ReadGroup(*dir)) != NULL)
		{
			strcpy(group, dirInfo->d_name);
			return B_OK;
		}
		else
		{
			CloseGroups(*dir);
			return ENOENT;
		}

	return EINVAL;
}

int cmdWhichGroups(DIR **dir, char *user, char *group)
{
	int error = cmdReadGroups(dir, group);
	while (error == B_OK)
	{
		if (isUserInGroup(user, group))
			return B_OK;

		error = cmdReadGroups(dir, group);
	}

	return error;
}

////////////////////////////////////////////////////////////////////

void netcmdAuthenticate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	char response[256];

	client = session->socket;

	error = cmdAuthenticate(session->client_s_addr, argv[0].data, argv[1].data, response);
	btRPCCreateAck(&packet, xid, error);
	if (error == B_OK)
		btRPCPutString(&packet, response, strlen(response));

	btRPCSendAck(client, &packet);
}

void netcmdReadUsers(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	DIR *dir = (DIR *)(*((int32 *) argv[0].data));
	char user[MAX_USERNAME_LENGTH + 1], fullName[MAX_DESC_LENGTH + 1];
	int entries = 0;

	client = session->socket;

	error = cmdReadUsers(&dir, user, fullName);
	if (error != B_OK)
	{
		btRPCCreateAck(&packet, xid, error);
		btRPCSendAck(client, &packet);
		return;
	}

	btRPCCreateAck(&packet, xid, B_OK);
	while (error == B_OK)
	{
		btRPCPutString(&packet, user, strlen(user));
		btRPCPutString(&packet, fullName, strlen(fullName));

		if (++entries >= 80)
			break;

		error = cmdReadUsers(&dir, user, fullName);
		btRPCPutInt32(&packet, error);
	}

	// If we exhausted the list of directory entries without filling
	// the buffer, add an error message that will prevent the client
	// from requesting further entries.
	if (entries < 80)
		btRPCPutInt32(&packet, ENOENT);

	btRPCSendAck(client, &packet);
}

void netcmdReadGroups(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	DIR *dir = (DIR *)(*((int32 *) argv[0].data));
	char group[MAX_USERNAME_LENGTH + 1];
	int entries = 0;

	client = session->socket;

	error = cmdReadGroups(&dir, group);
	if (error != B_OK)
	{
		btRPCCreateAck(&packet, xid, error);
		btRPCSendAck(client, &packet);
		return;
	}

	btRPCCreateAck(&packet, xid, B_OK);
	while (error == B_OK)
	{
		btRPCPutString(&packet, group, strlen(group));

		if (++entries >= 80)
			break;

		error = cmdReadGroups(&dir, group);
		btRPCPutInt32(&packet, error);
	}

	// If we exhausted the list of directory entries without filling
	// the buffer, add an error message that will prevent the client
	// from requesting further entries.
	if (entries < 80)
		btRPCPutInt32(&packet, ENOENT);

	btRPCSendAck(client, &packet);
}

void netcmdWhichGroups(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	DIR *dir = 0;
	char group[MAX_USERNAME_LENGTH + 1];
	int entries = 0;

	client = session->socket;

	error = cmdWhichGroups(&dir, argv[0].data, group);
	if (error != B_OK)
	{
		btRPCCreateAck(&packet, xid, error);
		btRPCSendAck(client, &packet);
		return;
	}

	btRPCCreateAck(&packet, xid, B_OK);
	while (error == B_OK)
	{
		btRPCPutString(&packet, group, strlen(group));

		if (++entries >= 80)
			break;

		error = cmdWhichGroups(&dir, argv[0].data, group);
		btRPCPutInt32(&packet, error);
	}

	// If we exhausted the list of directory entries without filling
	// the buffer, add an error message that will prevent the client
	// from requesting further entries.
	if (entries < 80)
		btRPCPutInt32(&packet, ENOENT);

	btRPCSendAck(client, &packet);
}

// netcmdQuit()
//
void netcmdQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, B_OK);
	btRPCSendAck(client, &packet);
}
