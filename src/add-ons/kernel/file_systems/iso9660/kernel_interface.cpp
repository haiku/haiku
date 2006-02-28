/*
**		Copyright 1999, Be Incorporated.   All Rights Reserved.
**		This file may be used under the terms of the Be Sample Code License.
**
**		Copyright 2001, pinc Software.  All Rights Reserved.
**
**		iso9960/multi-session, 1.0.0
**			2001-03-11: added multi-session support, axeld.
*/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <lock.h>
#include <malloc.h>

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

#include "iso.h"
#include "iso9660.h"

//#define TRACE_ISO9660 1
#if TRACE_ISO9660
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*  Start of fundamental (read-only) required functions */
static status_t		fs_mount(mount_id mountID, const char *device, uint32 flags, 
				const char *args, void **_data, vnode_id *_rootID);
static status_t		fs_unmount(void *ns);

static status_t		fs_get_vnode_name(void *_ns, void *_node, 
				char *buffer, size_t bufferSize);
static status_t		fs_walk(void *_ns, void *_base, const char *file, 
				vnode_id *_vnodeID, int *_type);

static status_t		fs_read_vnode(void *_ns, vnode_id vnid, void **node, bool reenter);
static status_t		fs_release_vnode(void *_ns, void *_node, bool reenter);
static status_t		fs_read_stat(void *_ns, void *_node, struct stat *st);
static status_t		fs_open(void *_ns, void *_node, int omode, void **cookie);
static status_t		fs_read(void *_ns, void *_node, void *cookie, off_t pos,
				void *buf, size_t *len);
/// fs_free_cookie - free cookie for file created in open.
static status_t		fs_free_cookie(void *ns, void *node, void *cookie);
static status_t		fs_close(void *ns, void *node, void *cookie);

// fs_access - checks permissions for access.
static status_t		fs_access(void *_ns, void *_node, int mode);

// fs_opendir - creates fs-specific "cookie" struct that can tell where
//					we are at in the directory list.
static status_t		fs_open_dir(void* _ns, void* _node, void** cookie);
// fs_readdir - read 1 or more dirents, keep state in cookie, return
//					0 when no more entries.
static status_t		fs_read_dir(void *_ns, void *_node, void *cookie, struct dirent *buf,
				size_t bufsize, uint32 *_num);
// fs_rewinddir - set cookie to represent beginning of directory, so
//					later fs_readdir calls start at beginning.
static status_t		fs_rewind_dir(void *_ns, void *_node, void *cookie);
// fs_closedir - Do whatever you need to to close a directory (sometimes
//					nothing), but DON'T free the cookie!
static status_t		fs_close_dir(void *_ns, void *_node, void *cookie);
// fs_free_dircookie - Free the fs-specific cookie struct
static status_t		fs_free_dir_cookie(void *_ns, void *_node, void *cookie);

// fs_rfsstat - Fill in fs_info struct for device.
static status_t		fs_read_fs_stat(void *_ns, struct fs_info *);

// fs_readlink - Read in the name of a symbolic link.
static status_t 	fs_read_link(void *_ns, void *_node, char *buf, size_t *bufsize);


//	#pragma mark - Scanning

struct identify_cookie {
	iso9660_info info;
};


static float
fs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	iso9660_info info;
	status_t status = iso9660_fs_identify(fd, &info);
	if (status != B_OK)
		return status;

	identify_cookie *cookie = new identify_cookie;
	memcpy(&cookie->info, &info, sizeof(info));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
fs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM | B_PARTITION_READ_ONLY ;
	partition->block_size = ISO_PVD_SIZE;
	partition->content_size = ISO_PVD_SIZE * cookie->info.maxBlocks;
	partition->content_name = strdup(cookie->info.get_preferred_volume_name());
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
fs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	delete cookie;
}


static status_t
fs_mount(mount_id mountID, const char *device, uint32 flags,
	const char *args, void **_data, vnode_id *_rootID)
{
	/*
	Kernel passes in nspace_id, (representing a disk or partition?)
	and a string representing the device (eg, "/dev/scsi/disk/030/raw)
	Flags will be used for things like specifying read-only mounting.
	parms is parameters passed in as switches from the mount command, 
	and len is the length of the otions. data is a pointer to a 
	driver-specific struct that should be allocated in this routine. 
	It will then be passed back in by the kernel to a number of the other 
	fs driver functions. vnid should also be passed back to the kernel, 
	representing the vnode id of the root vnode.
	*/
	status_t result = EINVAL;
		// return EINVAL if it's not a device compatible with the driver.
	bool allow_joliet = true;
	nspace *vol;

	(void)flags;
	
	/* Create semaphore if it's not already created. When do we need to
		use semaphores? */	

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
			allow_joliet = false;
	
		free(buf);
	}

	// Try and mount volume as an ISO volume.
	result = ISOMount(device, O_RDONLY, &vol, allow_joliet);

	// If it is ISO …
	if (result == B_NO_ERROR) {
		//vnode_id rootID = vol->rootDirRec.startLBN[FS_DATA_FORMAT];
		//*vnid = rootID;
		*_rootID = ISO_ROOTNODE_ID;
		*_data = (void*)vol;
				
		vol->id = mountID;

		// You MUST do this. Create the vnode for the root.
		result = publish_vnode(mountID, *_rootID, (void*)&(vol->rootDirRec));
		if (result != B_NO_ERROR) {
			block_cache_delete(vol->fBlockCache, false);
			free(vol);
			result = EINVAL;
		} else 
			result = B_NO_ERROR;	
	}
	return result;
}


static status_t
fs_unmount(void *_ns)
{
	status_t result = B_NO_ERROR;
	nspace *ns = (nspace *)_ns;

	TRACE(("fs_unmount - ENTER\n"));

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
	// Fill in fs_info struct for device.
	nspace *ns = (nspace *)_ns;
	int i;
	
	TRACE(("fs_rfsstat - ENTER\n"));
	
	// Fill in device id.
	//fss->dev = ns->fd;
	
	// Root vnode ID
	//fss->root = ISO_ROOTNODE_ID;
	
	// File system flags.
	fss->flags = B_FS_IS_PERSISTENT | B_FS_IS_READONLY;
	
	// FS block size.
	fss->block_size = ns->logicalBlkSize[FS_DATA_FORMAT];
	
	// IO size - specifies buffer size for file copying
	fss->io_size = 65536;
	
	// Total blocks?
	fss->total_blocks = ns->volSpaceSize[FS_DATA_FORMAT];
	
	// Free blocks = 0, read only
	fss->free_blocks = 0;
	
	// Device name.
	strncpy(fss->device_name, ns->devicePath, sizeof(fss->device_name));

	strncpy(fss->volume_name, ns->volIDString, sizeof(fss->volume_name));
	for (i = strlen(fss->volume_name)-1; i >=0 ; i--)
		if (fss->volume_name[i] != ' ')
			break;

	if (i < 0)
		strcpy(fss->volume_name, "UNKNOWN");
	else
		fss->volume_name[i + 1] = 0;
	
	// File system name
	strcpy(fss->fsh_name, "iso9660");
	
	TRACE(("fs_rfsstat - EXIT\n"));
	return 0;
}


static status_t
fs_get_vnode_name(void *ns, void *_node, char *buffer, size_t bufferSize)
{
	vnode *node = (vnode*)_node;

	strlcpy(buffer, node->fileIDString, bufferSize);
	return B_OK;
}


/* fs_walk - the walk function just "walks" through a directory looking for
	the specified file. When you find it, call get_vnode on its vnid to init
	it for the kernel.
*/
static status_t
fs_walk(void *_ns, void *base, const char *file, vnode_id *_vnodeID, int *_type)
{
	/* Starting at the base, find file in the subdir, and return path
		string and vnode id of file. */
	nspace *ns = (nspace *)_ns;
	vnode *baseNode = (vnode*)base;
	uint32 dataLen = baseNode->dataLen[FS_DATA_FORMAT];
	vnode *newNode = NULL;
	status_t result = ENOENT;
	bool done = FALSE;
	uint32 totalRead = 0;
	off_t block = baseNode->startLBN[FS_DATA_FORMAT];

	TRACE(("fs_walk - looking for %s in dir file of length %d\n", file,
		baseNode->dataLen[FS_DATA_FORMAT]));
	
	if (strcmp(file, ".") == 0)  {
		// base directory
		TRACE(("fs_walk - found \".\" file.\n"));
		*_vnodeID = baseNode->id;
		*_type = S_IFDIR;
		if (get_vnode(ns->id, *_vnodeID, (void **)&newNode) != 0)
    		result = EINVAL;
	    else
	    	result = B_NO_ERROR;
	} else if (strcmp(file, "..") == 0) {
		// parent directory
		TRACE(("fs_walk - found \"..\" file.\n"));
		*_vnodeID = baseNode->parID;
		*_type = S_IFDIR;
		if (get_vnode(ns->id, *_vnodeID, (void **)&newNode) != 0)
			result = EINVAL;
		else
			result = B_NO_ERROR;
	} else {
		// look up file in the directory
		char *blockData;

		while ((totalRead < dataLen) && !done) {
			off_t cachedBlock = block;
			
			blockData = (char *)block_cache_get_etc(ns->fBlockCache, block, 0, ns->logicalBlkSize[FS_DATA_FORMAT]);
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
				
				while (blockBytesRead  < 2*ns->logicalBlkSize[FS_DATA_FORMAT]
					&& totalRead + blockBytesRead < dataLen
					&& blockData[0] != 0
					&& !done)
				{
					initResult = InitNode(&node, blockData, &bytesRead, ns->joliet_level);
					TRACE(("fs_walk - InitNode returned %s, filename %s, %d bytes read\n", strerror(initResult), node.fileIDString, bytesRead));
	
					if (initResult == B_NO_ERROR) {
						if (strlen(node.fileIDString) == strlen(file)
							&& !strncmp(node.fileIDString, file, strlen(file)))
						{
							TRACE(("fs_walk - success, found vnode at block %Ld, pos %Ld\n", block, blockBytesRead));
							*_vnodeID = (block << 30) + (blockBytesRead & 0xFFFFFFFF);
							TRACE(("fs_walk - New vnode id is %Ld\n", *_vnodeID));

							if (get_vnode(ns->id, *_vnodeID, (void **)&newNode) != 0)
								result = EINVAL;
							else {
								newNode->parID = baseNode->id;
								done = TRUE;
								result = B_NO_ERROR;
							}
						} else {
							if (node.fileIDString != NULL) {
								free(node.fileIDString);
								node.fileIDString = NULL;
							}
							if (node.attr.slName != NULL) {
								free(node.attr.slName);
								node.attr.slName = NULL;
							}
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

	}
	TRACE(("fs_walk - EXIT, result is %s, vnid is %Lu\n", strerror(result), *_vnodeID));
	return result;
}


static status_t
fs_read_vnode(void *_ns, vnode_id vnid, void **node, bool reenter)
{
	uint32 block, pos;
	nspace *ns = (nspace*)_ns;
	status_t result = B_NO_ERROR;
	vnode *newNode = (vnode*)calloc(sizeof(vnode), 1);

	(void)reenter;

	pos = (vnid & 0x3FFFFFFF);
	block = (vnid >> 30);

	TRACE(("fs_read_vnode - ENTER, block = %ld, pos = %ld, raw = %Lu node 0x%x\n",
		block, pos, vnid, newNode));

	if (newNode != NULL) {
		if (vnid == ISO_ROOTNODE_ID) {
			TRACE(("fs_read_vnode - root node requested.\n"));
			memcpy(newNode, &(ns->rootDirRec), sizeof(vnode));
			*node = (void*)newNode;
		} else {
			char *blockData = (char *)block_cache_get_etc(ns->fBlockCache, block, 0, ns->logicalBlkSize[FS_DATA_FORMAT]);

			if (pos > ns->logicalBlkSize[FS_DATA_FORMAT]) {
				if (blockData != NULL)
					block_cache_put(ns->fBlockCache, block);

				result = EINVAL;
		 	} else if (blockData != NULL) {
				result = InitNode(newNode, blockData + pos, NULL, ns->joliet_level);
				block_cache_put(ns->fBlockCache, block);
				newNode->id = vnid;

				TRACE(("fs_read_vnode - init result is %s\n", strerror(result)));
				*node = (void *)newNode;
				TRACE(("fs_read_vnode - new file %s, size %ld\n", newNode->fileIDString, newNode->dataLen[FS_DATA_FORMAT]));
			}
		}
	} else
		result = ENOMEM;

	TRACE(("fs_read_vnode - EXIT, result is %s\n", strerror(result)));
	return result;
}


static status_t
fs_release_vnode(void *ns, void *_node, bool reenter)
{
	status_t result = B_NO_ERROR;
	vnode *node = (vnode*)_node;

	(void)ns;
	(void)reenter;

	TRACE(("fs_write_vnode - ENTER (0x%x)\n", node));

	if (node != NULL) { 
		if (node->id != ISO_ROOTNODE_ID) {
			if (node->fileIDString != NULL)
				free (node->fileIDString);
			if (node->attr.slName != NULL)
				free (node->attr.slName);

			free(node);
		}
	}

	TRACE(("fs_write_vnode - EXIT\n"));
	return result;
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
fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t *len)
{
	nspace *ns = (nspace *)_ns;			// global stuff
	vnode *node = (vnode *)_node;		// The read file vnode.
	uint16 blockSize = ns->logicalBlkSize[FS_DATA_FORMAT];
	uint32 startBlock = node->startLBN[FS_DATA_FORMAT] + (pos / blockSize);
	off_t blockPos = pos % blockSize;
	off_t numBlocks = 0;
	uint32 dataLen = node->dataLen[FS_DATA_FORMAT];
	status_t result = B_NO_ERROR;
	size_t endLen = 0;
	size_t reqLen = *len;
	size_t startLen =  0;

	(void)cookie;

	// Allow an open to work on a dir, but no reads
	if (node->flags & ISO_ISDIR)
		return EISDIR;

	if (pos < 0)
		pos = 0;
	*len = 0;	

	// If passed-in requested length is bigger than file size, change it to
	// file size.
	if (reqLen + pos > dataLen)
		reqLen = dataLen - pos;

	// Compute the length of the partial start-block read, if any.
	
	if (reqLen + blockPos <= blockSize)
		startLen = reqLen;
	else if (blockPos > 0)
		startLen = blockSize - blockPos;

	if (blockPos == 0 && reqLen >= blockSize) {
		TRACE(("Setting startLen to 0, even block read\n"));
		startLen = 0;
	}

	// Compute the length of the partial end-block read, if any.
	if (reqLen + blockPos > blockSize)
		endLen = (reqLen + blockPos) % blockSize;

	// Compute the number of middle blocks to read.
	numBlocks = ((reqLen - endLen - startLen) /  blockSize);

	//dprintf("fs_read - ENTER, pos is %Ld, len is %lu\n", pos, reqLen);
	//dprintf("fs_read - filename is %s\n", node->fileIDString);
	//dprintf("fs_read - total file length is %lu\n", dataLen);
	//dprintf("fs_read - start block of file is %lu\n", node->startLBN[FS_DATA_FORMAT]);
	//dprintf("fs_read - block pos is %Lu\n", blockPos);
	//dprintf("fs_read - read block will be %lu\n", startBlock);
	//dprintf("fs_read - startLen is %lu\n", startLen);
	//dprintf("fs_read - endLen is %lu\n", endLen);
	//dprintf("fs_read - num blocks to read is %Ld\n", numBlocks);
	
	if (pos >= dataLen) {
		// If pos >= file length, return length of 0.
		*len = 0;
		return B_OK;
	}

	// Read in the first, potentially partial, block.
	if (startLen > 0) {
		off_t cachedBlock = startBlock;
		char *blockData = (char *)block_cache_get_etc(ns->fBlockCache, startBlock, 0, blockSize);
		if (blockData != NULL) {
			//dprintf("fs_read - copying first block, len is %d.\n", startLen);
			memcpy(buf, blockData+blockPos, startLen);
			*len += startLen;
			block_cache_put(ns->fBlockCache, cachedBlock);
			startBlock++;	
		} else
			result = EIO;
	}

	// Read in the middle blocks.
	if (numBlocks > 0 && result == B_NO_ERROR) {
		TRACE(("fs_read - getting middle blocks\n"));
		char *endBuf = ((char *)buf) + startLen;
		for (int32 i=startBlock; i<startBlock+numBlocks; i++) {
			char *blockData = (char *)block_cache_get_etc(ns->fBlockCache, i, 0, blockSize);
			memcpy(endBuf, blockData, blockSize);
			*len += blockSize;
			endBuf += blockSize;
			block_cache_put(ns->fBlockCache, i);
		}
	}

	// Read in the last partial block.
	if (result == B_NO_ERROR && endLen > 0) {
		off_t endBlock = startBlock + numBlocks;
		char *endBlockData = (char*)block_cache_get_etc(ns->fBlockCache, endBlock, 0, blockSize);
		if (endBlockData != NULL) {
			char *endBuf = ((char *)buf) + (reqLen - endLen);

			memcpy(endBuf, endBlockData, endLen);
			block_cache_put(ns->fBlockCache, endBlock);
			*len += endLen;
		} else
			result = EIO;
	}

	TRACE(("fs_read - EXIT, result is %s\n", strerror(result)));
	return result;
}


static status_t
fs_close(void *ns, void *node, void *cookie)
{
	(void)ns;
	(void)node;
	(void)cookie;

	//dprintf("fs_close - ENTER\n");
	//dprintf("fs_close - EXIT\n");
	return B_OK;
}

static status_t
fs_free_cookie(void *ns, void *node, void *cookie)
{
	(void)ns;
	(void)node;
	(void)cookie;

	// We don't allocate file cookies, so we do nothing here.
	//dprintf("fs_free_cookie - ENTER\n");
	//if (cookie != NULL) free (cookie);
	//dprintf("fs_free_cookie - EXIT\n");
	return B_OK;
}

// fs_access - checks permissions for access.
static status_t
fs_access(void *ns, void *node, int mode)
{
	(void)ns;
	(void)node;
	(void)mode;

	// ns 	- global, fs-specific struct for device
	// node	- node to check permissions for
	// mode - requested permissions on node.
	//dprintf("fs_access - ENTER\n");
	//dprintf("fs_access - EXIT\n");
	return B_OK;
}

static status_t
fs_read_link(void *_ns, void *_node, char *buffer, size_t *_bufferSize)
{
	vnode *node = (vnode *)_node;
	status_t result = EINVAL;

	(void)_ns;

	if (S_ISLNK(node->attr.stat[FS_DATA_FORMAT].st_mode)) {
		size_t length = strlen(node->attr.slName);
		if (length > *_bufferSize)
			memcpy(buffer, node->attr.slName, *_bufferSize);
		else {
			memcpy(buffer, node->attr.slName, length);
			*_bufferSize = length;
		}

		result = B_NO_ERROR;
	}

	return result;
}


static status_t
fs_open_dir(void *_ns, void *_node, void **cookie)
{
	vnode *node = (vnode *)_node;
	status_t result = B_NO_ERROR;
	dircookie *dirCookie = (dircookie *)malloc(sizeof(dircookie));

	(void)_ns;

	TRACE(("fs_opendir - ENTER, node is 0x%x\n", _node));

	if (!(node->flags & ISO_ISDIR))
		result = EMFILE;

	if (dirCookie != NULL) {
		dirCookie->startBlock = node->startLBN[FS_DATA_FORMAT];
		dirCookie->block = node->startLBN[FS_DATA_FORMAT];
		dirCookie->totalSize = node->dataLen[FS_DATA_FORMAT];
		dirCookie->pos = 0;
		dirCookie->id = node->id;
		*cookie = (void *)dirCookie;
	} else
		result = ENOMEM;

	TRACE(("fs_opendir - EXIT\n"));
	return result;
}


static status_t
fs_read_dir(void *_ns, void *_node, void *_cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *num)
{
	status_t result = B_NO_ERROR;
	nspace *ns = (nspace *)_ns;
	dircookie *dirCookie = (dircookie *)_cookie;

	(void)_node;

	TRACE(("fs_readdir - ENTER\n"));

	result = ISOReadDirEnt(ns, dirCookie, buffer, bufferSize);

	// If we succeeded, return 1, the number of dirents we read.
	if (result == B_NO_ERROR)
		*num = 1;
	else
		*num = 0;

	// When you get to the end, don't return an error, just return
	// a zero in *num.

	if (result == ENOENT)
		result = B_NO_ERROR;

	TRACE(("fs_readdir - EXIT, result is %s\n", strerror(result)));
	return result;
}
			

static status_t
fs_rewind_dir(void *ns, void *node, void* _cookie)
{
	status_t result = EINVAL;
	dircookie *cookie = (dircookie*)_cookie;

	(void)ns;
	(void)node;

	//dprintf("fs_rewinddir - ENTER\n");
	if (cookie != NULL) {
		cookie->block = cookie->startBlock;
		cookie->pos = 0;
		result = B_NO_ERROR;
	}
	//dprintf("fs_rewinddir - EXIT, result is %s\n", strerror(result));
	return result;
}


static status_t
fs_close_dir(void *ns, void *node, void *cookie)
{
	(void)ns;
	(void)node;
	(void)cookie;

	// ns 		- global, fs-specific struct for device
	// node		- directory to close
	// cookie	- current cookie for directory.	
	//dprintf("fs_closedir - ENTER\n");
	//dprintf("fs_closedir - EXIT\n");
	return B_OK;
}


static status_t
fs_free_dir_cookie(void *ns, void *node, void *cookie)
{
	(void)ns;
	(void)node;

	// ns 		- global, fs-specific struct for device
	// node		- directory related to cookie
	// cookie	- current cookie for directory, to free.	
	//dprintf("fs_free_dircookie - ENTER\n");
	if (cookie != NULL)
		free(cookie);

	//dprintf("fs_free_dircookie - EXIT\n");
	return B_OK;
}

//	#pragma mark -


static status_t
iso_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
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
	NULL, 	// &fs_remove_vnode()

	/* VM file access */
	NULL, 	// &fs_can_page
	NULL,	// &fs_read_pages
	NULL, 	// &fs_write_pages

	NULL,	// &fs_get_file_map

	NULL, 	// &fs_ioctl
	NULL, 	// &fs_set_flags
	NULL,	// &fs_select
	NULL,	// &fs_deselect
	NULL, 	// &fs_fsync

	&fs_read_link,
	NULL,	// write link
	NULL, 	// &fs_create_symlink,

	NULL, 	// &fs_link,
	NULL,	// &fs_unlink
	NULL, 	// &fs_rename

	&fs_access,
	&fs_read_stat,
	NULL, 	// &fs_write_stat

	/* file operations */
	NULL, 	// &fs_create
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	NULL, 	// &fs_write

	/* directory operations */
	NULL, 	// &fs_create_dir
	NULL, 	// &fs_remove_dir
	&fs_open_dir,
	&fs_close_dir,
	&fs_free_dir_cookie,
	&fs_read_dir,
	&fs_rewind_dir,
	
	/* attribute directory operations */
	NULL, 	// &fs_open_attr_dir
	NULL, 	// &fs_close_attr_dir
	NULL,	// &fs_free_attr_dir_cookie
	NULL,	// &fs_read_attr_dir
	NULL,	// &fs_rewind_attr_dir

	/* attribute operations */
	NULL,	// &fs_create_attr
	NULL, 	// &fs_open_attr
	NULL,	// &fs_close_attr
	NULL,	// &fs_free_attr_cookie
	NULL,	// &fs_read_attr
	NULL,	// &fs_write_attr

	NULL,	// &fs_read_attr_stat
	NULL,	// &fs_write_attr_stat
	NULL,	// &fs_rename_attr
	NULL,	// &fs_remove_attr

	/* index directory & index operations */
	NULL,	// &fs_open_index_dir
	NULL,	// &fs_close_index_dir
	NULL,	// &fs_free_index_dir_cookie
	NULL,	// &fs_read_index_dir
	NULL,	// &fs_rewind_index_dir

	NULL,	// &fs_create_index
	NULL,	// &fs_remove_index
	NULL,	// &fs_stat_index

	/* query operations */
	NULL,	// &fs_open_query
	NULL,	// &fs_close_query
	NULL,	// &fs_free_query_cookie
	NULL,	// &fs_read_query
	NULL,	// &fs_rewind_query
};

module_info *modules[] = {
	(module_info *)&sISO660FileSystem,
	NULL,
};

