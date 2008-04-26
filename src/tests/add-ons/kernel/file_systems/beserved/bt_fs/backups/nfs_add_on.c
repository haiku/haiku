#define _BUILDING_fs 1

#include "betalk.h"
#include "nfs_add_on.h"

#include "ksocket.h"
#include <errno.h>
#include <string.h>
#include <KernelExport.h>
#include <sys/stat.h>
#include <dirent.h>
#include <SupportDefs.h>

_EXPORT vnode_ops fs_entry =
{
	(op_read_vnode *) &fs_read_vnode,
	(op_write_vnode *) &fs_write_vnode,
	(op_remove_vnode *) &fs_remove_vnode,
	(op_secure_vnode *) &fs_secure_vnode,
	(op_walk *) &fs_walk,
	(op_access *) &fs_access,
	(op_create *) &fs_create,
	(op_mkdir *) &fs_mkdir,
	(op_symlink *) &fs_symlink,
	NULL, //	&fs_link,
	(op_rename *) &fs_rename,
	(op_unlink *) &fs_unlink,
	(op_rmdir *) &fs_rmdir,
	(op_readlink *) &fs_readlink,
	(op_opendir *) &fs_opendir,
	(op_closedir *) &fs_closedir,
	(op_free_cookie *) &fs_free_dircookie,
	(op_rewinddir *) &fs_rewinddir,
	(op_readdir *) &fs_readdir,
	(op_open *) &fs_open,
	(op_close *) &fs_close,
	(op_free_cookie *) &fs_free_cookie,
	(op_read *) &fs_read,
	(op_write *) &fs_write,
	NULL, //	&fs_readv
	NULL, //	&fs_writev
	NULL, //	&fs_ioctl,
	NULL, //	&fs_setflags,
	(op_rstat *) &fs_rstat,
	(op_wstat *) &fs_wstat,
	NULL, //	&fs_fsync,
	NULL, //	&fs_initialize,
	(op_mount *) &fs_mount,
	(op_unmount *) &fs_unmount,
	NULL, //	&fs_sync,
	(op_rfsstat *) &fs_rfsstat,
	(op_wfsstat *) &fs_wfsstat,
	NULL, //	&fs_select
	NULL, //	&fs_deselect
	NULL, // (op_open_indexdir *) &fs_open_indexdir,
	NULL, // (op_close_indexdir *) &fs_close_indexdir,
	NULL, // (op_free_cookie *) &fs_free_indexdircookie,
	NULL, // (op_rewind_indexdir *) &fs_rewind_indexdir,
	NULL, // (op_read_indexdir *) &fs_read_indexdir,
	NULL, // (op_create_index *) &fsCreateIndex,
	NULL, // (op_remove_index *) &fsRemoveIndex,
	NULL, //	&fs_rename_index,
	NULL, // (op_stat_index *) &fsStatIndex,
	(op_open_attrdir *) &fs_open_attribdir,
	(op_close_attrdir *) &fs_close_attribdir,
	(op_free_cookie *) &fs_free_attribdircookie,
	(op_rewind_attrdir *) &fs_rewind_attribdir,
	(op_read_attrdir *) &fs_read_attribdir,
	(op_write_attr *) &fs_write_attrib,
	(op_read_attr *) &fs_read_attrib,
	(op_remove_attr *) &fs_remove_attrib,
	NULL, //	&fs_rename_attrib,
	(op_stat_attr *) &fs_stat_attrib,
	(op_open_query *) &fsOpenQuery,
	(op_close_query *) &fsCloseQuery,
	(op_free_cookie *) &fsFreeQueryCookie,
	(op_read_query *) &fsReadQuery	
};

_EXPORT int32 api_version = B_CUR_FS_API_VERSION;


extern btFileHandle handle_from_vnid(fs_nspace *ns, vnode_id vnid)
{
	fs_node *current;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	current = ns->first;
	while (current && current->vnid != vnid)
		current = current->next;

	release_sem(ns->sem);

	return current->fhandle;
}

btFileHandle *create_node(fs_nspace *ns, btFileHandle *dir, btFileHandle *file, char *name, vnode_id vnid)
{
	fs_node *newNode = (fs_node *) malloc(sizeof(fs_node));
	if (newNode)
	{
//		newNode->dhandle = *dir;
		newNode->fhandle = *file;
//		strcpy(newNode->name, name);
		newNode->vnid = vnid;

		insert_node(ns, newNode);
	}
	return newNode;
}

void insert_node(fs_nspace *ns, fs_node *node)
{
	fs_node *current;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	current = ns->first;
	while (current && current->vnid != node->vnid)
		current = current->next;

	if (current)
		free(node);
	else
	{
		node->next = ns->first;
		ns->first = node;
	}

	release_sem(ns->sem);
}

void remove_node(fs_nspace *ns, vnode_id vnid)
{
	fs_node *current;
	fs_node *previous;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);	

	current = ns->first;
	previous = NULL;

	while (current && current->vnid != vnid)
	{
		previous = current;
		current = current->next;
	}

	if (current)
	{
		if (previous)
			previous->next = current->next;
		else
			ns->first = current->next;

		free(current);
	}

	release_sem(ns->sem);
}

// fs_read_vnode()
//
// Description:  Allocate resources required to load the specified vnid.  Also returns
// a reference to the vnode via the supplied pointer.
//
// Parameters:
// ns (in):		Private file system structure
// vnid (in):	vnid of the vnode to load
// r (in):		??
// node (out):	A reference to the vnode loaded
//
// Returns:
// B_OK if everything is successful, or
// EINVAL if the specified vnid is not found
//
extern int fs_read_vnode(fs_nspace *ns, vnode_id vnid, char r, fs_node **node)
{
	fs_node *current;

	*node = NULL;

	// If we need to acquire the semaphone to search the list, do so now.
	if (!r)
		while (acquire_sem(ns->sem) == B_INTERRUPTED);

	current = ns->first;
	while (current && current->vnid != vnid)
		current = current->next;

	// TODO: shouldn't we release the semaphore here?
	// If the specified parameter is non-null, then return the node we found.
	if (current && node)
		*node = current;

	// Release the semaphore if necessary.
	if (!r)
		release_sem(ns->sem);

	return (current ? B_OK : EINVAL);	
}

// fs_write_vnode()
//
// Description:  Free resources allocated with the sister function fs_read_vnode().
// Parameters:
// ns (in):		Private file system structure
// node (in):	Node that can be freed
// r (in):		??
// Returns:
// B_OK if everything is successful (always in our case)
//
extern int fs_write_vnode(fs_nspace *ns, fs_node *node, char r)
{
	// In our case, there aren't any resources to free, so there isn't anything
	// for this function to do but return successfully.
	return B_OK;
}

// fs_walk()
//
// Description:  This function is the single file system location where a path is
// traversed by name.  Given a directory handle and a file name, this function returns
// the vnid of the file if it indeed exists.
//
// Parameters:
// ns (in):		Private file system structure
// base (in):	File handle of the directory to look in
// file (in):	Name of the file to look for
// newpath (out):	Name of the real file if the given file is a symbolic link
// vnid (out):	Vnid of the file given by "file"
//
// Returns:
// B_OK if everything is successful,
// Otherwise an error code detailing the problem
//
extern int fs_walk(fs_nspace *ns, fs_node *base, const char *file, char **newpath, vnode_id *vnid)
{
	bool isLink;
	fs_node *dummy;
	status_t result;

	// If the file specified is the current directory, then that parameter provides
	// the necessary vnid.  Also note that the current directory entry cannot be a
	// symbolic link.
	if (!strcmp(".", file))
	{
		*vnid = base->vnid;
		isLink = false;
	}
	else
	{
		btFileHandle fhandle;
		struct stat st;
		status_t result;

 		// Look up the requested entry, getting the stat structure and file handle
		// filled in.
		if ((result = btLookup(ns->s, &base->fhandle, file, &fhandle, &st)) != B_OK)
			return result;

		// The vnid is the resulting file's inode.  Once this is established, insert
		// the new node in our list.
		if (create_node(ns, NULL, &fhandle, NULL, st.st_ino) == NULL)
			return ENOMEM;

		*vnid = st.st_ino;
		isLink = S_ISLNK(st.st_mode);
	}

	if ((result = get_vnode(ns->nsid, *vnid, (void **) &dummy)) < B_OK)
		return result;

	// If we've found a symbolic link, and the supplied character pointer is non-null,
	// then the vnode layer is looking for the actual path.
	if (isLink && newpath)
	{
		char path[B_PATH_NAME_LENGTH + 1];
		size_t pathLen = B_PATH_NAME_LENGTH;

		if ((result = fs_readlink(ns, dummy, path, &pathLen)) < B_OK)
		{
			put_vnode(ns->nsid, *vnid);
			return result;
		}

		path[pathLen] = 0;
		result = new_path(path, newpath);

		if (result < B_OK)
		{
			put_vnode(ns->nsid, *vnid);
			return result;
		}

		return put_vnode(ns->nsid, *vnid);
	}

	return B_OK;
}

// fs_opendir()
//
extern int fs_opendir(fs_nspace *ns, fs_node *node, btCookie **cookie)
{
	struct stat st;
	btFileHandle fhandle;
	status_t result;

	// Do a basic stat() on this file to verify we indeed have a directory.
	// If we cannot obtain this information, we have an even bigger problem.
	if ((result = btStat(ns->s, &node->fhandle, &st)) < B_OK)
		return result;

	// Now that we have the right information, verify we're looking at a directory.
	if (!S_ISDIR(st.st_mode))
		return ENOTDIR;

	// Allocate and initialize the cookie.
	*cookie = (btCookie *) malloc(sizeof(btCookie));
	if (!*cookie)
		return ENOMEM;

	memset((*cookie)->opaque, 0, BT_COOKIE_SIZE);
	(*cookie)->lpbCache = false;
	(*cookie)->eof = false;

	return B_OK;
}

// fs_closedir()
//
extern int fs_closedir(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
	btEmptyLPBCache(cookie);
	return B_OK;
}

// fs_rewinddir()
//
extern int fs_rewinddir(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
	memset(cookie->opaque, 0, BT_COOKIE_SIZE);
	cookie->eof = false;
	btEmptyLPBCache(cookie);
	return B_OK;
}

// fs_readdir()
//
extern int fs_readdir(fs_nspace *ns, fs_node *node, btCookie *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	btFileHandle fhandle;
	struct stat st;
	status_t result;
	vnode_id vnid;
	char *filename;
	long max, bufContent;
	int32 value;

	max = *num;
	*num = 0;

	// Cause the directory to be read.
	while ((result = btReadDir(ns->s, &node->fhandle, &vnid, &filename, cookie, &fhandle, &st)) == B_OK)
	{
		if (strcmp(filename, ".") && strcmp(filename, ".."))
		{
			fs_node *newNode = (fs_node *) malloc(sizeof(fs_node));
			newNode->vnid = vnid;
			memcpy(&newNode->fhandle, &fhandle, BT_FILE_HANDLE_SIZE);

			insert_node (ns, newNode);

			bufContent = 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(filename) + 1;
			if (bufsize < bufContent)
				return B_OK;

			buf->d_dev = ns->nsid;
			buf->d_pdev = ns->nsid;
			buf->d_ino = vnid;
			buf->d_pino = node->vnid;
			buf->d_reclen = bufContent;
			strcpy(buf->d_name, filename);

			bufsize -= bufContent;
			buf = (struct dirent *)((char *) buf + bufContent);

			(*num)++;
		}

		free(filename);

		if (*num == max)
			return B_OK;
	}

	return B_OK;
}

// fs_free_dircookie()
//
extern int fs_free_dircookie(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
	if (cookie)
	{
		btEmptyLPBCache(cookie);
		free(cookie);
	}

	return B_OK;
}

// fs_rstat()
//
extern int fs_rstat(fs_nspace *ns, fs_node *node, struct stat *st)
{
	btFileHandle fhandle;
	status_t result;

	// This was btGetAttr(), but isn't that just btStat()?
	if ((result = btStat(ns->s, &node->fhandle, st)) < B_OK)
		return result;

	st->st_dev = ns->nsid;
	return B_OK;
}

// fs_mount()
//
extern int fs_mount(nspace_id nsid, const char *devname, ulong flags, struct mount_bt_params *parms, size_t len, fs_nspace **data, vnode_id *vnid)
{
	status_t result;
	fs_nspace *ns;
	fs_node *rootNode;
	struct stat st;

	// If we didn't receive any mount parameters, we won't know the server to connect to,
	// or the folder to be shared.
	if (parms == NULL)
		return EINVAL;

	// Initialize the ksocket library for kernel-based network communication.
	if ((result = ksocket_init()) < B_OK)
		return result;

	// Allocate our private file system information block.
	ns = (fs_nspace *) malloc(sizeof(fs_nspace));
	ns->nsid = nsid;

	// Copy over the parameters specified, including the server, folder, user and group
	// IDs, and so on.
	ns->params.serverIP = parms->serverIP;
	ns->params.server = strdup(parms->server);
	ns->params.export = strdup(parms->export);
	ns->params.uid = parms->uid;
	ns->params.gid = parms->gid;
	ns->params.hostname = strdup(parms->hostname);
	ns->params.folder = strdup(parms->folder);

	// Now connect to the remote server.  The socket returned will be used for all future
	// communication with that server.
	ns->s = btConnect(ns->params.serverIP, BT_TCPIP_PORT);
	if (ns->s != INVALID_SOCKET)
	{
		ns->xid=0;

		btRPCInit(ns);

		// Create a semaphore for exclusive access to the vnode list when we need to scan
		// for a particular vnode.
		ns->sem = create_sem(1, "bt_sem");
		if (ns->sem > 0)
		{
			// The system should own this semaphore.
			set_sem_owner(ns->sem, B_SYSTEM_TEAM);

			// Allocate the root node.
			rootNode = (fs_node *) malloc(sizeof(fs_node));
			rootNode->next = NULL;

			// Let the server know we are mounting the exported folder.  This operation
			// will return the root node file handle.
			result = btMount(ns->s, ns->params.export, &rootNode->fhandle);
			if (result == B_OK)
			{
				// This was btGetAttr()
				result = btStat(ns->s, &rootNode->fhandle, &st);
				if (result == B_OK)
				{
					ns->rootid = st.st_ino;
					rootNode->vnid = ns->rootid;
					*vnid = ns->rootid;

					result = new_vnode(nsid, *vnid, rootNode);
					if (result == B_OK)
					{
						*data = ns;
						ns->first = rootNode;
						notify_listener(B_DEVICE_MOUNTED, ns->nsid, 0, 0, 0, NULL);
						return B_OK;
					}
				}
			}

			// We've failed.  The first step is to free the root node we allocated, and
			// release the semaphore we created as well.
			free(rootNode);
			delete_sem(ns->sem);
		}

		// Disconnect from the server.
		btDisconnect(ns->s);
		btRPCClose();
	}
	else
		result = EHOSTUNREACH;

	// De-allocate our private file system structure and all the string space we duplicated
	// on startup.
	free(ns->params.hostname);
	free(ns->params.export);
	free(ns->params.server);
	free(ns);

	ksocket_cleanup();
	return result;		
}

extern int fs_unmount(fs_nspace *ns)
{
	notify_listener(B_DEVICE_UNMOUNTED, ns->nsid, 0, 0, 0, NULL);

	btDisconnect(ns->s);
	btRPCClose();

	free(ns->params.hostname);
	free(ns->params.export);
	free(ns->params.server);

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	while (ns->first)
	{
		fs_node *next = ns->first->next;
		free(ns->first);
		ns->first = next;
	}

	release_sem(ns->sem);
	delete_sem(ns->sem);

	ns->s = INVALID_SOCKET;
	free(ns);
	ksocket_cleanup();

	return B_OK;
}

// fs_rfsstat()
//
extern int fs_rfsstat(fs_nspace *ns, struct fs_info *info)
{
	btFileHandle rootHandle = handle_from_vnid(ns, ns->rootid);
	bt_fsinfo fsinfo;
	int result;

	result = btGetFSInfo(ns->s, &rootHandle, &fsinfo);
	if (result == B_OK)
	{
		info->dev = ns->nsid;
		info->root = ns->rootid;
		info->flags = B_FS_IS_SHARED | B_FS_IS_PERSISTENT |
			B_FS_HAS_MIME | B_FS_HAS_ATTR | B_FS_HAS_QUERY;
		info->block_size = fsinfo.blockSize;
		info->io_size = 8192;
		info->total_blocks = fsinfo.totalBlocks;
		info->free_blocks = fsinfo.freeBlocks;
		info->total_nodes = 100;
		info->free_nodes = 100;
		strcpy(info->device_name, "BeServed_Share");
		strcpy(info->volume_name, ns->params.folder);
		strcpy(info->fsh_name, "bt_fsh");
	}

	return result;
}

// fs_open()
//
extern int fs_open(fs_nspace *ns, fs_node *node, int omode, fs_file_cookie **cookie)
{
	struct stat st;
	btFileHandle fhandle;
	status_t result;

	// This was btGetAttr()
	if ((result = btStat(ns->s, &node->fhandle, &st)) < B_OK)
		return result;

	if (S_ISDIR(st.st_mode))
	{
		*cookie = NULL;
		return B_OK; // permit opening of directories
	}

	*cookie = (fs_file_cookie *) malloc(sizeof(fs_file_cookie));
	(*cookie)->omode = omode;
	(*cookie)->original_size = st.st_size;
	(*cookie)->st = st;

	return B_OK;
}

// fs_close()
//
extern int fs_close(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie)
{
	if (cookie && (cookie->omode & O_RDWR || cookie->omode & O_WRONLY))
		return notify_listener(B_STAT_CHANGED, ns->nsid, 0, 0, node->vnid, NULL);

	return B_OK;
}

// fs_free_cookie()
//
extern int fs_free_cookie(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie)
{
	if (cookie)
		free(cookie);

	return B_OK;
}

// fs_read()
//
extern int fs_read(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, void *buf, size_t *len)
{
	if (!len)
		return B_ERROR;

	// If we weren't given proper file system or vnode pointers, something is awry.
	if (!ns || !node || !buf)
		return B_ERROR;

	// Do not permit reading directories.
	if (!cookie)
		return EISDIR;

	*len = btRead(ns->s, &node->fhandle, pos, *len, buf);

	return B_OK;
}

extern int fs_write(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, const void *buf, size_t *len)
{
	// Do not permit writing directories.
	if (!cookie)
		return EISDIR;

	if (cookie->omode & O_APPEND)
		pos += cookie->original_size;

	*len = btWrite(ns->s, &node->fhandle, pos, *len, buf);
	btStat(ns->s, &node->fhandle, &cookie->st);

	return B_OK;
}

extern int fs_wstat(fs_nspace *ns, fs_node *node, struct stat *st, long mask)
{
	int error = btWStat(ns->s, &node->fhandle, st, mask);
	if (error != B_OK)
		return error;

	return notify_listener(B_STAT_CHANGED, ns->nsid, 0, 0, node->vnid, NULL);
}

extern int fs_wfsstat(fs_nspace *ns, struct fs_info *info, long mask)
{
	return B_OK;
}

extern int fs_create(fs_nspace *ns, fs_node *dir, const char *name, int omode, int perms, vnode_id *vnid,
					fs_file_cookie **cookie)
{
	btFileHandle fhandle;
	struct stat st;
	status_t result;
	int error;

	result = btLookup(ns->s, &dir->fhandle, name, &fhandle, &st);

	if (result == B_OK)
	{
		void *dummy;
		fs_node *newNode = (fs_node *) malloc(sizeof(fs_node));
		newNode->fhandle = fhandle;
		newNode->vnid = st.st_ino;
		insert_node(ns, newNode);

		*vnid = st.st_ino;

		if ((result = get_vnode(ns->nsid, *vnid, &dummy)) < B_OK)
			return result;

		if (S_ISDIR(st.st_mode))
			return EISDIR;

		if (omode & O_EXCL)
			return EEXIST;

		if (omode & O_TRUNC)
			if ((result = btTruncate(ns->s, &fhandle, 0L)) < B_OK)
				return result;

		*cookie = (fs_file_cookie *) malloc(sizeof(fs_file_cookie));
		(*cookie)->omode = omode;
		(*cookie)->original_size = st.st_size;
		(*cookie)->st = st;

		return B_OK;
	}
	else if (result != ENOENT)
		return result;
	else
	{
		uint8 *replyBuf;
		int32 status;
		fs_node *newNode;

		// If the file doesn't exist, it may have previously and another user has
		// deleted it.  If so, our vnode layer will still have the old vnid registered
		// and will panic when it gets a new one.  Let's make sure the old one, if
		// any, is unregistered.

		if (!(omode & O_CREAT))
			return ENOENT;

		error = btCreate(ns->s, &dir->fhandle, name, &fhandle, omode, perms, &st);
		if (error != B_OK)
			return error;

		newNode = (fs_node *) malloc(sizeof(fs_node));
		newNode->fhandle = fhandle;
		newNode->vnid = st.st_ino;
		insert_node(ns, newNode);

		*vnid = st.st_ino;
		*cookie = (fs_file_cookie *) malloc(sizeof(fs_file_cookie));
		(*cookie)->omode = omode;
		(*cookie)->original_size = st.st_size;
		(*cookie)->st = st;

		result = new_vnode(ns->nsid, *vnid, newNode);
		if (result < B_OK)
			return result;

		return notify_listener(B_ENTRY_CREATED, ns->nsid, dir->vnid, 0, *vnid, name);
	}
}

extern int fs_unlink(fs_nspace *ns, fs_node *dir, const char *name)
{
	fs_node *newNode;
	fs_node *dummy;
	struct stat st;
	btFileHandle fhandle;
	int error;

	error = btLookup(ns->s, &dir->fhandle, name, &fhandle, &st);
	if (error != B_OK)
		return error;

	newNode = (fs_node *) malloc(sizeof(fs_node));
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	insert_node(ns, newNode);

	error = get_vnode(ns->nsid, st.st_ino, (void **) &dummy);
	if (error != B_OK)
		return error;

	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
		return EISDIR;

	error = remove_vnode(ns->nsid, st.st_ino);
	if (error != B_OK)
		return error;

	error = put_vnode(ns->nsid, st.st_ino);
	if (error != B_OK)
		return error;

	error = btUnlink(ns->s, &dir->fhandle, name);
	if (error != B_OK)
		return error;

	return notify_listener(B_ENTRY_REMOVED, ns->nsid, dir->vnid, 0, st.st_ino, name);
}

extern int fs_remove_vnode(fs_nspace *ns, fs_node *node, char r)
{
	remove_node(ns, node->vnid);
	return B_OK;
}

// fs_secure_vnode()
//
// Description:  Given a node, this function determines whether the user has the
// required permission to access it.  This is currently unimplemented in BFS, and
// we consequently dismiss it here as well.
//
extern int fs_secure_vnode(fs_nspace *ns, fs_node *node)
{
	return B_OK;
}

extern int fs_mkdir(fs_nspace *ns, fs_node *dir, const char *name, int perms)
{
	btFileHandle dhandle, fhandle;
	fs_node *newNode;
	struct stat st;
	int error;

	error = btLookup(ns->s, &dir->fhandle, name, &dhandle, &st);
	if (error == B_OK)
	{
		void *dummy;
		error = get_vnode(ns->nsid, st.st_ino, &dummy);
		if (error != B_OK)
			return error;

		return EEXIST;
	}
	else if (error != ENOENT)
		return error;

	error = btCreateDir(ns->s, &dir->fhandle, name, perms | S_IFDIR, &fhandle, &st);
	if (error != B_OK)
		return error;

	newNode = (fs_node *) malloc(sizeof(fs_node));
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	insert_node(ns, newNode);

	return notify_listener(B_ENTRY_CREATED, ns->nsid, dir->vnid, 0, st.st_ino, name);
}

extern int fs_rename(fs_nspace *ns, fs_node *olddir, const char *oldname, fs_node *newdir, const char *newname)
{
	btFileHandle fhandle;
	struct stat st;
	int error;

	error = btLookup(ns->s, &newdir->fhandle, newname, &fhandle, &st);
	if (error == B_OK)
	{
		if (S_ISREG(st.st_mode))
			error = fs_unlink(ns, newdir, newname);
		else
			error = fs_rmdir(ns, newdir, newname);

		if (error != B_OK)
			return error;
	}

	error = btLookup(ns->s, &olddir->fhandle, oldname, &fhandle, &st);
	if (error != B_OK)
		return error;

	error = btRename(ns->s, &olddir->fhandle, oldname, &newdir->fhandle, newname);
	if (error != B_OK)
		return error;

	return notify_listener(B_ENTRY_MOVED, ns->nsid, olddir->vnid, newdir->vnid, st.st_ino, newname);
}

extern int fs_rmdir(fs_nspace *ns, fs_node *dir, const char *name)
{
	fs_node *newNode;
	fs_node *dummy;		
	btFileHandle fhandle;
	struct stat st;
	int error;

	error = btLookup(ns->s, &dir->fhandle, name, &fhandle, &st);
	if (error != B_OK)
		return error;

	newNode = (fs_node *) malloc(sizeof(fs_node));
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	insert_node(ns, newNode);

	error = get_vnode(ns->nsid, st.st_ino, (void **) &dummy);
	if (error != B_OK)
		return error;

	if (!S_ISDIR(st.st_mode))
		return ENOTDIR;

	error = remove_vnode(ns->nsid, st.st_ino);
	if (error != B_OK)
		return error;

	error = put_vnode(ns->nsid, st.st_ino);
	if (error != B_OK)
		return error;

	error = btDeleteDir(ns->s, &dir->fhandle, name);
	if (error != B_OK)
		return error;

	return notify_listener(B_ENTRY_REMOVED, ns->nsid, dir->vnid, 0, st.st_ino, name);
}

extern int fs_readlink(fs_nspace *ns, fs_node *node, char *buf, size_t *bufsize)
{
	return btReadLink(ns->s, &node->fhandle, buf, bufsize);
}

extern int fs_symlink(fs_nspace *ns, fs_node *dir, const char *name, const char *path)
{
	btFileHandle fhandle;
	fs_node *newNode;
	struct stat st;
	int error;

	error = btLookup(ns->s, &dir->fhandle, name, &fhandle, &st);
	if (error == B_OK)
	{
		void *dummy;
		if ((error = get_vnode(ns->nsid, st.st_ino, &dummy)) < B_OK)
			return error;

		return EEXIST;
	}
	else if (error != ENOENT)
		return error;

	error = btSymLink(ns->s, &dir->fhandle, name, path);
	if (error != B_OK)
		return error;

	error = btLookup(ns->s, &dir->fhandle, name, &fhandle, &st);
	if (error != B_OK)
		return error;

	newNode = (fs_node *) malloc(sizeof(fs_node));
	newNode->fhandle = fhandle;
	newNode->vnid = st.st_ino;
	insert_node(ns, newNode);

	error = notify_listener(B_ENTRY_CREATED, ns->nsid, dir->vnid, 0, st.st_ino, name);
	return error;
}

// fs_access()
//
// Description:  This function implements the access() system call.
//
int	fs_access(void *ns, void *node, int mode)
{
	return B_OK;
}

int fs_read_attrib(fs_nspace *ns, fs_node *node, const char *name, int type, void *buf, size_t *len, off_t pos)
{
	int bytes;

	// Automatically deny reading Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	bytes = btReadAttrib(ns->s, &node->fhandle, name, type, buf, *len, pos);
	if (bytes == -1)
		return -1;

	*len = bytes;
	return B_OK;
}

int	fs_write_attrib(fs_nspace *ns, fs_node *node, const char *name, int type, const void *buf, size_t *len, off_t pos)
{
	int bytes;

	// Automatically deny reading Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	bytes = btWriteAttrib(ns->s, &node->fhandle, name, type, buf, *len, pos);
	if (bytes == -1)
		return -1;

	*len = bytes;
	notify_listener(B_ATTR_CHANGED, ns->nsid, 0, 0, node->vnid, NULL);
	return B_OK;
}

// fs_open_attribdir()
//
extern int fs_open_attribdir(fs_nspace *ns, fs_node *node, btCookie **cookie)
{
	struct stat st;
	int error;

	// Do a basic stat() on this file to verify we indeed have a valid file.
	// If we cannot obtain this information, we have an even bigger problem.
	if ((error = btStat(ns->s, &node->fhandle, &st)) < B_OK)
		return error;

	// Allocate and initialize the cookie.
	*cookie = (btCookie *) malloc(sizeof(btCookie));
	if (!*cookie)
		return ENOMEM;

	memset((*cookie)->opaque, 0, BT_COOKIE_SIZE);
	(*cookie)->lpbCache = false;
	(*cookie)->eof = false;

	return B_OK;
}

// fs_close_attribdir()
//
extern int fs_close_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
	btEmptyLPBCache(cookie);
	return B_OK;
}

// fs_rewind_attribdir()
//
extern int fs_rewind_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
	if (cookie)
	{
		memset(cookie->opaque, 0, BT_COOKIE_SIZE);
		btEmptyLPBCache(cookie);
		cookie->eof = false;
	}

	return B_OK;
}

extern int fs_read_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	status_t result;
	char *attrName;
	long max;

	max = *num;
	*num = 0;

	// Cause the directory to be read.
	while ((result = btReadAttribDir(ns->s, &node->fhandle, &attrName, cookie)) == B_OK)
	{
		if (strncmp(attrName, "_trk/", 5) != 0)
		{
			if (bufsize < 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(attrName) + 1)
				return B_OK;

			buf->d_dev = 0;
			buf->d_pdev = 0;
			buf->d_ino = 0;
			buf->d_pino = 0;
			buf->d_reclen = 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(attrName) + 1;
			strcpy(buf->d_name, attrName);

			bufsize -= buf->d_reclen;
			buf = (struct dirent *)((char *) buf + buf->d_reclen);

			(*num)++;
		}

		free(attrName);

		if (*num == max)
			return B_OK;
	}

	return B_OK;
}

// fs_free_attribdircookie()
//
extern int fs_free_attribdircookie(fs_nspace *ns, btCookie *cookie)
{
//	if (cookie)
//	{
//		btEmptyLPBCache(cookie);
//		free(cookie);
//	}

	return B_OK;
}

// fs_remove_attrib()
//
extern int fs_remove_attrib(fs_nspace *ns, fs_node *node, const char *name)
{
	// Automatically deny removing Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	return btRemoveAttrib(ns->s, &node->fhandle, name);
}

// fs_stat_attrib()
//
extern int fs_stat_attrib(fs_nspace *ns, fs_node *node, const char *name, struct attr_info *buf)
{
	buf->type = 0;
	buf->size = 0;

	// Automatically deny reading Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	return btStatAttrib(ns->s, &node->fhandle, name, buf);
}

// fs_open_indexdir()
//
extern int fs_open_indexdir(fs_nspace *ns, btCookie **cookie)
{
	// Allocate and initialize the cookie.
	*cookie = (btCookie *) malloc(sizeof(btCookie));
	memset ((*cookie)->opaque, 0, BT_COOKIE_SIZE);
	(*cookie)->eof = false;

	return B_OK;
}

// fs_close_indexdir()
//
extern int fs_close_indexdir(fs_nspace *ns, btCookie *cookie)
{
	return B_OK;
}

// fs_rewind_indexdir()
//
extern int fs_rewind_indexdir(fs_nspace *ns, btCookie *cookie)
{
	memset(cookie->opaque, 0, BT_COOKIE_SIZE);
	cookie->eof = false;
	return B_OK;
}

// fs_read_indexdir()
//
extern int fs_read_indexdir(fs_nspace *ns, btCookie *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	status_t result;
	char indexName[100];
	long max;

	max = *num;
	*num = 0;

	// Cause the directory to be read.
	while ((result = btReadIndexDir(ns->s, indexName, sizeof(indexName) - 1, cookie)) == B_OK)
	{
		if (bufsize < 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(indexName) + 1)
			return B_OK;

		buf->d_dev = 0;
		buf->d_pdev = 0;
		buf->d_ino = 0;
		buf->d_pino = 0;
		buf->d_reclen = 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(indexName) + 1;
		strcpy(buf->d_name, indexName);

		bufsize -= buf->d_reclen;
		buf = (struct dirent *)((char *) buf + buf->d_reclen);

		(*num)++;

		if (*num == max)
			return B_OK;
	}

	return B_OK;
}

// fs_free_indexdircookie()
//
extern int fs_free_indexdircookie(fs_nspace *ns, btCookie *cookie)
{
	if (cookie)
		free(cookie);

	return B_OK;
}

extern int fsCreateIndex(fs_nspace *ns, const char *name, int type, int flags)
{
	return btCreateIndex(ns->s, name, type, flags);
}

extern int fsRemoveIndex(fs_nspace *ns, const char *name)
{
	return btRemoveIndex(ns->s, name);
}

extern int fsStatIndex(fs_nspace *ns, const char *name, struct index_info *buf)
{
	buf->type = 0;
	buf->size = 0;
	buf->modification_time = 0;
	buf->creation_time = 0;
	buf->uid = 0;
	buf->gid = 0;

	return btStatIndex(ns->s, name, buf);
}

extern int fsOpenQuery(fs_nspace *ns, const char *query, ulong flags, port_id port, long token, btQueryCookie **cookie)
{
	// Allocate and initialize the cookie.
	*cookie = (btQueryCookie *) malloc(sizeof(btQueryCookie));
	memset ((*cookie)->opaque, 0, BT_COOKIE_SIZE);
	(*cookie)->query = strdup(query);

	return B_OK;
}

extern int fsCloseQuery(fs_nspace *ns, btQueryCookie *cookie)
{
	return B_OK;
}

extern int fsReadQuery(fs_nspace *ns, btQueryCookie *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	char *filename;
	vnode_id vnid;
	long max, bufContent;

	max = *num;
	*num = 0;

	while (btReadQuery(ns->s, &filename, &vnid, cookie) == B_OK)
	{
		bufContent = 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(filename) + 1;
		if (bufsize < bufContent)
			return B_OK;

		buf->d_dev = ns->nsid;
		buf->d_pdev = ns->nsid;
		buf->d_ino = vnid;
		buf->d_pino = vnid;
		buf->d_reclen = bufContent;
		strcpy(buf->d_name, filename);

		bufsize -= bufContent;
		buf = (struct dirent *)((char *) buf + bufContent);

		free(filename);

		(*num)++;
		if (*num == max)
			return B_OK;
	}

	return B_OK;
}

extern int fsFreeQueryCookie(fs_nspace *ns, btQueryCookie *cookie)
{
	if (cookie)
	{
		if (cookie->query)
			free(cookie->query);

		free(cookie);
	}

	return B_OK;
}
