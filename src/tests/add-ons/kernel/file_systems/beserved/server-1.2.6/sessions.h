#ifndef _SESSIONS_H_
#define _SESSIONS_H_

#include "betalk.h"
#include "fsproto.h"

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

typedef struct session
{
	int socket;
	unsigned int client_s_addr;
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

	struct session *next;
} bt_session_t;

#endif
