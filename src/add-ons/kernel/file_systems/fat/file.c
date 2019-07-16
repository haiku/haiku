/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <fs_cache.h>
#include <fs_info.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include <time.h>

#include "iter.h"
#include "dosfs.h"
#include "dlist.h"
#include "fat.h"
#include "dir.h"
#include "file.h"
#include "attr.h"
#include "vcache.h"
#include "util.h"

#define DPRINTF(a,b) if (debug_file > (a)) dprintf b

#define MAX_FILE_SIZE 0xffffffffLL


typedef struct filecookie {
	uint32		mode;		// open mode
} filecookie;


mode_t
make_mode(nspace *volume, vnode *node)
{
	mode_t result = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
	if (node->mode & FAT_SUBDIR) {
		result &= ~S_IFREG;
		result |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	}
	if ((node->mode & FAT_READ_ONLY) != 0)
		result &= ~(S_IWUSR | S_IWGRP | S_IWOTH);

	return result;
}


status_t
dosfs_get_vnode_name(fs_volume *_ns, fs_vnode *_node, char *buffer,
	size_t bufferSize)
{
	vnode   *node = (vnode*)_node->private_node;
	strlcpy(buffer, node->filename, bufferSize);
	return B_OK;
}


status_t write_vnode_entry(nspace *vol, vnode *node)
{
	uint32 i;
	struct diri diri;
	uint8 *buffer;


	// TODO : is it needed ? vfs job ?
	// don't update entries of deleted files
	//if (is_vnode_removed(vol->id, node->vnid) > 0) return 0;

	// XXX: should check if directory position is still valid even
	// though we do the is_vnode_removed check above

	if ((node->cluster != 0) && !IS_DATA_CLUSTER(node->cluster)) {
		dprintf("write_vnode_entry called on invalid cluster (%" B_PRIu32 ")\n",
			node->cluster);
		return EINVAL;
	}

	buffer = diri_init(vol, VNODE_PARENT_DIR_CLUSTER(node), node->eindex, &diri);
	if (buffer == NULL)
		return ENOENT;

	diri_make_writable(&diri);
	buffer[0x0b] = node->mode; // file attributes

	memset(buffer+0xc, 0, 0x16-0xc);
	i = time_t2dos(node->st_crtim);
	buffer[0x0e] = i & 0xff;
	buffer[0x0f] = (i >> 8) & 0xff;
	buffer[0x10] = (i >> 16) & 0xff;
	buffer[0x11] = (i >> 24) & 0xff;
	i = time_t2dos(node->st_time);
	buffer[0x16] = i & 0xff;
	buffer[0x17] = (i >> 8) & 0xff;
	buffer[0x18] = (i >> 16) & 0xff;
	buffer[0x19] = (i >> 24) & 0xff;
	buffer[0x1a] = node->cluster & 0xff;	// starting cluster
	buffer[0x1b] = (node->cluster >> 8) & 0xff;
	if (vol->fat_bits == 32) {
		buffer[0x14] = (node->cluster >> 16) & 0xff;
		buffer[0x15] = (node->cluster >> 24) & 0xff;
	}
	if (node->mode & FAT_SUBDIR) {
		buffer[0x1c] = buffer[0x1d] = buffer[0x1e] = buffer[0x1f] = 0;
	} else {
		buffer[0x1c] = node->st_size & 0xff;	// file size
		buffer[0x1d] = (node->st_size >> 8) & 0xff;
		buffer[0x1e] = (node->st_size >> 16) & 0xff;
		buffer[0x1f] = (node->st_size >> 24) & 0xff;
	}

	diri_free(&diri);

	// TODO: figure out which stats have actually changed
	notify_stat_changed(vol->id, -1, node->vnid, B_STAT_MODE | B_STAT_UID
		| B_STAT_GID | B_STAT_SIZE | B_STAT_ACCESS_TIME
		| B_STAT_MODIFICATION_TIME | B_STAT_CREATION_TIME
		| B_STAT_CHANGE_TIME);

	return B_OK;
}


// called when fs is done with vnode
// after close, etc. free vnode resources here
status_t
dosfs_release_vnode(fs_volume *_vol, fs_vnode *_node, bool reenter)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;

	TOUCH(reenter);

	if (node != NULL) {
		DPRINTF(0, ("dosfs_release_vnode (ino_t %" B_PRIdINO ")\n", node->vnid));

		if ((vol->fs_flags & FS_FLAGS_OP_SYNC) && node->dirty) {
			LOCK_VOL(vol);
			_dosfs_sync(vol);
			UNLOCK_VOL(vol);
		}

#if TRACK_FILENAME
		if (node->filename) free(node->filename);
#endif

		if (node->vnid != vol->root_vnode.vnid) {
			file_cache_delete(node->cache);
			file_map_delete(node->file_map);
			free(node);
		}
	}

	return 0;
}


status_t
dosfs_rstat(fs_volume *_vol, fs_vnode *_node, struct stat *st)
{
	nspace	*vol = (nspace*)_vol->private_volume;
	vnode	*node = (vnode*)_node->private_node;

	LOCK_VOL(vol);

	DPRINTF(1, ("dosfs_rstat (vnode id %" B_PRIdINO ")\n", node->vnid));

	st->st_dev = vol->id;
	st->st_ino = node->vnid;
	st->st_mode = make_mode(vol, node);

	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_size = node->st_size;
	st->st_blocks = (node->st_size + 511) / 512;
	st->st_blksize = 0x10000; /* this value was chosen arbitrarily */
	st->st_atim.tv_sec = st->st_mtim.tv_sec = st->st_ctim.tv_sec
		= node->st_time;
	st->st_crtim.tv_sec = node->st_crtim;
	st->st_atim.tv_nsec = st->st_mtim.tv_nsec = st->st_ctim.tv_nsec
		= st->st_crtim.tv_nsec = 0;

	UNLOCK_VOL(vol);

	return B_NO_ERROR;
}


status_t
dosfs_wstat(fs_volume *_vol, fs_vnode *_node, const struct stat *st,
	uint32 mask)
{
	int err = B_OK;
	nspace	*vol = (nspace*)_vol->private_volume;
	vnode	*node = (vnode*)_node->private_node;
	bool dirty = false;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_wstat (vnode id %" B_PRIdINO ")\n", node->vnid));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("can't wstat on read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	if (node->disk_image == 2) {
		dprintf("can't wstat disk image\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}

	if ((mask & B_STAT_MODE) != 0) {
		DPRINTF(0, ("setting file mode to %o\n", st->st_mode));
		if (st->st_mode & S_IWUSR)
			node->mode &= ~FAT_READ_ONLY;
		else
			node->mode |= FAT_READ_ONLY;
		dirty = true;
	}

	if ((mask & B_STAT_SIZE) != 0) {
		DPRINTF(0, ("setting file size to %" B_PRIdOFF "\n", st->st_size));
		if (node->mode & FAT_SUBDIR) {
			dprintf("dosfs_wstat: can't set file size of directory!\n");
			err = EISDIR;
		} else if (st->st_size > MAX_FILE_SIZE) {
			dprintf("dosfs_wstat: desired file size exceeds fat limit\n");
			err = E2BIG;
		} else {
			uint32 clusters = (st->st_size + vol->bytes_per_sector
					* vol->sectors_per_cluster - 1) / vol->bytes_per_sector
				/ vol->sectors_per_cluster;
			DPRINTF(0, ("setting fat chain length to %" B_PRIu32 " clusters\n",
				clusters));
			if ((err = set_fat_chain_length(vol, node, clusters, false))
					== B_OK) {
				node->st_size = st->st_size;
				node->iteration++;
				dirty = true;
				file_cache_set_size(node->cache, node->st_size);
				file_map_set_size(node->file_map, node->st_size);
			}
		}
	}

	if ((mask & B_STAT_MODIFICATION_TIME) != 0) {
		DPRINTF(0, ("setting modification time\n"));
		if ((node->mode & FAT_SUBDIR) == 0)
			node->mode |= FAT_ARCHIVE;
		node->st_time = st->st_mtime;
		dirty = true;
	}

	if ((mask & B_STAT_CREATION_TIME) != 0) {
		DPRINTF(0, ("setting creation time\n"));
		// As a file's modification time is also set when it is created,
		// the archive bit should be set automatically.
		node->st_crtim = st->st_crtime;
		dirty = true;
	}

	if (dirty) {
		write_vnode_entry(vol, node);

		if (vol->fs_flags & FS_FLAGS_OP_SYNC) {
			// sync the filesystem
			_dosfs_sync(vol);
			node->dirty = false;
		}
	}

	if (err != B_OK) DPRINTF(0, ("dosfs_wstat (%s)\n", strerror(err)));

	UNLOCK_VOL(vol);

	return err;
}


status_t
dosfs_open(fs_volume *_vol, fs_vnode *_node, int omode, void **_cookie)
{
	status_t	result = EINVAL;
	nspace *vol = (nspace *)_vol->private_volume;
	vnode* 	node = (vnode*)_node->private_node;
	filecookie *cookie;

	*_cookie = NULL;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_open: vnode id %" B_PRIdINO ", omode %o\n", node->vnid,
		omode));

	if (omode & O_CREAT) {
		dprintf("dosfs_open called with O_CREAT. call dosfs_create instead!\n");
		result = EINVAL;
		goto error;
	}

	if ((vol->flags & B_FS_IS_READONLY) ||
		(node->mode & FAT_READ_ONLY) ||
		(node->disk_image != 0) ||
		// allow opening directories for ioctl() calls
		// and to let BVolume to work
		(node->mode & FAT_SUBDIR)) {
		omode = (omode & ~O_RWMASK) | O_RDONLY;
	}

	if ((omode & O_TRUNC) && ((omode & O_RWMASK) == O_RDONLY)) {
		DPRINTF(0, ("can't open file for reading with O_TRUNC\n"));
		result = EPERM;
		goto error;
	}

	if (omode & O_TRUNC) {
		DPRINTF(0, ("dosfs_open called with O_TRUNC set\n"));
		if ((result = set_fat_chain_length(vol, node, 0, false)) != B_OK) {
			dprintf("dosfs_open: error truncating file\n");
			goto error;
		}
		node->mode = 0;
		node->st_size = 0;
		node->iteration++;
	}

	if ((cookie = calloc(sizeof(filecookie), 1)) == NULL) {
		result = ENOMEM;
		goto error;
	}

	cookie->mode = omode;
	*_cookie = cookie;
	result = B_OK;

error:
	if (result != B_OK) DPRINTF(0, ("dosfs_open (%s)\n", strerror(result)));

	UNLOCK_VOL(vol);
	return result;
}


status_t
dosfs_read(fs_volume *_vol, fs_vnode *_node, void *_cookie, off_t pos,
			void *buf, size_t *len)
{
	nspace	*vol = (nspace *)_vol->private_volume;
	vnode	*node = (vnode *)_node->private_node;
	filecookie *cookie = (filecookie *)_cookie;
	int result = B_OK;

	LOCK_VOL(vol);

	if (node->mode & FAT_SUBDIR) {
		DPRINTF(0, ("dosfs_read called on subdirectory %" B_PRIdINO "\n",
			node->vnid));
		*len = 0;
		UNLOCK_VOL(vol);
		return EISDIR;
	}

	DPRINTF(0, ("dosfs_read called %" B_PRIuSIZE " bytes at %" B_PRIdOFF
		" (vnode id %" B_PRIdINO ")\n", *len, pos, node->vnid));

	result = file_cache_read(node->cache, cookie, pos, buf, len);

	if (result != B_OK) {
		DPRINTF(0, ("dosfs_read (%s)\n", strerror(result)));
	} else {
		DPRINTF(0, ("dosfs_read: read %" B_PRIuSIZE " bytes\n", *len));
	}
	UNLOCK_VOL(vol);

	return result;
}


status_t
dosfs_write(fs_volume *_vol, fs_vnode *_node, void *_cookie, off_t pos,
	const void *buf, size_t *len)
{
	nspace	*vol = (nspace *)_vol->private_volume;
	vnode	*node = (vnode *)_node->private_node;
	filecookie *cookie = (filecookie *)_cookie;
	int result = B_OK;

	LOCK_VOL(vol);

	if ((vol->flags & B_FS_IS_READONLY) != 0) {
		UNLOCK_VOL(vol);
		return EROFS;
	}

	if (node->mode & FAT_SUBDIR) {
		DPRINTF(0, ("dosfs_write called on subdirectory %" B_PRIdINO "\n",
			node->vnid));
		*len = 0;
		UNLOCK_VOL(vol);
		return EISDIR;

	}

	DPRINTF(0, ("dosfs_write called %" B_PRIuSIZE " bytes at %" B_PRIdOFF
		" from buffer at %p (vnode id %" B_PRIdINO ")\n", *len, pos, buf,
		node->vnid));

	if ((cookie->mode & O_RWMASK) == O_RDONLY) {
		dprintf("dosfs_write: called on file opened as read-only\n");
		*len = 0;
		result = EPERM;
		goto bi;
	}

	if (pos < 0) pos = 0;

	if (cookie->mode & O_APPEND) {
		pos = node->st_size;
	}

	if (pos >= MAX_FILE_SIZE) {
		dprintf("dosfs_write: write position exceeds fat limits\n");
		*len = 0;
		result = E2BIG;
		goto bi;
	}

	if (pos + *len >= MAX_FILE_SIZE) {
		*len = (size_t)(MAX_FILE_SIZE - pos);
	}

	// extend file size if needed
	if (pos + *len > node->st_size) {
		uint32 clusters = (pos + *len + vol->bytes_per_sector*vol->sectors_per_cluster - 1) / vol->bytes_per_sector / vol->sectors_per_cluster;
		if (node->st_size <= (clusters - 1) * vol->sectors_per_cluster * vol->bytes_per_sector) {
			if ((result = set_fat_chain_length(vol, node, clusters, false))
					!= B_OK) {
				goto bi;
			}
			node->iteration++;
		}
		node->st_size = pos + *len;
		/* needs to be written to disk asap so that later vnid calculations
		 * by get_next_dirent are correct
		 */
		write_vnode_entry(vol, node);

		DPRINTF(0, ("setting file size to %" B_PRIdOFF " (%" B_PRIu32
			" clusters)\n", node->st_size, clusters));
		node->dirty = true;
		file_cache_set_size(node->cache, node->st_size);
		file_map_set_size(node->file_map, node->st_size);
	}

	result = file_cache_write(node->cache, cookie, pos, buf, len);

bi:
	if (result != B_OK) {
		DPRINTF(0, ("dosfs_write (%s)\n", strerror(result)));
	} else {
		DPRINTF(0, ("dosfs_write: wrote %" B_PRIuSIZE " bytes\n", *len));
	}
	UNLOCK_VOL(vol);

	return result;
}


status_t
dosfs_close(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace	*vol = (nspace *)_vol->private_volume;
	vnode	*node = (vnode *)_node->private_node;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_close (vnode id %" B_PRIdINO ")\n", node->vnid));

	if ((vol->fs_flags & FS_FLAGS_OP_SYNC) && node->dirty) {
		_dosfs_sync(vol);
		node->dirty = false;
	}

	UNLOCK_VOL(vol);

	return 0;
}


status_t
dosfs_free_cookie(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *vol = _vol->private_volume;
	vnode *node = _node->private_node;
	filecookie *cookie = _cookie;
	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_free_cookie (vnode id %" B_PRIdINO ")\n", node->vnid));

	free(cookie);

	UNLOCK_VOL(vol);

	return 0;
}


status_t
dosfs_create(fs_volume *_vol, fs_vnode *_dir, const char *name, int omode,
	int perms, void **_cookie, ino_t *vnid)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *dir = (vnode *)_dir->private_node, *file;
	filecookie *cookie;
	status_t result = EINVAL;
	bool dups_exist;

	LOCK_VOL(vol);

	ASSERT(name != NULL);
	if (name == NULL) {
		dprintf("dosfs_create called with null name\n");
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	DPRINTF(0, ("dosfs_create called: %" B_PRIdINO "/%s perms=%o omode=%o\n",
		dir->vnid, name, perms, omode));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("dosfs_create called on read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	// TODO : is it needed ? vfs job ?
	/*if (is_vnode_removed(vol->id, dir->vnid) > 0) {
		dprintf("dosfs_create() called in removed directory. disallowed.\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}*/

	if ((omode & O_RWMASK) == O_RDONLY) {
		dprintf("invalid permissions used in creating file\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}

	// create file cookie; do it here to make cleaning up easier
	if ((cookie = calloc(sizeof(filecookie), 1)) == NULL) {
		result = ENOMEM;
		goto bi;
	}

	result = findfile_case_duplicates(vol, dir, name, vnid, &file, &dups_exist);
	if (result == B_OK) {
		if (omode & O_EXCL) {
			dprintf("exclusive dosfs_create called on existing file %s\n", name);
			put_vnode(_vol, file->vnid);
			result = EEXIST;
			goto bi;
		}

		if (file->mode & FAT_SUBDIR) {
			dprintf("can't dosfs_create over an existing subdirectory\n");
			put_vnode(_vol, file->vnid);
			result = EPERM;
			goto bi;
		}

		if (file->disk_image) {
			dprintf("can't dosfs_create over a disk image\n");
			put_vnode(_vol, file->vnid);
			result = EPERM;
			goto bi;
		}

		if (omode & O_TRUNC) {
			set_fat_chain_length(vol, file, 0, false);
			file->st_size = 0;
			file->iteration++;
		}
	} else if (result == ENOENT && dups_exist) {
		// the file doesn't exist in the exact case, but another does in the
		// non-exact case. We wont create the new file.
		result = EEXIST;
		goto bi;
	} else if (result == ENOENT && !dups_exist) {
		// the file doesn't already exist in any case
		vnode dummy; /* used only to create directory entry */

		dummy.dir_vnid = dir->vnid;
		dummy.cluster = 0;
		dummy.end_cluster = 0;
		dummy.mode = 0;
		dummy.st_size = 0;
		time(&(dummy.st_time));
		dummy.st_crtim = dummy.st_time;

		if ((result = create_dir_entry(vol, dir, &dummy, name, &(dummy.sindex), &(dummy.eindex))) != B_OK) {
			dprintf("dosfs_create: error creating directory entry for %s (%s)\n", name, strerror(result));
			goto bi;
		}
		dummy.vnid = GENERATE_DIR_INDEX_VNID(dummy.dir_vnid, dummy.sindex);
		// XXX: dangerous construct
		if (find_vnid_in_vcache(vol, dummy.vnid) == B_OK) {
			dummy.vnid = generate_unique_vnid(vol);
			if ((result = add_to_vcache(vol, dummy.vnid, GENERATE_DIR_INDEX_VNID(dummy.dir_vnid, dummy.sindex))) < 0) {
				// XXX: should remove entry on failure
				if (vol->fs_flags & FS_FLAGS_OP_SYNC)
					_dosfs_sync(vol);
				goto bi;
			}
		}
		*vnid = dummy.vnid;

		result = get_vnode(_vol, *vnid, (void **)&file);
		if (result < B_OK) {
			if (vol->fs_flags & FS_FLAGS_OP_SYNC)
				_dosfs_sync(vol);
			goto bi;
		}
	} else {
		goto bi;
	}

	cookie->mode = omode;
	*_cookie = cookie;

	notify_entry_created(vol->id, dir->vnid, name, *vnid);

	result = 0;

	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

bi:	if (result != B_OK) free(cookie);

	UNLOCK_VOL(vol);

	if (result != B_OK) DPRINTF(0, ("dosfs_create (%s)\n", strerror(result)));

	return result;
}


status_t
dosfs_mkdir(fs_volume *_vol, fs_vnode *_dir, const char *name, int perms)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *dir = (vnode *)_dir->private_node, dummy;
	status_t result = EINVAL;
	struct csi csi;
	uchar *buffer;
	uint32 i;

	LOCK_VOL(vol);

	// TODO : is it needed ? vfs job ?
	/*if (is_vnode_removed(vol->id, dir->vnid) > 0) {
		dprintf("dosfs_mkdir() called in removed directory. disallowed.\n");
		UNLOCK_VOL(vol);
		return EPERM;
	}*/

	DPRINTF(0, ("dosfs_mkdir called: %" B_PRIdINO "/%s (perm %o)\n", dir->vnid,
		name, perms));

	if ((dir->mode & FAT_SUBDIR) == 0) {
		dprintf("dosfs_mkdir: vnode id %" B_PRIdINO " is not a directory\n",
			dir->vnid);
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	// S_IFDIR is never set in perms, so we patch it
	perms &= ~S_IFMT; perms |= S_IFDIR;

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("mkdir called on read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	/* only used to create directory entry */
	dummy.dir_vnid = dir->vnid;
	if ((result = allocate_n_fat_entries(vol, 1, (int32 *)&(dummy.cluster))) < 0) {
		dprintf("dosfs_mkdir: error allocating space for %s (%s))\n", name, strerror(result));
		goto bi;
	}
	dummy.end_cluster = dummy.cluster;
	dummy.mode = FAT_SUBDIR;
	if (!(perms & (S_IWUSR | S_IWGRP | S_IWGRP))) {
		dummy.mode |= FAT_READ_ONLY;
	}
	dummy.st_size = vol->bytes_per_sector*vol->sectors_per_cluster;
	time(&(dummy.st_time));
	dummy.st_crtim = dummy.st_time;

	dummy.vnid = GENERATE_DIR_CLUSTER_VNID(dummy.dir_vnid, dummy.cluster);
	// XXX: dangerous construct
	if (find_vnid_in_vcache(vol, dummy.vnid) == B_OK) {
		dummy.vnid = generate_unique_vnid(vol);
		if ((result = add_to_vcache(vol, dummy.vnid, GENERATE_DIR_CLUSTER_VNID(dummy.dir_vnid, dummy.cluster))) < 0)
			goto bi2;
	}

	if ((result = dlist_add(vol, dummy.vnid)) < 0) {
		dprintf("dosfs_mkdir: error adding directory %s to dlist (%s)\n", name, strerror(result));
		goto bi3;
	}

	buffer = malloc(vol->bytes_per_sector);
	if (!buffer) {
		result = ENOMEM;
		goto bi4;
	}

	if ((result = create_dir_entry(vol, dir, &dummy, name, &(dummy.sindex), &(dummy.eindex))) != B_OK) {
		dprintf("dosfs_mkdir: error creating directory entry for %s (%s))\n", name, strerror(result));
		goto bi5;
	}

	// create '.' and '..' entries and then end of directories
	memset(buffer, 0, vol->bytes_per_sector);
	memset(buffer, ' ', 11);
	memset(buffer+0x20, ' ', 11);
	buffer[0] = buffer[0x20] = buffer[0x21] = '.';
	buffer[0x0b] = buffer[0x2b] = FAT_SUBDIR;
	i = time_t2dos(dummy.st_time);
	buffer[0x0e] = i & 0xff;
	buffer[0x0f] = (i >> 8) & 0xff;
	buffer[0x10] = (i >> 16) & 0xff;
	buffer[0x11] = (i >> 24) & 0xff;
	buffer[0x16] = i & 0xff;
	buffer[0x17] = (i >> 8) & 0xff;
	buffer[0x18] = (i >> 16) & 0xff;
	buffer[0x19] = (i >> 24) & 0xff;
	i = time_t2dos(dir->st_crtim);
	buffer[0x2e] = i & 0xff;
	buffer[0x2f] = (i >> 8) & 0xff;
	buffer[0x30] = (i >> 16) & 0xff;
	buffer[0x31] = (i >> 24) & 0xff;
	i = time_t2dos(dir->st_time);
	buffer[0x36] = i & 0xff;
	buffer[0x37] = (i >> 8) & 0xff;
	buffer[0x38] = (i >> 16) & 0xff;
	buffer[0x39] = (i >> 24) & 0xff;
	buffer[0x1a] = dummy.cluster & 0xff;
	buffer[0x1b] = (dummy.cluster >> 8) & 0xff;
	if (vol->fat_bits == 32) {
		buffer[0x14] = (dummy.cluster >> 16) & 0xff;
		buffer[0x15] = (dummy.cluster >> 24) & 0xff;
	}
	// root directory is always denoted by cluster 0, even for fat32 (!)
	if (dir->vnid != vol->root_vnode.vnid) {
		buffer[0x3a] = dir->cluster & 0xff;
		buffer[0x3b] = (dir->cluster >> 8) & 0xff;
		if (vol->fat_bits == 32) {
			buffer[0x34] = (dir->cluster >> 16) & 0xff;
			buffer[0x35] = (dir->cluster >> 24) & 0xff;
		}
	}

	init_csi(vol, dummy.cluster, 0, &csi);
	csi_write_block(&csi, buffer);

	// clear out rest of cluster to keep scandisk happy
	memset(buffer, 0, vol->bytes_per_sector);
	for (i=1;i<vol->sectors_per_cluster;i++) {
		if (iter_csi(&csi, 1) != B_OK) {
			dprintf("dosfs_mkdir: error writing directory cluster\n");
			break;
		}
		csi_write_block(&csi, buffer);
	}

	free(buffer);

	notify_entry_created(vol->id, dir->vnid, name, dummy.vnid);

	result = B_OK;

	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

	UNLOCK_VOL(vol);
	return result;

bi5:
	free(buffer);
bi4:
	dlist_remove(vol, dummy.vnid);
bi3:
	if (IS_ARTIFICIAL_VNID(dummy.vnid))
		remove_from_vcache(vol, dummy.vnid);
bi2:
	clear_fat_chain(vol, dummy.cluster, false);
	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);
bi:
	UNLOCK_VOL(vol);
	if (result != B_OK) DPRINTF(0, ("dosfs_mkdir (%s)\n", strerror(result)));
	return result;
}


status_t
dosfs_rename(fs_volume *_vol, fs_vnode *_odir, const char *oldname,
	fs_vnode *_ndir, const char *newname)
{
	status_t result = EINVAL;
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *odir = (vnode *)_odir->private_node;
	vnode *ndir = (vnode *)_ndir->private_node;
	vnode *file, *file2;
	uint32 ns, ne;
	bool dups_exist;
	bool dirty = false;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_rename called: %" B_PRIdINO "/%s->%" B_PRIdINO "/%s\n",
		odir->vnid, oldname, ndir->vnid, newname));

	if (!oldname || !(*oldname) || !newname || !(*newname)) {
		result = EINVAL;
		goto bi;
	}

	if(!is_filename_legal(newname)) {
		dprintf("dosfs_rename called with invalid name '%s'\n", newname);
		result = EINVAL;
		goto bi;
	}

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("rename called on read-only volume\n");
		result = EROFS;
		goto bi;
	}

	if ((odir->vnid == ndir->vnid) && !strcmp(oldname, newname)) {
		result = EPERM;
		goto bi;
	}

	// locate the file
	if ((result = findfile_case(vol,odir,oldname,NULL,&file)) != B_OK) {
		DPRINTF(0, ("dosfs_rename: can't find file %s in directory %" B_PRIdINO
			"\n", oldname, odir->vnid));
		goto bi;
	}

	if (file->disk_image) {
		dprintf("rename called on disk image or disk image directory\n");
		result = EPERM;
		goto bi1;
	}

	// don't move a directory into one of its children
	if (file->mode & FAT_SUBDIR) {
		ino_t vnid = ndir->vnid;
		while (1) {
			vnode *dir;
			ino_t parent;

			if (vnid == file->vnid) {
				result = EINVAL;
				goto bi1;
			}

			if (vnid == vol->root_vnode.vnid)
				break;

			result = get_vnode(_vol, vnid, (void **)&dir);
			if (result < B_OK)
				goto bi1;
			parent = dir->dir_vnid;
			put_vnode(_vol, vnid);
			vnid = parent;
		}
	}

	// see if file already exists and erase it if it does
	result = findfile_case_duplicates(vol, ndir, newname, NULL, &file2, &dups_exist);
	if (result == B_OK) {
		if (file2->mode & FAT_SUBDIR) {
			dprintf("destination already occupied by a directory\n");
			result = EPERM;
			goto bi2;
		}

		if (file2->disk_image) {
			DPRINTF(0, ("dosfs_rename: can't replace disk image or disk image directory\n"));
			result = EPERM;
			goto bi2;
		}
		ns = file2->sindex; ne = file2->eindex;

		// let others know the old file is gone
		notify_entry_removed(vol->id, ndir->vnid, oldname, file2->vnid);

		// Make sure this vnode 1) is in the vcache and 2) no longer has a
		// location associated with it. See discussion in dosfs_unlink()
		vcache_set_entry(vol, file2->vnid, generate_unique_vnid(vol));

		// mark vnode for removal (dosfs_remove_vnode will clear the fat chain)
		// note we don't have to lock the file because the fat chain doesn't
		// get wiped from the disk until dosfs_remove_vnode() is called; we'll
		// have a phantom chain in effect until the last file is closed.
		remove_vnode(_vol, file2->vnid); // must be done in this order
		put_vnode(_vol, file2->vnid);

		dirty = true;

		// erase old directory entry
		if ((result = erase_dir_entry(vol, file)) != B_OK) {
			dprintf("dosfs_rename: error erasing old directory entry for %s (%s)\n", newname, strerror(result));
			goto bi1;
		}
	} else if (result == ENOENT && (!dups_exist || (odir->vnid == ndir->vnid && !strcasecmp(oldname, newname)))) {
		// there isn't an entry and there are no duplicates in the target dir or
		// there isn't an entry and the target dir is the same as the source dir and
		//   the source and target name are the same, case-insensitively

		// erase old directory entry
		if ((result = erase_dir_entry(vol, file)) != B_OK) {
			dprintf("dosfs_rename: error erasing old directory entry for %s (%s)\n", newname, strerror(result));
			goto bi1;
		}

		dirty = true;

		// create the new directory entry
		if ((result = create_dir_entry(vol, ndir, file, newname, &ns, &ne)) != B_OK) {
			dprintf("dosfs_rename: error creating directory entry for %s\n", newname);
			goto bi1;
		}
	} else if (result == ENOENT && dups_exist) {
		// the entry doesn't exist but a non-case entry does, so we can't do it
		result = EEXIST;
		goto bi1;
	} else {
		goto bi1;
	}

	// shrink the directory (an error here is not disastrous)
	compact_directory(vol, odir);

	dirty = true;

	// update vnode information
	file->dir_vnid = ndir->vnid;
	file->sindex = ns;
	file->eindex = ne;

	// update vcache
	vcache_set_entry(vol, file->vnid,
			(file->st_size) ?
			GENERATE_DIR_CLUSTER_VNID(file->dir_vnid, file->cluster) :
			GENERATE_DIR_INDEX_VNID(file->dir_vnid, file->sindex));

	// XXX: only write changes in the directory entry if needed
	//      (i.e. old entry, not new)
	write_vnode_entry(vol, file);

	if (file->mode & FAT_SUBDIR) {
		// update '..' directory entry if needed
		// this should most properly be in write_vnode, but it is safe
		// to keep it here since this is the only way the cluster of
		// the parent can change.
		struct diri diri;
		uint8 *buffer;
		if ((buffer = diri_init(vol, file->cluster, 1, &diri)) == NULL) {
			dprintf("error opening directory :(\n");
			result = EIO;
			goto bi2;
		}

		diri_make_writable(&diri);

		if (memcmp(buffer, "..         ", 11)) {
			dprintf("invalid directory :(\n");
			result = EIO;
			goto bi2;
		}
		if (ndir->vnid == vol->root_vnode.vnid) {
			// root directory always has cluster = 0
			buffer[0x1a] = buffer[0x1b] = 0;
		} else {
			buffer[0x1a] = ndir->cluster & 0xff;
			buffer[0x1b] = (ndir->cluster >> 8) & 0xff;
			if (vol->fat_bits == 32) {
				buffer[0x14] = (ndir->cluster >> 16) & 0xff;
				buffer[0x15] = (ndir->cluster >> 24) & 0xff;
			}
		}
		diri_free(&diri);
	}

#if TRACK_FILENAME
	if (file->filename) free(file->filename);
	file->filename = malloc(strlen(newname) + 1);
	if (file->filename) strcpy(file->filename, newname);
#endif

	notify_entry_moved(vol->id, odir->vnid, oldname, ndir->vnid, newname,
		file->vnid);

	// update MIME information
	if(!(file->mode & FAT_SUBDIR)) {
		set_mime_type(file, newname);
		notify_attribute_changed(vol->id, -1, file->vnid, "BEOS:TYPE",
			B_ATTR_CHANGED);
	}

	result = 0;

bi2:
	if (result != B_OK)
		put_vnode(_vol, file2->vnid);
bi1:
	put_vnode(_vol, file->vnid);
bi:
	if ((vol->fs_flags & FS_FLAGS_OP_SYNC) && dirty)
		_dosfs_sync(vol);
	UNLOCK_VOL(vol);
	if (result != B_OK) DPRINTF(0, ("dosfs_rename (%s)\n", strerror(result)));
	return result;
}


status_t
dosfs_remove_vnode(fs_volume *_vol, fs_vnode *_node, bool reenter)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;

	LOCK_VOL(vol);

	DPRINTF(0, ("dosfs_remove_vnode (%" B_PRIdINO ")\n", node->vnid));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("dosfs_remove_vnode: read-only volume\n");
		UNLOCK_VOL(vol);
		return EROFS;
	}

	// clear the fat chain
	ASSERT((node->cluster == 0) || IS_DATA_CLUSTER(node->cluster));
	/* XXX: the following assertion was tripped */
	ASSERT((node->cluster != 0) || (node->st_size == 0));
	if (node->cluster != 0)
		clear_fat_chain(vol, node->cluster, (node->mode & FAT_SUBDIR) != 0);

	/* remove vnode id from the cache */
	if (find_vnid_in_vcache(vol, node->vnid) == B_OK)
		remove_from_vcache(vol, node->vnid);

	/* at this point, the node shouldn't be in the dlist anymore */
	if ((node->mode & FAT_SUBDIR) != 0) {
		ASSERT(dlist_find(vol, CLUSTER_OF_DIR_CLUSTER_VNID(node->vnid)) == -1);
	}

	free(node);

	if (!reenter && vol->fs_flags & FS_FLAGS_OP_SYNC) {
		// sync the entire filesystem,
		// but only if we're not reentrant. Presumably the
		// function that called this will sync.
		_dosfs_sync(vol);
	}

	UNLOCK_VOL(vol);

	return B_OK;
}


// get rid of node or directory
static status_t
do_unlink(fs_volume *_vol, fs_vnode *_dir, const char *name, bool is_file)
{
	status_t result = EINVAL;
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *dir = (vnode *)_dir->private_node, *file;
	ino_t vnid;

	if (!strcmp(name, "."))
		return EPERM;

	if (!strcmp(name, ".."))
		return EPERM;

	LOCK_VOL(vol);

	DPRINTF(0, ("do_unlink %" B_PRIdINO "/%s\n", dir->vnid, name));

	if (vol->flags & B_FS_IS_READONLY) {
		dprintf("do_unlink: read-only volume\n");
		result = EROFS;
		goto bi;
	}

	// locate the file
	if ((result = findfile_case(vol,dir,name,&vnid,&file)) != B_OK) {
		DPRINTF(0, ("do_unlink: can't find file %s in directory %" B_PRIdINO
			"\n", name, dir->vnid));
		result = ENOENT;
		goto bi;
	}

	if (file->disk_image) {
		DPRINTF(0, ("do_unlink: can't unlink disk image or disk image directory\n"));
		result = EPERM;
		goto bi1;
	}

	// don't need to check file permissions because it will be done for us
	// also don't need to lock the file (see dosfs_rename for reasons why)
	if (is_file) {
		if (file->mode & FAT_SUBDIR) {
			result = EISDIR;
			goto bi1;
		}
	} else {
		if ((file->mode & FAT_SUBDIR) == 0) {
			result = ENOTDIR;
			goto bi1;
		}

		if (file->vnid == vol->root_vnode.vnid) {
			// this actually isn't a problem since the root vnode
			// will always be busy while the volume mounted
			dprintf("dosfs_rmdir: don't call this on the root directory\n");
			result = EPERM;
			goto bi1;
		}

		if ((result = check_dir_empty(vol, file)) < 0) {
			if (result == ENOTEMPTY) DPRINTF(0, ("dosfs_rmdir called on non-empty directory\n"));
			goto bi1;
		}
	}

	// erase the entry in the parent directory
	if ((result = erase_dir_entry(vol, file)) != B_OK)
		goto bi1;

	// shrink the parent directory (errors here are not disastrous)
	compact_directory(vol, dir);

	notify_entry_removed(vol->id, dir->vnid, name, file->vnid);

	/* Set the loc to a unique value. This effectively removes it from the
	 * vcache without releasing its vnid for reuse. It also nicely reserves
	 * the vnid from use by other nodes. This is okay because the vnode is
	 * locked in memory after this point and loc will not be referenced from
	 * here on.
	 */
	vcache_set_entry(vol, file->vnid, generate_unique_vnid(vol));

	if (!is_file)
		dlist_remove(vol, file->vnid);

	// fsil doesn't call dosfs_write_vnode for us, so we have to free the
	// vnode manually here.
	remove_vnode(_vol, file->vnid);

	result = 0;

	if (vol->fs_flags & FS_FLAGS_OP_SYNC)
		_dosfs_sync(vol);

bi1:
	put_vnode(_vol, vnid);		// get 1 free
bi:
	UNLOCK_VOL(vol);

	if (result != B_OK) DPRINTF(0, ("do_unlink (%s)\n", strerror(result)));

	return result;
}


status_t
dosfs_unlink(fs_volume *vol, fs_vnode *dir, const char *name)
{
	DPRINTF(1, ("dosfs_unlink called\n"));

	return do_unlink(vol, dir, name, true);
}


status_t
dosfs_rmdir(fs_volume *vol, fs_vnode *dir, const char *name)
{
	DPRINTF(1, ("dosfs_rmdir called\n"));

	return do_unlink(vol, dir, name, false);
}


bool
dosfs_can_page(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	// ToDo: we're obviously not even asked...
	return false;
}


status_t
dosfs_read_pages(fs_volume *_vol, fs_vnode *_node, void *_cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;
	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	if (node->cache == NULL)
		return(B_BAD_VALUE);

	LOCK_VOL(vol);

	while (true) {
		struct file_io_vec fileVecs[8];
		size_t fileVecCount = 8;
		bool bufferOverflow;
		size_t bytes = bytesLeft;

		status = file_map_translate(node->file_map, pos, bytesLeft, fileVecs,
			&fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bufferOverflow = status == B_BUFFER_OVERFLOW;

		status = read_file_io_vec_pages(vol->fd, fileVecs,
			fileVecCount, vecs, count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	UNLOCK_VOL(vol);

	return status;
}


status_t
dosfs_write_pages(fs_volume *_vol, fs_vnode *_node, void *_cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;
	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	if (node->cache == NULL)
		return B_BAD_VALUE;

	LOCK_VOL(vol);

	if ((vol->flags & B_FS_IS_READONLY) != 0) {
		UNLOCK_VOL(vol);
		return EROFS;
	}

	while (true) {
		struct file_io_vec fileVecs[8];
		size_t fileVecCount = 8;
		bool bufferOverflow;
		size_t bytes = bytesLeft;

		status = file_map_translate(node->file_map, pos, bytesLeft, fileVecs,
			&fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bufferOverflow = status == B_BUFFER_OVERFLOW;

		status = write_file_io_vec_pages(vol->fd, fileVecs,
			fileVecCount, vecs, count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	UNLOCK_VOL(vol);

	return status;
}


status_t
dosfs_get_file_map(fs_volume *_vol, fs_vnode *_node, off_t position,
	size_t length, struct file_io_vec *vecs, size_t *_count)
{
	nspace *vol = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;
	struct csi iter;
	status_t result;
	uint32 skipSectors;
	off_t offset;
	size_t index = 0;
	size_t max = *_count;

	LOCK_VOL(vol);
	*_count = 0;

	if ((node->mode & FAT_SUBDIR) != 0) {
		DPRINTF(0, ("dosfs_get_file_map called on subdirectory %" B_PRIdINO
			"\n", node->vnid));
		UNLOCK_VOL(vol);
		return EISDIR;
	}

	DPRINTF(0, ("dosfs_get_file_map called %" B_PRIuSIZE " bytes at %" B_PRIdOFF
		" (vnode id %" B_PRIdINO ")\n", length, position, node->vnid));

	if (position < 0)
		position = 0;

	if (node->st_size == 0 || length == 0 || position >= node->st_size) {
		result = B_OK;
		goto bi;
	}

	// Truncate to file size, taking overflow into account.
	if (position + length >= node->st_size || position + length < position)
		length = node->st_size - position;

	result = init_csi(vol, node->cluster, 0, &iter);
	if (result != B_OK) {
		dprintf("dosfs_get_file_map: invalid starting cluster (%" B_PRIu32
			")\n", node->cluster);
		result = EIO;
		goto bi;
	}

	skipSectors = position / vol->bytes_per_sector;
	if (skipSectors > 0) {
		result = iter_csi(&iter, skipSectors);
		if (result != B_OK) {
			dprintf("dosfs_get_file_map: end of file reached (init)\n");
			result = EIO;
			goto bi;
		}
	}

	ASSERT(iter.cluster == get_nth_fat_entry(vol, node->cluster,
			position / vol->bytes_per_sector / vol->sectors_per_cluster));

	offset = position % vol->bytes_per_sector;
	while (length > 0) {
		off_t block = csi_to_block(&iter);
		uint32 sectors = 1;

		length -= min(length, vol->bytes_per_sector - offset);

		while (length > 0) {
			result = iter_csi(&iter, 1);
			if (result != B_OK) {
				dprintf("dosfs_get_file_map: end of file reached\n");
				result = EIO;
				goto bi;
			}

			if (block + sectors != csi_to_block(&iter)) {
				// Disjoint sectors, need to flush and begin a new vector.
				break;
			}

			length -= min(length, vol->bytes_per_sector);
			sectors++;
		}

		vecs[index].offset = block * vol->bytes_per_sector + offset;
		vecs[index].length = sectors * vol->bytes_per_sector - offset;
		index++;

		if (length == 0)
			break;

		if (index >= max) {
			// we're out of file_io_vecs; let's bail out
			result = B_BUFFER_OVERFLOW;
			goto bi;
		}

		offset = 0;
	}

	result = B_OK;
bi:
	*_count = index;

	if (result != B_OK) {
		DPRINTF(0, ("dosfs_get_file_map (%s)\n", strerror(result)));
	}
	UNLOCK_VOL(vol);

	return result;
}
