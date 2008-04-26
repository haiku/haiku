// btAdd.cpp : Defines the entry point for the console application.
//

// To-do:
//
// X Better reconnect logic
// _ Node monitoring
// _ Don't allow following links outside the shared folder
// _ Restart of server erases inode-to-filename map
// _ Ability to reconnect shares at login
// _ Restricting logon times via a schedule
// _ Distributed file sharing
//

// dividing the files:
// 1.  main(), start/stop service, request thread launching, misc
// 2.  signal handlers
// 3.  UDP command thread/handlers
// 4.  config file parsing
// 5.  RPC code
// 6.  actual file handling commands
// 7.  net file handling commands

// Potential Uses:
// 1.  Domain server, keeping track of users for logging in
// 2.  File server, to share BeOS volume files across a network
// 3.  Document management, to store documents with attributes
// 4.  Version Control System, to manage multiple versions of documents
// 5.  General directory server, for local or network settings and information

#include "FindDirectory.h"

#include "betalk.h"
#include "sessions.h"
#include "rpc_handlers.h"
#include "rpc_workers.h"
#include "file_shares.h"
#include "readerWriter.h"

#include "sysdepdefs.h"
#include "fsproto.h"
#include "netdb.h"

#include "ctype.h"
#include "time.h"
#include "signal.h"
#include "stdlib.h"
#include "syslog.h"
#include "sys/utsname.h"

#define BT_MAX_THREADS		100
#define BT_MAX_RETRIES		3

#define BT_MAIN_NAME		"BeServed Daemon"
#define BT_THREAD_NAME		"BeServed Handler"
#define BT_HOST_THREAD_NAME	"BeServed Host Publisher"
#define BT_SIGNATURE		"application/x-vnd.Teldar-BeServed"

#define PATH_ROOT			"/boot"
#define PATH_DELIMITER		'/'

#ifndef iswhite
#define iswhite(c)			((c == ' ' || c == '\t'))
#endif


int main(int argc, char *argv[]);
void daemonInit();
bool dateCheck();
int32 btSendHost(void *data);
int getSharedResources(char *buffer, int bufSize);
void getHostInfo(bt_hostinfo *info);
int getHostUsers(char *buffer);
void startService();
void endService(int sig);
void restartService();
void initShares();
void freeFileHandles();
void freeFileShares();
void getFileShare(const char *buffer);
void getShareProperty(const char *buffer);
void getGrant(const char *buffer);
void getAuthenticate(const char *buffer);
bool getAuthServerAddress(const char *name);
void addUserRights(char *share, char *user, int rights, bool isGroup);
int getToken();
int receiveRequest(bt_session_t *session);
void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[]);
void launchThread(int client, struct sockaddr_in *addr);
int tooManyConnections(unsigned int s_addr);
void sendErrorToClient(int client, unsigned int xid, int error);
void getArguments(bt_session_t *session, bt_inPacket *packet, unsigned char command);
int32 requestThread(void *data);


bt_node *rootNode = NULL;
bt_session_t *rootSession = NULL;
bt_fileShare_t fileShares[BT_MAX_FILE_SHARES];
char tokBuffer[B_PATH_NAME_LENGTH], *tokPtr;
bool running = true;
int server;
char authServerName[B_FILE_NAME_LENGTH];
unsigned int authServerIP;
thread_id hostThread;
bt_managed_data sessionData;
bt_managed_data handleData;

bt_command_t dirCommands[] =
{
	{ BT_CMD_PREMOUNT,			netbtPreMount,				true,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_MOUNT,				netbtMount,					true,	3,	{ B_STRING_TYPE, B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_FSINFO,			netbtFSInfo,				true,	1,	{ B_INT64_TYPE } },
	{ BT_CMD_LOOKUP,			netbtLookup,				true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_STAT,				netbtStat,					true,	1,	{ B_INT64_TYPE } },
	{ BT_CMD_READDIR,			netbtReadDir,				true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_READ,				netbtRead,					true,	3,	{ B_INT64_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_WRITE,				netbtWrite,					true,	5,	{ B_INT64_TYPE, B_INT64_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_STRING_TYPE } },
	{ BT_CMD_CREATE,			netbtCreate,				true,	4,	{ B_INT64_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_TRUNCATE,			netbtTruncate,				true,	2,	{ B_INT64_TYPE, B_INT64_TYPE } },
	{ BT_CMD_MKDIR,				netbtCreateDir, 			true,	3,	{ B_INT64_TYPE, B_STRING_TYPE, B_INT32_TYPE } },
	{ BT_CMD_RMDIR,				netbtDeleteDir,				true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_RENAME,			netbtRename,				true,	4,	{ B_INT64_TYPE, B_STRING_TYPE, B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_UNLINK,			netbtUnlink,				true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_READLINK,			netbtReadLink,				true,	1,	{ B_INT64_TYPE } },
	{ BT_CMD_SYMLINK,			netbtSymLink,				true,	3,	{ B_INT64_TYPE, B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_WSTAT,				netbtWStat,					true,	8,	{ B_INT64_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_READATTRIB,		netbtReadAttrib,			true,	5,	{ B_INT64_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_WRITEATTRIB,		netbtWriteAttrib,			true,	6,	{ B_INT64_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_READATTRIBDIR,		netbtReadAttribDir,			true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },		
	{ BT_CMD_REMOVEATTRIB,		netbtRemoveAttrib,			true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_STATATTRIB,		netbtStatAttrib,			true,	2,	{ B_INT64_TYPE, B_STRING_TYPE } },
	{ BT_CMD_READINDEXDIR,		netbtReadIndexDir,			true,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_CREATEINDEX,		netbtCreateIndex,			true,	3,	{ B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_REMOVEINDEX,		netbtRemoveIndex,			true,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_STATINDEX,			netbtStatIndex,				true,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_READQUERY,			netbtReadQuery,				true,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_COMMIT,			netbtCommit,				true,	1,	{ B_INT64_TYPE } },
	{ BT_CMD_PRINTJOB_NEW,		netbtPrintJobNew,			false,	0,	{ 0 } },
	{ BT_CMD_PRINTJOB_DATA,		netbtPrintJobData,			false,	0,	{ 0 } },
	{ BT_CMD_PRINTJOB_COMMIT,	netbtPrintJobCommit,		false,	0,	{ 0 } },
	{ BT_CMD_AUTHENTICATE,		netbtAuthenticate,			false,	0,	{ 0 } },
	{ BT_CMD_QUIT,				netbtQuit,					true,	0,	{ 0 } },
	{ 0,						NULL,						false,	0,	{ 0 } }
};

char *keywords[] =
{
	"share",
	"as",
	"set",
	"read",
	"write",
	"read-write",
	"promiscuous",
	"on",
	"to",
	"authenticate",
	"with",
	"group",
	"printer",
	"print",
	"is",
	"spooled",
	"device",
	"type",
	NULL
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
	daemonInit();

	initShares();
	signal(SIGINT, endService);
	signal(SIGTERM, endService);
	signal(SIGHUP, restartService);
	signal(SIGPIPE, SIG_IGN);

	if (initManagedData(&handleData))
	{
		if (initManagedData(&sessionData))
		{
			hostThread = spawn_thread(btSendHost, BT_HOST_THREAD_NAME, B_NORMAL_PRIORITY, 0);
			resume_thread(hostThread);

			// Run the daemon.  We will not return until the service is being stopped.
			startService();

			if (hostThread > 0)
				kill_thread(hostThread);

			closeManagedData(&sessionData);
		}

		closeManagedData(&handleData);
	}

	return 0;
}

bool dateCheck()
{
	struct stat st;
	time_t curTime;

	time(&curTime);
	if (curTime > 1012537700)
		return false;

	if (stat("/boot/home/config/servers/beserved_server", &st) == 0)
		if (curTime < st.st_ctime || curTime > st.st_ctime + 7776000)
			return false;

	return true;
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
	openlog("beserved_server", LOG_PID, LOG_DAEMON);
}

void restartService()
{
	bt_fileShare_t *oldShares;
	int i;

	// Delay all mounting and other file system operations.
	beginWriting(&handleData);
	beginWriting(&sessionData);

	// Copy existing share data.
	oldShares = (bt_fileShare_t *) malloc(sizeof(bt_fileShare_t) * BT_MAX_FILE_SHARES);
	if (oldShares)
	{
		for (i = 0; i < BT_MAX_FILE_SHARES; i++)
			memcpy(&oldShares[i], &fileShares[i], sizeof(bt_fileShare_t));

		// Reload the share data.
		initShares();

		// Now loop through the old file shares.  For each one, check if the same
		// path exists in the new shares.
		for (i = 0; i < BT_MAX_FILE_SHARES; i++)
			if (oldShares[i].used)
			{
				bt_session_t *s;
				int share = btGetShareIdByPath(oldShares[i].path);
				if (share == -1)
				{
					for (s = rootSession; s; s = s->next)
						if (s->share == i)
							s->killed = true;
				}
				else if (share != i)
				{
					for (s = rootSession; s; s = s->next)
						if (s->share == i)
							s->share = share;
				}
			}

		free(oldShares);
	}

	// Resume normal operation.
	endWriting(&sessionData);
	endWriting(&handleData);
}

int32 btSendHost(void *data)
{
	bt_request request;
	bt_hostinfo info;
	struct sockaddr_in serverAddr, clientAddr;
	char buffer[4096];
	int server, addrLen, bufLen, replyLen;

	buffer[0] = 0;
	bufLen = sizeof(buffer);

	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server == INVALID_SOCKET)
		return -1;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(BT_QUERYHOST_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind that socket to the address constructed above.
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return -1;

	while (running)
	{
		addrLen = sizeof(struct sockaddr_in);
		replyLen = 0;
		if (recvfrom(server, (char *) &request, sizeof(request), 0, (struct sockaddr *) &clientAddr, &addrLen) <= 0)
			continue;

		switch (request.command)
		{
			case BT_REQ_HOST_PROBE:
				gethostname(buffer, bufLen);
				break;

			case BT_REQ_SHARE_PROBE:
				replyLen = getSharedResources(buffer, sizeof(buffer));
				break;

			case BT_REQ_HOST_INFO:
				getHostInfo(&info);
				memcpy(buffer, &info, sizeof(bt_hostinfo));
				replyLen = sizeof(bt_hostinfo);
				break;

			case BT_REQ_HOST_USERS:
				replyLen = getHostUsers(buffer);
				break;

			case BT_REQ_AUTH_TYPES:
				break;
		}

		// If no reply length has been specified, calculate it now by taking the
		// length of the buffer.
		if (replyLen == 0)
			replyLen = strlen(buffer);

		sendto(server, buffer, replyLen, 0, (struct sockaddr *) &clientAddr, addrLen);
	}

	// Close the socket.  Technically, I believe we should call shutdown()
	// first, but the BeOS header file socket.h indicates that this
	// function is not currently working.  It is present but may not have
	// any effect.
	shutdown(server, 2);
	closesocket(server);
	return 0;
}

// getSharedResources()
//
int getSharedResources(char *buffer, int bufSize)
{
	bt_resource resource;
	int i, bufPos = 0;

	// If the supplied buffer can't hold at least two resource structures, one
	// for a shared resource and one to terminate the list, then don't bother
	// building the list.
	if (bufSize < 2 * sizeof(bt_resource))
		return 0;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
		{
			// If this is the last resource structure that will fit in the
			// buffer, then don't add any more into the list.
			if (bufPos + sizeof(bt_resource) >= bufSize)
				break;

			// Fill out the resource structure.
			resource.type = B_HOST_TO_LENDIAN_INT32(BT_SHARED_FOLDER);
			strcpy(resource.name, fileShares[i].name);

			// Copy the resource structure into the buffer at the current offset.
			memcpy(buffer + bufPos, &resource, sizeof(bt_resource));
			bufPos += sizeof(bt_resource);
		}

	// Copy the null terminating structure.
	resource.type = B_HOST_TO_LENDIAN_INT32(BT_SHARED_NULL);
	resource.name[0] = 0;
	memcpy(buffer + bufPos, &resource, sizeof(bt_resource));
	bufPos += sizeof(bt_resource);

	return bufPos;
}

// getHostInfo()
//
void getHostInfo(bt_hostinfo *info)
{
	system_info sysinfo; 
	bt_session_t *s;
	struct utsname uts;
	char buf[100];
	int i;

	struct cpuMap
	{
		int cpuType;
		char *cpuName;
	} cpuList[] =
	{
		{ B_CPU_PPC_601, "PowerPC 601" },
		{ B_CPU_PPC_603, "PowerPC 603" },
		{ B_CPU_PPC_603e, "PowerPC 603e" },
		{ B_CPU_PPC_604, "PowerPC 604" },
		{ B_CPU_PPC_604e, "PowerPC 604e" },
		{ B_CPU_PPC_750, "PowerPC 750" },
		{ B_CPU_PPC_686, "PowerPC 686" },

		{ B_CPU_INTEL_X86, "Intel 80x86" },
		{ B_CPU_INTEL_PENTIUM, "Intel Pentium" },
		{ B_CPU_INTEL_PENTIUM75, "Intel Pentium" },
		{ B_CPU_INTEL_PENTIUM_486_OVERDRIVE, "Intel 486 Overdrive" },
		{ B_CPU_INTEL_PENTIUM_MMX, "Intel Pentium MMX" },
		{ B_CPU_INTEL_PENTIUM_MMX_MODEL_4, "Intel Pentium MMX" },
		{ B_CPU_INTEL_PENTIUM_MMX_MODEL_8, "Intel Pentium MMX" },
		{ B_CPU_INTEL_PENTIUM75_486_OVERDRIVE, "Intel 486 Overdrive" },
		{ B_CPU_INTEL_PENTIUM_PRO, "Intel Pentium Pro" },
		{ B_CPU_INTEL_PENTIUM_II, "Intel Pentium II" },
		{ B_CPU_INTEL_PENTIUM_II_MODEL_3, "Intel Pentium II" },
		{ B_CPU_INTEL_PENTIUM_II_MODEL_5, "Intel Pentium II" },
		{ B_CPU_INTEL_CELERON, "Intel Celeron" },
		{ B_CPU_INTEL_PENTIUM_III, "Intel Pentium III" },

		{ B_CPU_AMD_X86, "AMD x86" },
		{ B_CPU_AMD_K5_MODEL0, "AMD K5" },
		{ B_CPU_AMD_K5_MODEL1, "AMD K5" },
		{ B_CPU_AMD_K5_MODEL2, "AMD K5" },
		{ B_CPU_AMD_K5_MODEL3, "AMD K5" },
		{ B_CPU_AMD_K6_MODEL6, "AMD K6" },
		{ B_CPU_AMD_K6_MODEL7, "AMD K6" },
		{ B_CPU_AMD_K6_MODEL8, "AMD K6" },
		{ B_CPU_AMD_K6_2, "AMD K6-2" },
		{ B_CPU_AMD_K6_MODEL9, "AMD K6" },
		{ B_CPU_AMD_K6_III, "AMD K6-3" },
		{ B_CPU_AMD_ATHLON_MODEL1, "AMD Athlon" },

		{ B_CPU_CYRIX_X86, "Cyrix x86" },
		{ B_CPU_CYRIX_GXm, "Cyrix GXm" },
		{ B_CPU_CYRIX_6x86MX, "Cyrix 6x86MX" },

		{ B_CPU_IDT_X86, "IDT x86" },
		{ B_CPU_IDT_WINCHIP_C6, "IDT WinChip C6" },
		{ B_CPU_IDT_WINCHIP_2, "IDT WinChip 2" },

		{ B_CPU_RISE_X86, "Rise x86" },
		{ B_CPU_RISE_mP6, "Rise mP6" },

		{ 0, NULL }
	};

	uname(&uts);
	get_system_info(&sysinfo);

	strcpy(info->system, uts.sysname);
	strcat(info->system, " ");
	strcat(info->system, uts.release);
	strcpy(info->beServed, "BeServed 1.2.6");

	info->cpus = B_HOST_TO_LENDIAN_INT32(sysinfo.cpu_count);
	info->maxConnections = B_HOST_TO_LENDIAN_INT32(BT_MAX_THREADS);

	strcpy(info->platform, "Unknown");
	for (i = 0; cpuList[i].cpuType; i++)
		if (cpuList[i].cpuType == sysinfo.cpu_type)
		{
			strcpy(info->platform, cpuList[i].cpuName);
			break;
		}

	sprintf(buf, " at %ldMHz", (long) (sysinfo.cpu_clock_speed / 1000000));
	strcat(info->platform, buf);

	// Delay all new session creation.
	beginReading(&sessionData);

	info->connections = 0;
	for (s = rootSession; s; s = s->next)
		if (s->socket != INVALID_SOCKET)
			info->connections++;

	info->connections = B_HOST_TO_LENDIAN_INT32(info->connections);
	endReading(&sessionData);
}

// getHostUsers()
//
int getHostUsers(char *buffer)
{
	bt_session_t *s;
	char addr[20];
	int len, bufSize;

	// Initialize the buffer to be empty.
	buffer[0] = 0;
	bufSize = 0;

	// Delay all new session creation.
	beginReading(&sessionData);

	for (s = rootSession; s; s = s->next)
		if (s->socket != INVALID_SOCKET)
		{
			uint8 *s_addr = (uint8 *) s->client_s_addr;
			sprintf(addr, "%d.%d.%d.%d", s_addr[0], s_addr[1], s_addr[2], s_addr[3]);
			len = strlen(buffer);
			strcpy(&buffer[len > 0 ? len + 1 : 0], addr);
			bufSize += len + 1;
		}

	endReading(&sessionData);

	buffer[bufSize++] = 0;
	return bufSize;
}

void initShares()
{
	FILE *fp;
	char path[B_PATH_NAME_LENGTH], buffer[512];
	int i, length;

	authServerIP = 0;
	authServerName[0] = 0;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
	{
		fileShares[i].name[0] = 0;
		fileShares[i].path[0] = 0;
		fileShares[i].used = false;
		fileShares[i].readOnly = true;
		fileShares[i].security = BT_AUTH_NONE;
		fileShares[i].rights = NULL;
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
			else if (strncmp(buffer, "set ", 4) == 0)
				getShareProperty(buffer);
			else if (strncmp(buffer, "grant ", 6) == 0)
				getGrant(buffer);
			else if (strncmp(buffer, "authenticate ", 13) == 0)
				getAuthenticate(buffer);
		}

		fclose(fp);
	}
}

void getFileShare(const char *buffer)
{
	struct stat st;
	char path[B_PATH_NAME_LENGTH], share[MAX_NAME_LENGTH + 1], *folder;
	int i, tok;

	// Skip over SHARE command.
	tokPtr = (char *) buffer + (6 * sizeof(char));

	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(path, tokBuffer);
	tok = getToken();
	if (tok != BT_TOKEN_AS)
		return;

	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(share, tokBuffer);

	// Now verify that the share name specified has not already been
	// used to share another path.
	folder = btGetSharePath(share);
	if (folder)
	{
		syslog(LOG_WARNING, "%s already defined as %s\n", share, folder);
		return;
	}

	// Check the path to ensure its validity.
	if (stat(path, &st) != 0)
		return;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (!fileShares[i].used)
		{
			syslog(LOG_INFO, "Defining %s as %s\n", share, path);
			strcpy(fileShares[i].name, share);
			strcpy(fileShares[i].path, path);
			fileShares[i].used = true;
			return;
		}

	syslog(LOG_WARNING, "Share %s could not be defined (too many shares)\n", share);
}

void getShareProperty(const char *buffer)
{
	char share[B_FILE_NAME_LENGTH + 1];
	int tok, shareId;

	// Skip over SET command.
	tokPtr = (char *) buffer + (4 * sizeof(char));

	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(share, tokBuffer);

	// Get the index of the share referred to.  If the named share cannot be
	// found, then abort.
	shareId = btGetShareId(share);
	if (shareId < 0)
		return;

	tok = getToken();
	if (tok == BT_TOKEN_READWRITE)
	{
		fileShares[shareId].readOnly = false;
		syslog(LOG_INFO, "%s permits writing\n", share);
	}
}

void getGrant(const char *buffer)
{
	char share[MAX_NAME_LENGTH + 1];
	int tok, rights;
	bool isGroup = false;

	// Skip over GRANT command.
	tokPtr = (char *) buffer + (6 * sizeof(char));
	rights = 0;

	do
	{
		tok = getToken();
		if (tok == BT_TOKEN_READ)
		{
			rights |= BT_RIGHTS_READ;
			tok = getToken();
		}
		else if (tok == BT_TOKEN_WRITE)
		{
			rights |= BT_RIGHTS_WRITE;
			tok = getToken();
		}
	} while (tok == BT_TOKEN_COMMA);

	if (tok != BT_TOKEN_ON)
		return;

	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(share, tokBuffer);
	tok = getToken();
	if (tok != BT_TOKEN_TO)
		return;

	tok = getToken();
	if (tok == BT_TOKEN_GROUP)
	{
		isGroup = true;
		tok = getToken();
	}

	if (tok != BT_TOKEN_STRING)
		return;

	addUserRights(share, tokBuffer, rights, isGroup);
}

void getAuthenticate(const char *buffer)
{
	struct hostent *ent;
	int i, tok;

	// Skip over AUTHENTICATE command.
	tokPtr = (char *) buffer + (13 * sizeof(char));

	tok = getToken();
	if (tok != BT_TOKEN_WITH)
		return;

	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	// Look up address for given host.
	getAuthServerAddress(tokBuffer);

	// Make all file shares use BeSure authentication.
	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		fileShares[i].security = BT_AUTH_BESURE;

	syslog(LOG_INFO, "Using authentication server at %s\n", tokBuffer);
}

bool getAuthServerAddress(const char *name)
{
	// Look up address for given host.
	struct hostent *ent = gethostbyname(name);
	if (ent == NULL)
	{
		syslog(LOG_ERR, "Authentication server %s is unavailable.\n", name);
		return false;
	}

	strcpy(authServerName, name);
	authServerIP = ntohl(*((unsigned int *) ent->h_addr));
	return true;
}

void addUserRights(char *share, char *user, int rights, bool isGroup)
{
	bt_user_rights *ur;
	int shareId;

	shareId = btGetShareId(share);
	if (shareId < 0)
		return;

	ur = (bt_user_rights *) malloc(sizeof(bt_user_rights));
	if (ur)
	{
		ur->user = (char *) malloc(strlen(user) + 1);
		if (ur->user)
		{
			strcpy(ur->user, user);
			ur->rights = rights;
			ur->isGroup = isGroup;
			ur->next = fileShares[shareId].rights;
			fileShares[shareId].rights = ur;
		}
		else
			free(ur);
	}
}

int getToken()
{
	bool quoted = false;

	tokBuffer[0] = 0;
	while (*tokPtr && iswhite(*tokPtr))
		tokPtr++;

	if (*tokPtr == ',')
	{
		tokPtr++;
		return BT_TOKEN_COMMA;
	}
	else if (*tokPtr == '\"')
	{
		quoted = true;
		tokPtr++;
	}

	if (isalnum(*tokPtr) || *tokPtr == '/')
	{
		int i = 0;
		while (isalnum(*tokPtr) || isValid(*tokPtr) || (quoted && *tokPtr == ' '))
			if (i < B_PATH_NAME_LENGTH)
				tokBuffer[i++] = *tokPtr++;
			else
				tokPtr++;

		tokBuffer[i] = 0;

		if (!quoted)
			for (i = 0; keywords[i]; i++)
				if (strcasecmp(tokBuffer, keywords[i]) == 0)
					return ++i;

		if (quoted)
			if (*tokPtr != '\"')
				return BT_TOKEN_ERROR;
			else
				tokPtr++;

		return BT_TOKEN_STRING;
	}

	return BT_TOKEN_ERROR;
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
	// Close the syslog.
	closelog();

	if (hostThread > 0)
		kill_thread(hostThread);

	closeManagedData(&sessionData);
	closeManagedData(&handleData);

	freeFileHandles();
	freeFileShares();

	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	exit(0);
}

// freeFileHandles()
//
void freeFileHandles()
{
	bt_node *nextNode, *curNode = rootNode;

	while (curNode)
	{
		nextNode = curNode->next;
		free(curNode);
		curNode = nextNode;
	}
}

// freeFileShares()
//
void freeFileShares()
{
	bt_user_rights *ur, *nextUr;
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		for (ur = fileShares[i].rights; ur; )
		{
			nextUr = ur->next;
			if (ur->user)
				free(ur->user);

			free(ur);
			ur = nextUr;
		}
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
	beginWriting(&sessionData);

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
			session->rootBlock = NULL;
			session->killed = false;
			session->rights = 0;

			session->handlerID =
				spawn_thread(requestThread, BT_THREAD_NAME, B_NORMAL_PRIORITY, session);
			resume_thread(session->handlerID);
	
			// Add this to the session list.
			session->next = rootSession;
			rootSession = session;
			endWriting(&sessionData);
			return;
		}
	}

	endWriting(&sessionData);

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

	// Ensure that this connection remains alive.  If a periodic message (handled by the OS)
	// fails, then blocked socket calls are interrupted and return with a ESIGPIPE error.
//	flags = 1;
//	setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));

	if ((session->blockSem = create_sem(0, "Gathered Write Semaphore")) > B_OK)
	{
		while (!session->killed && receiveRequest(session));
		delete_sem(session->blockSem);
	}

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
	return (command != BT_CMD_QUIT && command != BT_CMD_PREMOUNT);
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
			*(int32 *)(&packet->buffer[9]) = packet->length - 13;
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

////////////////////////////////////////////////////////////////////
