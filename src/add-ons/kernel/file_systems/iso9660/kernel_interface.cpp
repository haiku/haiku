/*
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Copyright 2001, pinc Software.  All Rights Reserved.
 *
 * iso9960/multi-session, 1.0.0
 */


#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <KernelExport.h>
#include <NodeMonitor.h>
#include <fs_interface.h>
#include <fs_cache.h>

#include <fs_attr.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_query.h>
#include <fs_volume.h>

#include <util/kernel_cpp.h>

#include "iso9660.h"
#include "iso9660_identify.h"


//#define TRACE_ISO9660
#ifdef TRACE_ISO9660
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct identify_cookie {
	iso9660_info info;
};


//	#pragma mark - Scanning


static float
fs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	iso9660_info *info = new iso9660_info;

	status_t status = iso9660_fs_identify(fd, info);
	if (status != B_OK) {
		delete info;
		return -1;
	}

	*_cookie = info;
	return 0.6f;
}


static status_t
fs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	iso9660_info *info = (iso9660_info *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM | B_PARTITION_READ_ONLY ;
	partition->block_size = ISO_PVD_SIZE;
	partition->content_size = ISO_PVD_SIZE * info->max_blocks;
	partition->content_name = strdup(info->PreferredName());

	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
fs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	delete (iso9660_info *)_cookie;
}


//	#pragma mark - FS hooks


static status_t
fs_mount(dev_t mountID, const char *device, uint32 flags,
	const char *args, void **_volume, ino_t *_rootID)
{
	bool allowJoliet = true;
	nspace *volume;

	// Check for a 'nojoliet' parm
	// all we check for is the existance of 'nojoliet' in the parms.
	if (args != NULL) {
		uint32 i;
		char *spot;
		char *buf = strdup(args);

		uint32 len = strlen(buf);
		// lower case the parms data
		for (i = 0; i < len + 1; i++) 
			buf[i] = tolower(buf[i]);

		// look for nojoliet
		spot = strstr(buf, "nojoliet");
		if (spot != NULL)
			allowJoliet = false;

		free(buf);
	}

	// Try and mount volume as an ISO volume.
	status_t result = ISOMount(device, O_RDONLY, &volume, allowJoliet);
	if (result == B_OK) {
		*_rootID = ISO_ROOTNODE_ID;
		*_volume = volume;

		volume->id = mountID;

		result = publish_vnode(mountID, *_rootID, &volume->rootDirRec);
		if (result != B_OK) {
			block_cache_delete(volume->fBlockCache, false);
			free(volume);
			result = B_ERROR;
		}
	}
	return result;
}


static status_t
fs_unmount(void *_ns)
{
	status_t result = B_NO_ERROR;
	nspace *ns = (nspace *)_ns;

	TRACE(("fs_unmount - ENTER\n"));

	// Unlike in BeOS, we need to put the reference to our root node ourselves
	put_vnode(ns->id, ISO_ROOTNODE_ID);

	block_cache_delete(ns->fBlockCache, false);
	close(ns->fdOfSession);
	result = close(ns->fd);

	free(ns);

	TRACE(("fs_unmount - EXIT, result is %s\n", strerror(result)));
	return result;
}


static status_t
fs_read_fs_stat(void *_ns, struct fs_info *fss)
{
	nspace *ns = (nspace *)_ns;
	int i;

	fss->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY;
	fss->block_size = ns->logicalBlkSize[FS_DATA_FORMAT];
	fss->io_size = 65536;
	fss->total_blocks = ns->volSpaceSize[FS_DATA_FORMAT];
	fss->free_blocks = 0;

	strncpy(fss->device_name, ns->devicePath, sizeof(fss->device_name));

	strncpy(fss->volume_name, ns->volIDString, sizeof(fss->volume_name));
	for (i = strlen(fss->volume_name) - 1; i >=0 ; i--) {
		if (fss->volume_name[i] != ' ')
			break;
	}

	if (i < 0)
		strcpy(fss->volume_name, "UNKNOWN");
	else
		fss->volume_name[i + 1] = 0;

	strcpy(fss->fsh_name, "iso9660");
	return B_OK;
}


static status_t
fs_get_vnode_name(void *ns, void *_node, char *buffer, size_t bufferSize)
{
	vnode *node = (vnode*)_node;

	strlcpy(buffer, node->fileIDString, bufferSize);
	return B_OK;
}


static status_t
fs_walk(void *_ns, void *base, const char *file, ino_t *_vnodeID, int *_type)
{
	nspace *ns = (nspace *)_ns;
	vnode *baseNode = (vnode*)base;
	vnode *newNode = NULL;

	TRACE(("fs_walk - looking for %s in dir file of length %d\n", file,
		baseNode->dataLen[FS_DATA_FORMAT]));

	if (strcmp(file, ".") == 0)  {
		// base directory
		TRACE(("fs_walk - found \".\" file.\n"));
		*_vnodeID = baseNode->id;
		*_type = S_IFDIR;
		return get_vnode(ns->id, *_vnodeID, (void **)&newNode);
	} else if (strcmp(file, "..") == 0) {
		// parent directory
		TRACE(("fs_walk - found \"..\" file.\n"));
		*_vnodeID = baseNode->parID;
		*_type = S_IFDIR;
		return get_vnode(ns->id, *_vnodeID, (void **)&newNode);
	}

	// look up file in the directory
	uint32 dataLength = baseNode->dataLen[FS_DATA_FORMAT];
	status_t result = ENOENT;
	uint32 totalRead = 0;
	off_t block = baseNode->startLBN[FS_DATA_FORMAT];
	bool done = false;

	while (totalRead < dataLength && !done) {
		off_t cachedBlock = block;
		char *blockData = (char *)block_cache_get_etc(ns->fBlockCache, block, 0,
			ns->logicalBlkSize[FS_DATA_FORMAT]);
		if (blockData != NULL) {
			int bytesRead = 0;
			off_t blockBytesRead = 0;
			vnode node;
			int initResult;

			TRACE(("fs_walk - read buffer from disk at LBN %Ld into buffer 0x%x.\n",
				block, blockData));

			// Move to the next 2-block set if necessary				
			// Don't go over end of buffer, if dir record sits on boundary.

			node.fileIDString = NULL;
			node.attr.slName = NULL;

			while (blockBytesRead < 2 * ns->logicalBlkSize[FS_DATA_FORMAT]
				&& totalRead + blockBytesRead < dataLength
				&& blockData[0] != 0
				&& !done) {
				initResult = InitNode(&node, blockData, &bytesRead,
					ns->joliet_level);
				TRACE(("fs_walk - InitNode returned %s, filename %s, %d bytes read\n", strerror(initResult), node.fileIDString, bytesRead));

				if (initResult == B_OK) {
					if (!strcmp(node.fileIDString, file)) {
						TRACE(("fs_walk - success, found vnode at block %Ld, pos %Ld\n", block, blockBytesRead));
						*_vnodeID = (block << 30)
							+ (blockBytesRead & 0xffffffff);
						TRACE(("fs_walk - New vnode id is %Ld\n", *_vnodeID));

						result = get_vnode(ns->id, *_vnodeID,
							(void **)&newNode);
						if (result == B_OK) {
							newNode->parID = baseNode->id;
							done = true;
						}
					} else {
						free(node.fileIDString);
						node.fileIDString = NULL;
						free(node.attr.slName);
						node.attr.slName = NULL;
					}
				} else {	
					result = initResult;
					if (bytesRead == 0)
						done = TRUE;
				}
				blockData += bytesRead;
				blockBytesRead += bytesRead;

				TRACE(("fs_walk - Adding %d bytes to blockBytes read (total %Ld/%Ld).\n",
					bytesRead, blockBytesRead, baseNode->dataLen[FS_DATA_FORMAT]));
			}
			totalRead += ns->logicalBlkSize[FS_DATA_FORMAT];
			block++;
			
			TRACE(("fs_walk - moving to next block %Ld, total read %Ld\n", block, totalRead));
			block_cache_put(ns->fBlockCache, cachedBlock);

		} else
			done = TRUE;
	}

	if (newNode)
		*_type = newNode->attr.stat[FS_DATA_FORMAT].st_mode;

	TRACE(("fs_walk - EXIT, result is %s, vnid is %Lu\n",
		strerror(result), *_vnodeID));
	return result;
}


static status_t
fs_read_vnode(void *_ns, ino_t vnodeID, void **_node, bool reenter)
{
	nspace *ns = (nspace*)_ns;

	vnode *newNode = (vnode*)calloc(sizeof(vnode), 1);
	if (newNode == NULL)
		return B_NO_MEMORY;

	uint32 pos = vnodeID & 0x3fffffff;
	uint32 block = vnodeID >> 30;

	TRACE(("fs_read_vnode - block = %ld, pos = %ld, raw = %Lu node 0x%x\n",
		block, pos, vnodeID, newNode));

	if (pos > ns->logicalBlkSize[FS_DATA_FORMAT])
		return B_BAD_VALUE;

	char *data = (char *)block_cache_get_etc(ns->fBlockCache,
		block, 0, ns->logicalBlkSize[FS_DATA_FORMAT]);
	if (data == NULL) {
		free(newNode);
		return B_IO_ERROR;
	}

	status_t result = InitNode(newNode, data + pos, NULL, ns->joliet_level);
	block_cache_put(ns->fBlockCache, block);
	
	if (result < B_OK) {
		free(newNode);
		return result;
	}
	
	newNode->id = vnodeID;
	*_node = (void *)newNode;

	if ((newNode->flags & ISO_ISDIR) == 0) {
		newNode->cache = file_cache_create(ns->id, vnodeID,
			newNode->dataLen[FS_DATA_FORMAT]);
	}

	return B_OK;
}


static status_t
fs_release_vnode(void *ns, void *_node, bool reenter)
{
	status_t result = B_NO_ERROR;
	vnode *node = (vnode*)_node;

	(void)ns;
	(void)reenter;

	TRACE(("fs_release_vnode - ENTER (0x%x)\n", node));

	if (node != NULL) { 
		if (node->id != ISO_ROOTNODE_ID) {
			if (node->fileIDString != NULL)
				free (node->fileIDString);
			if (node->attr.slName != NULL)
				free (node->attr.slName);
			if (node->cache != NULL)
				file_cache_delete(node->cache);

			free(node);
		}
	}

	TRACE(("fs_release_vnode - EXIT\n"));
	return result;
}


static status_t
fs_read_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	nspace *ns = (nspace *)_fs;
	vnode *node = (vnode *)_node;

	uint32 fileSize = node->dataLen[FS_DATA_FORMAT];
	size_t bytesLeft = *_numBytes;

	if (pos >= fileSize) {
		*_numBytes = 0;
		return B_OK;
	}
	if (pos + bytesLeft > fileSize) {
		bytesLeft = fileSize - pos;
		*_numBytes = bytesLeft;
	}

	file_io_vec fileVec;
	fileVec.offset = pos + node->startLBN[FS_DATA_FORMAT]
		* ns->logicalBlkSize[FS_DATA_FORMAT];
	fileVec.length = bytesLeft;

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	return read_file_io_vec_pages(ns->fd, &fileVec, 1, vecs, count,
		&vecIndex, &vecOffset, &bytesLeft);
}


static status_t
fs_read_stat(void *_ns, void *_node, struct stat *st)
{
	nspace *ns = (nspace*)_ns;
	vnode *node = (vnode*)_node;
	status_t result = B_NO_ERROR;
	time_t time;

	TRACE(("fs_read_stat - ENTER\n"));

	st->st_dev = ns->id;
	st->st_ino = node->id;
	st->st_nlink = node->attr.stat[FS_DATA_FORMAT].st_nlink;
	st->st_uid = node->attr.stat[FS_DATA_FORMAT].st_uid;
	st->st_gid = node->attr.stat[FS_DATA_FORMAT].st_gid;
	st->st_blksize = 65536;
	st->st_mode = node->attr.stat[FS_DATA_FORMAT].st_mode;

	// Same for file/dir in ISO9660
	st->st_size = node->dataLen[FS_DATA_FORMAT];
	if (ConvertRecDate(&(node->recordDate), &time) == B_NO_ERROR) 
		st->st_ctime = st->st_mtime = st->st_atime = time;

	TRACE(("fs_read_stat - EXIT, result is %s\n", strerror(result)));

	return result;
}


static status_t
fs_open(void *_ns, void *_node, int omode, void **cookie)
{
	status_t result = B_NO_ERROR;

	(void)_ns;
	(void)cookie;

	// Do not allow any of the write-like open modes to get by
	if ((omode == O_WRONLY) || (omode == O_RDWR))
		result = EROFS;
	else if((omode & O_TRUNC) || (omode & O_CREAT))
		result = EROFS;

	return result;
}


static status_t
fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buffer,
	size_t *_length)
{
	vnode *node = (vnode *)_node;

	if (node->flags & ISO_ISDIR)
		return EISDIR;

	uint32 fileSize = node->dataLen[FS_DATA_FORMAT];

	// set/check boundaries for pos/length
	if (pos < 0)
		return B_BAD_VALUE;
	if (pos >= fileSize) {
		*_length = 0;
		return B_OK;
	}

	return file_cache_read(node->cache, NULL, pos, buffer, _length);
}


static status_t
fs_close(void *ns, void *node, void *cookie)
{
	(void)ns;
	(void)node;
	(void)cookie;

	return B_OK;
}


static status_t
fs_free_cookie(void *ns, void *node, void *cookie)
{
	(void)ns;
	(void)node;
	(void)cookie;

	return B_OK;
}


static status_t
fs_access(void *ns, void *node, int mode)
{
	(void)ns;
	(void)node;
	(void)mode;

	return B_OK;
}


static status_t
fs_read_link(void */*_volume*/, void *_node, char *buffer, size_t *_bufferSize)
{
	vnode *node = (vnode *)_node;

	if (!S_ISLNK(node->attr.stat[FS_DATA_FORMAT].st_mode))
		return B_BAD_VALUE;

	size_t length = strlen(node->attr.slName);
	if (length > *_bufferSize)
		memcpy(buffer, node->attr.slName, *_bufferSize);
	else {
		memcpy(buffer, node->attr.slName, length);
		*_bufferSize = length;
	}

	return B_OK;
}


static status_t
fs_open_dir(void *_ns, void *_node, void **cookie)
{
	vnode *node = (vnode *)_node;

	TRACE(("fs_open_dir - node is 0x%x\n", _node));

	if (!(node->flags & ISO_ISDIR))
		return B_NOT_A_DIRECTORY;

	dircookie *dirCookie = (dircookie *)malloc(sizeof(dircookie));
	if (dirCookie == NULL)
		return B_NO_MEMORY;

	dirCookie->startBlock = node->startLBN[FS_DATA_FORMAT];
	dirCookie->block = node->startLBN[FS_DATA_FORMAT];
	dirCookie->totalSize = node->dataLen[FS_DATA_FORMAT];
	dirCookie->pos = 0;
	dirCookie->id = node->id;
	*cookie = (void *)dirCookie;

	return B_OK;
}


static status_t
fs_read_dir(void *_ns, void *_node, void *_cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *num)
{
	nspace *ns = (nspace *)_ns;
	dircookie *dirCookie = (dircookie *)_cookie;

	TRACE(("fs_read_dir - ENTER\n"));

	status_t result = ISOReadDirEnt(ns, dirCookie, buffer, bufferSize);

	// If we succeeded, return 1, the number of dirents we read.
	if (result == B_OK)
		*num = 1;
	else
		*num = 0;

	// When you get to the end, don't return an error, just return
	// a zero in *num.

	if (result == ENOENT)
		result = B_OK;

	TRACE(("fs_read_dir - EXIT, result is %s\n", strerror(result)));
	return result;
}
			

static status_t
fs_rewind_dir(void *ns, void *node, void* _cookie)
{
	dircookie *cookie = (dircookie*)_cookie;

	cookie->block = cookie->startBlock;
	cookie->pos = 0;
	return B_OK;
}


static status_t
fs_close_dir(void *ns, void *node, void *cookie)
{
	return B_OK;
}


static status_t
fs_free_dir_cookie(void *ns, void *node, void *cookie)
{
	free(cookie);
	return B_OK;
}


//	#pragma mark -


static status_t
iso_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
		default:
			return B_ERROR;
	}
}


static file_system_module_info sISO660FileSystem = {
	{
		"file_systems/iso9660" B_CURRENT_FS_API_VERSION,
		0,
		iso_std_ops,
	},

	"ISO9660 File System",
	0,	// DDM flags

	// scanning
	fs_identify_partition,
	fs_scan_partition,
	fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,
	&fs_unmount,
	&fs_read_fs_stat,
	NULL,
	NULL,

	/* vnode operations */
	&fs_walk,
	&fs_get_vnode_name,
	&fs_read_vnode,
	&fs_release_vnode,
	NULL, 	// fs_remove_vnode()

	/* VM file access */
	NULL, 	// fs_can_page
	&fs_read_pages,
	NULL, 	// fs_write_pages

	NULL,	// fs_get_file_map

	NULL, 	// fs_ioctl
	NULL, 	// fs_set_flags
	NULL,	// fs_select
	NULL,	// fs_deselect
	NULL, 	// fs_fsync

	&fs_read_link,
	NULL, 	// fs_create_symlink

	NULL, 	// fs_link,
	NULL,	// fs_unlink
	NULL, 	// fs_rename

	&fs_access,
	&fs_read_stat,
	NULL, 	// fs_write_stat

	/* file operations */
	NULL, 	// fs_create
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	NULL, 	// fs_write

	/* directory operations */
	NULL, 	// fs_create_dir
	NULL, 	// fs_remove_dir
	&fs_open_dir,
	&fs_close_dir,
	&fs_free_dir_cookie,
	&fs_read_dir,
	&fs_rewind_dir,
	
	/* attribute directory operations */
	NULL, 	// fs_open_attr_dir
	NULL, 	// fs_close_attr_dir
	NULL,	// fs_free_attr_dir_cookie
	NULL,	// fs_read_attr_dir
	NULL,	// fs_rewind_attr_dir

	/* attribute operations */
	NULL,	// fs_create_attr
	NULL, 	// fs_open_attr
	NULL,	// fs_close_attr
	NULL,	// fs_free_attr_cookie
	NULL,	// fs_read_attr
	NULL,	// fs_write_attr

	NULL,	// fs_read_attr_stat
	NULL,	// fs_write_attr_stat
	NULL,	// fs_rename_attr
	NULL,	// fs_remove_attr

	/* index directory & index operations */
	NULL,	// fs_open_index_dir
	NULL,	// fs_close_index_dir
	NULL,	// fs_free_index_dir_cookie
	NULL,	// fs_read_index_dir
	NULL,	// fs_rewind_index_dir

	NULL,	// fs_create_index
	NULL,	// fs_remove_index
	NULL,	// fs_stat_index

	/* query operations */
	NULL,	// fs_open_query
	NULL,	// fs_close_query
	NULL,	// fs_free_query_cookie
	NULL,	// fs_read_query
	NULL,	// fs_rewind_query
};

module_info *modules[] = {
	(module_info *)&sISO660FileSystem,
	NULL,
};

