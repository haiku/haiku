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
// _ Use a gathered write semaphore per thread, not one global semaphore
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
#include "authentication.h"
#include "sysdepdefs.h"
#include "fsproto.h"
#include "netdb.h"

#include "utime.h"
#include "ctype.h"
#include "time.h"
#include "signal.h"
#include "stdlib.h"
#include "syslog.h"
#include "sys/utsname.h"

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

typedef struct btblock
{
	vnode_id vnid;
	off_t pos;
	int32 len;
	int32 count;
	char *buffer;
	struct btblock *next;
	struct btblock *prev;
} bt_block;

typedef struct userRights
{
	char *user;
	int rights;
	bool isGroup;
	struct userRights *next;
} bt_user_rights;

typedef struct session
{
	int socket;
	unsigned int s_addr;
	thread_id handlerID;

	bool killed;

	// What share did the client connect to?  And when?
	int share;
	int rights;
	time_t logon;

	// Buffered write support.
	bt_block *rootBlock;
	sem_id blockSem;
	int32 blockVar;

	char ioBuffer[BT_MAX_IO_BUFFER + 9];
	char attrBuffer[BT_MAX_ATTR_BUFFER + 1];
	char pathBuffer[B_PATH_NAME_LENGTH + 1];
} bt_session_t;

typedef void (*bt_net_func)(bt_session_t *, unsigned int, int, bt_arg_t *);

typedef struct dirCommand
{
	unsigned char command;
	bt_net_func handler;
} bt_command_t;

typedef struct btnode
{
	vnode_id vnid;
	char name[B_FILE_NAME_LENGTH];
	int refCount;
	bool invalid;
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

	// What rights does each user have?
	bt_user_rights *rights;
	int security;

	struct fileShare *next;
} bt_fileShare_t;

bt_node *rootNode = NULL;


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
void initSessions();
void initShares();
void freeFileHandles();
void freeFileShares();
void getFileShare(const char *buffer);
void getShareProperty(const char *buffer);
void getGrant(const char *buffer);
void getAuthenticate(const char *buffer);
void addUserRights(char *share, char *user, int rights, bool isGroup);
int getToken();
int receiveRequest(bt_session_t *session);
void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[]);
void launchThread(int client, struct sockaddr_in *addr);
int tooManyConnections(unsigned int s_addr);
void sendErrorToClient(int client, unsigned int xid, int error);
void getArguments(bt_session_t *session, bt_inPacket *packet, unsigned char command);
int32 requestThread(void *data);

bt_node *btGetNodeFromVnid(vnode_id vnid);
void btAddHandle(vnode_id dir_vnid, vnode_id file_vnid, char *name);
void btRemoveHandle(vnode_id vnid);
void btPurgeNodes(vnode_id vnid);
void btRemoveNode(bt_node *deadNode);
bool btIsAncestorNode(vnode_id vnid, bt_node *node);
char *btGetLocalFileName(char *path, vnode_id vnid);
bt_node *btFindNode(bt_node *parent, char *fileName);
char *btGetSharePath(char *shareName);
int btGetShareId(char *shareName);
int btGetShareIdByPath(char *path);
void btGetRootPath(vnode_id vnid, char *path);
void btLock(sem_id semaphore, int32 *atomic);
void btUnlock(sem_id semaphore, int32 *atomic);

int btPreMount(bt_session_t *session, char *shareName);
int btMount(bt_session_t *session, char *shareName, char *user, char *password, vnode_id *vnid);
int btGetFSInfo(char *rootPath, fs_info *fsInfo);
int btLookup(char *pathBuf, vnode_id dir_vnid, char *fileName, vnode_id *file_vnid);
int btStat(char *pathBuf, vnode_id vnid, struct stat *st);
int btReadDir(char *pathBuf, vnode_id dir_vnid, DIR **dir, vnode_id *file_vnid, char *filename, struct stat *st);
int32 btRead(char *pathBuf, vnode_id vnid, off_t pos, int32 len, char *buffer);
int32 btWrite(bt_session_t *session, vnode_id vnid, off_t pos, int32 len, int32 totalLen, char *buffer);
int btCreate(char *pathBuf, vnode_id dir_vnid, char *name, int omode, int perms, vnode_id *file_vnid);
int btTruncate(char *pathBuf, vnode_id vnid, int64 len);
int btCreateDir(char *pathBuf, vnode_id dir_vnid, char *name, int perms, vnode_id *file_vnid, struct stat *st);
int btDeleteDir(char *pathBuf, vnode_id vnid, char *name);
int btRename(char *pathBuf, vnode_id old_vnid, char *oldName, vnode_id new_vnid, char *newName);
int btUnlink(char *pathBuf, vnode_id vnid, char *name);
int btReadLink(char *pathBuf, vnode_id vnid, char *buffer, int length);
int btSymLink(char *pathBuf, vnode_id vnid, char *name, char *dest);
int btWStat(char *pathBuf, vnode_id vnid, long mask, int32 mode, int32 uid, int32 gid, int64 size, int32 atime, int32 mtime);
int btReadAttrib(char *pathBuf, vnode_id vnid, char *name, int32 dataType, void *buffer, int32 pos, int32 len);
int btWriteAttrib(char *pathBuf, vnode_id vnid, char *name, int32 dataType, void *buffer, int32 pos, int32 len);
int btReadAttribDir(char *pathBuf, vnode_id vnid, DIR **dir, char *attrName);
int btRemoveAttrib(char *pathBuf, vnode_id vnid, char *name);
int btStatAttrib(char *pathBuf, vnode_id vnid, char *name, struct attr_info *info);
int btReadIndexDir(char *rootPath, DIR **dir, char *indexName);
int btCreateIndex(char *rootPath, char *name, int type, int flags);
int btRemoveIndex(char *rootPath, char *name);
int btStatIndex(char *rootPath, char *name, struct index_info *info);
int btReadQuery(char *rootPath, DIR **dir, char *query, char *fileName, vnode_id *vnid, vnode_id *parent);
int btCommit(bt_session_t *session, vnode_id vnid);

bt_block *btGetWriteBlock(bt_session_t *session, vnode_id vnid);
void btInsertWriteBlock(bt_session_t *session, bt_block *block);
void btRemoveWriteBlock(bt_session_t *session, bt_block *block);

void netbtPreMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
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
void netbtCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtPrintJobNew(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtPrintJobData(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtPrintJobCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtAuthenticate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);
void netbtQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[]);

bt_session_t sessions[BT_MAX_THREADS];
bt_fileShare_t fileShares[BT_MAX_FILE_SHARES];
char tokBuffer[B_PATH_NAME_LENGTH], *tokPtr;
bool running = true;
int server;
unsigned int authServerIP;
thread_id hostThread;
sem_id handleSem;
int32 handleVar;

bt_command_t dirCommands[] =
{
	{ BT_CMD_PREMOUNT,			netbtPreMount },
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
	{ BT_CMD_COMMIT,			netbtCommit },
	{ BT_CMD_PRINTJOB_NEW,		netbtPrintJobNew },
	{ BT_CMD_PRINTJOB_DATA,		netbtPrintJobData },
	{ BT_CMD_PRINTJOB_COMMIT,	netbtPrintJobCommit },
	{ BT_CMD_AUTHENTICATE,		netbtAuthenticate },
	{ BT_CMD_QUIT,				netbtQuit },
	{ 0,						NULL }
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

	initSessions();
	initShares();
	signal(SIGINT, endService);
	signal(SIGTERM, endService);
	signal(SIGHUP, restartService);
	signal(SIGPIPE, SIG_IGN);

	if ((handleSem = create_sem(0, "File Handle Semaphore")) > B_OK)
	{
		hostThread = spawn_thread(btSendHost, BT_HOST_THREAD_NAME, B_NORMAL_PRIORITY, 0);
		resume_thread(hostThread);
	
		// Run the daemon.  We will not return until the service is being stopped.
		startService();
	
		if (hostThread > 0)
			kill_thread(hostThread);

		delete_sem(handleSem);
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
	btLock(handleSem, &handleVar);

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
				int share = btGetShareIdByPath(oldShares[i].path);
				if (share == -1)
				{
					for (i = 0; i < BT_MAX_THREADS; i++)
						if (sessions[i].share == i)
							sessions[i].killed = true;
				}
				else if (share != i)
					for (i = 0; i < BT_MAX_THREADS; i++)
						if (sessions[i].share == i)
							sessions[i].share = share;
			}

		free(oldShares);
	}

	// Resume normal operation.
	btUnlock(handleSem, &handleVar);
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
	strcpy(info->beServed, "BeServed 1.2.5");

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

	info->connections = 0;
	for (i = 0; i < BT_MAX_THREADS; i++)
		if (sessions[i].socket != INVALID_SOCKET)
			info->connections++;

	info->connections = B_HOST_TO_LENDIAN_INT32(info->connections);
}

// getHostUsers()
//
int getHostUsers(char *buffer)
{
	char addr[20];
	int i, len, bufSize;

	// Initialize the buffer to be empty.
	buffer[0] = 0;
	bufSize = 0;

	for (i = 0; i < BT_MAX_THREADS; i++)
		if (sessions[i].socket != INVALID_SOCKET)
		{
			uint8 *s_addr = (uint8 *) sessions[i].s_addr;
			sprintf(addr, "%d.%d.%d.%d", s_addr[0], s_addr[1], s_addr[2], s_addr[3]);
			len = strlen(buffer);
			strcpy(&buffer[len > 0 ? len + 1 : 0], addr);
			bufSize += len + 1;
		}

	buffer[bufSize++] = 0;
	return bufSize;
}

// initSessions()
//
void initSessions()
{
	int i;

	for (i = 0; i < BT_MAX_THREADS; i++)
	{
		sessions[i].socket = INVALID_SOCKET;
		sessions[i].handlerID = 0;
		sessions[i].killed = false;
		sessions[i].rights = 0;
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
	ent = gethostbyname(tokBuffer);
	if (ent == NULL)
	{
		syslog(LOG_ERR, "Authentication server %s is unavailable.\n", tokBuffer);
		endService(0);
	}

	authServerIP = ntohl(*((unsigned int *) ent->h_addr));

	// Make all file shares use BeSure authentication.
	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		fileShares[i].security = BT_AUTH_BESURE;

	syslog(LOG_INFO, "Using authentication server at %s\n", tokBuffer);
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

	if (handleSem > 0)
		delete_sem(handleSem);

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
		sendErrorToClient(client, 0, EBUSY);
		shutdown(client, 2);
		closesocket(client);
	}
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
		session->rootBlock = NULL;
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
	int i;

	for (i = 0; dirCommands[i].handler; i++)
		if (command == dirCommands[i].command)
		{
			(*dirCommands[i].handler)(session, xid, argc, argv);
			return;
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

bt_node *btGetNodeFromVnid(vnode_id vnid)
{
	register bt_node *curNode = rootNode;

	while (curNode && curNode->vnid != vnid)
		curNode = curNode->next;

	return curNode;
}

// btAddHandle()
//
void btAddHandle(vnode_id dir_vnid, vnode_id file_vnid, char *name)
{
	bt_node *curNode, *dirNode;

	// We don't store the references to the current and the parent directory.
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return;

	btLock(handleSem, &handleVar);

	// Obtain the parent node.  If no parent vnid is provided, then this must be
	// the root node.  The parent of the root node is NULL, but it should be the
	// only node for which this is true.
	if (dir_vnid)
		dirNode = btGetNodeFromVnid(dir_vnid);
	else
		dirNode = NULL;

	// If a node already exists with the given vnid, then it is either a symbolic
	// link or an attempt to add the same node again, such as when mounting or
	// walking a directory tree.  If we find a matching vnid whose parent directory
	// and name also match, this is a duplicate and can be ignored.
	curNode = btGetNodeFromVnid(file_vnid);
	if (curNode)
		if (curNode->parent == dirNode && strcmp(curNode->name, name) == 0)
		{
			btUnlock(handleSem, &handleVar);
			return;
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
	curNode->refCount = 0;
	curNode->invalid = false;
	curNode->vnid = file_vnid;
	curNode->parent = dirNode;

	// Add the node to the head of the list.
	curNode->next = rootNode;
	curNode->prev = NULL;
	if (rootNode)
		rootNode->prev = curNode;
	rootNode = curNode;

	btUnlock(handleSem, &handleVar);
}

// btRemoveHandle()
//
void btRemoveHandle(vnode_id vnid)
{
	bt_node *deadNode;

	btLock(handleSem, &handleVar);

	// Obtain the node in question.  If no such node exists, then we
	// probably have a bad handle.
	deadNode = btGetNodeFromVnid(vnid);
	btRemoveNode(deadNode);

	btUnlock(handleSem, &handleVar);
}

// btRemoveNode()
//
void btRemoveNode(bt_node *deadNode)
{
	if (deadNode)
	{
		// If this node is the root node, then we need to reset the root node
		// to the next node in the list.
		if (deadNode == rootNode)
			rootNode = deadNode->next;

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
void btPurgeNodes(vnode_id vnid)
{
	bt_node *curNode, *nextNode;

	btLock(handleSem, &handleVar);

	// First loop through, marking this node and all its children as invalid.
	curNode = rootNode;
	while (curNode)
	{
		if (curNode->vnid == vnid || btIsAncestorNode(vnid, curNode))
			curNode->invalid = true;

		curNode = curNode->next;
	}

	// Now loop through again, removing all invalid nodes.  This prevents removing
	// a parent node and all its children being orphaned (with invalid pointers
	// back to the destroyed parent).
	curNode = rootNode;
	while (curNode)
		if (curNode->invalid)
		{
			nextNode = curNode->next;
			btRemoveNode(curNode);
			curNode = nextNode;
		}
		else
			curNode = curNode->next;

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
char *btGetLocalFileName(char *path, vnode_id vnid)
{
	bt_node *node, *nodeStack[100];
	int stackSize;

	path[0] = 0;
	stackSize = 1;

	btLock(handleSem, &handleVar);

	node = btGetNodeFromVnid(vnid);
	if (node == NULL)
	{
		btUnlock(handleSem, &handleVar);
		return NULL;
	}

	nodeStack[0] = node;
	while ((node = node->parent) != NULL)
		nodeStack[stackSize++] = node;

	while (--stackSize >= 0)
	{
		strcat(path, nodeStack[stackSize]->name);
		if (stackSize)
			strcat(path, "/");
	}

	btUnlock(handleSem, &handleVar);
	return path;
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
			if (strcasecmp(fileShares[i].name, shareName) == 0)
				return fileShares[i].path;

	return NULL;
}

int btGetShareId(char *shareName)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (strcasecmp(fileShares[i].name, shareName) == 0)
				return i;

	return -1;
}

int btGetShareIdByPath(char *path)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (strcmp(fileShares[i].path, path) == 0)
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
		if (strcasecmp(sessions[i].share, shareName) == 0)
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

int btPreMount(bt_session_t *session, char *shareName)
{
	// Look for the specified share name.  If it can't be found, it must no longer be
	// shared on this host.
	int shareId = btGetShareId(shareName);
	if (shareId < 0)
		return ENOENT;

	return fileShares[shareId].security;
}

int btMount(bt_session_t *session, char *shareName, char *user, char *password, vnode_id *vnid)
{
	bt_user_rights *ur;
	struct stat st;
	char *path, *groups[MAX_GROUPS_PER_USER];
	int i, shareId;
	bool authenticated = false;

	// Initialize the groups array.  We may need to release the memory later.
	for (i = 0; i < MAX_GROUPS_PER_USER; i++)
		groups[i] = NULL;

	// Look for the specified share name.  If it can't be found, it must no longer be
	// shared on this host.
	shareId = btGetShareId(shareName);
	if (shareId < 0)
		return ENOENT;

	if (fileShares[shareId].security != BT_AUTH_NONE)
	{
		// Authenticate the user with name/password
		authenticated = authenticateUser(user, password);
		if (!authenticated)
			return EACCES;

		// Does the authenticated user have any rights on this file share?
		session->rights = 0;
		for (ur = fileShares[shareId].rights; ur; ur = ur->next)
			if (!ur->isGroup && strcasecmp(ur->user, user) == 0)
				session->rights |= ur->rights;

		// Does the authenticated user belong to any groups that have any rights on this
		// file share?
		getUserGroups(user, groups);
		for (ur = fileShares[shareId].rights; ur; ur = ur->next)
			if (ur->isGroup)
				for (i = 0; i < MAX_GROUPS_PER_USER; i++)
					if (groups[i] && strcasecmp(ur->user, groups[i]) == 0)
					{
						session->rights |= ur->rights;
						break;
					}

		// Free the memory occupied by the group list.
		for (i = 0; i < MAX_GROUPS_PER_USER; i++)
			if (groups[i])
				free(groups[i]);

		// If no rights have been granted, deny access.
		if (session->rights == 0)
			return EACCES;

		// If write access has been globally disabled, this user's rights must be
		// correspondingly synchronized.
		if (fileShares[shareId].readOnly)
			session->rights = BT_RIGHTS_READ;
	}
	else
		session->rights = fileShares[shareId].readOnly
			? BT_RIGHTS_READ
			: BT_RIGHTS_READ | BT_RIGHTS_WRITE;

	// Make sure the folder we want to share still exists.
	path = fileShares[shareId].path;
	if (stat(path, &st) != 0)
		return ENOENT;

	// Make sure it really is a folder and not a file.
	if (!S_ISDIR(st.st_mode))
		return EACCES;

	*vnid = st.st_ino;
	btAddHandle(0, *vnid, path);
	return B_OK;
}

int btGetFSInfo(char *rootPath, fs_info *fsInfo)
{
	dev_t device = dev_for_path(rootPath);
	if (device < 0)
		return device;

	if (fs_stat_dev(device, fsInfo) != 0)
		return errno;

	return B_OK;
}

int btLookup(char *pathBuf, vnode_id dir_vnid, char *fileName, vnode_id *file_vnid)
{
	bt_node *dnode, *fnode;
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;

	*file_vnid = 0;

	btLock(handleSem, &handleVar);
	dnode = btGetNodeFromVnid(dir_vnid);
	btUnlock(handleSem, &handleVar);

	if (!dnode)
		return EACCES;

	// Search all nodes for one with the given parent vnid and file
	// name.  If one is found, we can simply use that node to fill in
	// the new handle.
	fnode = btFindNode(dnode, fileName);
	if (fnode)
	{
		*file_vnid = fnode->vnid;

		folder = btGetLocalFileName(pathBuf, *file_vnid);
		if (folder)
			if (lstat(folder, &st) != 0)
			{
				btRemoveHandle(*file_vnid);
				*file_vnid = 0;
				return ENOENT;
			}
	}
	else
	{
		folder = btGetLocalFileName(pathBuf, dir_vnid);
		if (folder)
		{
			sprintf(path, "%s/%s", folder, fileName);
			if (lstat(path, &st) != 0)
				return ENOENT;

			*file_vnid = st.st_ino;
			btAddHandle(dir_vnid, *file_vnid, fileName);
		}
	}

	return B_OK;
}

int btStat(char *pathBuf, vnode_id vnid, struct stat *st)
{
	char *fileName;
	int error;

	fileName = btGetLocalFileName(pathBuf, vnid);
	if (fileName)
	{
		error = lstat(fileName, st);
		return (error != 0 ? ENOENT : B_OK);
	}

	return ENOENT;
}

int btReadDir(char *pathBuf, vnode_id dir_vnid, DIR **dir, vnode_id *file_vnid, char *filename, struct stat *st)
{
	struct dirent *dirInfo;
	char *folder, path[B_PATH_NAME_LENGTH];

	if (dir_vnid == 0 || !file_vnid || !filename)
		return EINVAL;

	if (!*dir)
	{
		folder = btGetLocalFileName(pathBuf, dir_vnid);
		if (folder)
			*dir = opendir(folder);
	}

	if (*dir)
	{
		if ((dirInfo = readdir(*dir)) != NULL)
		{
			folder = btGetLocalFileName(pathBuf, dir_vnid);
			if (folder)
			{
				sprintf(path, "%s/%s", folder, dirInfo->d_name);
				if (lstat(path, st) != 0)
					return ENOENT;

				strcpy(filename, dirInfo->d_name);
				*file_vnid = st->st_ino;
				btAddHandle(dir_vnid, *file_vnid, filename);
				return B_OK;
			}
		}
		else
		{
			closedir(*dir);
			return ENOENT;
		}
	}

	return EINVAL;
}

int32 btRead(char *pathBuf, vnode_id vnid, off_t pos, int32 len, char *buffer)
{
	char *path;
	int bytes;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		int file = open(path, O_RDONLY);
		if (file < 0)
			return errno;

		lseek(file, (int32) pos, SEEK_SET);
		bytes = read(file, buffer, len);
		close(file);

		// Return zero on any error.
		if (bytes == -1)
			bytes = 0;

		buffer[bytes] = 0;
		return bytes;
	}

	return 0;
}

int32 btWrite(bt_session_t *session, vnode_id vnid, off_t pos, int32 len, int32 totalLen, char *buffer)
{
	bt_block *block;

	// If we've been given a total length, then we have a new buffered write
	// session coming.  A block will need to be allocated.
	if (totalLen > 0)
	{
		// Make sure we don't have a wildly inaccurate total length to allocate.
		if (totalLen > 10 * 1024 * 1024)
			return 0;

		// Allocate a new buffered I/O block.
		block = (bt_block *) malloc(sizeof(bt_block));
		if (block)
		{
			block->vnid = vnid;
			block->pos = pos;
			block->len = totalLen;
			block->count = 0;

			block->buffer = (char *) malloc(totalLen + 1);
			if (!block->buffer)
			{
				free(block);
				return 0;
			}

			btInsertWriteBlock(session, block);
		}
		else
			return 0;
	}
	else
	{
		block = btGetWriteBlock(session, vnid);
		if (!block)
			return 0;
	}

	memcpy(block->buffer + block->count, buffer, len);
	block->count += len;
	return len;
}

// btGetWriteBlock()
//
bt_block *btGetWriteBlock(bt_session_t *session, vnode_id vnid)
{
	bt_block *block;

	btLock(session->blockSem, &session->blockVar);

	block = session->rootBlock;
	while (block && block->vnid != vnid)
		block = block->next;

	btUnlock(session->blockSem, &session->blockVar);
	return block;
}

// btInsertWriteBlock()
//
void btInsertWriteBlock(bt_session_t *session, bt_block *block)
{
	btLock(session->blockSem, &session->blockVar);

	block->next = session->rootBlock;
	block->prev = NULL;
	if (session->rootBlock)
		session->rootBlock->prev = block;

	session->rootBlock = block;

	btUnlock(session->blockSem, &session->blockVar);
}

int btCommit(bt_session_t *session, vnode_id vnid)
{
	bt_block *block;
	char *path;
	int file;

	// Get the full path for the specified file.
	path = btGetLocalFileName(session->pathBuffer, vnid);
	if (!path)
		return ENOENT;

	// Obtain the buffered I/O block.  If one can't be found, no buffered I/O
	// session was started for this vnode.
	block = btGetWriteBlock(session, vnid);
	if (!block)
		return ENOENT;

	// Open the file for writing.
	file = open(path, O_WRONLY | O_CREAT);
	if (file < 0)
		return errno;

	btLock(session->blockSem, &session->blockVar);

	// Write the data.
	lseek(file, (int32) block->pos, SEEK_SET);
	write(file, block->buffer, block->len);

	btRemoveWriteBlock(session, block);
	btUnlock(session->blockSem, &session->blockVar);

	close(file);
	return B_OK;
}

void btRemoveWriteBlock(bt_session_t *session, bt_block *block)
{
	// If we're removing the root, then adjust the root block pointer.
	if (session->rootBlock == block)
		session->rootBlock = block->next;

	// If there's a previous block, it should now point beyond this block.
	if (block->prev)
		block->prev->next = block->next;

	// If there's a next block, it should now point to the current predecessor.
	if (block->next)
		block->next->prev = block->prev;

	// Release the memory used by this block.
	free(block->buffer);
	free(block);
}

int btCreate(char *pathBuf, vnode_id dir_vnid, char *name, int omode, int perms, vnode_id *file_vnid)
{
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;
	int fh;

	folder = btGetLocalFileName(pathBuf, dir_vnid);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		fh = open(path, O_WRONLY | O_CREAT | O_TRUNC | omode, perms);
		if (fh == -1)
			return errno;
		else
		{
			close(fh);
			if (lstat(path, &st) == 0)
			{
				*file_vnid = st.st_ino;
				btAddHandle(dir_vnid, *file_vnid, name);
			}
			else
				return EACCES;
		}
	}

	return B_OK;
}

int btTruncate(char *pathBuf, vnode_id vnid, int64 len)
{
	char *path;
	int error;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		error = truncate(path, len);
		if (error == -1)
			return errno;

		return B_OK;
	}

	return EACCES;
}

// btCreateDir()
//
int btCreateDir(char *pathBuf, vnode_id dir_vnid, char *name, int perms, vnode_id *file_vnid, struct stat *st)
{
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, dir_vnid);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		if (mkdir(path, perms) != B_OK)
			return EACCES;

		if (lstat(path, st) != 0)
			return errno;

		*file_vnid = st->st_ino;
		btAddHandle(dir_vnid, *file_vnid, name);
		return B_OK;
	}

	return ENOENT;
}

// btDeleteDir()
//
int btDeleteDir(char *pathBuf, vnode_id vnid, char *name)
{
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, vnid);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		if (lstat(path, &st) != 0)
			return errno;

		if (rmdir(path) == -1)
			return errno;

		btPurgeNodes(st.st_ino);
		return B_OK;
	}

	return ENOENT;
}

// btRename()
//
int btRename(char *pathBuf, vnode_id old_vnid, char *oldName, vnode_id new_vnid, char *newName)
{
	struct stat st;
	char oldPath[B_PATH_NAME_LENGTH], newPath[B_PATH_NAME_LENGTH], *oldFolder, *newFolder;

	oldFolder = btGetLocalFileName(pathBuf, old_vnid);
	if (oldFolder)
	{
		sprintf(oldPath, "%s/%s", oldFolder, oldName);

		newFolder = btGetLocalFileName(pathBuf, new_vnid);
		if (newFolder)
		{
			sprintf(newPath, "%s/%s", newFolder, newName);

			if (lstat(oldPath, &st) != 0)
				return errno;

			btPurgeNodes(st.st_ino);

			if (rename(oldPath, newPath) == -1)
				return errno;

			return B_OK;
		}
	}

	return ENOENT;
}

// btUnlink()
//
int btUnlink(char *pathBuf, vnode_id vnid, char *name)
{
	struct stat st;
	char path[B_PATH_NAME_LENGTH], *folder;
	int error;

	folder = btGetLocalFileName(pathBuf, vnid);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);

		// Obtain the inode (vnid) of the specified file through lstat().
		if (lstat(path, &st) != 0)
			return errno;

		// Construct a dummy file descriptor and cause it to be removed from
		// the list.
		btRemoveHandle(st.st_ino);

		error = unlink(path);
		return (error == -1 ? errno : B_OK);
	}

	return EACCES;
}

int btReadLink(char *pathBuf, vnode_id vnid, char *buffer, int length)
{
	char *path;
	int error;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		error = readlink(path, buffer, length);
		if (error == -1)
			return errno;

		// If readlink() didn't return -1, it returned the number of bytes supplied in the
		// buffer.  It seems, however, that it does not null-terminate the string for us.
		buffer[error] = 0;
		return B_OK;
	}

	return ENOENT;
}

int btSymLink(char *pathBuf, vnode_id vnid, char *name, char *dest)
{
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, vnid);
	if (folder)
	{
		sprintf(path, "%s/%s", folder, name);
		if (symlink(dest, path) == -1)
			return errno;

		return B_OK;
	}

	return ENOENT;
}

int btWStat(char *pathBuf, vnode_id vnid, long mask, int32 mode, int32 uid, int32 gid, int64 size, int32 atime, int32 mtime)
{
	struct utimbuf ftimes;
	struct stat st;
	char *path;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		if (mask & WSTAT_MODE)
			chmod(path, mode);

		// BeOS doesn't support passing -1 as the user ID or group ID, which normally would
		// simply leave the value unchanged.  This complicates things a bit, but keep in
		// mind that BeOS doesn't really support multiple users anyway.
		if (mask & WSTAT_UID && mask & WSTAT_GID)
			chown(path, uid, gid);

//		if (mask & WSTAT_UID)
//			chown(path, uid, -1);

//		if (mask & WSTAT_GID)
//			chown(path, -1, gid);

		if (mask & WSTAT_SIZE)
			truncate(path, size);

		if (lstat(path, &st) == 0)
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

int btReadAttrib(char *pathBuf, vnode_id vnid, char *name, int32 dataType, void *buffer, int32 pos, int32 len)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		file = open(path, O_RDONLY);

		if (file)
		{
			int bytes = (int) fs_read_attr(file, name, dataType, pos, buffer, len);
			close(file);

			if ((dataType == B_STRING_TYPE || dataType == B_MIME_TYPE) && bytes < len && bytes >= 0)
				((char *) buffer)[bytes] = 0;

			return bytes;
		}
	}

	return ENOENT;
}

int btWriteAttrib(char *pathBuf, vnode_id vnid, char *name, int32 dataType, void *buffer, int32 pos, int32 len)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, vnid);
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

int btReadAttribDir(char *pathBuf, vnode_id vnid, DIR **dir, char *attrName)
{
	dirent_t *entry;
	char *path;

	if (!attrName)
		return EINVAL;

	if (!*dir)
	{
		path = btGetLocalFileName(pathBuf, vnid);
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

int btRemoveAttrib(char *pathBuf, vnode_id vnid, char *name)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, vnid);
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

int btStatAttrib(char *pathBuf, vnode_id vnid, char *name, struct attr_info *info)
{
	char *path;
	int file;

	path = btGetLocalFileName(pathBuf, vnid);
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

int btReadIndexDir(char *rootPath, DIR **dir, char *indexName)
{
	struct dirent *dirInfo;

	if (!*dir)
	{
		dev_t device = dev_for_path(rootPath);
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

int btCreateIndex(char *rootPath, char *name, int type, int flags)
{
	dev_t device = dev_for_path(rootPath);
	if (device < 0)
		return device;

	if (fs_create_index(device, name, type, flags) == -1)
		return errno;

	return B_OK;
}

int btRemoveIndex(char *rootPath, char *name)
{
	dev_t device = dev_for_path(rootPath);
	if (device < 0)
		return device;

	if (fs_remove_index(device, name) == -1)
		return errno;

	return B_OK;
}

int btStatIndex(char *rootPath, char *name, struct index_info *info)
{
	dev_t device = dev_for_path(rootPath);
	if (device < 0)
		return device;

	if (fs_stat_index(device, name, info) == -1)
		return errno;

	return B_OK;
}

int btReadQuery(char *rootPath, DIR **dir, char *query, char *fileName, vnode_id *vnid, vnode_id *parent)
{
	struct dirent *dirInfo;

	if (!*dir)
	{
		dev_t device = dev_for_path(rootPath);
		if (device < 0)
			return device;

		*dir = fs_open_query(device, query, 0);
	}

	if (*dir)
		if ((dirInfo = fs_read_query(*dir)) != NULL)
		{
			*vnid = dirInfo->d_ino;
			*parent = dirInfo->d_pino;
			strcpy(fileName, dirInfo->d_name);
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

void netbtPreMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, security;

	client = session->socket;
	if (argc == 1)
	{
		if (argv[0].type == B_STRING_TYPE)
		{
			security = btPreMount(session, argv[0].data);
			btRPCCreateAck(&packet, xid, security);
			btRPCSendAck(client, &packet);
			return;
		}
	}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	vnode_id vnid;
	int client, error;

	client = session->socket;
	if (argc == 3)
	{
		if (argv[0].type == B_STRING_TYPE && argv[1].type == B_STRING_TYPE &&
			argv[2].type == B_STRING_TYPE)
		{
			char *shareName = argv[0].data;
			char *user = argv[1].data;
			char *password = argv[2].data;
			error = btMount(session, shareName, user, password, &vnid);
			if (error == B_OK)
			{
				// Record this session having logged in to a specific share.
				session->share = btGetShareId(shareName);
				session->logon = time(NULL);

				// Now send the client a response with the root vnid.
				btRPCCreateAck(&packet, xid, error);
				btRPCPutInt64(&packet, vnid);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}
	}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtFSInfo(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
	{
		if (argv[0].type == B_INT64_TYPE)
		{
			fs_info info;

			error = btGetFSInfo(fileShares[session->share].path, &info);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, error);
				btRPCPutInt32(&packet, info.block_size);
				btRPCPutInt32(&packet, info.total_blocks);
				btRPCPutInt32(&packet, info.free_blocks);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}
	}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtLookup(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
	int client, error;

	client = session->socket;
	if (argc == 2)
	{
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id dir_vnid = *((vnode_id *) argv[0].data);
			vnode_id file_vnid;
			error = btLookup(session->pathBuffer, dir_vnid, argv[1].data, &file_vnid);
			if (error == B_OK)
				error = btStat(session->pathBuffer, file_vnid, &st);

			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt64(&packet, file_vnid);
				btRPCPutStat(&packet, &st);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}
	}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtReadDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
	int client, error;

	client = session->socket;
	if (argc == 2)
	{
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id dir_vnid = *((vnode_id *) argv[0].data);
			vnode_id file_vnid;
			DIR *dir;
			char filename[B_PATH_NAME_LENGTH];
			int entries = 0;

			dir = (DIR *)(*((int32 *) argv[1].data));
			error = btReadDir(session->pathBuffer, dir_vnid, &dir, &file_vnid, filename, &st);

			if (error != B_OK)
			{
				btRPCCreateAck(&packet, xid, error);
				btRPCSendAck(client, &packet);
				return;
			}

			btRPCCreateAck(&packet, xid, B_OK);
			while (error == B_OK)
			{
				btRPCPutInt64(&packet, file_vnid);
				btRPCPutString(&packet, filename, strlen(filename));
				btRPCPutInt32(&packet, (int32) dir);
				btRPCPutStat(&packet, &st);

				if (++entries >= 32)
					break;

				error = btReadDir(session->pathBuffer, dir_vnid, &dir, &file_vnid, filename, &st);
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

	sendErrorToClient(client, xid, EINVAL);
}

void netbtStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_INT64_TYPE)
		{
			struct stat info;
			vnode_id vnid = *((vnode_id *) argv[0].data);

			error = btStat(session->pathBuffer, vnid, &info);

			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, error);
				btRPCPutStat(&packet, &info);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtRead(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_INT32_TYPE && argv[2].type == B_INT32_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);
			off_t pos = *((off_t *) argv[1].data);
			int32 len = *((int32 *) argv[2].data);
			int32 bytes = 0;

			session->ioBuffer[len] = 0;
			bytes = btRead(session->pathBuffer, vnid, pos, len, session->ioBuffer);

			btRPCCreateAck(&packet, xid, B_OK);
			btRPCPutString(&packet, session->ioBuffer, bytes);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtWrite(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	int client;

	client = session->socket;
	if (argc == 5)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_INT64_TYPE &&
			argv[2].type == B_INT32_TYPE && argv[3].type == B_INT32_TYPE &&
			argv[4].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);
			off_t pos = *((off_t *) argv[1].data);
			int32 len = *((int32 *) argv[2].data);
			int32 totalLen = *((int32 *) argv[3].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (session->rights & BT_RIGHTS_WRITE)
				btWrite(session, vnid, pos, len, totalLen, argv[4].data);
		}
}

void netbtCreate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
	int client, error;

	client = session->socket;
	if (argc == 4)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE && argv[3].type == B_INT32_TYPE)
		{
			vnode_id dir_vnid = *((vnode_id *) argv[0].data);
			vnode_id file_vnid;
			int omode = *((int *) argv[2].data);
			int perms = *((int *) argv[3].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btCreate(session->pathBuffer, dir_vnid, argv[1].data, omode, perms, &file_vnid);
			if (error == B_OK)
				error = btStat(session->pathBuffer, file_vnid, &st);

			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt64(&packet, file_vnid);
				btRPCPutStat(&packet, &st);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtTruncate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_INT64_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btTruncate(session->pathBuffer, vnid, *((int64 *) argv[1].data));
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtUnlink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btUnlink(session->pathBuffer, vnid, argv[1].data);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtRename(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 4)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT64_TYPE && argv[3].type == B_STRING_TYPE)
		{
			vnode_id old_vnid = *((vnode_id *) argv[0].data);
			vnode_id new_vnid = *((vnode_id *) argv[2].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btRename(session->pathBuffer, old_vnid, argv[1].data, new_vnid, argv[3].data);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtCreateDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE)
		{
			vnode_id dir_vnid = *((vnode_id *) argv[0].data);
			vnode_id file_vnid;
			struct stat st;

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btCreateDir(session->pathBuffer, dir_vnid, argv[1].data, *((int *) argv[2].data), &file_vnid, &st);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt64(&packet, file_vnid);
				btRPCPutStat(&packet, &st);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtDeleteDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btDeleteDir(session->pathBuffer, vnid, argv[1].data);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtReadLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_INT64_TYPE)
		{
			char path[B_PATH_NAME_LENGTH];
			vnode_id vnid = *((vnode_id *) argv[0].data);

			error = btReadLink(session->pathBuffer, vnid, path, B_PATH_NAME_LENGTH);
			if (error == B_OK)
			{
				int length = strlen(path);
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutString(&packet, path, length);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtSymLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 3)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btSymLink(session->pathBuffer, vnid, argv[1].data, argv[2].data);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtWStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 8)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_INT32_TYPE && argv[2].type == B_INT32_TYPE &&
			argv[3].type == B_INT32_TYPE && argv[4].type == B_INT32_TYPE && argv[5].type == B_INT32_TYPE &&
			argv[6].type == B_INT32_TYPE && argv[7].type == B_INT32_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);
			int32 mask = *((int32 *) argv[1].data);
			int32 mode = *((int32 *) argv[2].data);
			int32 uid = *((int32 *) argv[3].data);
			int32 gid = *((int32 *) argv[4].data);
			int64 size = (int64) *((int32 *) argv[5].data);
			int32 atime = *((int32 *) argv[6].data);
			int32 mtime = *((int32 *) argv[7].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btWStat(session->pathBuffer, vnid, mask, mode, uid, gid, size, atime, mtime);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtReadAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesRead;

	client = session->socket;
	if (argc == 5)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE &&
			argv[3].type == B_INT32_TYPE && argv[4].type == B_INT32_TYPE)
		{
			char *buffer;
			vnode_id vnid = *((vnode_id *) argv[0].data);
			int32 type = *((int32 *) argv[2].data);
			int32 pos = *((int32 *) argv[3].data);
			int32 len = *((int32 *) argv[4].data);

			if (len <= BT_MAX_ATTR_BUFFER)
				buffer = session->attrBuffer;
			else
				buffer = (char *) malloc(len + 1);

			if (buffer)
			{
				bytesRead = btReadAttrib(session->pathBuffer, vnid, argv[1].data, type, buffer, pos, len);
				if (bytesRead >= 0)
				{
					btRPCCreateAck(&packet, xid, B_OK);
					btRPCPutInt32(&packet, (int32) bytesRead);
					if (bytesRead > 0)
						btRPCPutString(&packet, buffer, bytesRead);
				}
				else
					btRPCCreateAck(&packet, xid, B_ENTRY_NOT_FOUND);

				if (len > BT_MAX_ATTR_BUFFER)
					free(buffer);
			}
			else
				btRPCCreateAck(&packet, xid, ENOMEM);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtWriteAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesWritten;

	client = session->socket;
	if (argc == 6)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE && argv[2].type == B_INT32_TYPE &&
			argv[3].type == B_STRING_TYPE && argv[4].type == B_INT32_TYPE && argv[5].type == B_INT32_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);
			int32 type = *((int32 *) argv[2].data);
			int32 pos = *((int32 *) argv[4].data);
			int32 len = *((int32 *) argv[5].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			bytesWritten = btWriteAttrib(session->pathBuffer, vnid, argv[1].data, type, argv[3].data, pos, len);
			if (bytesWritten >= 0)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt32(&packet, (int32) bytesWritten);
			}
			else
				btRPCCreateAck(&packet, xid, B_ENTRY_NOT_FOUND);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtReadAttribDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
	{
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);
			DIR *dir = (DIR *)(*((int32 *) argv[1].data));
			char attrName[100];
			int entries = 0;

			error = btReadAttribDir(session->pathBuffer, vnid, &dir, attrName);

			if (error != B_OK)
			{
				btRPCCreateAck(&packet, xid, error);
				btRPCSendAck(client, &packet);
				return;
			}

			btRPCCreateAck(&packet, xid, B_OK);
			while (error == B_OK)
			{
				btRPCPutString(&packet, attrName, strlen(attrName));
				btRPCPutInt32(&packet, (int32) dir);

				if (++entries >= 32)
					break;

				error = btReadAttribDir(session->pathBuffer, vnid, &dir, attrName);
				btRPCPutInt32(&packet, error);
			}

			if (entries < 32)
				btRPCPutInt32(&packet, ENOENT);

			btRPCSendAck(client, &packet);
			return;
		}
	}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtRemoveAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btRemoveAttrib(session->pathBuffer, vnid, argv[1].data);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtStatAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 2)
		if (argv[0].type == B_INT64_TYPE && argv[1].type == B_STRING_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);
			struct attr_info info;

			error = btStatAttrib(session->pathBuffer, vnid, argv[1].data, &info);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt32(&packet, info.type);
				btRPCPutInt64(&packet, info.size);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
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

			error = btReadIndexDir(fileShares[session->share].path, &dir, indexName);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutString(&packet, indexName, strlen(indexName));
				btRPCPutInt32(&packet, (int32) dir);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
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

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btCreateIndex(fileShares[session->share].path, argv[0].data, type, flags);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtRemoveIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_STRING_TYPE)
		{
			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btRemoveIndex(fileShares[session->share].path, argv[0].data);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
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

			error = btStatIndex(fileShares[session->share].path, argv[0].data, &info);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt32(&packet, info.type);
				btRPCPutInt64(&packet, info.size);
				btRPCPutInt32(&packet, info.modification_time);
				btRPCPutInt32(&packet, info.creation_time);
				btRPCPutInt32(&packet, info.uid);
				btRPCPutInt32(&packet, info.gid);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
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
			vnode_id vnid, parent;

			dir = (DIR *)(*((int32 *) argv[0].data));

			error = btReadQuery(fileShares[session->share].path, &dir, argv[1].data, fileName, &vnid, &parent);
			if (error == B_OK)
			{
				btRPCCreateAck(&packet, xid, B_OK);
				btRPCPutInt64(&packet, vnid);
				btRPCPutInt64(&packet, parent);
				btRPCPutString(&packet, fileName, strlen(fileName));
				btRPCPutInt32(&packet, (int32) dir);
			}
			else
				btRPCCreateAck(&packet, xid, error);

			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

// netbtCommit()
//
void netbtCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;
	if (argc == 1)
		if (argv[0].type == B_INT64_TYPE)
		{
			vnode_id vnid = *((vnode_id *) argv[0].data);

			// If the file share this user is connected to is read-only, the command
			// cannot be honored.
			if (!(session->rights & BT_RIGHTS_WRITE))
			{
				btRPCCreateAck(&packet, xid, EACCES);
				btRPCSendAck(client, &packet);
				return;
			}

			error = btCommit(session, vnid);
			btRPCCreateAck(&packet, xid, error);
			btRPCSendAck(client, &packet);
			return;
		}

	sendErrorToClient(client, xid, EINVAL);
}

void netbtPrintJobNew(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, EOPNOTSUPP);
	btRPCSendAck(client, &packet);
}

void netbtPrintJobData(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, EOPNOTSUPP);
	btRPCSendAck(client, &packet);
}

void netbtPrintJobCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, EOPNOTSUPP);
	btRPCSendAck(client, &packet);
}

void netbtAuthenticate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, EOPNOTSUPP);
	btRPCSendAck(client, &packet);
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
		btRPCCreateAck(&packet, xid, B_OK);
		btRPCSendAck(client, &packet);
		return;
	}

	sendErrorToClient(client, xid, EINVAL);
}
