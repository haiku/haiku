// btAdd.cpp : Defines the entry point for the console application.
//

// To-do:
//
// 1.0.67
// X Auto-mount from My Network
// _ BONE-compliance
// X Better reconnect logic
// X INT and TERM signal handlers
// X Lingering buffer mechanism
// X Test for duplicate share names defined in settings file
//
// 1.0.68 (stability)
// _ Fix problems with share browsing
// _ Fix duplicate vnid problem
// X Fix large file write problem
// X Correct crash during unmount process
// _ Packet buffer allocation/de-allocation cleanup
// _ Query capabilities
// _ Ability to mount multiple shares
// _ Node monitoring
//
// 1.0.69 (security)
// _ Don't allow following links outside the shared folder
// _ Make a file share read-only
// _ Restart of server erases inode-to-filename map
//
// Long-term
// _ Ability to reconnect shares at login
// _ Preferences application for defining file shares
//

// Potential Uses:
// 1.  Domain server, keeping track of users for logging in
// 2.  File server, to share BeOS volume files across a network
// 3.  Document management, to store documents with attributes
// 4.  Version Control System, to manage multiple versions of documents
// 5.  General directory server, for local or network settings and information

#include "FindDirectory.h"

#include "betalk.h"
#include "fsproto.h"
#include "netdb.h"

#include "utime.h"
#include "ctype.h"
#include "time.h"
#include "signal.h"

#define BT_MAX_THREADS		100
#define BT_MAX_RETRIES		3
#define BT_MAX_FILE_SHARES	128

#define BT_MAIN_NAME		"BeServed Daemon"
#define BT_THREAD_NAME		"BeServed Handler"
#define BT_HOST_THREAD_NAME	"BeServed Host Publisher"
#define BT_SIGNATURE		"application/x-vnd.Teldar-BeServed"

#define PATH_ROOT			"/boot"
#define PATH_DELIMITER		'/'

#ifndef iswhite
#define iswhite(c)			((c == ' ' || c == '\t'))
#endif

typedef struct
{
	unsigned int type;
	unsigned int length;
	char *data;
} bt_arg_t;

typedef struct session
{
	int socket;
	unsigned int s_addr;
	thread_id handlerID;

	// What share did the client connect to?
	int share;

	char ioBuffer[BT_MAX_IO_BUFFER + 1];
	char attrBuffer[BT_MAX_ATTR_BUFFER + 1];
	char pathBuffer[B_PATH_NAME_LENGTH];
} bt_session_t;

typedef void (*bt_net_func)(bt_session_t *, unsigned int, int, bt_arg_t *);

typedef struct dirCommand
{
	unsigned char command;
	bt_net_func handler;
} bt_command_t;

typedef struct
{
	vnode_id vnid;
	int shareId;
	char unused[20];
} bt_fdesc;

typedef struct btnode
{
	vnode_id vnid;
	char name[100];
	int refCount;
	struct btnode *next;
	struct btnode *prev;
	struct btnode *parent;
} bt_node;

typedef struct fileShare
{
	char path[B_PATH_NAME_LENGTH];
	char name[B_FILE_NAME_LENGTH];
	bool used;
	bool readOnly;
	struct fileShare *next;
} bt_fileShare_t;

bt_node *rootNode = NULL;


int main(int argc, char *argv[]);
bool dateCheck();
int32 btSendHost(void *data);
void startService();
void endService(int sig);
void initSessions();
void initShares();
void getFileShare(const char *buffer);
int receiveRequest(bt_session_t *session);
void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[]);
void launchThread(int client, struct sockaddr_in *addr);
int tooManyConnections(unsigned int s_addr);
void sendErrorToClient(int client, int error);
void printToClient(int client, char *format, ...);
void getArguments(bt_session_t *session, bt_inPacket *packet, unsigned char command);
int32 requestThread(void *data);
int copyFile(char *source, char *dest);
int stricmp(char *str1, char *str2);

bt_node *btGetNodeFromVnid(vnode_id vnid);
void btAddHandle(bt_fdesc *dhandle, bt_fdesc *fhandle, char *name);
void btRemoveHandle(bt_fdesc *fhandle);
void btPurgeNodes(bt_fdesc *fdesc);
void btRemoveNode(bt_node *deadNode);
bool btIsAncestorNode(vnode_id vnid, bt_node *node);
char *btGetLocalFileName(char *path, bt_fdesc *fhandle);
void btMakeHandleFromNode(bt_node *node, bt_fdesc *fdesc);
bt_node *btFindNode(bt_node *parent, char *fileName);
char *btGetSharePath(char *shareName);
int btGetShareId(char *shareName);
void btGetRootPath(vnode_id vnid, char *path);
void btLock(sem_id semaphore, int32 *atomic);
void btUnlock(sem_id semaphore, int32 *atomic);

int btMount(char *shareName, btFileHandle *fhandle);
int btGetFSInfo(fs_info *fsInfo);
int btLookup(char *pathBuf, bt_fdesc *ddesc, char *fileName, bt_fdesc *fdesc);
int btStat(char *pathBuf, bt_fdesc *fdesc, struct stat *st);
int btReadDir(char *pathBuf, bt_fdesc *ddesc, DIR **dir, bt_fdesc *fdesc, char *filename, struct stat *st);
int32 btRead(char *pathBuf, bt_fdesc *fdesc, off_t pos, int32 len, char *buffer);
int32 btWrite(char *pathBuf, bt_fdesc *fdesc, off_t pos, int32 len, char *buffer);
int btCreate(char *pathBuf, bt_fdesc *ddesc, char *name, int omode, int perms, bt_fdesc *fdesc);
int btTruncate(char *pathBuf, bt_fdesc *fdesc, int64 len);
int btCreateDir(char *pathBuf, bt_fdesc *ddesc, char *name, int perms, bt_fdesc *fdesc, struct stat *st);
int btDeleteDir(char *pathBuf, bt_fdesc *ddesc, char *name);
int btRename(char *pathBuf, bt_fdesc *oldfdesc, char *oldName, bt_fdesc *newfdesc, char *newName);
int btUnlink(char *pathBuf, bt_fdesc *ddesc, char *name);
int btReadLink(char *pathBuf, bt_fdesc *fdesc, char *buffer, int length);
int btSymLink(char *pathBuf, bt_fdesc *fdesc, char *name, char *dest);
int btWStat(char *pathBuf, bt_fdesc *fdesc, long mask, int32 mode, int32 uid, int32 gid, int64 size, int32 atime, int32 mtime);
int btReadAttrib(char *pathBuf, bt_fdesc *fdesc, char *name, int32 dataType, void *buffer, int32 pos, int32 len);
int btWriteAttrib(char *pathBuf, bt_fdesc *fdesc, char *name, int32 dataType, void *buffer, int32 pos, int32 len);
int btReadAttribDir(char *pathBuf, bt_fdesc *fdesc, DIR **dir, char *attrName);
int btRemoveAttrib(char *pathBuf, bt_fdesc *fdesc, char *name);
int btStatAttrib(char *pathBuf, bt_fdesc *fdesc, char *name, struct attr_info *info);
int btReadIndexDir(DIR **dir, char *indexName);
int btCreateIndex(char *name, int type, int flags);
int btRemoveIndex(char *name);
int btStatIndex(char *name, struct index_info *info);
int btReadQuery(DIR **dir, char *query, char *fileName, int64 *inode);

void netbtMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtFSInfo(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtLookup(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtReadDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtRead(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtWrite(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtCreate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtTruncate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtCreateDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtDeleteDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtRename(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtUnlink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtReadLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtSymLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtWStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtReadAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtWriteAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtReadAttribDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtRemoveAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtStatAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtReadIndexDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtCreateIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtRemoveIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtStatIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtReadQuery(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);

bt_session_t sessions[BT_MAX_THREADS];
bt_fileShare_t fileShares[BT_MAX_FILE_SHARES];
bool running = true;
int server;
thread_id hostThread;
sem_id handleSem;
int32 handleVar;

bt_command_t dirCommands[] =
{
	{ BT_CMD_MOUNT,				netbtMount },
	{ BT_CMD_FSINFO,			netbtFSInfo },
	{ BT_CMD_LOOKUP,			netbtLookup },
	{ BT_CMD_STAT,				netbtStat },
	{ BT_CMD_READDIR,			netbtReadDir },
	{ BT_CMD_READ,				netbtRead },
	{ BT_CMD_WRITE,				netbtWrite },
	{ BT_CMD_CREATE,			netbtCreate },
	{ BT_CMD_TRUNCATE,			netbtTruncate },
	{ BT_CMD_MKDIR,				netbtCreateDir },
	{ BT_CMD_RMDIR,				netbtDeleteDir },
	{ BT_CMD_RENAME,			netbtRename },
	{ BT_CMD_UNLINK,			netbtUnlink },
	{ BT_CMD_READLINK,			netbtReadLink },
	{ BT_CMD_SYMLINK,			netbtSymLink },
	{ BT_CMD_WSTAT,				netbtWStat },
	{ BT_CMD_READATTRIB,		netbtReadAttrib },
	{ BT_CMD_WRITEATTRIB,		netbtWriteAttrib },
	{ BT_CMD_READATTRIBDIR,		netbtReadAttribDir },
	{ BT_CMD_REMOVEATTRIB,		netbtRemoveAttrib },
	{ BT_CMD_STATATTRIB,		netbtStatAttrib },
	{ BT_CMD_READINDEXDIR,		netbtReadIndexDir },
	{ BT_CMD_CREATEINDEX,		netbtCreateIndex },
	{ BT_CMD_REMOVEINDEX,		netbtRemoveIndex },
	{ BT_CMD_STATINDEX,			netbtStatIndex },
	{ BT_CMD_READQUERY,			netbtReadQuery },
	{ BT_CMD_QUIT,				netbtQuit },
	{ 0,						NULL }
};

/*----------------------------------------------------------------
class BeServedServer : public BApplication
{
	thread_id appThread;
	bool running;

	public:
		BeServedServer(const char *signature);

		virtual void ReadyToRun();
		virtual bool QuitRequested();
};

BeServedServer::BeServedServer(const char *signature)
	: BApplication(signature)
{
}

void BeServedServer::ReadyToRun()
{
	running = true;
	appThread = spawn_thread(appMain, BT_MAIN_NAME, B_NORMAL_PRIORITY, this);
	resume_thread(appThread);
}

bool BeServedServer::QuitRequested()
{
	status_t result;

	if (!BApplication::QuitRequested())
		return false;

	running = false;
	wait_for_thread(appThread, &result);
	return true;
}

int main(int argc, char *argv[])
{
	BeServedServer app(BT_SIGNATURE);
	app.Run();
	return 0;
}
----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	if (!dateCheck())
		return 0;

	initSessions();
	initShares();
	signal(SIGINT, endService);
	signal(SIGTERM, endService);

	if ((handleSem = create_sem(0, "File Handle Semaphore")) > B_OK)
	{
		hostThread = spawn_thread(btSendHost, BT_HOST_THREAD_NAME, B_NORMAL_PRIORITY, 0);
		resume_thread(hostThread);

		// Run the daemon.  We will not return until the service is being
		// stopped.
		startService();

		if (hostThread > 0)
			kill_thread(hostThread);

		if (handleSem > 0)
			delete_sem(handleSem);
	}

	return 0;
}

bool dateCheck()
{
	struct stat st;
	time_t curTime;

	time(&curTime);
	if (curTime > 999491300)
		return false;

	if (stat("/boot/home/config/servers/beserved_server", &st) == 0)
		if (curTime < st.st_ctime || curTime > st.st_ctime + 5184000)
			return false;

	return true;
}

int32 btSendHost(void *data)
{
	struct sockaddr_in serverAddr, clientAddr;
	char buffer[4096];
	int i, server, addrLen, bufLen;

	buffer[0] = 0;
	bufLen = sizeof(buffer);

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(BT_QUERYHOST_PORT);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	server = socket(AF_INET, SOCK_DGRAM, 0);
	if (server == INVALID_SOCKET)
		return -1;

	// Bind that socket to the address constructed above.
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return -1;

	while (running)
	{
		addrLen = sizeof(struct sockaddr_in);
		recvfrom(server, buffer, bufLen - 1, 0, (struct sockaddr *) &clientAddr, &addrLen);

		if (strcmp(buffer, "btGetHost") == 0)
			gethostname(buffer, bufLen);
		else if (strcmp(buffer, "btGetShares") == 0)
		{
			buffer[0] = 0;
			for (i = 0; i < BT_MAX_FILE_SHARES; i++)
				if (fileShares[i].used)
				{
					strcat(buffer, fileShares[i].name);
					strcat(buffer, "/");
				}
		}
		else
			continue;

		sendto(server, buffer, strlen(buffer), 0, (struct sockaddr *) &clientAddr, addrLen);
	}

	// Close the socket.  Technically, I believe we should call shutdown()
	// first, but the BeOS header file socket.h indicates that this
	// function is not currently working.  It is present but may not have
	// any effect.
	shutdown(server, 2);
	closesocket(server);
	return 0;
}

void initSessions()
{
	int i;

	for (i = 0; i < BT_MAX_THREADS; i++)
	{
		sessions[i].socket = INVALID_SOCKET;
		sessions[i].handlerID = 0;
	}
}

void initShares()
{
	FILE *fp;
	char path[B_PATH_NAME_LENGTH], buffer[512];
	int i, length;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
	{
		fileShares[i].name[0] = 0;
		fileShares[i].path[0] = 0;
		fileShares[i].used = false;
		fileShares[i].readOnly = false;
		fileShares[i].next = NULL;
	}

	find_directory(B_COMMON_SETTINGS_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/BeServed-Settings");

	fp = fopen(path, "r");
	if (fp)
	{
		while (fgets(buffer, sizeof(buffer) - 1, fp))
		{
			length = strlen(buffer);
			if (length <= 1 || buffer[0] == '#')
				continue;

			if (buffer[length - 1] == '\n')
				buffer[--length] = 0;

			if (strncmp(buffer, "share ", 6) == 0)
				getFileShare(buffer);
		}

		fclose(fp);
	}
}

void getFileShare(const char *buffer)
{
	struct stat st;
	char path[B_PATH_NAME_LENGTH], share[B_FILE_NAME_LENGTH], *tok, *folder, delimiter;
	int i;

	tok = (char *) buffer + (6 * sizeof(char));
	while (*tok && iswhite(*tok))
		tok++;

	if (!*tok)
		return;

	if (*tok == '"')
	{
		delimiter = '"';
		tok++;
	}
	else
		delimiter = ' ';

	i = 0;
	while (*tok && *tok != delimiter)
		path[i++] = *tok++;

	if (!*tok)
		return;

	path[i] = 0;
	while (*tok && iswhite(*tok))
		tok++;

	if (!*tok)
		return;

	if (strncmp(tok, "as ", 3) != 0)
		return;

	tok += 3;
	while (*tok && iswhite(*tok))
		tok++;

	if (*tok == '"')
	{
		delimiter = '"';
		tok++;
	}
	else
		delimiter = ' ';

	i = 0;
	while (*tok && (isalnum(*tok) || (*tok == ' ' && delimiter == '"')))
		share[i++] = *tok++;

	if (i == 0)
		return;

	// Now verify that the share name specified has not already been
	// used to share another path.
	share[i] = 0;
	folder = btGetSharePath(share);
	if (folder)
	{
		printf("%s already defined as %s\n", share, folder);
		return;
	}

	// Check the path to ensure its validity.
	if (stat(path, &st) != 0)
		return;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (!fileShares[i].used)
		{
			printf("Defining %s as %s\n", share, path);
			strcpy(fileShares[i].name, share);
			strcpy(fileShares[i].path, path);
			fileShares[i].used = true;
			return;
		}

	printf("Share %s could not be defined (too many shares)\n", share);
}

void startService()
{
	struct sockaddr_in serverAddr, clientAddr;
	int client, addrLen;
	char flags;

	// Store the length of the socket addressing structure for accept().
	addrLen = sizeof(struct sockaddr_in);

	// Initialize the server address structure.
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_port = htons(BT_TCPIP_PORT);
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

void endService(int sig)
{
	if (hostThread > 0)
		kill_thread(hostThread);

	if (handleSem > 0)
		delete_sem(handleSem);

	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	exit(0);
}

// launchThread()
//
void launchThread(int client, struct sockaddr_in *addr)
{
	int i, retry, found;

	// We need to find an available session for this connection.  We don't
	// want to create threads willy nilly until we bring down the OS, so we
	// establish a pool of available sessions (threads) from which we must
	// allocate with every request.
	found = FALSE;
	for (retry = 0; retry < BT_MAX_RETRIES && !found; retry++)
	{
		for (i = 0; i < BT_MAX_THREADS; i++)
			if (sessions[i].socket == INVALID_SOCKET)
			{
				found = TRUE;
				sessions[i].socket = client;
				sessions[i].s_addr = addr->sin_addr.s_addr;
				sessions[i].handlerID =
					spawn_thread(requestThread, BT_THREAD_NAME, B_NORMAL_PRIORITY, &sessions[i]);
				resume_thread(sessions[i].handlerID);
				break;
			}

		if (!found)
			snooze(100000);
	}

	if (!found)
	{
		sendErrorToClient(client, EBUSY);
		shutdown(client, 2);
		closesocket(client);
	}
}

int32 requestThread(void *data)
{
	bt_session_t *session = (bt_session_t *) data;

	if (!session)
		return 0;

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
	int32 bytesRead, length;

	client = session->socket;

	// Read the BeTalk RPC header.
	sigLen = strlen(BT_RPC_SIGNATURE);
	recv(client, signature, sigLen, 0);
//	recv(client, &verHi, sizeof(verHi), 0);
//	recv(client, &verLo, sizeof(verLo), 0);

	signature[sigLen] = 0;
	if (strcmp(signature, BT_RPC_SIGNATURE))
		return 0;

	// Read in the rest of the packet.
	recv(client, &length, sizeof(int32), 0);
	if (length == 0 || length > BT_RPC_MAX_PACKET_SIZE)
		return 0;

	buffer = (char *) malloc(length + 1);
	if (!buffer)
		return 0;

	bytesRead = 0;
	do
	{
		bytesRead += recv(client, buffer + bytesRead, length - bytesRead, 0);
	} while (bytesRead < length);

	buffer[length] = 0;
	packet.buffer = buffer;
	packet.length = length;
	packet.offset = 0;

	// Read the transmission ID and command.
	command = btRPCGetChar(&packet);
	getArguments(session, &packet, command);
	free(buffer);
	return (command != BT_CMD_QUIT);
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

	while (--i >= 0)
		free(args[i].data);
}

void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[])
{
	int i;

	for (i = 0; dirCommands[i].handler; i++)
		if (command == dirCommands[i].command)
		{
			(*dirCommands[i].handler)(session, xid, argc, argv);
			return;
		}

//	sendErrorToClient(sessions[sessionID].socket, E_BAD_COMMAND);
}

void sendErrorToClient(int client, int error)
{
//	send(client, &error, sizeof(error), 0);
}

void printToClient(int client, char *format, ...)
{
	va_list args;
	char buffer[256];

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	send(client, buffer, strlen(buffer), 0);
}

int copyFile(char *source, char *dest)
{
	FILE *srcFile, *dstFile;
	int c;

	srcFile = fopen(source, "rb");
	if (srcFile == NULL)
		return EACCES;

	dstFile = fopen(dest, "wb");
	if (dstFile == NULL)
	{
		fclose(srcFile);
		return EACCES;
	}

	while ((c = fgetc(srcFile)) != EOF)
		fputc(c, dstFile);

	fclose(dstFile);
	fclose(srcFile);
	return B_OK;
}

int stricmp(char *str1, char *str2)
{
	char *p1, *p2, c1, c2;

	for (p1 = str1, p2 = str2; *p1; p1++, p2++)
	{
		if (!*p2)
			return 1;

		c1 = toupper(*p1);
		c2 = toupper(*p2);
		if (c1 < c2)
			return -1;
		else if (c1 > c2)
			return 1;
	}

	if (*p2)
		return -1;

	return 0;
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
	int32 bytesSent;

	if (packet)
		if (packet->buffer)
		{
			*(int32 *)(&packet->buffer[9]) = packet->length - 13;

			bytesSent = 0;
			do
			{
				bytesSent += send(client, packet->buffer + bytesSent, packet->length - bytesSent, 0);
			} while (bytesSent < packet->length);

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
		return ERANGE;

	bytes = *((int32 *) &packet->buffer[packet->offset]);
	packet->offset += sizeof(bytes);
	if (!bytes)
		return ERANGE;

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

////////////////////////////////////////////////////////////////////

bt_node *btGetNodeFromVnid(vnode_id vnid)
{
	bt_node *curNode = rootNode;

	while (curNode)
	{
		if (curNode->vnid == vnid)
			break;

		curNode = curNode->next;
	}

	return curNode;
}

// btAddHandle()
//
void btAddHandle(bt_fdesc *dhandle, bt_fdesc *fhandle, char *name)
{
	bt_node *curNode, *lastNode;

	// We don't store the references to the current and the parent directory.
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return;

	btLock(handleSem, &handleVar);

	// Move to the last node in the list.
	curNode = lastNode = rootNode;
	while (curNode)
	{
		lastNode = curNode;
		curNode = curNode->next;
	}

	// Allocate a new node.
	curNode = (bt_node *) malloc(sizeof(bt_node));
	if (curNode == NULL)
	{
		btUnlock(handleSem, &handleVar);
		return;
	}

	// Copy over the name, vnid, and parent node.  Obtaining the parent
	// node requires scanning the list.
	strcpy(curNode->name, name);
	curNode->prev = lastNode;
	curNode->next = NULL;
	curNode->refCount = 0;
	curNode->vnid = fhandle->vnid;

	// If we weren't provided with the directory node, then this must be
	// the root node we're adding.
	if (dhandle)
		curNode->parent = btGetNodeFromVnid(dhandle->vnid);
	else
		curNode->parent = NULL;

	// If we didn't have a last node in the list, that means we're
	// creating the first (or root) node.  The global root node pointer
	// needs to be updated accordingly.  Otherwise, we just tack this
	// item on to the end of the list.
	if (lastNode == NULL)
		rootNode = curNode;
	else
		lastNode->next = curNode;

	btUnlock(handleSem, &handleVar);
}

// btRemoveHandle()
//
void btRemoveHandle(bt_fdesc *fhandle)
{
	bt_node *deadNode;

	btLock(handleSem, &handleVar);

	// Obtain the node in question.  If no such node exists, then we
	// probably have a bad handle.
	deadNode = btGetNodeFromVnid(fhandle->vnid);
	btRemoveNode(deadNode);

	btUnlock(handleSem, &handleVar);
}

// btRemoveNode()
//
void btRemoveNode(bt_node *deadNode)
{
	if (deadNode)
	{
		// Make this entry's predecessor point to its successor.
		if (deadNode->prev)
			deadNode->prev->next = deadNode->next;

		// Make this entry's successor point to its predecessor.
		if (deadNode->next)
			deadNode->next->prev = deadNode->prev;

		// Now deallocate this node.
		free(deadNode);
	}
}

// btPurgeNodes()
//
void btPurgeNodes(bt_fdesc *fdesc)
{
	bt_node *curNode;
	vnode_id targetVnid;

	targetVnid = fdesc->vnid;

	btLock(handleSem, &handleVar);

	curNode = rootNode;
	while (curNode)
	{
		if (curNode->vnid == targetVnid || btIsAncestorNode(targetVnid, curNode))
			btRemoveNode(curNode);

		curNode = curNode->next;
	}

	btUnlock(handleSem, &handleVar);
}

// btIsAncestorNode()
//
bool btIsAncestorNode(vnode_id vnid, bt_node *node)
{
	bt_node *curNode = node->parent;

	while (curNode)
	{
		if (curNode->vnid == vnid)
			return true;

		curNode = curNode->parent;
	}

	return false;
}

// btGetLocalFileName()
//
char *btGetLocalFileName(char *path, bt_fdesc *fhandle)
{
	bt_node *node, *nodeStack[100];
	int stackSize;

	path[0] = 0;
	stackSize = 1;

	btLock(handleSem, &handleVar);

	node = btGetNodeFromVnid(fhandle->vnid);
	if (node == NULL)
	{
		btUnlock(handleSem, &handleVar);
		return NULL;
	}

	nodeStack[0] = node;
	while ((node = node->parent) != NULL)
		nodeStack[stackSize++] = node;

	btUnlock(handleSem, &handleVar);

	while (--stackSize >= 0)
	{
		strcat(path, nodeStack[stackSize]->name);
		if (stackSize)
			strcat(path, "/");
	}

	return path;
}

void btMakeHandleFromNode(bt_node *node, bt_fdesc *fdesc)
{
	if (node && fdesc)
		fdesc->vnid = node->vnid;
}

bt_node *btFindNode(bt_node *parent, char *fileName)
{
	bt_node *node;

	btLock(handleSem, &handleVar);

	node = rootNode;
	while (node)
	{
		if (node->parent == parent)
			if (strcmp(node->name, fileName) == 0)
				break;

		node = node->next;
	}

	btUnlock(handleSem, &handleVar);
	return node;
}

char *btGetSharePath(char *shareName)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (stricmp(fileShares[i].name, shareName) == 0)
				return fileShares[i].path;

	return NULL;
}

int btGetShareId(char *shareName)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (stricmp(fileShares[i].name, shareName) == 0)
				return i;

	return -1;
}

// btGetRootPath()
//
void btGetRootPath(vnode_id vnid, char *path)
{
	bt_node *curNode;

	btLock(handleSem, &handleVar);

	curNode = btGetNodeFromVnid(vnid);
	while (curNode && curNode->parent)
		curNode = curNode->parent;

	if (curNode)
		strcpy(path, curNode->name);
	else
		path[0] = 0;

	btUnlock(handleSem, &handleVar);
}

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
/*
void btNotifyListeners(char *shareName)
{	
	struct sockaddr_in toAddr, fromAddr;
	int i;

	for (i = 0; i < BT_MAX_THREADS; i++)
		if (stricmp(sessions[i].share, shareName) == 0)
		{
			memset(&toAddr, 0, sizeof(toAddr));
			toAddr.sin_port = htons(BT_NODE_MONITOR_PORT);
			toAddr.sin_family = AF_INET;
			toAddr.sin_addr.s_addr = sessions[i].s_addr;

			sendto(sock, packet, sizeof(packet), 0, &fromAddr, sizeof(fromAddr));
		}
}
*/
////////////////////////////////////////////////////////////////////

int btMount(char *shareName, btFileHandle *fhandle)
{
	struct stat st;
	bt_fdesc *fdesc;
	char *path;

	path = btGetSharePath(shareName);
	if (!path)
		return ENOENT;

	if (stat(path, &st) != 0)
		return ENOENT;

	if (!S_ISDIR(st.st_mode))
		return EACCES;

	fdesc = (bt_fdesc *) fhandle;
	fdesc->vnid = st.st_ino;
	fdesc->shareId = btGetShareId(shareName);

	btAddHandle(NULL, fdesc, path);
	return B_OK;
}

int btGetFSInfo(fs_info *fsInfo)
{
	dev_t device = dev_for_path(rootNode->name);
	if (device < 0)
		return device;

	if (fs_stat_dev(device, fsInfo) != 0)
		return errno;

	return B_OK;
}

int btLookup(char *pathBuf, bt_fdesc *ddesc, char *fileName, bt_fdesc *fdesc)
{
	bt_node *dnode, *fnode;
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;

	fdesc->vnid = 0;

	btLock(handleSem, &handleVar);
	dnode = btGetNodeFromVnid(ddesc->vnid);
	btUnlock(handleSem, &handleVar);

	if (!dnode)
		return EACCES;

	// Search all nodes for one with the given parent vnid and file
	// name.  If one is found, we can simply use that node to fill in
	// the new handle.
	fnode = btFindNode(dnode, fileName);
	if (fnode)
	{
//		btMakeHandleFromNode(fnode, fdesc);
		fdesc->vnid = fnode->vnid;

		folder = btGetLocalFileName(pathBuf, fdesc);
		if (folder)
			if (stat(folder, &st) != 0)
			{
				btRemoveHandle(fdesc);
				fdesc->vnid = 0;
				return ENOENT;
			}
	}
	else
	{
		folder = btGetLocalFileName(pathBuf, ddesc);
		if (folder)
		{
			sprintf(path, "%s/%s", folder, fileName);
			if (stat(path, &st) != 0)
				return ENOENT;

			fdesc->vnid = st.st_ino;
			btAddHandle(ddesc, fdesc, fileName);
		}
	}

	return B_OK;
}

int btStat(char *pathBuf, bt_fdesc *fdesc, struct stat *st)
{
	char *fileName;
	int error;

	fileName = btGetLocalFileName(pathBuf, fdesc);
	if (fileName)
	{
		error = stat(fileName, st);
		return (error != 0 ? ENOENT : B_OK);
	}

	return ENOENT;
}

int btReadDir(char *pathBuf, bt_fdesc *ddesc, DIR **dir, bt_fdesc *fdesc, char *filename, struct stat *st)
{
	struct dirent *dirInfo;
	char *folder, path[B_PATH_NAME_LENGTH];

	if (!ddesc || !fdesc || !filename)
		return EINVAL;

	if (!*dir)
	{
		folder = btGetLocalFileName(pathBuf, ddesc);
		if (folder)
			*dir = opendir(folder);
	}

	if (*dir)
		if ((dirInfo = readdir(*dir)) != NULL)
		{
			folder = btGetLocalFileName(pathBuf, ddesc);
			if (folder)
			{
				sprintf(path, "%s/%s", folder, dirInfo->d_name);
				if (stat(path, st) != 0)
					return ENOENT;

				strcpy(filename, dirInfo->d_name);
				fdesc->vnid = st->st_ino;
				btAddHandle(ddesc, fdesc, filename);
				return B_OK;
			}
		}
		else
		{
			closedir(*dir);
			return ENOENT;
		}

	return EINVAL;
}

int32 btRead(char *pathBuf, bt_fdesc *fdesc, off_t pos, int32 len, char *buffer)
{
	char *path;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		FILE *fp = fopen(path, "rb");
		if (fp)
		{
			fseek(fp, pos, SEEK_SET);
			len = fread(buffer, 1, len, fp);
			fclose(fp);

			buffer[len] = 0;
			return len;
		}
	}

	return 0;
}

int32 btWrite(char *pathBuf, bt_fdesc *fdesc, off_t pos, int32 len, char *buffer)
{
	char *path;
	int32 bytes;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		int file;
		file = open(path, O_WRONLY | O_CREAT);
		if (file >= 0)
		{
			lseek(file, pos, SEEK_SET);
			bytes = write(file, buffer, len);
			close(file);
			return bytes;
		}
	}

	return 0;
}

int btCreate(char *pathBuf, bt_fdesc *ddesc, char *name, int omode, int perms, bt_fdesc *fdesc)
{
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;
	int fh;

	folder = btGetLocalFileName(pathBuf, ddesc);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		fh = open(path, O_RDWR | O_CREAT | omode);
		if (fh == -1)
			return errno;
		else
		{
			close(fh);
			chmod(path, perms);
			if (stat(path, &st) == 0)
			{
				fdesc->vnid = st.st_ino;
				btAddHandle(ddesc, fdesc, name);
			}
			else
				return EACCES;
		}
	}

	return B_OK;
}

int btTruncate(char *pathBuf, bt_fdesc *fdesc, int64 len)
{
	char *path;
	int error;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		error = truncate(path, len);
		if (error == -1)
			error = errno;

		return error;
	}

	return EACCES;
}

// btCreateDir()
//
int btCreateDir(char *pathBuf, bt_fdesc *ddesc, char *name, int perms, bt_fdesc *fdesc, struct stat *st)
{
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, ddesc);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		if (mkdir(path, perms) == -1)
			return errno;

		if (stat(path, st) != 0)
			return errno;

		fdesc->vnid = st->st_ino;
		btAddHandle(ddesc, fdesc, name);
		return B_OK;
	}

	return ENOENT;
}

// btDeleteDir()
//
int btDeleteDir(char *pathBuf, bt_fdesc *ddesc, char *name)
{
	bt_fdesc fdesc;
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, ddesc);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		if (stat(path, &st) != 0)
			return errno;

		if (rmdir(path) == -1)
			return errno;

		fdesc.vnid = st.st_ino;
		btPurgeNodes(&fdesc);
		return B_OK;
	}

	return ENOENT;
}

// btRename()
//
int btRename(char *pathBuf, bt_fdesc *oldfdesc, char *oldName, bt_fdesc *newfdesc, char *newName)
{
	bt_fdesc fdesc;
	struct stat st;
	char oldPath[B_PATH_NAME_LENGTH], newPath[B_PATH_NAME_LENGTH], *oldFolder, *newFolder;

	oldFolder = btGetLocalFileName(pathBuf, oldfdesc);
	if (oldFolder)
	{
		sprintf(oldPath, "%s/%s", oldFolder, oldName);

		newFolder = btGetLocalFileName(pathBuf, newfdesc);
		if (newFolder)
		{
			sprintf(newPath, "%s/%s", newFolder, newName);

			if (stat(oldPath, &st) != 0)
				return errno;

			fdesc.vnid = st.st_ino;
			btPurgeNodes(&fdesc);

			if (rename(oldPath, newPath) == -1)
				return errno;

			return B_OK;
		}
	}

	return ENOENT;
}

// btUnlink()
//
int btUnlink(char *pathBuf, bt_fdesc *ddesc, char *name)
{
	bt_fdesc fdesc;
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;
	int error;

	folder = btGetLocalFileName(pathBuf, ddesc);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);

		// Obtain the inode (vnid) of the specified file through stat().
		if (stat(path, &st) != 0)
			return errno;

		// Construct a dummy file descriptor and cause it to be removed from
		// the list.
		fdesc.vnid = st.st_ino;
		btRemoveHandle(&fdesc);

		error = unlink(path);
		return (error == -1 ? errno : B_OK);
	}

	return EACCES;
}

int btReadLink(char *pathBuf, bt_fdesc *fdesc, char *buffer, int length)
{
	char *path;
	int error;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		error = readlink(path, buffer, length);
		if (error == -1)
			return errno;

		return B_OK;
	}

	return ENOENT;
}

int btSymLink(char *pathBuf, bt_fdesc *fdesc, char *name, char *dest)
{
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, fdesc);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		if (symlink(dest, path) == -1)
			return errno;

		return B_OK;
	}

	return ENOENT;
}

int btWStat(char *pathBuf, bt_fdesc *fdesc, long mask, int32 mode, int32 uid, int32 gid, int64 size, int32 atime, int32 mtime)
{
	struct utimbuf ftimes;
	struct stat st;
	char *path;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		if (mask & WSTAT_MODE)
			chmod(path, mode);

		if (mask & WSTAT_UID)
			chown(path, uid, -1);

		if (mask & WSTAT_GID)
			chown(path, -1, gid);

		if (mask & WSTAT_SIZE)
			truncate(path, size);

		if (stat(path, &st) == 0)
			if (mask & WSTAT_ATIME || mask & WSTAT_MTIME)
			{
				ftimes.actime = mask & WSTAT_ATIME ? atime : st.st_atime;
				ftimes.modtime = mask & WSTAT_MTIME ? mtime : st.st_mtime;
				utime(path, &ftimes);
			}

		return B_OK;
	}

	return ENOENT;
}

int btReadAttrib(char *pathBuf, bt_fdesc *fdesc, char *name, int32 dataType, void *buffer, int32 pos, int32 len)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		file = open(path, O_RDONLY);

		if (file)
		{
			int bytes = (int) fs_read_attr(file, name, dataType, pos, buffer, len);
			close(file);

			if (dataType == B_STRING_TYPE && bytes < len && bytes >= 0)
				((char *) buffer)[bytes] = 0;

			return bytes;
		}
	}

	return ENOENT;
}

int btWriteAttrib(char *pathBuf, bt_fdesc *fdesc, char *name, int32 dataType, void *buffer, int32 pos, int32 len)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		file = open(path, O_RDONLY);

		if (file)
		{
			int bytes = (int) fs_write_attr(file, name, dataType, pos, buffer, len);
			close(file);
			return bytes;
		}
	}

	return ENOENT;
}

int btReadAttribDir(char *pathBuf, bt_fdesc *fdesc, DIR **dir, char *attrName)
{
	dirent_t *entry;
	char *path;

	if (!fdesc || !attrName)
		return EINVAL;

	if (!*dir)
	{
		path = btGetLocalFileName(pathBuf, fdesc);
		if (path)
			*dir = fs_open_attr_dir(path);
	}

	if (*dir)
		do
		{
			entry = fs_read_attr_dir(*dir);
			if (entry)
			{
				if (strncmp(entry->d_name, "_trk/", 5) == 0)
					continue;
	
				strcpy(attrName, entry->d_name);
				return B_OK;
			}
		} while (entry);

	if (*dir)
		fs_close_attr_dir(*dir);

	return ENOENT;
}

int btRemoveAttrib(char *pathBuf, bt_fdesc *fdesc, char *name)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		file = open(path, O_RDONLY);

		if (file)
		{
			int error = fs_remove_attr(file, name);
			if (error == -1)
				error = errno;

			close(file);
			return error;
		}
	}

	return ENOENT;
}

int btStatAttrib(char *pathBuf, bt_fdesc *fdesc, char *name, struct attr_info *info)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, fdesc);
	if (path)
	{
		file = open(path, O_RDONLY);

		if (file)
		{
			int error = fs_stat_attr(file, name, info);
			if (error == -1)
				error = errno;

			close(file);
			return error;
		}
	}

	return ENOENT;
}

int btReadIndexDir(DIR **dir, char *indexName)
{
	struct dirent *dirInfo;

	if (!*dir)
	{
		dev_t device = dev_for_path(rootNode->name);
		if (device < 0)
			return device;

		*dir = fs_open_index_dir(device);
	}

	if (*dir)
		if ((dirInfo = fs_read_index_dir(*dir)) != NULL)
		{
			strcpy(indexName, dirInfo->d_name);
			return B_OK;
		}
		else
		{
			fs_close_index_dir(*dir);
			*dir = NULL;
			return ENOENT;
		}

	return ENOENT;
}

int btCreateIndex(char *name, int type, int flags)
{
	dev_t device = dev_for_path(rootNode->name);
	if (device < 0)
		return device;

	if (fs_create_index(device, name, type, flags) == -1)
		return errno;

	return B_OK;
}

int btRemoveIndex(char *name)
{
	dev_t device = dev_for_path(rootNode->name);
	if (device < 0)
		return device;

	if (fs_remove_index(device, name) == -1)
		return errno;

	return B_OK;
}

int btStatIndex(char *name, struct index_info *info)
{
	dev_t device = dev_for_path(rootNode->name);
	if (device < 0)
		return device;

	if (fs_stat_index(device, name, info) == -1)
		return errno;

	return B_OK;
}

int btReadQuery(DIR **dir, char *query, char *fileName, int64 *inode)
{
	struct dirent *dirInfo;

	if (!*dir)
	{
		dev_t device = dev_for_path(rootNode->name);
		if (device < 0)
			return device;

		printf("Querying:  %s\n", query);
		*dir = fs_open_query(device, query, 0);
	}

	if (*dir)
		if ((dirInfo = fs_read_query(*dir)) != NULL)
		{
			*inode = dirInfo->d_ino;
			strcpy(fileName, dirInfo->d_name);
			printf("Query matched %s\n", fileName);
			return B_OK;
		}
		else
		{
			fs_close_query(*dir);
			*dir = NULL;
			return ENOENT;
		}

	return ENOENT;
}

////////////////////////////////////////////////////////////////////

void netbtMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	btFileHandle rootNode;
	int client, error;

	client = session->socket;
	if (argc == 1)
	{
		if (argv[0].type == B_STRING_TYPE)
		{
			char *shareName = argv[0].data;
			error = btMount(shareName, &rootNode);
			if (error == B_OK)
			{
				strcpy(session->share, shareName);

				btRPCCreateAck(&packet, xid, error, BT_FILE_HANDLE_SIZE);
				btRPCPutHandle(&packet, &rootNode);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
	}

//	sendErrorToClient(client, EINVAL);
}

void netbtFSInfo(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
	{
		if (argv[0].type == B_STRING_TYPE)
		{
			fs_info info;

			error = btGetFSInfo(&info);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, error, 12);
				btRPCPutInt32(&packet, info.block_size);
				btRPCPutInt32(&packet, info.total_blocks);
				btRPCPutInt32(&packet, info.free_blocks);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
	}

//	sendErrorToClient(client, EINVAL);
}

void netbtLookup(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
	int client, error;

	client = session->socket;
	if (argc == 2)
	{
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			bt_fdesc fdesc;
			error = btLookup(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, &fdesc);
			if (error == B_OK)
				error = btStat(session->pathBuffer, &fdesc, &st);

			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, BT_FILE_HANDLE_SIZE + 52);
				btRPCPutHandle(&packet, (btFileHandle *) &fdesc);
				btRPCPutStat(&packet, &st);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
	}

//	sendErrorToClient(client, EINVAL);
}

void netbtReadDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
	int client, error;

	client = session->socket;
	if (argc == 2)
	{
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			bt_fdesc fdesc;
			DIR *dir;
			char filename[B_PATH_NAME_LENGTH];
			int entries = 0;

			dir = (DIR *)(*((int32 *) argv[1].data));
			error = btReadDir(session->pathBuffer, (bt_fdesc *) argv[0].data, &dir, &fdesc, filename, &st);

			if (error != B_OK)
			{
				btRPCCreateAck(&packet, xid, error, 0);
				btRPCSendAck(client, &packet);
				return;
			}

			btRPCCreateAck(&packet, xid, B_OK, 0);
			while (error == B_OK)
			{
				btRPCPutInt64(&packet, fdesc.vnid);
				btRPCPutString(&packet, filename, strlen(filename));
				btRPCPutInt32(&packet, (int32) dir);
				btRPCPutHandle(&packet, (btFileHandle *) &fdesc);
				btRPCPutStat(&packet, &st);

				if (++entries >= 32)
					break;

				error = btReadDir(session->pathBuffer, (bt_fdesc *) argv[0].data, &dir, &fdesc, filename, &st);
				btRPCPutInt32(&packet, error);
			}

			// If we exhausted the list of directory entries without filling
			// the buffer, add an error message that will prevent the client
			// from requesting further entries.
			if (entries < 32)
				btRPCPutInt32(&packet, ENOENT);

			btRPCSendAck(client, &packet);
			return;
		}
	}

//	sendErrorToClient(client, EINVAL);
}

void netbtStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_STRING_TYPE)
		{
			struct stat info;

			error = btStat(session->pathBuffer, (bt_fdesc *) argv[0].data, &info);

			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, error, 52);
				btRPCPutStat(&packet, &info);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}

//	sendErrorToClient(client, EINVAL);
}

void netbtRead(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_INT32_TYPE && argv[2].type == B_INT32_TYPE)
		{
			off_t pos = *((off_t *) argv[1].data);
			int32 len = *((int32 *) argv[2].data);
			int32 bytes = 0;

			session->ioBuffer[len] = 0;
			bytes = btRead(session->pathBuffer, (bt_fdesc *) argv[0].data, pos, len, session->ioBuffer);

			btRPCCreateAck(&packet, xid, B_OK, bytes + 4);
			btRPCPutString(&packet, session->ioBuffer, bytes);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtWrite(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	if (argc == 4)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_INT64_TYPE && argv[2].type == B_INT32_TYPE && argv[3].type == B_STRING_TYPE)
		{
			off_t pos = *((off_t *) argv[1].data);
			int32 len = *((int32 *) argv[2].data);
			int32 bytes;

			bytes = btWrite(session->pathBuffer, (bt_fdesc *) argv[0].data, pos, len, argv[3].data);
			if (bytes == len)
			{
				btRPCCreateAck(&packet, xid, B_OK, 4);
				btRPCPutInt32(&packet, bytes);
			}
			else
				btRPCCreateAck(&packet, xid, EACCES, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtCreate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
	int client, error;

	client = session->socket;
	if (argc == 4)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE && argv[3].type == B_INT32_TYPE)
		{
			bt_fdesc fdesc;
			int omode = *((int *) argv[2].data);
			int perms = *((int *) argv[3].data);

			error = btCreate(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, omode, perms, &fdesc);
			if (error == B_OK)
				error = btStat(session->pathBuffer, &fdesc, &st);

			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, 0);
				btRPCPutHandle(&packet, (btFileHandle *) &fdesc);
				btRPCPutStat(&packet, &st);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtTruncate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_INT64_TYPE)
		{
			error = btTruncate(session->pathBuffer, (bt_fdesc *) argv[0].data, *((int64 *) argv[1].data));
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtUnlink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			error = btUnlink(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtRename(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 4)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_STRING_TYPE && argv[3].type == B_STRING_TYPE)
		{
			error = btRename(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, (bt_fdesc *) argv[2].data, argv[3].data);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtCreateDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE)
		{
			bt_fdesc fdesc;
			struct stat st;

			error = btCreateDir(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, *((int *) argv[2].data), &fdesc, &st);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, BT_FILE_HANDLE_SIZE + 52);
				btRPCPutHandle(&packet, (btFileHandle *) &fdesc);
				btRPCPutStat(&packet, &st);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtDeleteDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			error = btDeleteDir(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtReadLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_STRING_TYPE)
		{
			char path[B_PATH_NAME_LENGTH];
			error = btReadLink(session->pathBuffer, (bt_fdesc *) argv[0].data, path, B_PATH_NAME_LENGTH);
			if (error == B_OK)
			{
				int length = strlen(path);
				btRPCCreateAck(&packet, xid, B_OK, length);
				btRPCPutString(&packet, path, length);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
		}
}

void netbtSymLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_STRING_TYPE)
		{
			error = btSymLink(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, argv[2].data);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
		}
}

void netbtWStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 8)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_INT32_TYPE && argv[2].type == B_INT32_TYPE &&
			argv[3].type == B_INT32_TYPE && argv[4].type == B_INT32_TYPE && argv[5].type == B_INT32_TYPE &&
			argv[6].type == B_INT32_TYPE && argv[7].type == B_INT32_TYPE)
		{
			int32 mask = *((int32 *) argv[1].data);
			int32 mode = *((int32 *) argv[2].data);
			int32 uid = *((int32 *) argv[3].data);
			int32 gid = *((int32 *) argv[4].data);
			int64 size = (int64) *((int32 *) argv[5].data);
			int32 atime = *((int32 *) argv[6].data);
			int32 mtime = *((int32 *) argv[7].data);

			error = btWStat(session->pathBuffer, (bt_fdesc *) argv[0].data, mask, mode, uid, gid, size, atime, mtime);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
		}
}

void netbtReadAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesRead;

	client = session->socket;
	if (argc == 5)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE &&
			argv[3].type == B_INT32_TYPE && argv[4].type == B_INT32_TYPE)
		{
			char *buffer;
			int32 type = *((int32 *) argv[2].data);
			int32 pos = *((int32 *) argv[3].data);
			int32 len = *((int32 *) argv[4].data);

			if (len <= BT_MAX_ATTR_BUFFER)
				buffer = session->attrBuffer;
			else
				buffer = (char *) malloc(len + 1);

			if (buffer)
			{
				bytesRead = btReadAttrib(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, type, buffer, pos, len);
				if (bytesRead >= 0)
				{
					btRPCCreateAck(&packet, xid, B_OK, bytesRead + 8);
					btRPCPutInt32(&packet, (int32) bytesRead);
					if (bytesRead > 0)
						btRPCPutString(&packet, buffer, bytesRead);
				}
				else
					btRPCCreateAck(&packet, xid, B_ENTRY_NOT_FOUND, 0);

				if (len > BT_MAX_ATTR_BUFFER)
					free(buffer);
			}
			else
				btRPCCreateAck(&packet, xid, ENOMEM, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtWriteAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesWritten;

	client = session->socket;
	if (argc == 6)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE &&
			argv[3].type == B_STRING_TYPE && argv[4].type == B_INT32_TYPE && argv[5].type == B_INT32_TYPE)
		{
			int32 type = *((int32 *) argv[2].data);
			int32 pos = *((int32 *) argv[4].data);
			int32 len = *((int32 *) argv[5].data);

			bytesWritten = btWriteAttrib(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, type, argv[3].data, pos, len);
			if (bytesWritten >= 0)
			{
				btRPCCreateAck(&packet, xid, B_OK, 4);
				btRPCPutInt32(&packet, (int32) bytesWritten);
			}
			else
				btRPCCreateAck(&packet, xid, B_ENTRY_NOT_FOUND, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtReadAttribDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
	{
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			DIR *dir;
			char attrName[100];
			int entries = 0;

			dir = (DIR *)(*((int32 *) argv[1].data));
			error = btReadAttribDir(session->pathBuffer, (bt_fdesc *) argv[0].data, &dir, attrName);

			if (error != B_OK)
			{
				btRPCCreateAck(&packet, xid, error, 0);
				btRPCSendAck(client, &packet);
				return;
			}

			btRPCCreateAck(&packet, xid, B_OK, 0);
			while (error == B_OK)
			{
				btRPCPutString(&packet, attrName, strlen(attrName));
				btRPCPutInt32(&packet, (int32) dir);

				if (++entries >= 32)
					break;

				error = btReadAttribDir(session->pathBuffer, (bt_fdesc *) argv[0].data, &dir, attrName);
				btRPCPutInt32(&packet, error);
			}

			if (entries < 32)
				btRPCPutInt32(&packet, ENOENT);

			btRPCSendAck(client, &packet);
			return;
		}
	}

//	sendErrorToClient(client, EINVAL);
}

void netbtRemoveAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			error = btRemoveAttrib(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtStatAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			struct attr_info info;

			error = btStatAttrib(session->pathBuffer, (bt_fdesc *) argv[0].data, argv[1].data, &info);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, 12);
				btRPCPutInt32(&packet, info.type);
				btRPCPutInt64(&packet, info.size);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtReadIndexDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_STRING_TYPE)
		{
			DIR *dir;
			char indexName[100];

			dir = (DIR *)(*((int32 *) argv[0].data));

			error = btReadIndexDir(&dir, indexName);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, strlen(indexName) + 8);
				btRPCPutString(&packet, indexName, strlen(indexName));
				btRPCPutInt32(&packet, (int32) dir);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtCreateIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_INT32_TYPE && argv[2].type == B_INT32_TYPE)
		{
			int type = *((int32 *) argv[1].data);
			int flags = *((int32 *) argv[2].data);

			error = btCreateIndex(argv[0].data, type, flags);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtRemoveIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_STRING_TYPE)
		{
			error = btRemoveIndex(argv[0].data);
			btRPCCreateAck(&packet, xid, error, 0);
			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtStatIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_STRING_TYPE)
		{
			struct index_info info;

			error = btStatIndex(argv[0].data, &info);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, 28);
				btRPCPutInt32(&packet, info.type);
				btRPCPutInt64(&packet, info.size);
				btRPCPutInt32(&packet, info.modification_time);
				btRPCPutInt32(&packet, info.creation_time);
				btRPCPutInt32(&packet, info.uid);
				btRPCPutInt32(&packet, info.gid);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

void netbtReadQuery(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE)
		{
			DIR *dir;
			char fileName[B_PATH_NAME_LENGTH];
			int64 inode;

			dir = (DIR *)(*((int32 *) argv[0].data));

			error = btReadQuery(&dir, argv[1].data, fileName, &inode);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK, strlen(fileName) + 16);
				btRPCPutInt64(&packet, inode);
				btRPCPutString(&packet, fileName, strlen(fileName));
				btRPCPutInt32(&packet, (int32) dir);
			}
			else
				btRPCCreateAck(&packet, xid, error, 0);

			btRPCSendAck(client, &packet);
			return;
		}
}

// netbtQuit()
//
void netbtQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	if (argc == 0)
	{
		btRPCCreateAck(&packet, xid, B_OK, 0);
		btRPCSendAck(client, &packet);
	}
}

