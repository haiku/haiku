#ifndef _BETALK_H_
#define _BETALK_H_

// BeOS-specific includes
#include "TypeConstants.h"
#include "Errors.h"
#include "OS.h"
#include "fs_attr.h"
#include "fs_query.h"
#include "fs_index.h"
#include "Mime.h"
#include "netdb.h"

// POSIX includes
#include "stdio.h"
#include "dirent.h"
#include "malloc.h"
#include "string.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "errno.h"
#include "socket.h"
#include "ctype.h"

#ifndef NULL
#define NULL			0L
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET		(int)(~0)
#endif

#define BT_TCPIP_PORT		9092
#define BT_QUERYHOST_PORT	9093
#define BT_BESURE_PORT		9094

#define BT_CMD_TERMINATOR			13
#define BT_CMD_PREMOUNT				0
#define BT_CMD_MOUNT				1
#define BT_CMD_FSINFO				2
#define BT_CMD_LOOKUP				3
#define BT_CMD_STAT					4
#define BT_CMD_READDIR				5
#define BT_CMD_READ					6
#define BT_CMD_WRITE				7
#define BT_CMD_CREATE				8
#define BT_CMD_TRUNCATE				9
#define BT_CMD_MKDIR				10
#define BT_CMD_RMDIR				11
#define BT_CMD_RENAME				12
#define BT_CMD_UNLINK				13
#define BT_CMD_READLINK				14
#define BT_CMD_SYMLINK				15
#define BT_CMD_WSTAT				16
#define BT_CMD_READATTRIB			50
#define BT_CMD_WRITEATTRIB			51
#define BT_CMD_READATTRIBDIR		52
#define BT_CMD_REMOVEATTRIB			53
#define BT_CMD_STATATTRIB			54
#define BT_CMD_READINDEXDIR			60
#define BT_CMD_CREATEINDEX			61
#define BT_CMD_REMOVEINDEX			62
#define BT_CMD_STATINDEX			63
#define BT_CMD_READQUERY			70
#define BT_CMD_COMMIT				80
#define BT_CMD_PRINTJOB_NEW			200
#define BT_CMD_PRINTJOB_DATA		201
#define BT_CMD_PRINTJOB_COMMIT		202
#define BT_CMD_AUTHENTICATE			210
#define BT_CMD_QUIT					255

#define BT_CMD_AUTH					1
#define BT_CMD_READUSERS			2
#define BT_CMD_READGROUPS			3
#define BT_CMD_WHICHGROUPS			4

#define BT_REQ_HOST_PROBE			1
#define BT_REQ_SHARE_PROBE			2
#define BT_REQ_HOST_INFO			3
#define BT_REQ_HOST_USERS			4
#define BT_REQ_AUTH_TYPES			5

// The different types of network resources supported by BeServed.
#define BT_SHARED_NULL				0
#define BT_SHARED_FOLDER			1
#define BT_SHARED_PRINTER			2

#define BT_PRINTER_PCL3				0
#define BT_PRINTER_POSTSCRIPT		1
#define BT_PRITNER_INKJET			2

#define BT_AUTH_REQ_CONNECT			1
#define BT_AUTH_REQ_USERS			2

#define BT_AUTH_NONE				0
#define BT_AUTH_BESURE				1

#define BT_RIGHTS_READ				0x00000001
#define BT_RIGHTS_WRITE				0x00000002
#define BT_RIGHTS_PRINT				0x00000004

#define BT_RPC_SIGNATURE		"btRPC"
#define BT_RPC_VERSION_HI		0
#define BT_RPC_VERSION_LO		1

#define BT_MAX_IO_BUFFER		8192
#define BT_MAX_ATTR_BUFFER		256
#define BT_RPC_MIN_PACKET_SIZE	128
#define BT_RPC_MAX_PACKET_SIZE	(BT_MAX_IO_BUFFER + 1024)

#define BT_TOKEN_SHARE			1
#define BT_TOKEN_AS				2
#define BT_TOKEN_SET			3
#define BT_TOKEN_READ			4
#define BT_TOKEN_WRITE			5
#define BT_TOKEN_READWRITE		6
#define BT_TOKEN_PROMISCUOUS	7
#define BT_TOKEN_ON				8
#define BT_TOKEN_TO				9
#define BT_TOKEN_AUTHENTICATE	10
#define BT_TOKEN_WITH			11
#define BT_TOKEN_GROUP			12
#define BT_TOKEN_PRINTER		13
#define BT_TOKEN_PRINT			14
#define BT_TOKEN_IS				15
#define BT_TOKEN_SPOOLED		16
#define BT_TOKEN_DEVICE			17
#define BT_TOKEN_TYPE			18
#define BT_TOKEN_COMMA			200
#define BT_TOKEN_QUOTE			201
#define BT_TOKEN_STRING			202
#define BT_TOKEN_NUMBER			203
#define BT_TOKEN_ERROR			255

#define isValid(c)				((c)=='.' || (c)=='_' || (c)=='-' || (c)=='/' || (c)=='\\' || (c)==':' || (c)=='&' || (c)=='\'')

#define MAX_COMMAND_ARGS		10
#define MAX_NAME_LENGTH			32
#define MAX_KEY_LENGTH			MAX_NAME_LENGTH
#define MAX_USERNAME_LENGTH		MAX_NAME_LENGTH
#define MAX_GROUPNAME_LENGTH	MAX_NAME_LENGTH
#define BT_AUTH_TOKEN_LENGTH	(B_FILE_NAME_LENGTH + MAX_USERNAME_LENGTH)
#define MAX_DESC_LENGTH			64
#define MAX_GROUPS_PER_USER		80

typedef struct
{
	char signature[6];
	uint8 command;
	char share[MAX_NAME_LENGTH + 1];
} bt_request;

typedef struct
{
	uint32 type;
	uint32 subType;
	char name[B_FILE_NAME_LENGTH + 1];
} bt_resource;

typedef struct
{
	char system[B_FILE_NAME_LENGTH];
	char beServed[B_FILE_NAME_LENGTH];
	char platform[B_FILE_NAME_LENGTH];
	int cpus;
	int connections;
	int maxConnections;
} bt_hostinfo;

typedef struct
{
	unsigned int blockSize;
	unsigned int totalBlocks;
	unsigned int freeBlocks;
} bt_fsinfo;

typedef struct
{
	unsigned int size;
	unsigned int length;
	char *buffer;
} bt_outPacket;

typedef struct
{
	unsigned int length;
	unsigned int offset;
	char *buffer;
} bt_inPacket;

typedef struct rpcCall
{
	unsigned int nsid;
	unsigned int xid;
	sem_id sem;
	bt_inPacket *inPacket;
	bool finished;
	struct rpcCall *next;
	struct rpcCall *prev;
} bt_rpccall;

typedef struct rpcInfo
{
	thread_id rpcThread;
	int32 quitXID;

	uint32 serverIP;
	int32 serverPort;
	int s;
} bt_rpcinfo;

#define BT_COOKIE_SIZE			4

typedef struct btCookie
{
	char opaque[BT_COOKIE_SIZE];
	bt_inPacket inPacket;
	bool lpbCache;
	bool eof;
} btCookie;

typedef struct btQueryCookie
{
	char opaque[BT_COOKIE_SIZE];
	char *query;
} btQueryCookie;

// RPC Operations
unsigned char	btRPCGetChar(bt_inPacket *packet);
unsigned int	btRPCGetInt32(bt_inPacket *packet);
int64			btRPCGetInt64(bt_inPacket *packet);
char *			btRPCGetNewString(bt_inPacket *packet);
int				btRPCGetString(bt_inPacket *packet, char *buffer, int length);
int				btRPCGetStat(bt_inPacket *packet, struct stat *st);
bt_outPacket *	btRPCPutHeader(unsigned char command, unsigned char argc, int32 length);
void			btRPCPutArg(bt_outPacket *packet, unsigned int type, void *data, int length);
void			btRPCPutChar(bt_outPacket *packet, char value);
void			btRPCPutInt32(bt_outPacket *packet, int32 value);
void			btRPCPutInt64(bt_outPacket *packet, int64 value);
void			btRPCPutString(bt_outPacket *packet, char *buffer, int length);
void			btRPCPutBinary(bt_outPacket *packet, void *buffer, int length);
void			btRPCPutStat(bt_outPacket *packet, struct stat *st);

void btRPCCreateAck(bt_outPacket *packet, unsigned int xid, int error);
void btRPCSendAck(int client, bt_outPacket *packet);

int btRecv(int sock, void *data, int dataLen, int flags);
int btSend(int sock, void *data, int dataLen, int flags);
int btRecvMsg(int sock, void *data, int dataLen, int flags);
int btSendMsg(int sock, void *data, int dataLen, int flags);

#endif
