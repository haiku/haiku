#ifndef _RPC_HANDLERS_H_
#define _RPC_HANDLERS_H_

#include "betalk.h"
#include "sessions.h"

typedef void (*bt_net_func)(bt_session_t *, unsigned int, int, bt_arg_t *);

typedef struct dirCommand
{
	unsigned char command;
	bt_net_func handler;
	bool supported;
	uint8 args;
	uint32 argTypes[MAX_COMMAND_ARGS];
} bt_command_t;


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

#endif
