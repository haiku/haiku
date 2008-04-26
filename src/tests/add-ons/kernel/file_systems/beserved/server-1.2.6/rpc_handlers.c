#include "betalk.h"
#include "sessions.h"
#include "rpc_handlers.h"
#include "rpc_workers.h"
#include "file_shares.h"

extern bt_fileShare_t fileShares[];


void netbtPreMount(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, security;

	client = session->socket;
	security = btPreMount(session, argv[0].data);
	btRPCCreateAck(&packet, xid, security);
	btRPCSendAck(client, &packet);
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

		// Now send the client a response with the root vnid.
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
	fs_info info;
	int client, error;

	client = session->socket;

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
}

void netbtLookup(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
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
	bt_outPacket packet;
	struct stat st;
	int client, error;
	vnode_id dir_vnid = *((vnode_id *) argv[0].data);
	vnode_id file_vnid;
	DIR *dir;
	char filename[B_PATH_NAME_LENGTH];
	int entries = 0;

	client = session->socket;

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
}

void netbtStat(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	struct stat info;
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
	off_t pos = *((off_t *) argv[1].data);
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
	vnode_id vnid = *((vnode_id *) argv[0].data);
	off_t pos = *((off_t *) argv[1].data);
	int32 len = *((int32 *) argv[2].data);
	int32 totalLen = *((int32 *) argv[3].data);

	// If the file share this user is connected to is read-only, the command
	// cannot be honored.
	if (session->rights & BT_RIGHTS_WRITE)
		btWrite(session, vnid, pos, len, totalLen, argv[4].data);
}

void netbtCreate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	struct stat st;
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
		btRPCCreateAck(&packet, xid, EACCES);
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
		btRPCCreateAck(&packet, xid, EACCES);
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
		btRPCCreateAck(&packet, xid, EACCES);
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
	struct stat st;

	client = session->socket;

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
		btRPCCreateAck(&packet, xid, EACCES);
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
	int client, error;
	char path[B_PATH_NAME_LENGTH];
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

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
}

void netbtSymLink(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

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
		btRPCCreateAck(&packet, xid, EACCES);
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
	char *buffer;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	int32 type = *((int32 *) argv[2].data);
	int32 pos = *((int32 *) argv[3].data);
	int32 len = *((int32 *) argv[4].data);

	client = session->socket;

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
}

void netbtWriteAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, bytesWritten;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	int32 type = *((int32 *) argv[2].data);
	int32 pos = *((int32 *) argv[4].data);
	int32 len = *((int32 *) argv[5].data);

	client = session->socket;

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
}

void netbtReadAttribDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	DIR *dir = (DIR *)(*((int32 *) argv[1].data));
	char attrName[100];
	int entries = 0;

	client = session->socket;

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
}

void netbtRemoveAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);

	client = session->socket;

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
}

void netbtStatAttrib(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	vnode_id vnid = *((vnode_id *) argv[0].data);
	struct attr_info info;

	client = session->socket;

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
}

void netbtReadIndexDir(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	DIR *dir;
	char indexName[100];

	client = session->socket;

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
}

void netbtCreateIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	int type = *((int32 *) argv[1].data);
	int flags = *((int32 *) argv[2].data);

	client = session->socket;

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
}

void netbtRemoveIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;

	client = session->socket;

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
}

void netbtStatIndex(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	struct index_info info;

	client = session->socket;

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
}

void netbtReadQuery(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client, error;
	DIR *dir;
	char fileName[B_PATH_NAME_LENGTH];
	vnode_id vnid, parent;

	client = session->socket;

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
}

// netbtCommit()
//
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
		btRPCCreateAck(&packet, xid, EACCES);
		btRPCSendAck(client, &packet);
		return;
	}

	error = btCommit(session, vnid);
	btRPCCreateAck(&packet, xid, error);
	btRPCSendAck(client, &packet);
}

void netbtPrintJobNew(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
}

void netbtPrintJobData(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
}

void netbtPrintJobCommit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
}

void netbtAuthenticate(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
}

// netbtQuit()
//
void netbtQuit(bt_session_t *session, unsigned int xid, int argc, bt_arg_t argv[])
{
	bt_outPacket packet;
	int client;

	client = session->socket;
	btRPCCreateAck(&packet, xid, B_OK);
	btRPCSendAck(client, &packet);
}
