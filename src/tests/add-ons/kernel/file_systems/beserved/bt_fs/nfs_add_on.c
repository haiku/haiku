#define _BUILDING_fs 1

#include "betalk.h"
#include "nfs_add_on.h"

#ifndef BONE_VERSION
#include "ksocket.h"
#else
#include <sys/socket_module.h>
#endif

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
	(op_open_indexdir *) &fs_open_indexdir,
	(op_close_indexdir *) &fs_close_indexdir,
	(op_free_cookie *) &fs_free_indexdircookie,
	(op_rewind_indexdir *) &fs_rewind_indexdir,
	(op_read_indexdir *) &fs_read_indexdir,
	(op_create_index *) &fsCreateIndex,
	(op_remove_index *) &fsRemoveIndex,
	NULL, //	&fs_rename_index,
	(op_stat_index *) &fsStatIndex,
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

#ifdef BONE_VERSION

struct bone_socket_info *bone_module = NULL;

// bone_init()
//
bool bone_init()
{
	// Get a pointer to the BONE module.
	get_module(BONE_SOCKET_MODULE, (module_info **) &bone_module);
	return (bone_module != NULL);
}

// bone_cleanup()
//
void bone_cleanup()
{
	put_module(BONE_SOCKET_MODULE);
}
#endif

fs_node *create_node(fs_nspace *ns, vnode_id dir_vnid, const char *name, vnode_id file_vnid, bool newVnid)
{
	fs_node *newNode, *dupNode;
	bool obsolete_nodes = false;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	// Check to see if we find a node with this vnid.  If we do, we need to know if it
	// is the same file.  This can be determined by examining the parent directory vnid
	// and the filename.  If everything matches, just return that node.
	dupNode = getDuplicateVnid(ns, file_vnid);
	if (dupNode)
	{
		// If it looks like this vnid, smells like this vnid, and tastes like this vnid,
		// it must be this vnid.  Alright, there's one exception -- symbolic links.  These
		// have the same vnid, but another name.
		if (dupNode->parent == dir_vnid)
			if (strcmp(dupNode->name, name) == 0)
			{
				release_sem(ns->sem);
				return dupNode;
			}

		// If we still found a vnid, but the parent directory vnid or filename don't
		// match, then the original file is gone and another file has taken this vnid.
		// Another user caused this change, which isn't registered with the local
		// system's vnid table.  In this case we adjust our internal node, and notify
		// any listeners that the file has been renamed.  Note that if this is the user
		// that renamed the file, a duplicate notify_listener() call will be made by
		// fs_rename(), but this seems to have no adverse effect.
		dprintf("DUP VNID:  %s, %s\n", name, dupNode->name);
//		dprintf("notify_listener(B_ENTRY_MOVED, %s, %lu); -- duplicate vnid notice\n", name, file_vnid);
//		notify_listener(B_ENTRY_MOVED, ns->nsid, dupNode->parent, dir_vnid, file_vnid, name);
//		dupNode->parent = dir_vnid;
//		strcpy(dupNode->name, name);
//		release_sem(ns->sem);
//		return dupNode;
	}

	// Well, we now know we have a unique vnid.  However, it is possible that the same
	// file exists in our list under a different vnid.  If a file is deleted, then re-created
	// by another user and assigned a different vnid, then our existing vnid is obsolete
	// and we must abandon it in favor of the replacement.  Are we having fun yet?
	dupNode = getDuplicateName(ns, dir_vnid, name);
	if (dupNode)
		if (dupNode->vnid != file_vnid)
		{
			dprintf("DUP NAME:  %s (%lu)\n", dupNode->name, dupNode->vnid);
			remove_obsolete_node(ns, dupNode);
			obsolete_nodes = true;
		}

	newNode = (fs_node *) malloc(sizeof(fs_node));
	if (newNode)
	{
		// Initialize the members of this new node.
		newNode->parent = dir_vnid;
		strcpy(newNode->name, name);
		newNode->vnid = file_vnid;

		// Link the node in the list by making it the head.
		newNode->next = ns->first;
		ns->first = newNode;

		// If we're creating a new file, or we just had to delete our old vnid-cookie pair
		// above, then we need to register this vnid with the newly created node.
		if (newVnid || obsolete_nodes)
			new_vnode(ns->nsid, file_vnid, newNode);

		dprintf("NEW NODE:  %s (%lu)\n", name, file_vnid);
//		print_vnode_list(ns);
	}

	release_sem(ns->sem);

	if (obsolete_nodes)
		notify_listener(B_ENTRY_CREATED, ns->nsid, dir_vnid, 0, file_vnid, name);

	return newNode;
}

void print_vnode_list(fs_nspace *ns)
{
	fs_node *current;

	dprintf("    --- VNODE LIST --------------------------\n");
	current = ns->first;
	while (current)
	{
		dprintf("        %-35s   %lu\n", current->name, current->vnid);
		current = current->next;
	}
	dprintf("    --- END OF LIST -------------------------\n");
}

void remove_obsolete_node(fs_nspace *ns, fs_node *node)
{
	release_sem(ns->sem);

	remove_node(ns, node->vnid);

	dprintf("    Notifying others that %s (%lu) was removed\n", node->name, node->vnid);
	remove_vnode(ns->nsid, node->vnid);
	put_vnode(ns->nsid, node->vnid);
	notify_listener(B_ENTRY_REMOVED, ns->nsid, node->parent, 0, node->vnid, node->name);

	while (acquire_sem(ns->sem) == B_INTERRUPTED);
}

void rename_node(fs_nspace *ns, vnode_id old_dir_vnid, const char *oldname, vnode_id new_dir_vnid, const char *newname, vnode_id file_vnid)
{
	fs_node *current;

	while (acquire_sem(ns->sem) == B_INTERRUPTED);

	// Check to see if we find a node with this vnid.  If we do, we need to know if it
	// is the same file.  This can be determined by examining the parent directory vnid
	// and the filename.  If everything matches, rename the node to the new name.
	current = ns->first;
	while (current)
	{
		if (current->vnid == file_vnid)
			if (current->parent == old_dir_vnid)
				if (strcmp(current->name, oldname) == 0)
				{
					strcpy(current->name, newname);
					current->parent = new_dir_vnid;
					break;
				}

		current = current->next;
	}

	release_sem(ns->sem);
}

fs_node *getDuplicateVnid(fs_nspace *ns, vnode_id vnid)
{
	fs_node *current;

	current = ns->first;
	while (current && current->vnid != vnid)
		current = current->next;

	return current;
}

fs_node *getDuplicateName(fs_nspace *ns, vnode_id parent, const char *name)
{
	fs_node *current;

	current = ns->first;
	while (current)
	{
		if (current->parent == parent)
			if (strcmp(current->name, name) == 0)
				break;

		current = current->next;
	}

	return current;
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

//dprintf("fs_read_vnode()\n");
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
//dprintf("fs_write_vnode()\n");
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
	void *dummy;
	status_t result;

//dprintf("fs_walk()\n");
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
		struct stat st;
		status_t result;

 		// Look up the requested entry, getting the stat structure and file handle
		// filled in.
		if ((result = btLookup(ns, base->vnid, file, vnid, &st)) != B_OK)
			return result;

		// The vnid is the resulting file's inode.  Once this is established, insert
		// the new node in our list.
		if (create_node(ns, base->vnid, file, *vnid, false) == NULL)
			return ENOMEM;

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
	status_t result;

//dprintf("fs_opendir()\n");
	// Do a basic stat() on this file to verify we indeed have a directory.
	// If we cannot obtain this information, we have an even bigger problem.
	if ((result = btStat(ns, node->vnid, &st)) < B_OK)
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
//dprintf("fs_closedir()\n");
	btEmptyLPBCache(cookie);
	return B_OK;
}

// fs_rewinddir()
//
extern int fs_rewinddir(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
//dprintf("fs_rewinddir()\n");
	memset(cookie->opaque, 0, BT_COOKIE_SIZE);
	cookie->eof = false;
	btEmptyLPBCache(cookie);
	return B_OK;
}

// fs_readdir()
//
extern int fs_readdir(fs_nspace *ns, fs_node *node, btCookie *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	status_t result;
	vnode_id vnid;
	char *filename;
	long max, bufContent;
	int32 value;

//dprintf("fs_readdir()\n");
	max = *num;
	*num = 0;

	// Cause the directory to be read.
	while ((result = btReadDir(ns, node->vnid, &vnid, &filename, cookie)) == B_OK)
	{
		if (strcmp(filename, ".") && strcmp(filename, ".."))
		{
			create_node(ns, node->vnid, filename, vnid, false);

//			dirCount = bufsize / sizeof(struct dirent);
//			pktCount = BT_MAX_IO_BUFFER / sizeof(struct dirent);
//			dirCount = min((), ());

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
//dprintf("fs_free_dircookie()\n");
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
	status_t result;

//dprintf("fs_rstat()\n");
	if ((result = btStat(ns, node->vnid, st)) < B_OK)
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

	result = EINVAL;

dprintf("fs_mount running\n");
	// If we didn't receive any mount parameters, we won't know the server to connect to,
	// or the folder to be shared.
	if (parms == NULL)
		return EINVAL;

	// Initialize the ksocket library for kernel-based network communication.
#ifndef BONE_VERSION
	if ((result = ksocket_init()) < B_OK)
		return result;
#else
	if (!bone_init())
		return ENETUNREACH;
#endif

dprintf("net initialized\n");
	// Allocate our private file system information block.
	ns = (fs_nspace *) malloc(sizeof(fs_nspace));
	ns->nsid = nsid;
	ns->dnlcRoot = NULL;

	// Copy over the parameters specified, including the server, folder, user and group
	// IDs, and so on.
	ns->params.serverIP = parms->serverIP;
	ns->params.server = strdup(parms->server);
	ns->params.export = strdup(parms->export);
	ns->params.uid = parms->uid;
	ns->params.gid = parms->gid;
	ns->params.hostname = strdup(parms->hostname);
	ns->params.folder = strdup(parms->folder);

	// The password is a binary token that is of fixed length, but can contain null characters.
	// A strdup() therefore won't capture the entire string.  Manually copying the data is the
	// only reliable way.
	strcpy(ns->params.user, parms->user);
	memcpy(ns->params.password, parms->password, BT_AUTH_TOKEN_LENGTH);
	ns->params.password[BT_AUTH_TOKEN_LENGTH] = 0;

	// Now connect to the remote server.  The socket returned will be used for all future
	// communication with that server.
	ns->s = btConnect(ns, ns->params.serverIP, BT_TCPIP_PORT);
	if (ns->s != INVALID_SOCKET)
	{
		dprintf("connection established\n");
		ns->xid = 0;

		btRPCInit(ns);

		// Create a semaphore for exclusive access to the vnode list when we need to scan
		// for a particular vnode.
		ns->sem = create_sem(1, "VNode List Semaphore");
		if (ns->sem > 0)
		{
			dprintf("vnode semaphore created\n");
			if (initManagedData(&ns->dnlcData))
			{
				dprintf("managed data struct initialized\n");
				// The system should own this semaphore.
				set_sem_owner(ns->sem, B_SYSTEM_TEAM);

				// Allocate the root node.
				rootNode = (fs_node *) malloc(sizeof(fs_node));
				rootNode->next = NULL;

				// Let the server know we are mounting the exported folder.  This operation
				// will return the root node file handle.
				result = btMount(ns, ns->params.export, ns->params.user, ns->params.password, &rootNode->vnid);
				if (result == B_OK)
				{
					ns->rootid = rootNode->vnid;
					*vnid = ns->rootid;

					result = new_vnode(nsid, *vnid, rootNode);
					if (result == B_OK)
					{
						*data = ns;
						ns->first = rootNode;
						notify_listener(B_DEVICE_MOUNTED, ns->nsid, 0, 0, 0, NULL);
						dprintf("Mount successful.");
						return B_OK;
					}
				}
				else dprintf("Mount failed %d\n", result);

				// We've failed.  The first step is to free the root node we allocated, and
				// release the semaphore we created as well.
				free(rootNode);
				closeManagedData(&ns->dnlcData);
			}

			delete_sem(ns->sem);
		}

		// Disconnect from the server.
		btDisconnect(ns);
		btRPCClose(ns);
	}
	else
		result = EHOSTUNREACH;

	// De-allocate our private file system structure and all the string space we duplicated
	// on startup.
	free(ns->params.hostname);
	free(ns->params.folder);
	free(ns->params.export);
	free(ns->params.server);
	free(ns);

#ifndef BONE_VERSION
	ksocket_cleanup();
#else
	bone_cleanup();
#endif

	return result;		
}

extern int fs_unmount(fs_nspace *ns)
{
//dprintf("fs_unmount()\n");
	notify_listener(B_DEVICE_UNMOUNTED, ns->nsid, 0, 0, 0, NULL);

	btDisconnect(ns);
	btRPCClose(ns);

	free(ns->params.hostname);
	free(ns->params.folder);
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

#ifndef BONE_VERSION
	ksocket_cleanup();
#else
	bone_cleanup();
#endif

	return B_OK;
}

// fs_rfsstat()
//
extern int fs_rfsstat(fs_nspace *ns, struct fs_info *info)
{
	bt_fsinfo fsinfo;
	int result;

//dprintf("fs_rfsstat()\n");
	result = btGetFSInfo(ns, ns->rootid, &fsinfo);
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
		info->total_nodes = 0;
		info->free_nodes = 0;
		strcpy(info->device_name, "BeServed Shared Volume");
		strcpy(info->volume_name, ns->params.folder);
		strcpy(info->fsh_name, "");
	}

	return result;
}

// fs_open()
//
extern int fs_open(fs_nspace *ns, fs_node *node, int omode, fs_file_cookie **cookie)
{
	struct stat st;
	status_t result;

//dprintf("fs_open()\n");
	if ((result = btStat(ns, node->vnid, &st)) < B_OK)
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
//dprintf("fs_close()\n");
	if (cookie && (cookie->omode & O_RDWR || cookie->omode & O_WRONLY))
		return notify_listener(B_STAT_CHANGED, ns->nsid, 0, 0, node->vnid, NULL);

	return B_OK;
}

// fs_free_cookie()
//
extern int fs_free_cookie(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie)
{
//dprintf("fs_free_cookie()\n");
	if (cookie)
		free(cookie);

	return B_OK;
}

// fs_read()
//
extern int fs_read(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, void *buf, size_t *len)
{
//dprintf("fs_read()\n");
	if (!len)
		return B_ERROR;

	// If we weren't given proper file system or vnode pointers, something is awry.
	if (!ns || !node || !buf)
		return B_ERROR;

	// Do not permit reading directories.
	if (!cookie)
		return EISDIR;

	*len = btRead(ns, node->vnid, pos, *len, buf);

	return B_OK;
}

extern int fs_write(fs_nspace *ns, fs_node *node, fs_file_cookie *cookie, off_t pos, const void *buf, size_t *len)
{
//dprintf("fs_write()\n");
	// Do not permit writing directories.
	if (!cookie)
		return EISDIR;

	if (cookie->omode & O_APPEND)
		pos += cookie->original_size;

	*len = btWrite(ns, node->vnid, pos, *len, buf);
	btStat(ns, node->vnid, &cookie->st);

	return B_OK;
}

extern int fs_wstat(fs_nspace *ns, fs_node *node, struct stat *st, long mask)
{
	int error;

//dprintf("fs_wstat()\n");
	error = btWStat(ns, node->vnid, st, mask);
	if (error != B_OK)
		return error;

	return notify_listener(B_STAT_CHANGED, ns->nsid, 0, 0, node->vnid, NULL);
}

extern int fs_wfsstat(fs_nspace *ns, struct fs_info *info, long mask)
{
//dprintf("fs_wfsstat()\n");
	return B_OK;
}

extern int fs_create(fs_nspace *ns, fs_node *dir, const char *name, int omode, int perms, vnode_id *vnid,
					fs_file_cookie **cookie)
{
	struct stat st;
	status_t result;
	int error;

//dprintf("fs_create()\n");
	result = btLookup(ns, dir->vnid, name, vnid, &st);

	if (result == B_OK)
	{
		void *dummy;
		create_node(ns, dir->vnid, name, *vnid, false);

		if ((result = get_vnode(ns->nsid, *vnid, (void **) &dummy)) < B_OK)
			return result;

		if (S_ISDIR(st.st_mode))
			return EISDIR;

		if (omode & O_EXCL)
			return EEXIST;

		if (omode & O_TRUNC)
			if ((result = btTruncate(ns, *vnid, 0L)) < B_OK)
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

		if (!(omode & O_CREAT))
			return ENOENT;

		error = btCreate(ns, dir->vnid, name, vnid, omode, perms, &st);
		if (error != B_OK)
			return error;

		newNode = create_node(ns, dir->vnid, name, *vnid, true);

		*cookie = (fs_file_cookie *) malloc(sizeof(fs_file_cookie));
		(*cookie)->omode = omode;
		(*cookie)->original_size = st.st_size;
		(*cookie)->st = st;

		return notify_listener(B_ENTRY_CREATED, ns->nsid, dir->vnid, 0, *vnid, name);
	}
}

extern int fs_unlink(fs_nspace *ns, fs_node *dir, const char *name)
{
	fs_node *newNode;
	fs_node *dummy;
	struct stat st;
	vnode_id vnid;
	int error;

//dprintf("fs_unlink()\n");
	error = btLookup(ns, dir->vnid, name, &vnid, &st);
	if (error != B_OK)
		return error;

	create_node(ns, dir->vnid, name, vnid, false);

	error = get_vnode(ns->nsid, vnid, (void **) &dummy);
	if (error != B_OK)
		return error;

	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
		return EISDIR;

	error = remove_vnode(ns->nsid, vnid);
	if (error != B_OK)
		return error;

	error = put_vnode(ns->nsid, vnid);
	if (error != B_OK)
		return error;

	error = btUnlink(ns, dir->vnid, name);
	if (error != B_OK)
		return error;

	return notify_listener(B_ENTRY_REMOVED, ns->nsid, dir->vnid, 0, vnid, name);
}

extern int fs_remove_vnode(fs_nspace *ns, fs_node *node, char r)
{
//dprintf("fs_remove_vnode()\n");
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
//dprintf("fs_secure_vnode()\n");
	return B_OK;
}

extern int fs_mkdir(fs_nspace *ns, fs_node *dir, const char *name, int perms)
{
	fs_node *newNode;
	struct stat st;
	vnode_id vnid;
	int error;

//dprintf("fs_mkdir()\n");
	error = btLookup(ns, dir->vnid, name, &vnid, &st);
	if (error == B_OK)
	{
		void *dummy;
		error = get_vnode(ns->nsid, vnid, (void **) &dummy);
		if (error != B_OK)
			return error;

		return EEXIST;
	}
	else if (error != ENOENT)
		return error;

	error = btCreateDir(ns, dir->vnid, name, perms | S_IFDIR, &vnid, &st);
	if (error != B_OK)
		return error;

	create_node(ns, dir->vnid, name, vnid, false);
	return notify_listener(B_ENTRY_CREATED, ns->nsid, dir->vnid, 0, vnid, name);
}

extern int fs_rename(fs_nspace *ns, fs_node *olddir, const char *oldname, fs_node *newdir, const char *newname)
{
	struct stat st;
	fs_node *oldNode;
	vnode_id vnid;
	int error;

//dprintf("fs_rename()\n");
	error = btLookup(ns, newdir->vnid, newname, &vnid, &st);
	if (error == B_OK)
	{
		if (S_ISREG(st.st_mode))
			error = fs_unlink(ns, newdir, newname);
		else
			error = fs_rmdir(ns, newdir, newname);

		if (error != B_OK)
			return error;
	}

	error = btLookup(ns, olddir->vnid, oldname, &vnid, &st);
	if (error != B_OK)
		return error;

	error = btRename(ns, olddir->vnid, oldname, newdir->vnid, newname);
	if (error != B_OK)
		return error;

	rename_node(ns, olddir->vnid, oldname, newdir->vnid, newname, vnid);

	return notify_listener(B_ENTRY_MOVED, ns->nsid, olddir->vnid, newdir->vnid, vnid, newname);
}

extern int fs_rmdir(fs_nspace *ns, fs_node *dir, const char *name)
{
	fs_node *newNode;
	fs_node *dummy;		
	vnode_id vnid;
	struct stat st;
	int error;

//dprintf("fs_rmdir()\n");
	error = btLookup(ns, dir->vnid, name, &vnid, &st);
	if (error != B_OK)
		return error;

	create_node(ns, dir->vnid, name, vnid, false);

	error = get_vnode(ns->nsid, vnid, (void **) &dummy);
	if (error != B_OK)
		return error;

	if (!S_ISDIR(st.st_mode))
		return ENOTDIR;

	error = remove_vnode(ns->nsid, vnid);
	if (error != B_OK)
		return error;

	error = put_vnode(ns->nsid, vnid);
	if (error != B_OK)
		return error;

	error = btDeleteDir(ns, dir->vnid, name);
	if (error != B_OK)
		return error;

	return notify_listener(B_ENTRY_REMOVED, ns->nsid, dir->vnid, 0, vnid, name);
}

extern int fs_readlink(fs_nspace *ns, fs_node *node, char *buf, size_t *bufsize)
{
//dprintf("fs_readlink()\n");
	return btReadLink(ns, node->vnid, buf, bufsize);
}

extern int fs_symlink(fs_nspace *ns, fs_node *dir, const char *name, const char *path)
{
	struct stat st;
	vnode_id vnid;
	int error;

//dprintf("fs_symlink()\n");
	error = btLookup(ns, dir->vnid, name, &vnid, &st);
	if (error == B_OK)
	{
		void *dummy;
		if ((error = get_vnode(ns->nsid, vnid, (void **) &dummy)) < B_OK)
			return error;

		return EEXIST;
	}
	else if (error != ENOENT)
		return error;

	error = btSymLink(ns, dir->vnid, name, path);
	if (error != B_OK)
		return error;

	error = btLookup(ns, dir->vnid, name, &vnid, &st);
	if (error != B_OK)
		return error;

	create_node(ns, dir->vnid, name, vnid, false);

	error = notify_listener(B_ENTRY_CREATED, ns->nsid, dir->vnid, 0, vnid, name);
	return error;
}

// fs_access()
//
// Description:  This function implements the access() system call.
//
int	fs_access(void *ns, void *node, int mode)
{
//dprintf("fs_access()\n");
	return B_OK;
}

int fs_read_attrib(fs_nspace *ns, fs_node *node, const char *name, int type, void *buf, size_t *len, off_t pos)
{
	int bytes;

	// Automatically deny reading Tracker attributes.
//dprintf("fs_read_attrib()\n");
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	bytes = btReadAttrib(ns, node->vnid, name, type, buf, *len, pos);
	if (bytes == -1)
		return -1;

	*len = bytes;
	return B_OK;
}

int	fs_write_attrib(fs_nspace *ns, fs_node *node, const char *name, int type, const void *buf, size_t *len, off_t pos)
{
	int bytes;

//dprintf("fs_write_attrib()\n");
	// Automatically deny reading Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	bytes = btWriteAttrib(ns, node->vnid, name, type, buf, *len, pos);
	if (bytes == -1)
		return -1;

	*len = bytes;
	notify_listener(B_ATTR_CHANGED, ns->nsid, 0, 0, node->vnid, node->name);
	return B_OK;
}

// fs_open_attribdir()
//
extern int fs_open_attribdir(fs_nspace *ns, fs_node *node, btCookie **cookie)
{
	struct stat st;
	int error;

//dprintf("fs_open_attribdir()\n");
	// Do a basic stat() on this file to verify we indeed have a valid file.
	// If we cannot obtain this information, we have an even bigger problem.
	if ((error = btStat(ns, node->vnid, &st)) < B_OK)
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
//dprintf("fs_close_attribdir()\n");
	btEmptyLPBCache(cookie);
	return B_OK;
}

// fs_rewind_attribdir()
//
extern int fs_rewind_attribdir(fs_nspace *ns, fs_node *node, btCookie *cookie)
{
//dprintf("fs_rewind_attribdir()\n");
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

//dprintf("fs_read_attribdir()\n");
	max = *num;
	*num = 0;

	// Cause the directory to be read.
	while ((result = btReadAttribDir(ns, node->vnid, &attrName, cookie)) == B_OK)
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
			break;
	}

	return B_OK;
}

// fs_free_attribdircookie()
//
extern int fs_free_attribdircookie(fs_nspace *ns, btCookie *cookie)
{
//dprintf("fs_free_attribdircookie()\n");
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
//dprintf("fs_remove_attrib()\n");
	// Automatically deny removing Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	return btRemoveAttrib(ns, node->vnid, name);
}

// fs_stat_attrib()
//
extern int fs_stat_attrib(fs_nspace *ns, fs_node *node, const char *name, struct attr_info *buf)
{
	buf->type = 0;
	buf->size = 0;

//dprintf("fs_stat_attrib()\n");
	// Automatically deny reading Tracker attributes.
	if (strncmp(name, "_trk/", 5) == 0)
	{
		errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	return btStatAttrib(ns, node->vnid, name, buf);
}

// fs_open_indexdir()
//
extern int fs_open_indexdir(fs_nspace *ns, btCookie **cookie)
{
//dprintf("fs_open_indexdir()\n");
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
//dprintf("fs_close_indexdir()\n");
	return B_OK;
}

// fs_rewind_indexdir()
//
extern int fs_rewind_indexdir(fs_nspace *ns, btCookie *cookie)
{
//dprintf("fs_rewind_indexdir()\n");
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

//dprintf("fs_read_indexdir()\n");
	max = *num;
	*num = 0;

	// Cause the directory to be read.
	while ((result = btReadIndexDir(ns, indexName, sizeof(indexName) - 1, cookie)) == B_OK)
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
//dprintf("fs_free_indexdircookie()\n");
	if (cookie)
		free(cookie);

	return B_OK;
}

extern int fsCreateIndex(fs_nspace *ns, const char *name, int type, int flags)
{
//dprintf("fs_create_index()\n");
	return btCreateIndex(ns, name, type, flags);
}

extern int fsRemoveIndex(fs_nspace *ns, const char *name)
{
//dprintf("fs_remove_index()\n");
	return btRemoveIndex(ns, name);
}

extern int fsStatIndex(fs_nspace *ns, const char *name, struct index_info *buf)
{
//dprintf("fs_stat_index()\n");
	buf->type = 0;
	buf->size = 0;
	buf->modification_time = 0;
	buf->creation_time = 0;
	buf->uid = 0;
	buf->gid = 0;

	return btStatIndex(ns, name, buf);
}

extern int fsOpenQuery(fs_nspace *ns, const char *query, ulong flags, port_id port, long token, btQueryCookie **cookie)
{
//dprintf("fs_openquery()\n");
	// Allocate and initialize the cookie.
	*cookie = (btQueryCookie *) malloc(sizeof(btQueryCookie));
	memset ((*cookie)->opaque, 0, BT_COOKIE_SIZE);
	(*cookie)->query = strdup(query);

	return B_OK;
}

extern int fsCloseQuery(fs_nspace *ns, btQueryCookie *cookie)
{
//dprintf("fs_closequery()\n");
	return B_OK;
}

extern int fsReadQuery(fs_nspace *ns, btQueryCookie *cookie, long *num, struct dirent *buf, size_t bufsize)
{
	char *filename;
	vnode_id vnid, parent;
	long max, bufContent;

//dprintf("fs_readquery()\n");
	max = *num;
	*num = 0;

	while (btReadQuery(ns, &filename, &vnid, &parent, cookie) == B_OK)
	{
		bufContent = 2 * (sizeof(dev_t) + sizeof(ino_t)) + sizeof(unsigned short) + strlen(filename) + 1;
		if (bufsize < bufContent)
			return B_OK;

		buf->d_dev = ns->nsid;
		buf->d_pdev = ns->nsid;
		buf->d_ino = vnid;
		buf->d_pino = parent;
		buf->d_reclen = bufContent;
		strcpy(buf->d_name, filename);

		bufsize -= bufContent;
		buf = (struct dirent *)((char *) buf + bufContent);

		free(filename);

		(*num)++;
		if (*num == max)
			break;
	}

	return B_OK;
}

extern int fsFreeQueryCookie(fs_nspace *ns, btQueryCookie *cookie)
{
//dprintf("fs_freequerycookie()\n");
	if (cookie)
	{
		if (cookie->query)
			free(cookie->query);

		free(cookie);
	}

	return B_OK;
}
