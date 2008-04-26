#ifndef _RPC_WORKERS_
#define _RPC_WORKERS_

#include "betalk.h"
#include "sessions.h"

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

#endif
