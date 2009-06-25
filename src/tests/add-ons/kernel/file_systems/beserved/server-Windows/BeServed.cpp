// BeServed.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "beCompat.h"
#include "betalk.h"
#include "authentication.h"
#include "readerWriter.h"
#include "printing.h"
#include "ubi_SplayTree.h"

// Windows service-specific header files.
#include "winsvc.h"
#include "NTServApp.h"
#include "myservice.h"

#define BT_MAX_THREADS			100
#define BT_MAX_RETRIES			3
#define BT_MAX_FILE_SHARES		128
#define BT_MAX_PRINTER_SHARES	16

#define BT_THREAD_NAME		"BeServed Handler"
#define BT_HOST_THREAD_NAME	"BeServed Host Publisher"

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
	beos_off_t pos;
	int32 len;
	int32 count;
	char *buffer;
	struct btblock *next;
	struct btblock *prev;
} bt_block;

typedef struct session
{
	int socket;
	unsigned int client_s_addr;
	HANDLE handlerID;

	bool killed;

	// The user that is connected.
	char user[MAX_NAME_LENGTH + 1];

	// What share did the client connect to?  And when?
	int share;
	int rights;
	time_t logon;

	// Buffered write support.
	bt_block *rootBlock;
	sem_id blockSem;

	char ioBuffer[BT_MAX_IO_BUFFER + 1];
	char attrBuffer[BT_MAX_ATTR_BUFFER + 1];
	char pathBuffer[B_PATH_NAME_LENGTH];

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

typedef struct
{
	vnode_id vnid;
	int shareId;
	char unused[20];
} bt_fdesc;

typedef struct btnode
{
	ubi_trNode node;
	vnode_id vnid;
	char *name;
	bool invalid;
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

typedef struct mime_mapping
{
	char extension[7];
	char *mimeType;
	struct mime_mapping *next;
} bt_mime_mapping;


void BeServedStartup(CMyService *service);
bool dateCheck();
DWORD WINAPI btSendHost(LPVOID data);
void getHostInfo(bt_hostinfo *info);
int getSharedResources(char *buffer, int bufSize, bool shares, bool printers);
int getHostUsers(char *buffer);
void startService();
void endService(int sig);
void initMimeMap();
void closeMimeMap();
void closePrinters();
void freeFileShares();
void waitForThread(HANDLE threadId);

void initShares();
void initPrinters();
void loadShares();
void getFileShare(const char *buffer);
void loadFolder(const char *path);
void getShareProperty(const char *buffer);
void getAuthenticate(const char *buffer);
bool getAuthServerAddress(const char *name);
void addUserRights(char *share, char *user, int rights, bool isGroup);
void getGrant(const char *buffer);
void getPrinter(const char *buffer);
int getToken();

int receiveRequest(bt_session_t *session);
void handleRequest(bt_session_t *session, unsigned int xid, unsigned char command, int argc, bt_arg_t argv[]);
void launchThread(int client, struct sockaddr_in *addr);
int tooManyConnections(unsigned int _s_addr);
void sendErrorToClient(int client, unsigned int xid, int error);
void getArguments(bt_session_t *session, bt_inPacket *packet, unsigned char command);
DWORD WINAPI requestThread(LPVOID data);
DWORD WINAPI printerThread(LPVOID data);

void KillNode(ubi_trNodePtr node);
int CompareVnidNodes(ubi_trItemPtr item, ubi_trNodePtr node);
int CompareNameNodes(ubi_trItemPtr item, ubi_trNodePtr node);

bt_node *btGetNodeFromVnid(vnode_id vnid);
void btAddHandle(vnode_id dir_vnid, vnode_id file_vnid, char *name);
void btRemoveHandle(vnode_id vnid);
void btPurgeNodes(vnode_id vnid);
bool btIsAncestorNode(vnode_id vnid, bt_node *node);
char *btGetLocalFileName(char *path, vnode_id vnid);
void btMakeHandleFromNode(bt_node *node, vnode_id vnid);
bt_node *btFindNode(bt_node *parent, char *fileName);
char *btGetSharePath(char *shareName);
int btGetShareId(char *shareName);
int btGetShareIdByPath(char *path);
bt_printer *btFindPrinter(char *printerName);
uint32 btGetWinInode(const char *path);
int btGetBeosStat(char *fileName, beos_stat *st);
void btMakePath(char *path, char *dir, char *file);

int btPreMount(bt_session_t *session, char *shareName);
int btMount(bt_session_t *session, char *shareName, char *user, char *password, vnode_id *vnid);
int btGetFSInfo(fs_info *fsInfo, char *path);
int btLookup(char *pathBuf, vnode_id dir_vnid, char *fileName, vnode_id *file_vnid);
int btStat(char *pathBuf, vnode_id vnid, beos_stat *st);
int btReadDir(char *pathBuf, vnode_id dir_vnid, long **dir, vnode_id *file_vnid, char *filename, beos_stat *st);
int32 btRead(char *pathBuf, vnode_id vnid, beos_off_t pos, int32 len, char *buffer);
int32 btWrite(bt_session_t *session, vnode_id vnid, beos_off_t pos, int32 len, int32 totalLen, char *buffer);
int btCreate(char *pathBuf, vnode_id dir_vnid, char *name, int omode, int perms, vnode_id *file_vnid);
int btTruncate(char *pathBuf, vnode_id vnid, int64 len);
int btCreateDir(char *pathBuf, vnode_id dir_vnid, char *name, int perms, vnode_id *file_vnid, beos_stat *st);
int btDeleteDir(char *pathBuf, vnode_id dir_vnid, char *name);
int btRename(char *pathBuf, vnode_id old_vnid, char *oldName, vnode_id new_vnid, char *newName);
int btUnlink(char *pathBuf, vnode_id vnid, char *name);
int btReadLink(char *pathBuf, vnode_id vnid, char *buffer, int length);
int btSymLink(char *pathBuf, vnode_id vnid, char *name, char *dest);
int btWStat(char *pathBuf, vnode_id vnid, long mask, int32 mode, int32 uid, int32 gid, int64 size, int32 atime, int32 mtime);
int btCommit(bt_session_t *session, vnode_id vnid);
int btReadAttrib(char *pathBuf, vnode_id vnid, char *name, char *buffer);
int btAuthenticate(char *resource, char *user, char *password);

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

CMyService *winService;
bt_node *rootNode = NULL;
ubi_btRoot vnidTree;
ubi_btRoot nameTree;
bt_mime_mapping *mimeMap = NULL;
bt_session_t *rootSession = NULL;
bt_fileShare_t fileShares[BT_MAX_FILE_SHARES];
bt_printer sharedPrinters[BT_MAX_PRINTER_SHARES];
char tokBuffer[MAX_NAME_LENGTH + 1], *tokPtr;
bool running = true;
bool preload = false;
int server;
char authServerName[B_FILE_NAME_LENGTH];
unsigned int authServerIP;
bt_managed_data handleData;
bt_managed_data sessionData;
HANDLE hostThread;

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
	"preload",
	NULL
};

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
	{ BT_CMD_READLINK,			netbtReadLink,				false,	1,	{ B_INT64_TYPE } },
	{ BT_CMD_SYMLINK,			netbtSymLink,				false,	3,	{ B_INT64_TYPE, B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_WSTAT,				netbtWStat,					true,	8,	{ B_INT64_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_READATTRIB,		netbtReadAttrib,			true,	5,	{ B_STRING_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_WRITEATTRIB,		netbtWriteAttrib,			false,	6,	{ B_STRING_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_READATTRIBDIR,		netbtReadAttribDir,			false,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },		
	{ BT_CMD_REMOVEATTRIB,		netbtRemoveAttrib,			false,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_STATATTRIB,		netbtStatAttrib,			true,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_READINDEXDIR,		netbtReadIndexDir,			false,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_CREATEINDEX,		netbtCreateIndex,			false,	3,	{ B_STRING_TYPE, B_INT32_TYPE, B_INT32_TYPE } },
	{ BT_CMD_REMOVEINDEX,		netbtRemoveIndex,			false,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_STATINDEX,			netbtStatIndex,				false,	1,	{ B_STRING_TYPE } },
	{ BT_CMD_READQUERY,			netbtReadQuery,				false,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_COMMIT,			netbtCommit,				true,	1,	{ B_INT64_TYPE } },
	{ BT_CMD_PRINTJOB_NEW,		netbtPrintJobNew,			true,	4,	{ B_STRING_TYPE, B_STRING_TYPE, B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_PRINTJOB_DATA,		netbtPrintJobData,			true,	4,	{ B_STRING_TYPE, B_STRING_TYPE, B_STRING_TYPE, B_INT32_TYPE } },
	{ BT_CMD_PRINTJOB_COMMIT,	netbtPrintJobCommit,		true,	2,	{ B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_AUTHENTICATE,		netbtAuthenticate,			true,	3,	{ B_STRING_TYPE, B_STRING_TYPE, B_STRING_TYPE } },
	{ BT_CMD_QUIT,				netbtQuit,					true,	0,	{ 0 } },
	{ 0,						NULL,						false,	0,	{ 0 } }
};


//int APIENTRY WinMain(HINSTANCE hInstance,
//                     HINSTANCE hPrevInstance,
//                     LPSTR     lpCmdLine,
//                     int       nCmdShow)
void BeServedStartup(CMyService *service)
{
	WSADATA wsaData;
	DWORD threadId;

	ASSERT(service);
	winService = service;

	initPrinters();

	initMimeMap();
	initShares();
	signal(SIGINT, endService);
	signal(SIGTERM, endService);

	// Initialize the trees sorted by vnode_id and by name.
	ubi_trInitTree(&vnidTree, CompareVnidNodes, 0);
	ubi_trInitTree(&nameTree, CompareNameNodes, 0);

	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		return;

	if (initManagedData(&handleData))
		if (initManagedData(&sessionData))
			hostThread = CreateThread(NULL, 0, btSendHost, NULL, 0, &threadId);
}

bool dateCheck()
{
	struct _stat st;
	time_t curTime;

	time(&curTime);
	if (curTime > 1012537700)
		return false;

	if (_stat("/boot/home/config/servers/beserved_server", &st) == 0)
		if (curTime < st.st_ctime || curTime > st.st_ctime + 7776000)
			return false;

	return true;
}

void restartService()
{
	bt_fileShare_t *oldShares;
	int i;

	// Printers are easier because the printer name is sent with each print request.
	// If the printer is no longer available, the next transmission sent regarding
	// that printer will result in an error, terminating the print job on the client.
	// All we have to do is clear out the structures, and initShares will read the
	// configuration file.
	initPrinters();

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

DWORD WINAPI btSendHost(LPVOID data)
{
	bt_request request;
	bt_hostinfo info;
	struct sockaddr_in serverAddr, clientAddr;
	char buffer[4096];
	int server, addrLen, bufLen, replyLen;

	buffer[0] = 0;
	bufLen = sizeof(buffer);

	server = socket(AF_INET, SOCK_DGRAM, 0);
	if (server == INVALID_SOCKET)
		return 0;

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(BT_QUERYHOST_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind that socket to the address constructed above.
	if (bind(server, (struct sockaddr *) &serverAddr, sizeof(serverAddr)))
		return 0;

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
				replyLen = getSharedResources(buffer, sizeof(buffer), true, true);
				break;

			case BT_REQ_PRINTER_PROBE:
				replyLen = getSharedResources(buffer, sizeof(buffer), false, true);
				break;

			case BT_REQ_HOST_INFO:
				getHostInfo(&info);
				memcpy(buffer, &info, sizeof(bt_hostinfo));
				replyLen = sizeof(bt_hostinfo);
				break;

			case BT_REQ_HOST_USERS:
				replyLen = getHostUsers(buffer);
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
int getSharedResources(char *buffer, int bufSize, bool shares, bool printers)
{
	bt_resource resource;
	int i, bufPos = 0;

	// If the supplied buffer can't hold at least two resource structures, one
	// for a shared resource and one to terminate the list, then don't bother
	// building the list.
	if (bufSize < 2 * sizeof(bt_resource))
		return 0;

	if (shares)
		for (i = 0; i < BT_MAX_FILE_SHARES; i++)
			if (fileShares[i].used)
			{
				// If this is the last resource structure that will fit in the
				// buffer, then don't add any more into the list.
				if (bufPos + sizeof(bt_resource) >= bufSize)
					break;

				// Fill out the resource structure.
				resource.type = BT_SHARED_FOLDER;
				resource.subType = 0;
				strcpy(resource.name, fileShares[i].name);

				// Copy the resource structure into the buffer at the current offset.
				memcpy(buffer + bufPos, &resource, sizeof(bt_resource));
				bufPos += sizeof(bt_resource);
			}

	if (printers)
		for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
			if (sharedPrinters[i].used)
			{
				// If this is the last resource structure that will fit in the
				// buffer, then don't add any more into the list.
				if (bufPos + sizeof(bt_resource) >= bufSize)
					break;

				// Fill out the resource structure.
				resource.type = BT_SHARED_PRINTER;
				resource.subType = BT_PRINTER_PCL3;
				strcpy(resource.name, sharedPrinters[i].printerName);

				// Copy the resource structure into the buffer at the current offset.
				memcpy(buffer + bufPos, &resource, sizeof(bt_resource));
				bufPos += sizeof(bt_resource);
			}

	// Copy the null terminating structure.
	resource.type = BT_SHARED_NULL;
	resource.subType = 0;
	resource.name[0] = 0;
	memcpy(buffer + bufPos, &resource, sizeof(bt_resource));
	bufPos += sizeof(bt_resource);

	return bufPos;
}

// getHostInfo()
//
void getHostInfo(bt_hostinfo *info)
{
	SYSTEM_INFO systemInfo;
	OSVERSIONINFOEX osInfo;
	bt_session_t *s;

	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (!GetVersionEx((OSVERSIONINFO *) &osInfo))
	{
		osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx((OSVERSIONINFO *) &osInfo);
	}

	GetSystemInfo(&systemInfo);

	switch (osInfo.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_WINDOWS:
			if (osInfo.dwMinorVersion == 0)
			{
				strcpy(info->system, "Windows 95");
				if (osInfo.szCSDVersion[1] == 'C' || osInfo.szCSDVersion[1] == 'B')
					strcat(info->system, " OSR2");
			}
			else if (osInfo.dwMinorVersion == 10)
			{
				strcpy(info->system, "Windows 98");
				if (osInfo.szCSDVersion[1] == 'A')
					strcat(info->system, " SE");
			}
			else if (osInfo.dwMinorVersion == 90)
				strcpy(info->system, "Windows Me");
			break;

		case VER_PLATFORM_WIN32_NT:
			if (osInfo.dwMajorVersion <= 4)
				sprintf(info->system, "Windows NT %d.%d %s (Build %d)",
					osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.szCSDVersion,
					osInfo.dwBuildNumber);
			else if (osInfo.dwMajorVersion == 5)
				if (osInfo.dwMinorVersion == 0)
					sprintf(info->system, "Windows 2000 %s (Build %d)",
						osInfo.szCSDVersion, osInfo.dwBuildNumber & 0xffff);
				else
					sprintf(info->system, "Windows XP %s (Build %d)",
						osInfo.szCSDVersion, osInfo.dwBuildNumber & 0xffff);

			break;
	}

	strcpy(info->beServed, "BeServed 1.2.6");

	info->cpus = systemInfo.dwNumberOfProcessors;
	info->maxConnections = BT_MAX_THREADS;

	switch (systemInfo.wProcessorArchitecture)
	{
		case PROCESSOR_ARCHITECTURE_MIPS:
			strcpy(info->platform, "MIPS");
			break;

		case PROCESSOR_ARCHITECTURE_ALPHA:
			strcpy(info->platform, "DEC Alpha");
			break;

		case PROCESSOR_ARCHITECTURE_PPC:
			strcpy(info->platform, "PowerPC");
			break;

		case PROCESSOR_ARCHITECTURE_INTEL:
			strcpy(info->platform, "Intel x86");
			break;

		default:
			strcpy(info->platform, "Unknown");
	}

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
			uint8 *client_s_addr = (uint8 *) s->client_s_addr;
			sprintf(addr, "%d.%d.%d.%d", client_s_addr[0], client_s_addr[1], client_s_addr[2], client_s_addr[3]);
			len = strlen(buffer);
			strcpy(&buffer[len > 0 ? len + 1 : 0], addr);
			bufSize += len + 1;
		}

	endReading(&sessionData);

	buffer[bufSize++] = 0;
	return bufSize;
}

// initMimeMap()
//
void initMimeMap()
{
	FILE *fp;
	bt_mime_mapping *map;
	char mimeLine[256];
	int i, mimeLen;

	fp = fopen("BeServed-MimeMap", "r");
	if (fp)
	{
		while (fgets(mimeLine, sizeof(mimeLine) - 1, fp))
		{
			// Strip off the carriage return.
			mimeLine[strlen(mimeLine) - 1] = 0;

			// Create a new MIME map entry.
			map = (bt_mime_mapping *) malloc(sizeof(bt_mime_mapping));
			if (!map)
				continue;

			map->next = mimeMap;
			mimeMap = map;

			// Grab the extension.
			for (i = 0; mimeLine[i] != ' '; i++)
				map->extension[i] = mimeLine[i];

			map->extension[i] = 0;

			// Skip to the start of the MIME type.
			while (mimeLine[i] == ' ')
				i++;

			// Grab the MIME type.
			mimeLen = strlen(&mimeLine[i]);
			map->mimeType = (char *) malloc(mimeLen + 1);
			if (map->mimeType)
				strncpy(map->mimeType, &mimeLine[i], mimeLen + 1);
		}

		fclose(fp);
	}
}

void closeMimeMap()
{
	bt_mime_mapping *map = mimeMap;

	while (map)
	{
		map = mimeMap->next;
		free(mimeMap->mimeType);
		free(mimeMap);
		mimeMap = map;
	}
}

void initShares()
{
	FILE *fp;
	char buffer[512];
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

	fp = fopen("BeServed-Settings", "r");
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
			else if (strncmp(buffer, "printer ", 8) == 0)
				getPrinter(buffer);
			else if (strcmp(buffer, "preload") == 0)
				preload = true;
		}

		fclose(fp);
	}
}

void initPrinters()
{
	int i;

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
	{
		sharedPrinters[i].printerName[0] = 0;
		sharedPrinters[i].deviceName[0] = 0;
		sharedPrinters[i].spoolDir[0] = 0;
		sharedPrinters[i].rights = NULL;
		sharedPrinters[i].security = BT_AUTH_NONE;
		sharedPrinters[i].used = false;
		sharedPrinters[i].killed = false;
	}
}

void loadShares()
{
	int i;

	if (preload)
		for (i = 0; i < BT_MAX_FILE_SHARES; i++)
			if (fileShares[i].used)
				loadFolder(fileShares[i].path);
}

void getFileShare(const char *buffer)
{
	struct _stat st;
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
		winService->LogEvent(EVENTLOG_WARNING_TYPE, EVMSG_DUPLICATE_SHARE, share);
		return;
	}

	// Check the path to ensure its validity.
	if (_stat(path, &st) != 0)
	{
		winService->LogEvent(EVENTLOG_WARNING_TYPE, EVMSG_INVALID_SHARE_PATH, path);
		return;
	}

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (!fileShares[i].used)
		{
			strcpy(fileShares[i].name, share);
			strcpy(fileShares[i].path, path);
			fileShares[i].used = true;
			return;
		}

	winService->LogEvent(EVENTLOG_WARNING_TYPE, EVMSG_TOO_MANY_SHARES, share);
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
		fileShares[shareId].readOnly = false;
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
		else if (tok == BT_TOKEN_PRINT)
		{
			rights |= BT_RIGHTS_PRINT;
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
	int i, tok;

	// Skip over AUTHENTICATE command.
	tokPtr = (char *) buffer + (13 * sizeof(char));

	tok = getToken();
	if (tok != BT_TOKEN_WITH)
		return;

	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	// Loop up address for given host.
	getAuthServerAddress(tokBuffer);

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		fileShares[i].security = BT_AUTH_BESURE;

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		sharedPrinters[i].security = BT_AUTH_BESURE;
}

bool getAuthServerAddress(const char *name)
{
	// Look up address for given host.
	struct hostent *ent = gethostbyname(name);
	if (ent == NULL)
	{
		unsigned long addr = inet_addr(tokBuffer);
		authServerIP = ntohl(addr);
	}
	else
		authServerIP = ntohl(*((unsigned int *) ent->h_addr));

	strcpy(authServerName, name);
	return true;
}

void addUserRights(char *share, char *user, int rights, bool isGroup)
{
	bt_printer *printer;
	bt_user_rights *ur;
	bool isPrinter = false;
	int shareId;

	// Now make sure that the rights make sense.  If we're granting the
	// right to print, we should not have granted the right to read or write.
	// We should also be working exclusively with a printer.
	if (rights & BT_RIGHTS_PRINT)
	{
		if (rights & BT_RIGHTS_READ || rights & BT_RIGHTS_WRITE)
			return;

		printer = btFindPrinter(share);
		if (!printer)
			return;

		isPrinter = true;
	}
	else
	{
		shareId = btGetShareId(share);
		if (shareId < 0)
			return;
	}

	// Allocate a new user rights structure, populate it, and attach it to
	// either a file share or printer.
	ur = (bt_user_rights *) malloc(sizeof(bt_user_rights));
	if (ur)
	{
		ur->user = (char *) malloc(strlen(user) + 1);
		if (ur->user)
		{
			strcpy(ur->user, user);
			ur->rights = rights;
			ur->isGroup = isGroup;

			if (isPrinter)
			{
				ur->next = printer->rights;
				printer->rights = ur;
			}
			else
			{
				ur->next = fileShares[shareId].rights;
				fileShares[shareId].rights = ur;
			}
		}
		else
			free(ur);
	}
}

void getPrinter(const char *buffer)
{
	bt_printer printer;
	DWORD threadId;
	int i, tok;

	// Skip over PRINTER command.
	tokPtr = (char *) buffer + (8 * sizeof(char));

	// Now get the name this printer will be shared as.
	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.printerName, tokBuffer);

	// Bypass the IS and TYPE keywords.
	tok = getToken();
	if (tok != BT_TOKEN_IS)
		return;

	tok = getToken();
	if (tok != BT_TOKEN_TYPE)
		return;

	// Get the device type of the printer.
	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.deviceType, tokBuffer);

	// Bypass the DEVICE keyword.
	tok = getToken();
	if (tok != BT_TOKEN_DEVICE)
		return;

	// Get the system device name of the printer.
	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.deviceName, tokBuffer);

	// Bypass the SPOOLED TO keywords.
	tok = getToken();
	if (tok != BT_TOKEN_SPOOLED)
		return;

	tok = getToken();
	if (tok != BT_TOKEN_TO)
		return;

	// Get the spooling folder, where temporary files will be stored.
	tok = getToken();
	if (tok != BT_TOKEN_STRING)
		return;

	strcpy(printer.spoolDir, tokBuffer);

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		if (!sharedPrinters[i].used)
		{
			sharedPrinters[i].used = true;
			strcpy(sharedPrinters[i].printerName, printer.printerName);
			strcpy(sharedPrinters[i].deviceType, printer.deviceType);
			strcpy(sharedPrinters[i].deviceName, printer.deviceName);
			strcpy(sharedPrinters[i].spoolDir, printer.spoolDir);
			sharedPrinters[i].handlerID = CreateThread(NULL, 0, printerThread, (void *) &sharedPrinters[i], 0, &threadId);
			break;
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
		*tokPtr++;
		return BT_TOKEN_COMMA;
	}
	else if (*tokPtr == '\"')
	{
		quoted = true;
		tokPtr++;
	}

	if (isalnum(*tokPtr) || *tokPtr == '\\')
	{
		int i = 0;
		while (isalnum(*tokPtr) || isValid(*tokPtr) || (quoted && *tokPtr == ' '))
			if (i < MAX_NAME_LENGTH)
				tokBuffer[i++] = *tokPtr++;
			else
				tokPtr++;

		tokBuffer[i] = 0;

		if (!quoted)
			for (i = 0; keywords[i]; i++)
				if (stricmp(tokBuffer, keywords[i]) == 0)
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
	WSACleanup();

	TerminateThread(hostThread, 0);
	closeManagedData(&sessionData);
	closeManagedData(&handleData);

	closePrinters();
	closeMimeMap();
	freeFileShares();

	ubi_trKillTree(&vnidTree, KillNode);
	ubi_trKillTree(&nameTree, KillNode);

	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
}

void closePrinters()
{
	int i;

	// Close down shared printer spooling threads.
	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		if (sharedPrinters[i].used)
		{
			sharedPrinters[i].killed = true;
			waitForThread(sharedPrinters[i].handlerID);
		}
}

// waitForThread()
//
void waitForThread(HANDLE threadId)
{
	DWORD exitCode;

	while (true)
	{
		// Get the exit code of the thread.  If the function fails, the
		// thread is invalid and thus already gone.
		if (!GetExitCodeThread(threadId, &exitCode))
			break;

		// If we have an exit code, the thread isn't running.
		if (exitCode != STILL_ACTIVE)
			break;

		// Otherwise, wait and retry.
		Sleep(100);
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
	DWORD threadId;
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
			session->handlerID = CreateThread(NULL, 0, requestThread, (void *) session, 0, &threadId);

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

DWORD WINAPI requestThread(LPVOID data)
{
	bt_session_t *session = (bt_session_t *) data;

	if (!session)
		return 0;

	session->blockSem = CreateSemaphore(NULL, 1, 1, NULL);
	if (session->blockSem)
	{
		session->rootBlock = NULL;
		while (!session->killed && receiveRequest(session));
		CloseHandle(session->blockSem);
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
	if (btRecvMsg(client, (char *) &length, sizeof(int32), 0) == -1)
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
				sendErrorToClient(session->socket, xid, ENOTSUP);
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

// btRecvMsg()
//
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
		bytes = recv(sock, (char *) data, dataLen, flags);
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

// btSendMsg()
//
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
		bytes = send(sock, (char *) data, dataLen, flags);
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

uint32 btSwapInt32(uint32 num)
{
	uint8 byte;
	union
	{
		uint32 value;
		uint8 bytes[4];
	} convert;

	convert.value = num;
	byte = convert.bytes[0];
	convert.bytes[0] = convert.bytes[3];
	convert.bytes[3] = byte;
	byte = convert.bytes[1];
	convert.bytes[1] = convert.bytes[2];
	convert.bytes[2] = byte;
	return convert.value;
}

uint64 btSwapInt64(uint64 num)
{
	uint8 byte;
	union
	{
		uint64 value;
		uint8 bytes[8];
	} convert;

	convert.value = num;
	byte = convert.bytes[0];
	convert.bytes[0] = convert.bytes[7];
	convert.bytes[7] = byte;
	byte = convert.bytes[1];
	convert.bytes[1] = convert.bytes[6];
	convert.bytes[6] = byte;
	byte = convert.bytes[2];
	convert.bytes[2] = convert.bytes[5];
	convert.bytes[5] = byte;
	byte = convert.bytes[3];
	convert.bytes[3] = convert.bytes[4];
	convert.bytes[4] = byte;
	return convert.value;
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

void btRPCGetStat(bt_inPacket *packet, beos_stat *st)
{
	st->st_dev = 0;
	st->st_nlink = btRPCGetInt32(packet);
	st->st_uid = btRPCGetInt32(packet);
	st->st_gid = btRPCGetInt32(packet);
	st->st_size = (int32) btRPCGetInt64(packet);
	st->st_blksize = btRPCGetInt32(packet);
	st->st_rdev = btRPCGetInt32(packet);
	st->st_ino = (int32) btRPCGetInt64(packet);
	st->st_mode = btRPCGetInt32(packet);
	st->st_atime = btRPCGetInt32(packet);
	st->st_mtime = btRPCGetInt32(packet);
	st->st_ctime = btRPCGetInt32(packet);
}

void btRPCPutStat(bt_outPacket *packet, beos_stat *st)
{
	if (packet && st)
	{
		int64 size = (int64) st->st_size;
		int64 inode = (int64) st->st_ino;

		btRPCPutInt32(packet, (int) st->st_nlink);
		btRPCPutInt32(packet, (int) st->st_uid);
		btRPCPutInt32(packet, (int) st->st_gid);
		btRPCPutInt64(packet, size);
		btRPCPutInt32(packet, (int) 1024);
		btRPCPutInt32(packet, (int) st->st_rdev);
		btRPCPutInt64(packet, inode);
		btRPCPutInt32(packet, (int) st->st_mode);
		btRPCPutInt32(packet, (int) st->st_atime);
		btRPCPutInt32(packet, (int) st->st_mtime);
		btRPCPutInt32(packet, (int) st->st_ctime);
	}
}

////////////////////////////////////////////////////////////////////

bt_node *btGetNodeFromVnid(vnode_id vnid)
{
	return (bt_node *) ubi_trFind(&vnidTree, &vnid);
}

// btAddHandle()
//
void btAddHandle(vnode_id dir_vnid, vnode_id file_vnid, char *name)
{
	bt_node *vnidNode, *nameNode, *dirNode;

	// We don't store the references to the current and the parent directory.
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return;

	beginWriting(&handleData);

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
	if (btGetNodeFromVnid(file_vnid))
	{
		endWriting(&handleData);
		return;
	}

	// Allocate a new node for the vnid and name-based trees.  A separate node must exist for
	// each tree or the tree structure will go haywire.
	vnidNode = (bt_node *) malloc(sizeof(bt_node));
	if (vnidNode == NULL)
	{
		endWriting(&handleData);
		return;
	}

	nameNode = (bt_node *) malloc(sizeof(bt_node));
	if (nameNode == NULL)
	{
		free(vnidNode);
		endWriting(&handleData);
		return;
	}

	// Allocate memory for the file name itself.  This prevents huge memory consumption used by
	// a static buffer that assumes the worst case file name length.
	vnidNode->name = (char *) malloc(strlen(name) + 1);
	if (vnidNode->name == NULL)
	{
		free(nameNode);
		free(vnidNode);
		endWriting(&handleData);
		return;
	}

	// Copy the name into the allocated buffer.
	strcpy(vnidNode->name, name);

	nameNode->name = strdup(vnidNode->name);
	if (nameNode->name == NULL)
	{
		free(vnidNode->name);
		free(nameNode);
		free(vnidNode);
		endWriting(&handleData);
		return;
	}

	// Copy over the vnid, and parent node.
	vnidNode->invalid = nameNode->invalid = false;
	vnidNode->vnid = nameNode->vnid = file_vnid;
	vnidNode->parent = nameNode->parent = dirNode;

	// Now insert the new nodes into the tree.
	ubi_trInsert(&vnidTree, vnidNode, &vnidNode->vnid, NULL);
	ubi_trInsert(&nameTree, nameNode, nameNode, NULL);

	endWriting(&handleData);
}

// btRemoveHandle()
//
void btRemoveHandle(vnode_id vnid)
{
	bt_node *deadVnidNode, *deadNameNode;

	beginWriting(&handleData);

	deadVnidNode = (bt_node *) ubi_trFind(&vnidTree, &vnid);
	if (deadVnidNode)
	{
		ubi_trRemove(&vnidTree, deadVnidNode);

		deadNameNode = (bt_node *) ubi_trFind(&nameTree, deadVnidNode);
		if (deadNameNode)
			ubi_trRemove(&nameTree, deadNameNode);
	}

	endWriting(&handleData);
}

// btPurgeNodes()
//
void btPurgeNodes(vnode_id vnid)
{
	bt_node *curNode;
	ubi_trNodePtr cur, next;
	ubi_trRootPtr tree;
	ubi_trRootPtr trees[] = { &vnidTree, &nameTree, NULL };
	int i;

	beginWriting(&handleData);

	// First loop through, marking this node and all its children as invalid.
	for (i = 0, tree = trees[i]; trees[i]; i++)
	{
		cur = ubi_trFirst(tree->root);
		while (cur)
		{
			next = ubi_trNext(cur);

			curNode = (bt_node *) cur;
			if (curNode->vnid == vnid || btIsAncestorNode(vnid, curNode))
				curNode->invalid = true;

			cur = next;
		}

		// Now loop through again, removing all invalid nodes.  This prevents removing
		// a parent node and all its children being orphaned (with invalid pointers
		// back to the destroyed parent).
		cur = ubi_trFirst(tree->root);
		while (cur)
		{
			next = ubi_trNext(cur);

			curNode = (bt_node *) cur;
			if (curNode->invalid)
			{
				ubi_trRemove(&vnidTree, curNode);
				free(curNode);
			}

			cur = next;
		}
	}

	endWriting(&handleData);
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

	beginReading(&handleData);

	node = btGetNodeFromVnid(vnid);
	if (node == NULL)
	{
		endReading(&handleData);
		return NULL;
	}

	nodeStack[0] = node;
	while ((node = node->parent) != NULL)
		nodeStack[stackSize++] = node;

	while (--stackSize >= 0)
	{
		strcat(path, nodeStack[stackSize]->name);
		if (stackSize)
		{
			int length = strlen(path);
			if (length > 0 && path[length - 1] != '\\')
				strcat(path, "\\");
		}
	}

	endReading(&handleData);
	return path;
}

bt_node *btFindNode(bt_node *parent, char *fileName)
{
	bt_node search, *node;

	// Initialize the node for tree searching purposes.
	search.parent = parent;
	search.name = strdup(fileName);
	if (search.name == NULL)
		return NULL;

	beginReading(&handleData);
	node = (bt_node *) ubi_trFind(&nameTree, &search);
	endReading(&handleData);

	free(search.name);
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

int btGetShareIdByPath(char *path)
{
	int i;

	for (i = 0; i < BT_MAX_FILE_SHARES; i++)
		if (fileShares[i].used)
			if (stricmp(fileShares[i].path, path) == 0)
				return i;

	return -1;
}

// btFindPrinter()
//
bt_printer *btFindPrinter(char *printerName)
{
	int i;

	for (i = 0; i < BT_MAX_PRINTER_SHARES; i++)
		if (sharedPrinters[i].used)
			if (stricmp(printerName, sharedPrinters[i].printerName) == 0)
				return &sharedPrinters[i];

	return NULL;
}

void btGetRootPath(vnode_id vnid, char *path)
{
	bt_node *curNode;

	beginReading(&handleData);

	curNode = btGetNodeFromVnid(vnid);
	while (curNode && curNode->parent)
		curNode = curNode->parent;

	if (curNode)
		strcpy(path, curNode->name);
	else
		path[0] = 0;

	endReading(&handleData);
}

uint32 btGetWinInode(const char *path)
{
	const int MAX_FOLDER_NESTING = 100;
	char *folders[MAX_FOLDER_NESTING + 1];
	char newPath[B_PATH_NAME_LENGTH + 1];
	int i, level, len, charSum;

	// Subdivide the path into its components.
	char *p, *s = strdup(path);
	level = 0;
	folders[level++] = s;
	for (p = s; *p; p++)
		if (*p == '\\')
		{
			folders[level++] = p + 1;
			*p = 0;
		}

	folders[level] = 0;

	// Now look through the folders, to see if any are . and .. references.
	for (i = 0; i < level; i++)
		if (*folders[i])
			if (strcmp(folders[i], ".") == 0)
				folders[i] = NULL;
			else if (strcmp(folders[i], "..") == 0)
			{
				int j = i;
				folders[i] = 0;
				while (j > 0 && folders[j] == NULL)
					j--;

				if (j >= 0)
					folders[j] = 0;
			}

	// Now reconstruct the path without the folders eliminated above.
	newPath[0] = 0;
	for (i = 0; i < level; i++)
		if (folders[i])
		{
			strcat(newPath, folders[i]);
			if (i < level - 1)
				strcat(newPath, "\\");
		}

	// If we eliminated folders at the end of the path, the level will have
	// resulted in a trailing backslash.
	len = strlen(newPath);
	if (newPath[len - 1] == '\\')
		newPath[--len] = 0;

	// Now compute the checksum, i.e., inode number, using the revised path.
	free(s);
	p = newPath;
	for (charSum = len = 0; *p; p++, len++)
		charSum += (*p * len);

	return ((len << 24) + charSum);
}

int btGetBeosStat(char *fileName, beos_stat *st)
{
	struct _stat _st;

	if (_stat(fileName, &_st) != 0)
		return BEOS_ENOENT;

	st->st_atime = _st.st_atime;
	st->st_blksize = 1024;
	st->st_ctime = _st.st_ctime;
	st->st_dev = 0;
	st->st_gid = 0;
	st->st_uid = 0;
	st->st_ino = btGetWinInode(fileName);
	st->st_mode = _st.st_mode;
	st->st_mtime = _st.st_mtime;
	st->st_nlink = 0;
	st->st_rdev = 0;
	st->st_size = _st.st_size;

	return B_OK;
}

void btMakePath(char *path, char *dir, char *file)
{
	int length;

	strcpy(path, dir);
	length = strlen(path);
	if (length > 0)
		if (path[length - 1] != '\\')
			strcat(path, "\\");

	strcat(path, file);
}

////////////////////////////////////////////////////////////////////

int btPreMount(bt_session_t *session, char *shareName)
{
	// Look for the specified share name.  If it can't be found, check to see if it's
	// a printer.  If not, then there is no such resource available on this host.
	int shareId = btGetShareId(shareName);
	if (shareId < 0)
	{
		bt_printer *printer = btFindPrinter(shareName);
		if (!printer)
			return BEOS_ENOENT;

		return printer->security;
	}

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
		return BEOS_ENOENT;

	if (fileShares[shareId].security != BT_AUTH_NONE)
	{
		// Authenticate the user with name/password.
		authenticated = authenticateUser(user, password);
		if (!authenticated)
			return BEOS_EACCES;

		// Does the authenticated user have any rights on this file share?
		session->rights = 0;
		for (ur = fileShares[shareId].rights; ur; ur = ur->next)
			if (!ur->isGroup && stricmp(ur->user, user) == 0)
				session->rights |= ur->rights;

		getUserGroups(user, groups);
		for (ur = fileShares[shareId].rights; ur; ur = ur->next)
			if (ur->isGroup)
				for (i = 0; i < MAX_GROUPS_PER_USER; i++)
					if (groups[i] && stricmp(ur->user, groups[i]) == 0)
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
			return BEOS_EACCES;

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
		return BEOS_ENOENT;

	if (!S_ISDIR(st.st_mode))
		return BEOS_EACCES;

	// Mark this session as owned by this user.
	strcpy(session->user, user);
	return B_OK;
}

int btGetFSInfo(fs_info *fsInfo, char *path)
{
	DWORD secsPerClstr, bytesPerSec, freeClstrs, totalClstrs;
	char rootDir[5];

	// We only want the root directory specification, not the entire path.
	strncpy(rootDir, path, 3);
	rootDir[3] = 0;

	if (GetDiskFreeSpace(rootDir, &secsPerClstr, &bytesPerSec, &freeClstrs, &totalClstrs))
	{
		fsInfo->block_size = bytesPerSec;
		fsInfo->total_blocks = secsPerClstr * totalClstrs;
		fsInfo->free_blocks = secsPerClstr * freeClstrs;
		return B_OK;
	}

	return ENOTSUP;
}

int btLookup(char *pathBuf, vnode_id dir_vnid, char *fileName, vnode_id *file_vnid)
{
	struct _stat st;
	if (_stat(fileName, &st) != 0)
		return BEOS_ENOENT;

	return B_OK;
}

int btStat(char *pathBuf, vnode_id vnid, beos_stat *st)
{
	char *fileName;
	int error;

	fileName = btGetLocalFileName(pathBuf, vnid);
	if (fileName)
	{
		error = btGetBeosStat(fileName, st);
		return error;
	}

	return BEOS_ENOENT;
}

int btReadDir(char *pathBuf, vnode_id dir_vnid, long **dir, vnode_id *file_vnid, char *filename, beos_stat *st)
{
	struct _finddata_t fileInfo;
	char *folder, path[B_PATH_NAME_LENGTH];
	long result = -1;

	if (dir_vnid == 0 || !file_vnid || !filename)
		return EINVAL;

	if (!*dir)
	{
		folder = btGetLocalFileName(pathBuf, dir_vnid);
		if (folder)
		{
			char search[B_PATH_NAME_LENGTH];
			btMakePath(search, folder, "*.*");
			result = _findfirst(search, &fileInfo);
			*dir = (long *) malloc(sizeof(long *));
			memcpy(*dir, &result, sizeof(result));
		}
	}
	else
		result = _findnext((long) **dir, &fileInfo);

	if (result != -1)
	{
		folder = btGetLocalFileName(pathBuf, dir_vnid);
		if (folder)
		{
			btMakePath(path, folder, fileInfo.name);
			if (btGetBeosStat(path, st) != 0)
				return BEOS_ENOENT;

			strcpy(filename, fileInfo.name);
			*file_vnid = (vnode_id) st->st_ino;
			btAddHandle(dir_vnid, *file_vnid, filename);
			return B_OK;
		}
	}
	else if (*dir)
	{
		_findclose((long) **dir);
		return BEOS_ENOENT;
	}

	return EINVAL;
}

int32 btRead(char *pathBuf, vnode_id vnid, beos_off_t pos, int32 len, char *buffer)
{
	char *path;
	int bytes;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		FILE *fp = fopen(path, "rb");
		if (!fp)
			return EACCES;

		fseek(fp, (int) pos, SEEK_SET);
		bytes = fread(buffer, 1, len, fp);
		fclose(fp);

		buffer[bytes] = 0;
		return bytes;
	}

	return 0;
}

int32 btWrite(bt_session_t *session, vnode_id vnid, beos_off_t pos, int32 len, int32 totalLen, char *buffer)
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

	WaitForSingleObject(session->blockSem, INFINITE);

	block = session->rootBlock;
	while (block && block->vnid != vnid)
		block = block->next;

	ReleaseSemaphore(session->blockSem, 1, NULL);
	return block;
}

// btInsertWriteBlock()
//
void btInsertWriteBlock(bt_session_t *session, bt_block *block)
{
	WaitForSingleObject(session->blockSem, INFINITE);

	block->next = session->rootBlock;
	block->prev = NULL;
	if (session->rootBlock)
		session->rootBlock->prev = block;

	session->rootBlock = block;

	ReleaseSemaphore(session->blockSem, 1, NULL);
}

int btCommit(bt_session_t *session, vnode_id vnid)
{
	bt_block *block;
	char *path;
	int file;

	// Get the full path for the specified file.
	path = btGetLocalFileName(session->pathBuffer, vnid);
	if (!path)
		return BEOS_ENOENT;

	// Obtain the buffered I/O block.  If one can't be found, no buffered I/O
	// session was started for this vnode.
	block = btGetWriteBlock(session, vnid);
	if (!block)
		return BEOS_ENOENT;

	// Open the file for writing.
	file = _open(path, _O_WRONLY | _O_CREAT | _O_BINARY);
	if (file < 0)
		return errno;

	WaitForSingleObject(session->blockSem, INFINITE);

	// Write the data.
	_lseek(file, (int32) block->pos, SEEK_SET);
	_write(file, block->buffer, block->len);

	btRemoveWriteBlock(session, block);
	ReleaseSemaphore(session->blockSem, 1, NULL);

	_close(file);
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

	// If there's a next block, it hsould now point to the current predecessor.
	if (block->next)
		block->next->prev = block->prev;

	// Release the memory used by this block.
	free(block->buffer);
	free(block);
}

int btCreate(char *pathBuf, vnode_id dir_vnid, char *name, int omode, int perms, vnode_id *file_vnid)
{
	beos_stat st;
	char path[B_PATH_NAME_LENGTH], *folder;
	int fh;

	folder = btGetLocalFileName(pathBuf, dir_vnid);
	if (folder)
	{
		btMakePath(path, folder, name);
		fh = open(path, O_WRONLY | O_CREAT | O_TRUNC, _S_IREAD | _S_IWRITE);
		if (fh == -1)
			return errno;
		else
		{
			close(fh);
			if (btGetBeosStat(path, &st) == 0)
			{
				*file_vnid = (vnode_id) st.st_ino;
				btAddHandle(dir_vnid, *file_vnid, name);
			}
			else
				return BEOS_EACCES;
		}
	}

	return B_OK;
}

int btTruncate(char *pathBuf, vnode_id vnid, int64 len)
{
	char *path;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		FILE *fp = fopen(path, "w");
		fclose(fp);
		return B_OK;
	}

	return BEOS_EACCES;
}

// btCreateDir()
//
int btCreateDir(char *pathBuf, vnode_id dir_vnid, char *name, int perms, vnode_id *file_vnid, beos_stat *st)
{
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, dir_vnid);
	if (folder)
	{
		btMakePath(path, folder, name);
		if (_mkdir(path) == -1)
			return errno;

		if (btGetBeosStat(path, st) != 0)
			return BEOS_ENOENT;

		*file_vnid = (vnode_id) st->st_ino;
		btAddHandle(dir_vnid, *file_vnid, name);
		return B_OK;
	}

	return BEOS_ENOENT;
}

// btDeleteDir()
//
int btDeleteDir(char *pathBuf, vnode_id vnid, char *name)
{
	struct _stat st;
	char path[B_PATH_NAME_LENGTH], *folder;

	folder = btGetLocalFileName(pathBuf, vnid);
	if (folder)
	{
		btMakePath(path, folder, name);
		if (_stat(path, &st) != 0)
			return errno;

		if (_rmdir(path) == -1)
			return errno;

		btPurgeNodes(btGetWinInode(path));
		return B_OK;
	}

	return BEOS_ENOENT;
}

// btRename()
//
int btRename(char *pathBuf, vnode_id old_vnid, char *oldName, vnode_id new_vnid, char *newName)
{
	struct _stat st;
	char oldPath[B_PATH_NAME_LENGTH], newPath[B_PATH_NAME_LENGTH], *oldFolder, *newFolder;

	oldFolder = btGetLocalFileName(pathBuf, old_vnid);
	if (oldFolder)
	{
		btMakePath(oldPath, oldFolder, oldName);

		newFolder = btGetLocalFileName(pathBuf, new_vnid);
		if (newFolder)
		{
			btMakePath(newPath, newFolder, newName);

			if (_stat(oldPath, &st) != 0)
				return errno;

			btPurgeNodes(btGetWinInode(oldPath));

			if (rename(oldPath, newPath) == -1)
				return errno;

			return B_OK;
		}
	}

	return BEOS_ENOENT;
}

// btUnlink()
//
int btUnlink(char *pathBuf, vnode_id vnid, char *name)
{
	struct _stat st;
	char path[B_PATH_NAME_LENGTH], *folder;
	int error;

	folder = btGetLocalFileName(pathBuf, vnid);
	if (folder)
	{
		btMakePath(path, folder, name);

		// Obtain the inode (vnid) of the specified file through stat().
		if (_stat(path, &st) != 0)
			return errno;

		// Construct a dummy file descriptor and cause it to be removed from
		// the list.
		btRemoveHandle(btGetWinInode(path));

		error = unlink(path);
		return (error == -1 ? errno : B_OK);
	}

	return BEOS_EACCES;
}

int btReadLink(char *pathBuf, vnode_id vnid, char *buffer, int length)
{
	return BEOS_ENOENT;
}

int btSymLink(char *pathBuf, vnode_id vnid, char *name, char *dest)
{
	return BEOS_ENOENT;
}

int btWStat(char *pathBuf, vnode_id vnid, long mask, int32 mode, int32 uid, int32 gid, int64 size, int32 atime, int32 mtime)
{
	struct _utimbuf ftimes;
	struct _stat st;
	char *path;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
//		if (mask & WSTAT_MODE)
//			chmod(path, mode);

//		if (mask & WSTAT_UID)
//			chown(path, uid, -1);

//		if (mask & WSTAT_GID)
//			chown(path, -1, gid);

//		if (mask & WSTAT_SIZE)
//			truncate(path, size);

		if (_stat(path, &st) == 0)
			if (mask & WSTAT_ATIME || mask & WSTAT_MTIME)
			{
				ftimes.actime = mask & WSTAT_ATIME ? atime : st.st_atime;
				ftimes.modtime = mask & WSTAT_MTIME ? mtime : st.st_mtime;
				_utime(path, &ftimes);
			}

		return B_OK;
	}

	return BEOS_ENOENT;
}

int btReadAttrib(char *pathBuf, vnode_id vnid, char *name, char *buffer)
{
	bt_mime_mapping *map;
	char *path;

	path = btGetLocalFileName(pathBuf, vnid);
	if (path)
	{
		char *ext = strrchr(path, '.');
		if (ext)
		{
			ext++;
			for (map = mimeMap; map; map = map->next)
				if (stricmp(ext, map->extension) == 0)
				{
					strcpy(buffer, map->mimeType);
					return (strlen(buffer));
				}
		}
	}

	return 0;
}

int btAuthenticate(char *resource, char *user, char *password)
{
	bt_user_rights *ur, *rootUr;
	bt_printer *printer;
	char *groups[MAX_GROUPS_PER_USER];
	int i, rights, share;
	bool authenticated = false;

	// Determine if the resource is a file share or a printer.
	share = btGetShareId(resource);
	if (share >= 0)
	{
		if (fileShares[share].security == BT_AUTH_NONE)
			return B_OK;

		rootUr = fileShares[share].rights;
	}
	else
	{
		printer = btFindPrinter(resource);
		if (printer)
			if (printer->security == BT_AUTH_NONE)
				return B_OK;

		rootUr = printer->rights;
	}

	// Authenticate the user with name/password.
	authenticated = authenticateUser(user, password);
	if (!authenticated)
		return BEOS_EACCES;

	// Does the authenticated user have any rights on this file share?
	rights = 0;
	for (ur = rootUr; ur; ur = ur->next)
		if (!ur->isGroup && stricmp(ur->user, user) == 0)
			rights |= ur->rights;

	// Does the authenticated user belong to any groups that have any rights on this
	// file share?
	for (i = 0; i < MAX_GROUPS_PER_USER; i++)
		groups[i] = NULL;

	getUserGroups(user, groups);
	for (ur = rootUr; ur; ur = ur->next)
		if (ur->isGroup)
			for (i = 0; i < MAX_GROUPS_PER_USER; i++)
				if (groups[i] && stricmp(ur->user, groups[i]) == 0)
				{
					rights |= ur->rights;
					break;
				}

	// Free the memory occupied by the group list.
	for (i = 0; i < MAX_GROUPS_PER_USER; i++)
		if (groups[i])
			free(groups[i]);

	// If no rights have been granted, deny access.
	if (!rights)
		return BEOS_EACCES;

	return B_OK;
}

////////////////////////////////////////////////////////////////////

void netbtPreMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, security;

	client = session->socket;
	security = btPreMount(session, argv[0].data);
	btRPCCreateAck(&packet, xid, security);
	btRPCSendAck(session->socket, &packet);
}

void netbtMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	vnode_id vnid;
	int client, error;
	char *shareName = argv[0].data;
	char *user = argv[1].data;
	char *password = argv[2].data;

	client = session->socket;

	error = btMount(session, shareName, user, password, &vnid);
	if (error == B_OK)
	{
		// Record this session having logged in to a specific share.
		session->share = btGetShareId(shareName);
		session->logon = time(NULL);

		// Now send the client a response with the root vnid;
		btRPCCreateAck(&packet, xid, error);
		btRPCPutInt64(&packet, vnid);
	}
	else
		btRPCCreateAck(&packet, xid, error);

	btRPCSendAck(client, &packet);
}

void netbtFSInfo(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	fs_info info;

	client = session->socket;

	error = btGetFSInfo(&info, fileShares[session->share].path);
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
}

void netbtLookup(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	beos_stat st;
	int client, error;
	vnode_id dir_vnid = *((vnode_id *) argv[0].data);
	vnode_id file_vnid;

	client = session->socket;

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
}

void netbtReadDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	const int MAX_FOLDER_ENTRIES = 128;
	bt_outPacket packet;
	beos_stat st;
	int client, error;
	vnode_id dir_vnid = *((vnode_id *) argv[0].data);
	vnode_id file_vnid;
	long *dir;
	char filename[B_PATH_NAME_LENGTH];
	int entries = 0;

	client = session->socket;

	dir = (long *)(*((int32 *) argv[1].data));
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

		if (++entries >= MAX_FOLDER_ENTRIES)
			break;

		error = btReadDir(session->pathBuffer, dir_vnid, &dir, &file_vnid, filename, &st);
		btRPCPutInt32(&packet, error);
	}

	// If we exhausted the list of directory entries without filling
	// the buffer, add an error message that will prevent the client
	// from requesting further entries.
	if (entries < MAX_FOLDER_ENTRIES)
		btRPCPutInt32(&packet, BEOS_ENOENT);

	btRPCSendAck(client, &packet);
}

void netbtStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	beos_stat info;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

	error = btStat(session->pathBuffer, vnid, &info);

	if (error == B_OK)
	{
		btRPCCreateAck(&packet, xid, error);
		btRPCPutStat(&packet, &info);
	}
	else
		btRPCCreateAck(&packet, xid, error);

	btRPCSendAck(client, &packet);
}

void netbtRead(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	beos_off_t pos = *((beos_off_t *) argv[1].data);
	int32 len = *((int32 *) argv[2].data);
	int32 bytes = 0;

	client = session->socket;

	session->ioBuffer[len] = 0;
	bytes = btRead(session->pathBuffer, vnid, pos, len, session->ioBuffer);

	btRPCCreateAck(&packet, xid, B_OK);
	btRPCPutString(&packet, session->ioBuffer, bytes);
	btRPCSendAck(client, &packet);
}

void netbtWrite(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	int client;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	beos_off_t pos = *((beos_off_t *) argv[1].data);
	int32 len = *((int32 *) argv[2].data);
	int32 totalLen = *((int32 *) argv[3].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (session->rights & BT_RIGHTS_WRITE)
		btWrite(session, vnid, pos, len, totalLen, argv[4].data);
}

void netbtCreate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	beos_stat st;
	int client, error;
	vnode_id dir_vnid = *((vnode_id *) argv[0].data);
	vnode_id file_vnid;
	int omode = *((int *) argv[2].data);
	int perms = *((int *) argv[3].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
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
}

void netbtTruncate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btTruncate(session->pathBuffer, vnid, *((int64 *) argv[1].data));
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtUnlink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btUnlink(session->pathBuffer, vnid, argv[1].data);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtRename(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id old_vnid = *((vnode_id *) argv[0].data);
	vnode_id new_vnid = *((vnode_id *) argv[2].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btRename(session->pathBuffer, old_vnid, argv[1].data, new_vnid, argv[3].data);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtCreateDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id dir_vnid = *((vnode_id *) argv[0].data);
	vnode_id file_vnid;
	beos_stat st;

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
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
}

void netbtDeleteDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btDeleteDir(session->pathBuffer, vnid, argv[1].data);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtReadLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtSymLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtWStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	int32 mask = *((int32 *) argv[1].data);
	int32 mode = *((int32 *) argv[2].data);
	int32 uid = *((int32 *) argv[3].data);
	int32 gid = *((int32 *) argv[4].data);
	int64 size = (int64) *((int32 *) argv[5].data);
	int32 atime = *((int32 *) argv[6].data);
	int32 mtime = *((int32 *) argv[7].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btWStat(session->pathBuffer, vnid, mask, mode, uid, gid, size, atime, mtime);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtReadAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesRead;

	client = session->socket;

	if (stricmp(argv[1].data, "BEOS:TYPE") == 0)
	{
		vnode_id vnid = *((vnode_id *) argv[0].data);
		bytesRead = btReadAttrib(session->pathBuffer, vnid, argv[1].data, session->attrBuffer);
		if (bytesRead > 0)
		{
			btRPCCreateAck(&packet, xid, B_OK);
			btRPCPutInt32(&packet, (int32) bytesRead + 1);
			btRPCPutString(&packet, session->attrBuffer, bytesRead + 1);
			btRPCSendAck(client, &packet);
			return;
		}
	}

	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtWriteAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtReadAttribDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtRemoveAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtStatAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesRead;

	client = session->socket;
	if (strcmp(argv[1].data, "BEOS:TYPE") == 0)
	{
		vnode_id vnid = *((vnode_id *) argv[0].data);
		bytesRead = btReadAttrib(session->pathBuffer, vnid, argv[1].data, session->attrBuffer);
		btRPCCreateAck(&packet, xid, B_OK);
		btRPCPutInt32(&packet, B_STRING_TYPE);
		btRPCPutInt64(&packet, bytesRead + 1);
		btRPCSendAck(client, &packet);
		return;
	}

	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtReadIndexDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtCreateIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtRemoveIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtStatIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtReadQuery(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, ENOTSUP);
	btRPCSendAck(client, &packet);
}

void netbtCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (!(session->rights & BT_RIGHTS_WRITE))
	{
		btRPCCreateAck(&packet, xid, BEOS_EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btCommit(session, vnid);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtPrintJobNew(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	char jobId[MAX_NAME_LENGTH];
	char *printerName = argv[0].data;
	char *user = argv[1].data;
	char *password = argv[2].data;
	char *jobName = argv[3].data;

	client = session->socket;

	error = btPrintJobNew(printerName, user, password, session->client_s_addr, jobName, jobId);
	btRPCCreateAck(&packet, xid, error);
	if (error == B_OK)
		btRPCPutString(&packet, jobId, strlen(jobId));

	btRPCSendAck(client, &packet);
}

void netbtPrintJobData(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	char *printerName = argv[0].data;
	char *jobId = argv[1].data;
	char *jobData = argv[2].data;
	int32 dataLen = *((int32 *) argv[3].data);

	client = session->socket;

	error = btPrintJobData(printerName, jobId, jobData, dataLen);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtPrintJobCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	char *printerName = argv[0].data;
	char *jobId = argv[1].data;

	client = session->socket;

	error = btPrintJobCommit(printerName, jobId);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtAuthenticate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	char *resource = argv[0].data;
	char *user = argv[1].data;
	char *password = argv[2].data;

	client = session->socket;

	error = btAuthenticate(resource, user, password);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, B_OK);
	btRPCSendAck(client, &packet);
}

/*
bool IsValidUser(char *user, char *domain, char *password)
{
	HANDLE hUser, hTok;
	OSVERSIONINFO osInfo;
	bool authenticated;

	authenticated = false;

	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osInfo))
		if (osInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
			return true;

	hTok = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hTok))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValue(0, SE_TCB_NAME, &tp.Privileges[0].Luid))
			if (AdjustTokenPrivileges(hTok, FALSE, &tp, 0, NULL, NULL))
				if (LogonUser(user, domain, password, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hUser))
				{
					CloseHandle(hUser);
					authenticated = true;
				}

		CloseHandle(hTok);
	}

	return authenticated;
}
*/

void loadFolder(const char *path)
{
	struct _finddata_t fileInfo;
	struct _stat st;
	char *search;
	long dir, result;
	uint32 dir_vnid;

	search = (char *) malloc(MAX_PATH);
	if (search == NULL)
		return;

	dir_vnid = btGetWinInode(path);
	sprintf(search, "%s\\*.*", path);

	dir = result = _findfirst(search, &fileInfo);
	while (result != -1)
	{
		btMakePath(search, (char *) path, fileInfo.name);
		btAddHandle(dir_vnid, btGetWinInode(search), fileInfo.name);

		_stat(search, &st);
		if (st.st_mode & _S_IFDIR)
			if (strcmp(fileInfo.name, ".") && strcmp(fileInfo.name, ".."))
				loadFolder(search);

		result = _findnext(dir, &fileInfo);
	}

	_findclose(dir);
	free(search);
}



////////

void KillNode(ubi_trNodePtr node)
{
	bt_node *bn = (bt_node *) node;
	free(bn->name);
	free(node);
}

int CompareVnidNodes(ubi_trItemPtr item, ubi_trNodePtr node)
{
	vnode_id vnid1 = *((vnode_id *) item);
	vnode_id vnid2 = ((bt_node *) node)->vnid;
	if (vnid1 < vnid2)
		return -1;
	else if (vnid1 > vnid2)
		return 1;
	else
		return 0;
}

int CompareNameNodes(ubi_trItemPtr item, ubi_trNodePtr node)
{
	bt_node *node1 = (bt_node *) item;
	bt_node *node2 = (bt_node *) node;

	if (node1->parent < node2->parent)
		return -1;
	else if (node1->parent > node2->parent)
		return 1;
	else
		return strcmp(node1->name, node2->name);
}
