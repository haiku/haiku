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

#define BT_TCPIP_PORT		8000
#define BT_QUERYHOST_PORT	8001

#define BT_CMD_TERMINATOR			13
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
#define BT_CMD_QUIT					255

#define BT_RPC_SIGNATURE		"btRPC"
#define BT_RPC_VERSION_HI		0
#define BT_RPC_VERSION_LO		1

#define BT_MAX_IO_BUFFER	16384
#define BT_RPC_MIN_PACKET_SIZE	64
#define BT_RPC_MAX_PACKET_SIZE	(BT_MAX_IO_BUFFER + 1024)

#define MAX_COMMAND_ARGS		10
#define MAX_ENTRY_VERSIONS		10

typedef struct
{
	unsigned int blockSize;
	unsigned int totalBlocks;
	unsigned int freeBlocks;
} bt_fsinfo;

#define BT_COOKIE_SIZE			4
#define BT_QUERY_COOKIE_SIZE	4
#define BT_FILE_HANDLE_SIZE		32

typedef struct btFileHandle
{
	char opaque[BT_FILE_HANDLE_SIZE];
} btFileHandle;

typedef struct btCookie
{
	char opaque[BT_COOKIE_SIZE];
} btCookie;

typedef struct btQueryCookie
{
	char opaque[BT_COOKIE_SIZE];
	char *query;
} btQueryCookie;

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
	unsigned int xid;
	sem_id sem;
	bt_inPacket *inPacket;
	struct rpcCall *next;
	struct rpcCall *prev;
} bt_rpccall;

// RPC Operations
unsigned int	btRPCGetInt32(bt_inPacket *packet);
int64			btRPCGetInt64(bt_inPacket *packet);
char *			btRPCGetNewString(bt_inPacket *packet);
int				btRPCGetString(bt_inPacket *packet, char *buffer, int length);
int				btRPCGetHandle(bt_inPacket *packet, btFileHandle *fhandle);
int				btRPCGetStat(bt_inPacket *packet, struct stat *st);
bt_outPacket *	btRPCPutHeader(unsigned char command, unsigned char argc);
void			btRPCPutArg(bt_outPacket *packet, unsigned int type, void *data, int length);
void			btRPCPutChar(bt_outPacket *packet, char value);
void			btRPCPutInt32(bt_outPacket *packet, int32 value);
void			btRPCPutInt64(bt_outPacket *packet, int64 value);
void			btRPCPutString(bt_outPacket *packet, char *buffer, int length);
void			btRPCPutBinary(bt_outPacket *packet, void *buffer, int length);
void			btRPCPutHandle(bt_outPacket *packet, btFileHandle *fhandle);
void			btRPCPutStat(bt_outPacket *packet, struct stat *st);

bt_rpccall *	btRPCInvoke(int session, bt_outPacket *packet);
bt_outPacket *btRPCCreateAck(unsigned int xid, int error, int length);
void btRPCSendAck(int client, bt_outPacket *packet);

#endif
