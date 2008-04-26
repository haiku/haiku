#include "betalk.h"
#include "sessions.h"
#include "file_shares.h"
#include "rpc_workers.h"
#include "authentication.h"
#include "readerWriter.h"
#include "fsproto.h"

#include "utime.h"

extern bt_fileShare_t fileShares[];
extern bt_node *rootNode;
extern bt_managed_data handleData;


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
	curNode = btGetNodeFromVnid(file_vnid);
	if (curNode)
		if (curNode->parent == dirNode && strcmp(curNode->name, name) == 0)
		{
			endWriting(&handleData);
			return;
		}

	// Allocate a new node.
	curNode = (bt_node *) malloc(sizeof(bt_node));
	if (curNode == NULL)
	{
		endWriting(&handleData);
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

	endWriting(&handleData);
}

// btRemoveHandle()
//
void btRemoveHandle(vnode_id vnid)
{
	bt_node *deadNode;

	beginWriting(&handleData);

	// Obtain the node in question.  If no such node exists, then we
	// probably have a bad handle.
	deadNode = btGetNodeFromVnid(vnid);
	btRemoveNode(deadNode);

	endWriting(&handleData);
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

	beginWriting(&handleData);

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
			strcat(path, "/");
	}

	endReading(&handleData);
	return path;
}

bt_node *btFindNode(bt_node *parent, char *fileName)
{
	bt_node *node;

	beginReading(&handleData);

	node = rootNode;
	while (node)
	{
		if (node->parent == parent)
			if (strcmp(node->name, fileName) == 0)
				break;

		node = node->next;
	}

	endReading(&handleData);
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

	beginReading(&handleData);
	dnode = btGetNodeFromVnid(dir_vnid);
	endReading(&handleData);

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
